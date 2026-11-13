// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ManageSmartFlasherDialog.hpp"
#include "Device/Driver/SmartFlasher/Device.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Form/DataField/ComboList.hpp"
#include "Form/DataField/Enum.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Formatter/UserUnits.hpp"
#include "Atmosphere/Temperature.hpp"
#include "Language/Language.hpp"
#include "Operation/Cancelled.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "io/CSVLine.hpp"
#include "util/NumberParser.hpp"
#include "util/StringAPI.hxx"
#include "util/TruncateString.hpp"
#include "UIGlobals.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

static constexpr const char *CAPTION = "SmartFlasher";

/**
 * Below this, the SmartFlasher reports a temperature read error
 * (it sends -1000.0).
 */
static constexpr double MIN_TEMPERATURE = -100;

[[gnu::pure]]
static std::string
FormatError(const std::string &code) noexcept
{
  if (code == "READ_ONLY")
    return _("This setting cannot be changed.");

  if (code == "INVALID_VALUE" || code == "INVALID_MODE" ||
      code == "INVALID_BRIGHTNESS")
    return _("The SmartFlasher does not accept this value.");

  if (code == "TEST_ALREADY_ACTIVE")
    return _("The test is already running.");

  /* no translation for the rest: the code is what the SmartFlasher
     manual and its support know */
  return fmt::format("{}: {}", _("Error"), code);
}

/**
 * Tell the pilot when a command has failed.
 *
 * @return true if the SmartFlasher has confirmed the command
 */
static bool
CheckReply(const std::optional<SmartFlasherDevice::Reply> &reply,
           const char *caption) noexcept
{
  if (!reply) {
    ShowMessageBox(_("No answer"), caption, MB_OK | MB_ICONERROR);
    return false;
  }

  if (!reply->ack) {
    ShowMessageBox(FormatError(reply->value).c_str(), caption,
                   MB_OK | MB_ICONERROR);
    return false;
  }

  return true;
}

class SmartFlasherLightWidget final : public RowFormWidget {
  enum Controls {
    ON,
    MODE,
    BRIGHTNESS,
  };

  SmartFlasherDevice &device;
  SmartFlasherDevice::FlashStatus status;

public:
  SmartFlasherLightWidget(const DialogLook &look, SmartFlasherDevice &_device,
                          const SmartFlasherDevice::FlashStatus &_status)
    :RowFormWidget(look), device(_device), status(_status) {}

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
SmartFlasherLightWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                                 [[maybe_unused]] const PixelRect &rc) noexcept
{
  static constexpr StaticEnumChoice mode_list[] = {
    { 0u, N_("Steady"), N_("No flashing; the light stays on.") },
    { 1u, N_("Two quick flashes, long pause") },
    { 2u, N_("Two medium flashes, medium pause") },
    { 3u, N_("Two long flashes, short pause") },
    { 4u, N_("Three quick flashes, short pause") },
    nullptr
  };

  static constexpr StaticEnumChoice brightness_list[] = {
    { 0u, "20 %" },
    { 1u, "27 %" },
    { 2u, "35 %" },
    { 3u, "40 %" },
    { 4u, "45 %" },
    { 5u, "100 %" },
    nullptr
  };

  static_assert(std::size(mode_list) == SmartFlasherDevice::MAX_FLASH_MODE + 2);
  static_assert(std::size(brightness_list) ==
                SmartFlasherDevice::MAX_BRIGHTNESS + 2);

  AddBoolean(_("Light"), _("Switch the anti-collision light on or off."),
             status.on);
  AddEnum(_("Flash pattern"), nullptr, mode_list, status.mode);
  AddEnum(_("Brightness"), nullptr, brightness_list, status.brightness);
}

bool
SmartFlasherLightWidget::Save(bool &_changed) noexcept
try {
  PopupOperationEnvironment env;

  if (SaveValueEnum(MODE, status.mode)) {
    if (!CheckReply(device.SetFlashMode(status.mode, env), CAPTION))
      return false;
    _changed = true;
  }

  if (SaveValueEnum(BRIGHTNESS, status.brightness)) {
    if (!CheckReply(device.SetBrightness(status.brightness, env), CAPTION))
      return false;
    _changed = true;
  }

  if (SaveValue(ON, status.on)) {
    if (!CheckReply(device.SetFlash(status.on, env), CAPTION))
      return false;
    _changed = true;
  }

  return true;
} catch (OperationCancelled) {
  return false;
} catch (...) {
  ShowError(std::current_exception(), CAPTION);
  return false;
}

class ManageSmartFlasherWidget final : public RowFormWidget {
  SmartFlasherDevice &device;

public:
  ManageSmartFlasherWidget(const DialogLook &look,
                           SmartFlasherDevice &_device) noexcept
    :RowFormWidget(look), device(_device) {}

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

private:
  void AddStatus() noexcept;
  void EditLight() noexcept;
  void EditParameters() noexcept;
  void RunFlashTest() noexcept;
  void RestoreDefaults() noexcept;
  void Reboot() noexcept;
};

static constexpr const char *variable_names[] = {
  "SW_VERSION",
  "BUILD_TIMESTAMP",
  "VOLTAGE",
  "TEMP",
  "FLARM_CONNECTED",
};

