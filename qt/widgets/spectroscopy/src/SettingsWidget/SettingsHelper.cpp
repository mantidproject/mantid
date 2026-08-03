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

#include <utility>

namespace {

QMap<std::string, QVariant> defaultSettings = {
    {"restrict-input-by-name", true}, {"plot-error-bars-external", false}, {"load-history", true}};

QVariant getSetting(std::string const &settingGroup, std::string const &settingName) {
  QSettings settings;
  settings.beginGroup(QString::fromStdString(settingGroup));
  auto const settingValue = settings.value(QString::fromStdString(settingName), defaultSettings[settingName]);
  settings.endGroup();

  return settingValue;
}

} // namespace

namespace MantidQt::CustomInterfaces::SettingsHelper {

SpectroscopySettings::SpectroscopySettings(bool restrictInputByName, bool externalPlotErrorBars, bool loadHistory,
                                           QStringList developerFeatureFlags)
    : m_restrictInputByName(restrictInputByName), m_externalPlotErrorBars(externalPlotErrorBars),
      m_loadHistory(loadHistory), m_developerFeatureFlags(std::move(developerFeatureFlags)) {}

bool SpectroscopySettings::restrictInputByName() const { return m_restrictInputByName; }

bool SpectroscopySettings::externalPlotErrorBars() const { return m_externalPlotErrorBars; }

bool SpectroscopySettings::loadHistory() const { return m_loadHistory; }

const QStringList &SpectroscopySettings::developerFeatureFlags() const { return m_developerFeatureFlags; }

static std::string const INDIRECT_SETTINGS_GROUP("Indirect Settings");
static std::string const RESTRICT_DATA_PROPERTY("restrict-input-by-name");
static std::string const ERROR_BARS_PROPERTY("plot-error-bars-external");
static std::string const LOAD_HISTORY_PROPERTY("load-history");
static std::string const FEATURE_FLAGS_PROPERTY("developer-feature-flags");

SpectroscopySettings readSettings(const QSettings &settings) {
  return SpectroscopySettings(settings.value(QString::fromStdString(RESTRICT_DATA_PROPERTY), true).toBool(),
                              settings.value(QString::fromStdString(ERROR_BARS_PROPERTY), false).toBool(),
                              settings.value(QString::fromStdString(LOAD_HISTORY_PROPERTY), true).toBool(),
                              settings.value(QString::fromStdString(FEATURE_FLAGS_PROPERTY)).toStringList());
}

SpectroscopySettings readSettings() {
  QSettings settings;
  settings.beginGroup(QString::fromStdString(INDIRECT_SETTINGS_GROUP));
  auto const values = readSettings(settings);
  settings.endGroup();
  return values;
}

void saveSettings(QSettings &settings, const SpectroscopySettings &values) {
  settings.setValue(QString::fromStdString(RESTRICT_DATA_PROPERTY), values.restrictInputByName());
  settings.setValue(QString::fromStdString(ERROR_BARS_PROPERTY), values.externalPlotErrorBars());
  settings.setValue(QString::fromStdString(LOAD_HISTORY_PROPERTY), values.loadHistory());
  settings.setValue(QString::fromStdString(FEATURE_FLAGS_PROPERTY), values.developerFeatureFlags());
}

void saveSettings(const SpectroscopySettings &values) {
  QSettings settings;
  settings.beginGroup(QString::fromStdString(INDIRECT_SETTINGS_GROUP));
  saveSettings(settings, values);
  settings.endGroup();
}

bool restrictInputDataByName() { return getSetting(INDIRECT_SETTINGS_GROUP, RESTRICT_DATA_PROPERTY).toBool(); }

bool externalPlotErrorBars() { return getSetting(INDIRECT_SETTINGS_GROUP, ERROR_BARS_PROPERTY).toBool(); }

bool loadHistory() { return getSetting(INDIRECT_SETTINGS_GROUP, LOAD_HISTORY_PROPERTY).toBool(); }

QStringList developerFeatureFlags() {
  return getSetting(INDIRECT_SETTINGS_GROUP, FEATURE_FLAGS_PROPERTY).toStringList();
}

bool hasDevelopmentFlag(std::string const &flag) {
  return developerFeatureFlags().contains(QString::fromStdString(flag));
}

} // namespace MantidQt::CustomInterfaces::SettingsHelper
