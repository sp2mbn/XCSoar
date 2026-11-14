// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device.hpp"
#include "Device/Driver/SmartFlasher.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"

#include <string_view>

using std::string_view_literals::operator""sv;

void
SmartFlasherDevice::StartFlashTest(OperationEnvironment &env)
{
  PortWriteNMEA(port, "SFCTL,FLASH,TEST", env);
}

bool
SmartFlasherDevice::PutPilotEvent(OperationEnvironment &env)
{
  PortWriteNMEA(port, "SFCTL,EVENT", env);
  return true;
}

/**
 * Parse the answers that arrive while the receive thread runs: those
 * to "$SFCTL,FLASH,TEST" ("ACK,FLASH_TEST,STARTED" or
 * "NACK,FLASH,<error code>") and to "$SFCTL,EVENT" ("ACK,EVENT").
 * The "Manage" dialog reads the answers to its own commands with the
 * receive thread stopped.
 */
static void
ParseSFCTL(NMEAInputLine &line, NMEAInfo &info) noexcept
{
  const auto type = line.ReadView();
  if (type == "ACK"sv) {
    if (line.ReadView() == "FLASH_TEST"sv)
      info.smart_flasher_test_started.Update(info.clock);
  } else if (type == "NACK"sv) {
    /* "NACK,<command>,<error code>" */
    if (line.ReadView() == "FLASH"sv)
      info.smart_flasher_test_refused.Update(info.clock);
  }
}

/**
 * Parse "$SFINF,<uptime>,<alertState>,<deviceMode>,<flashMode>,
 * <testFlash>,...".
 */
static void
ParseSFINF(NMEAInputLine &line, NMEAInfo &info) noexcept
{
  line.Skip(2);

  unsigned mode, test;
  if (!line.ReadChecked(mode) || mode > 1)
    return;

  line.Skip();
  if (!line.ReadChecked(test) || test > 1)
    return;

  info.smart_flasher_mode = mode == 0
    ? NMEAInfo::SmartFlasherMode::FLARM
    : NMEAInfo::SmartFlasherMode::STANDALONE;
  info.smart_flasher_test_running = test != 0;
  info.smart_flasher_info_available.Update(info.clock);
}

bool
SmartFlasherDevice::ParseNMEA(const char *_line, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(_line))
    return false;

  NMEAInputLine line(_line);
  const auto type = line.ReadView();
  if (type == "$SFCTL"sv)
    ParseSFCTL(line, info);
  else if (type == "$SFINF"sv)
    ParseSFINF(line, info);
  else if (type == "$SFEVT"sv) {
    /* "$SFEVT,<event>"; other events are for future firmware */
    if (line.ReadView() == "BUTTON"sv)
      info.smart_flasher_button.Update(info.clock);
  } else
    return false;

  return true;
}

static Device *
SmartFlasherCreateOnPort([[maybe_unused]] const DeviceConfig &config,
                         Port &com_port)
{
  return new SmartFlasherDevice(com_port);
}

const struct DeviceRegister smart_flasher_driver = {
  "SmartFlasher",
  "SmartFlasher",
  DeviceRegister::MANAGE,
  SmartFlasherCreateOnPort,
};
