// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "OptionsDialogModel.h"
#include "MantidQtWidgets/Common/QSettingsHelper.h"

#include <QSettings>

#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

using namespace MantidQt::MantidWidgets::QSettingsHelper;

namespace {
std::string const SETTINGS_GROUP = "ISISReflectometryUI";
}

OptionsDialogSettings::OptionsDialogSettings(std::map<std::string, bool> boolOptions,
                                             std::map<std::string, int> intOptions)
    : m_boolOptions(std::move(boolOptions)), m_intOptions(std::move(intOptions)) {}

std::map<std::string, bool> const &OptionsDialogSettings::boolOptions() const { return m_boolOptions; }

std::map<std::string, int> const &OptionsDialogSettings::intOptions() const { return m_intOptions; }

OptionsDialogSettings OptionsDialogSettings::readSettings(QSettings const &settings) {
  return OptionsDialogSettings(getSettingsAsMap<bool>(settings, SETTINGS_GROUP),
                               getSettingsAsMap<int>(settings, SETTINGS_GROUP));
}

void OptionsDialogSettings::saveSettings(QSettings &settings, OptionsDialogSettings const &values) {
  for (auto const &[name, value] : values.boolOptions())
    setSetting(settings, SETTINGS_GROUP, name, value);
  for (auto const &[name, value] : values.intOptions())
    setSetting(settings, SETTINGS_GROUP, name, value);
}

OptionsDialogModel::OptionsDialogModel() = default;

OptionsDialogSettings OptionsDialogModel::defaultSettings() const {
  return OptionsDialogSettings(
      {{"WarnProcessAll", true}, {"WarnDiscardChanges", true}, {"WarnProcessPartialGroup", true}, {"Round", false}},
      {{"RoundPrecision", 3}});
}

OptionsDialogSettings OptionsDialogModel::readSettings() const {
  QSettings settings;
  return OptionsDialogSettings::readSettings(settings);
}

void OptionsDialogModel::saveSettings(OptionsDialogSettings const &values) {
  QSettings settings;
  OptionsDialogSettings::saveSettings(settings, values);
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
