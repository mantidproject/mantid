// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSettingsHelper.h"

#include <QSettings>
#include <QString>
#include <QVariant>

namespace {

template <typename T> void setSetting(std::string const &settingGroup, std::string const &settingName, T const &value) {
  QSettings settings;
  settings.beginGroup(QString::fromStdString(settingGroup));
  settings.setValue(QString::fromStdString(settingName), value);
  settings.endGroup();
}

QVariant getSetting(std::string const &settingGroup, std::string const &settingName) {
  QSettings settings;
  settings.beginGroup(QString::fromStdString(settingGroup));
  auto const settingValue = settings.value(QString::fromStdString(settingName));
  settings.endGroup();

  return settingValue;
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IndirectSettingsHelper {

static std::string const INDIRECT_SETTINGS_GROUP("Indirect Settings");
static std::string const RESTRICT_DATA_PROPERTY("restrict-input-by-name");
static std::string const ERROR_BARS_PROPERTY("plot-error-bars-external");

bool restrictInputDataByName() { return getSetting(INDIRECT_SETTINGS_GROUP, RESTRICT_DATA_PROPERTY).toBool(); }

bool externalPlotErrorBars() { return getSetting(INDIRECT_SETTINGS_GROUP, ERROR_BARS_PROPERTY).toBool(); }

void setRestrictInputDataByName(bool restricted) {
  setSetting(INDIRECT_SETTINGS_GROUP, RESTRICT_DATA_PROPERTY, restricted);
}

void setExternalPlotErrorBars(bool errorBars) { setSetting(INDIRECT_SETTINGS_GROUP, ERROR_BARS_PROPERTY, errorBars); }

} // namespace IndirectSettingsHelper
} // namespace CustomInterfaces
} // namespace MantidQt