void
ManageSmartFlasherWidget::AddStatus() noexcept
{
  std::vector<std::optional<std::string>> values;
  try {
    PopupOperationEnvironment env;
    values = device.ReadVariables(variable_names, env);
  } catch (OperationCancelled) {
    return;
  } catch (...) {
    ShowError(std::current_exception(), CAPTION);
    return;
  }

  if (std::none_of(values.begin(), values.end(),
                   [](const auto &v){ return v.has_value(); })) {
    AddReadOnly(CAPTION, nullptr, _("No answer"));
    return;
  }

  const auto &version = values[0];
  const auto &build = values[1];
  const auto &voltage = values[2];
  const auto &temperature = values[3];
  const auto &flarm = values[4];

  if (version)
    AddReadOnly(_("Firmware version"), nullptr, version->c_str());

  if (build)
    AddReadOnly(_("Firmware build"), nullptr, build->c_str());

  if (voltage) {
    char *endptr;
    const double value = ParseDouble(voltage->c_str(), &endptr);
    if (endptr != voltage->c_str() && *endptr == '\0')
      AddReadOnly(_("Supply voltage"), nullptr, "%.2f V", value);
  }

  if (temperature) {
    char *endptr;
    const double celsius = ParseDouble(temperature->c_str(), &endptr);
    if (endptr != temperature->c_str() && *endptr == '\0' &&
        celsius > MIN_TEMPERATURE) {
      char buffer[32];
      FormatUserTemperature(Temperature::FromCelsius(celsius).ToKelvin(),
                            buffer);
      AddReadOnly(_("Temperature"), nullptr, buffer);
    }
  }

  if (flarm)
    AddReadOnly(_("FLARM connected"), nullptr,
                *flarm == "1" ? _("Yes") : _("No"));
}

void
ManageSmartFlasherWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                                  [[maybe_unused]] const PixelRect &rc) noexcept
{
  AddStatus();

  AddButton(_("LED test"), [this](){ RunFlashTest(); });
  AddButton(_("Light"), [this](){ EditLight(); });
  AddButton(_("Parameters"), [this](){ EditParameters(); });
  AddButton(_("Restore defaults"), [this](){ RestoreDefaults(); });
  AddButton(_("Reboot"), [this](){ Reboot(); });
}

void
ManageSmartFlasherWidget::RunFlashTest() noexcept
try {
  PopupOperationEnvironment env;
  if (CheckReply(device.RunFlashTest(env), _("LED test")))
    ShowMessageBox(_("The SmartFlasher has started the LED test."),
                   _("LED test"), MB_OK | MB_ICONINFORMATION);
} catch (OperationCancelled) {
} catch (...) {
  ShowError(std::current_exception(), CAPTION);
}

void
ManageSmartFlasherWidget::EditLight() noexcept
try {
  std::optional<SmartFlasherDevice::FlashStatus> status;
  {
    PopupOperationEnvironment env;
    status = device.ReadFlashStatus(env);
  }

  if (!status) {
    ShowMessageBox(_("No answer"), _("Light"), MB_OK | MB_ICONERROR);
    return;
  }

  SmartFlasherLightWidget widget(GetLook(), device, *status);
  DefaultWidgetDialog(UIGlobals::GetMainWindow(), GetLook(), _("Light"),
                      widget);
} catch (OperationCancelled) {
} catch (...) {
  ShowError(std::current_exception(), CAPTION);
}

void
ManageSmartFlasherWidget::EditParameters() noexcept
try {
  PopupOperationEnvironment env;
  auto parameters = device.ReadParameters(env);
  if (parameters.empty()) {
    ShowMessageBox(_("No answer"), _("Parameters"), MB_OK | MB_ICONERROR);
    return;
  }

  while (true) {
    ComboList list;
    for (const auto &p : parameters)
      list.Append(fmt::format("{} = {}", p.name, p.value).c_str());

    const int i = ComboPicker(_("Parameters"), list);
    if (i < 0)
      break;

    auto &p = parameters[i];

    char value[64];
    CopyTruncateString(value, sizeof(value), p.value.c_str());
    if (!TextEntryDialog(value, sizeof(value), p.name.c_str()))
      continue;

    if (!SmartFlasherDevice::IsValidValue(value)) {
      ShowMessageBox(_("Use only letters, digits and punctuation other "
                       "than \",\", \"*\" and \"$\"."),
                     p.name.c_str(), MB_OK | MB_ICONERROR);
      continue;
    }

    const auto reply = device.WriteParameter(p.name.c_str(), value, env);
    if (CheckReply(reply, p.name.c_str())) {
      /* "<NAME>,<VALUE>": the value the SmartFlasher has stored */
      CSVLine csv(reply->value.c_str());
      csv.Skip();
      p.value = csv.Rest();
    }
  }
} catch (OperationCancelled) {
} catch (...) {
  ShowError(std::current_exception(), CAPTION);
}

void
ManageSmartFlasherWidget::RestoreDefaults() noexcept
try {
  if (ShowMessageBox(_("Reset all SmartFlasher settings to the factory "
                       "defaults?"),
                     _("Restore defaults"),
                     MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  PopupOperationEnvironment env;
  CheckReply(device.RestoreDefaults(env), _("Restore defaults"));
} catch (OperationCancelled) {
} catch (...) {
  ShowError(std::current_exception(), CAPTION);
}

void
ManageSmartFlasherWidget::Reboot() noexcept
try {
  if (ShowMessageBox(_("Reboot the SmartFlasher? The light and the FLARM "
                       "data are interrupted for a few seconds."),
                     _("Reboot"), MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  PopupOperationEnvironment env;
  CheckReply(device.Reboot(env), _("Reboot"));
} catch (OperationCancelled) {
} catch (...) {
  ShowError(std::current_exception(), CAPTION);
}

void
ManageSmartFlasherDialog(Device &device)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      look, CAPTION,
                      new ManageSmartFlasherWidget(
                        look, static_cast<SmartFlasherDevice &>(device)));
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}
