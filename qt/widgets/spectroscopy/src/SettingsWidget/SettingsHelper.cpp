// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h"

#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace {

QMap<std::string, QVariant> defaultSettings = {
    {"restrict-input-by-name", true}, {"plot-error-bars-external", false}, {"load-history", true}};

template <typename T> void setSetting(std::string const &settingGroup, std::string const &settingName, T const &value) {
  QSettings settings;
  settings.beginGroup(QString::fromStdString(settingGroup));
  settings.setValue(QString::fromStdString(settingName), value);
  settings.endGroup();
}

QVariant getSetting(std::string const &settingGroup, std::string const &settingName) {
  QSettings settings;
  settings.beginGroup(QString::fromStdString(settingGroup));
  auto const settingValue = settings.value(QString::fromStdString(settingName), defaultSettings[settingName]);
  settings.endGroup();

  return settingValue;
}

} // namespace

namespace MantidQt::CustomInterfaces::SettingsHelper {

static std::string const INDIRECT_SETTINGS_GROUP("Indirect Settings");
static std::string const RESTRICT_DATA_PROPERTY("restrict-input-by-name");
static std::string const ERROR_BARS_PROPERTY("plot-error-bars-external");
static std::string const LOAD_HISTORY_PROPERTY("load-history");
static std::string const FEATURE_FLAGS_PROPERTY("developer-feature-flags");

bool restrictInputDataByName() { return getSetting(INDIRECT_SETTINGS_GROUP, RESTRICT_DATA_PROPERTY).toBool(); }

bool externalPlotErrorBars() { return getSetting(INDIRECT_SETTINGS_GROUP, ERROR_BARS_PROPERTY).toBool(); }

bool loadHistory() { return getSetting(INDIRECT_SETTINGS_GROUP, LOAD_HISTORY_PROPERTY).toBool(); }

QStringList developerFeatureFlags() {
  return getSetting(INDIRECT_SETTINGS_GROUP, FEATURE_FLAGS_PROPERTY).toStringList();
}

bool hasDevelopmentFlag(std::string const &flag) {
  return developerFeatureFlags().contains(QString::fromStdString(flag));
}

void setRestrictInputDataByName(bool restricted) {
  setSetting(INDIRECT_SETTINGS_GROUP, RESTRICT_DATA_PROPERTY, restricted);
}

void setLoadHistory(bool loadHistory) { setSetting(INDIRECT_SETTINGS_GROUP, LOAD_HISTORY_PROPERTY, loadHistory); }

void setExternalPlotErrorBars(bool errorBars) { setSetting(INDIRECT_SETTINGS_GROUP, ERROR_BARS_PROPERTY, errorBars); }

void setDeveloperFeatureFlags(QStringList const &flags) {
  setSetting(INDIRECT_SETTINGS_GROUP, FEATURE_FLAGS_PROPERTY, flags);
}
} // namespace MantidQt::CustomInterfaces::SettingsHelper
