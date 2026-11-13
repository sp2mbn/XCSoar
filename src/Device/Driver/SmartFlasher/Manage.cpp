// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device.hpp"
#include "Device/Error.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Util/NMEAReader.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "io/CSVLine.hpp"
#include "time/TimeoutClock.hpp"
#include "util/StringCompare.hxx"

#include <fmt/format.h>

#include <algorithm>
#include <cassert>

using std::string_view_literals::operator""sv;

/**
 * How long to wait for the answer to a request.  The SmartFlasher
 * answers right away; the time covers a slow Bluetooth link.
 */
static constexpr auto REPLY_TIMEOUT = std::chrono::seconds(2);

/**
 * The parameter list has no end marker that reliably comes last
 * (the ACK may come first), so it is complete once the SmartFlasher
 * has been quiet for this long.
 */
static constexpr auto LIST_QUIET = std::chrono::milliseconds(500);

static constexpr auto LIST_TIMEOUT = std::chrono::seconds(5);

bool
SmartFlasherDevice::IsValidValue(std::string_view value) noexcept
{
  return std::all_of(value.begin(), value.end(), [](char ch){
    return ch >= 0x20 && ch < 0x7f && ch != ',' && ch != '*' && ch != '$';
  });
}

void
SmartFlasherDevice::BeginRequest(PortNMEAReader &reader)
{
  port.StopRxThread();
  reader.Flush();
}

/**
 * Parse "SFCTL,ACK[,...]" or "SFCTL,NACK[,...]" (without "$" and
 * checksum).
 */
static std::optional<SmartFlasherDevice::Reply>
ParseReply(const char *line) noexcept
{
  bool ack;
  const char *rest = StringAfterPrefix(line, "SFCTL,ACK"sv);
  if (rest != nullptr)
    ack = true;
  else if ((rest = StringAfterPrefix(line, "SFCTL,NACK"sv)) != nullptr)
    ack = false;
  else
    return std::nullopt;

  if (*rest == ',')
    ++rest;
  else if (*rest != '\0')
    return std::nullopt;

  return SmartFlasherDevice::Reply{ack, rest};
}

std::optional<SmartFlasherDevice::Reply>
SmartFlasherDevice::Command(const char *command, OperationEnvironment &env)
{
  PortNMEAReader reader(port, env);
  BeginRequest(reader);

  PortWriteNMEA(port, fmt::format("SFCTL,{}", command).c_str(), env);

  const TimeoutClock timeout(REPLY_TIMEOUT);
  try {
    while (true) {
      const char *line = reader.ReadLine(timeout);
      if (line == nullptr)
        return std::nullopt;

      if (auto reply = ParseReply(line))
        return reply;
    }
  } catch (const DeviceTimeout &) {
    return std::nullopt;
  }
}

std::optional<SmartFlasherDevice::Reply>
SmartFlasherDevice::RunFlashTest(OperationEnvironment &env)
{
  return Command("FLASH,TEST", env);
}

std::vector<std::optional<std::string>>
SmartFlasherDevice::ReadVariables(std::span<const char *const> names,
                                  OperationEnvironment &env)
{
  std::vector<std::optional<std::string>> values(names.size());
  std::vector<bool> answered(names.size(), false);
  std::size_t remaining = names.size();

  /* store the answer for one name; ignores names not asked for */
  const auto Answer = [&](std::string_view name,
                          std::optional<std::string> value){
    const auto i = std::find_if(names.begin(), names.end(),
                                [name](const char *n){ return name == n; });
    if (i == names.end())
      return;

    const std::size_t index = std::distance(names.begin(), i);
    if (!answered[index]) {
      answered[index] = true;
      --remaining;
    }

    values[index] = std::move(value);
  };

  PortNMEAReader reader(port, env);
  BeginRequest(reader);

  for (const char *name : names)
    PortWriteNMEA(port, fmt::format("SFVAR,{}", name).c_str(), env);

  const TimeoutClock timeout(REPLY_TIMEOUT);
  try {
    while (remaining > 0) {
      const char *line = reader.ReadLine(timeout);
      if (line == nullptr)
        break;

      CSVLine csv(line);
      if (csv.ReadView() != "SFVAR"sv)
        continue;

      auto name = csv.ReadView();
      if (name == "NACK"sv) {
        /* "$SFVAR,NACK,UNKNOWN_VAR,<NAME>" */
        csv.Skip();
        Answer(csv.ReadView(), std::nullopt);
        continue;
      }

      /* "$SFVAR,<NAME1>,<VALUE1>[,<NAME2>,<VALUE2>...]" */
      while (!name.empty()) {
        Answer(name, std::string{csv.ReadView()});
        name = csv.ReadView();
      }
    }
  } catch (const DeviceTimeout &) {
  }

  return values;
}

