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
  test_pending = true;
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
 * "NACK,<error code>") and to "$SFCTL,EVENT" ("ACK,EVENT").  The
 * "Manage" dialog reads the answers to its own commands with the
 * receive thread stopped.
 */
inline void
SmartFlasherDevice::ParseSFCTL(NMEAInputLine &line, NMEAInfo &info) noexcept
{
  const auto type = line.ReadView();
  if (type == "ACK"sv) {
    if (line.ReadView() == "FLASH_TEST"sv) {
      test_pending = false;
      info.smart_flasher_test_started.Update(info.clock);
    }
  } else if (type == "NACK"sv && test_pending.exchange(false))
    info.smart_flasher_test_refused.Update(info.clock);
}

/**
 * Parse "$SFINF,<uptime>,<alertState>,<deviceMode>,...".  Only
 * "deviceMode" is used: 0 = FLARM, 1 = STANDALONE.
 */
static void
ParseSFINF(NMEAInputLine &line, NMEAInfo &info) noexcept
{
  line.Skip(2);

  unsigned mode;
  if (!line.ReadChecked(mode))
    return;

  switch (mode) {
  case 0:
    info.smart_flasher_mode = NMEAInfo::SmartFlasherMode::FLARM;
    break;

  case 1:
    info.smart_flasher_mode = NMEAInfo::SmartFlasherMode::STANDALONE;
    break;

  default:
    return;
  }

  info.smart_flasher_mode_available.Update(info.clock);
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
  else
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
