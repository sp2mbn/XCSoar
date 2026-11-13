// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Driver.hpp"

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class Port;
class PortNMEAReader;

/**
 * Driver for the SmartFlasher anti-collision light.  The SmartFlasher
 * sits between the FLARM and the display and passes the FLARM data
 * on, so FLARM sentences are left to the generic NMEA parser.  This
 * driver adds the SmartFlasher's own "$SFCTL" control sentences and
 * the "$SFVAR" runtime variables.
 *
 * The methods for the "Manage" dialog stop the receive thread and
 * read the answer themselves; the caller must have borrowed the
 * device.  They throw on port errors and return "no answer" when the
 * SmartFlasher does not answer in time.
 */
class SmartFlasherDevice final : public AbstractDevice {
  Port &port;

public:
  /**
   * The answer to a command: "$SFCTL,ACK,..." or "$SFCTL,NACK,...".
   */
  struct Reply {
    bool ack;

    /**
     * The fields after "ACK," (e.g. "POWER_LEVEL,40"), or the error
     * code after "NACK," (e.g. "READ_ONLY").
     */
    std::string value;
  };

  struct FlashStatus {
    bool on;

    /** the flash pattern, 0 to #MAX_FLASH_MODE */
    unsigned mode;

    /** 0 to #MAX_BRIGHTNESS */
    unsigned brightness;

    /** is the LED test sequence running? ("FLASH_TEST") */
    bool test;
  };

  struct Parameter {
    std::string name, value;
  };

  static constexpr unsigned MAX_FLASH_MODE = 4;
  static constexpr unsigned MAX_BRIGHTNESS = 5;

  explicit SmartFlasherDevice(Port &_port) noexcept
    :port(_port) {}

  /**
   * Can this text be sent as a parameter value?  Only printable
   * ASCII without the NMEA field and sentence delimiters fits into
   * a sentence.
   */
  [[gnu::pure]]
  static bool IsValidValue(std::string_view value) noexcept;

  /**
   * Ask the SmartFlasher to run its LED test sequence.  The answer
   * arrives later through ParseNMEA(), so this needs the receive
   * thread.
   *
   * Throws on error.
   */
  void StartFlashTest(OperationEnvironment &env);

  /**
   * Run the LED test sequence and wait for the answer.
   */
  std::optional<Reply> RunFlashTest(OperationEnvironment &env);

  /**
   * Read runtime variables ("$SFVAR").  Each name is requested on its
   * own, so an unknown name does not hide the others.
   *
   * @return one value per name; std::nullopt where the variable is
   * unknown or there was no answer
   */
  std::vector<std::optional<std::string>>
  ReadVariables(std::span<const char *const> names,
                OperationEnvironment &env);

  std::optional<FlashStatus> ReadFlashStatus(OperationEnvironment &env);

  std::optional<Reply> SetFlash(bool on, OperationEnvironment &env);
  std::optional<Reply> SetFlashMode(unsigned mode, OperationEnvironment &env);
  std::optional<Reply> SetBrightness(unsigned brightness,
                                     OperationEnvironment &env);
  std::optional<Reply> SetFlashTest(OperationEnvironment &env);

  /**
   * Read all configuration parameters ("$SFCTL,LIST").
   *
   * @return the parameters, empty if there was no answer
   */
  std::vector<Parameter> ReadParameters(OperationEnvironment &env);

  /**
   * Write a configuration parameter; the caller must have checked
   * the value with IsValidValue().
   */
  std::optional<Reply> WriteParameter(const char *name, const char *value,
                                      OperationEnvironment &env);

  std::optional<Reply> RestoreDefaults(OperationEnvironment &env);

  std::optional<Reply> Reboot(OperationEnvironment &env);

  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, NMEAInfo &info) override;

private:
  /**
   * Stop the receive thread, so the answer can be read with @p
   * reader, and discard pending input.
   */
  void BeginRequest(PortNMEAReader &reader);

  /**
   * Send a "$SFCTL" command and wait for its ACK or NACK.
   */
  std::optional<Reply> Command(const char *command,
                               OperationEnvironment &env);
};
