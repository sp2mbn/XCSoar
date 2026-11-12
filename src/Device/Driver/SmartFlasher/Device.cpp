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

/**
 * Parse the answer to "$SFCTL,FLASH,TEST": "ACK,FLASH_TEST,STARTED"
 * or "NACK,<error code>".  XCSoar sends no other SFCTL command, and
 * the SmartFlasher answers only on the port the command came from, so
 * every NACK on this port refuses the test.
 */
static void
ParseSFCTL(NMEAInputLine &line, NMEAInfo &info) noexcept
{
  const auto type = line.ReadView();
  if (type == "ACK"sv) {
    if (line.ReadView() == "FLASH_TEST"sv)
      info.smart_flasher_test_started.Update(info.clock);
  } else if (type == "NACK"sv)
    info.smart_flasher_test_refused.Update(info.clock);
}

bool
SmartFlasherDevice::ParseNMEA(const char *_line, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(_line))
    return false;

  NMEAInputLine line(_line);
  if (line.ReadView() != "$SFCTL"sv)
    return false;

  ParseSFCTL(line, info);
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
  0,
  SmartFlasherCreateOnPort,
};
