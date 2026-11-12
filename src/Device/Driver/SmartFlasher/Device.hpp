// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Driver.hpp"

class Port;

/**
 * Driver for the SmartFlasher anti-collision light.  The SmartFlasher
 * sits between the FLARM and the display and passes the FLARM data
 * on, so FLARM sentences are left to the generic NMEA parser.  This
 * driver adds the SmartFlasher's own "$SFCTL" control sentences.
 */
class SmartFlasherDevice final : public AbstractDevice {
  Port &port;

public:
  explicit SmartFlasherDevice(Port &_port) noexcept
    :port(_port) {}

  /**
   * Ask the SmartFlasher to run its LED test sequence.  The answer
   * arrives later through ParseNMEA().
   *
   * Throws on error.
   */
  void StartFlashTest(OperationEnvironment &env);

  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, NMEAInfo &info) override;
};