std::optional<SmartFlasherDevice::FlashStatus>
SmartFlasherDevice::ReadFlashStatus(OperationEnvironment &env)
{
  PortNMEAReader reader(port, env);
  BeginRequest(reader);

  PortWriteNMEA(port, "SFCTL,FLASH,STATUS", env);

  /* "$SFCTL,STATUS,<KEY>,<VALUE>" for each key, in any order relative
     to "$SFCTL,ACK,FLASH_STATUS,COMPLETE" */
  std::optional<unsigned> on, mode, brightness;

  const TimeoutClock timeout(REPLY_TIMEOUT);
  try {
    while (!on || !mode || !brightness) {
      const char *line = reader.ReadLine(timeout);
      if (line == nullptr)
        return std::nullopt;

      if (const auto reply = ParseReply(line); reply && !reply->ack)
        return std::nullopt;

      const char *status = StringAfterPrefix(line, "SFCTL,STATUS,"sv);
      if (status == nullptr)
        continue;

      CSVLine csv(status);
      const auto key = csv.ReadView();
      unsigned value;
      if (!csv.ReadChecked(value))
        continue;

      if (key == "FLASH_ON"sv)
        on = value;
      else if (key == "FLASH_MODE"sv && value <= MAX_FLASH_MODE)
        mode = value;
      else if (key == "FLASH_BRIGHTNESS"sv && value <= MAX_BRIGHTNESS)
        brightness = value;
    }
  } catch (const DeviceTimeout &) {
    return std::nullopt;
  }

  return FlashStatus{*on != 0, *mode, *brightness};
}

std::optional<SmartFlasherDevice::Reply>
SmartFlasherDevice::SetFlash(bool on, OperationEnvironment &env)
{
  return Command(on ? "FLASH,ON" : "FLASH,OFF", env);
}

std::optional<SmartFlasherDevice::Reply>
SmartFlasherDevice::SetFlashMode(unsigned mode, OperationEnvironment &env)
{
  assert(mode <= MAX_FLASH_MODE);

  return Command(fmt::format("FLASH,MODE,{}", mode).c_str(), env);
}

std::optional<SmartFlasherDevice::Reply>
SmartFlasherDevice::SetBrightness(unsigned brightness,
                                  OperationEnvironment &env)
{
  assert(brightness <= MAX_BRIGHTNESS);

  return Command(fmt::format("FLASH,BRIGHTNESS,{}", brightness).c_str(),
                 env);
}

std::vector<SmartFlasherDevice::Parameter>
SmartFlasherDevice::ReadParameters(OperationEnvironment &env)
{
  PortNMEAReader reader(port, env);
  BeginRequest(reader);

  PortWriteNMEA(port, "SFCTL,LIST", env);

  std::vector<Parameter> parameters;
  bool acknowledged = false;

  const TimeoutClock timeout(LIST_TIMEOUT);
  try {
    while (!timeout.HasExpired()) {
      const std::chrono::steady_clock::duration wait =
        acknowledged || !parameters.empty()
        ? std::chrono::steady_clock::duration{LIST_QUIET}
        : timeout.GetRemainingOrZero();

      const char *line = reader.ReadLine(TimeoutClock{wait});
      if (line == nullptr)
        break;

      CSVLine csv(line);
      if (csv.ReadView() != "SFCTL"sv)
        continue;

      auto name = csv.ReadView();
      if (name == "ACK"sv) {
        if (csv.ReadView() == "LIST"sv)
          acknowledged = true;
        continue;
      }

      if (name == "NACK"sv)
        break;

      if (name == "LIST"sv)
        /* "$SFCTL,LIST,<NAME>,<VALUE>" */
        name = csv.ReadView();
      else if (name == "STATUS"sv || name == "STATS"sv ||
               name == "FLASH"sv)
        /* not a parameter; otherwise "$SFCTL,<NAME>,<VALUE>" */
        continue;

      if (!name.empty() && IsValidValue(name))
        parameters.push_back({std::string{name}, std::string{csv.Rest()}});
    }
  } catch (const DeviceTimeout &) {
  }

  return parameters;
}

std::optional<SmartFlasherDevice::Reply>
SmartFlasherDevice::WriteParameter(const char *name, const char *value,
                                   OperationEnvironment &env)
{
  assert(IsValidValue(name));
  assert(IsValidValue(value));

  return Command(fmt::format("W,{},{}", name, value).c_str(), env);
}

std::optional<SmartFlasherDevice::Reply>
SmartFlasherDevice::RestoreDefaults(OperationEnvironment &env)
{
  return Command("DEFAULTS", env);
}

std::optional<SmartFlasherDevice::Reply>
SmartFlasherDevice::Reboot(OperationEnvironment &env)
{
  return Command("RESET", env);
}
