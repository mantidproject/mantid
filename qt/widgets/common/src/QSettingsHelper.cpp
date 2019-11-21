// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/QSettingsHelper.h"

#include <QSettings>
#include <QString>

namespace {

QVariant getSettingAsQVariant(std::string const &settingGroup,
                    std::string const &settingName) {
  QSettings settings;
  settings.beginGroup(QString::fromStdString(settingGroup));
  auto const settingValue = settings.value(QString::fromStdString(settingName));
  settings.endGroup();

  return settingValue;
}

} // namespace

namespace MantidQt {
namespace MantidWidgets {
namespace QSettingsHelper {

/* Load an individual setting from disk */
template <typename T>
T getSetting(std::string const& settingGroup, std::string const& settingName) {
  return getSettingAsQVariant(std::string const &settingGroup,
                              std::string const &settingName)
      .value<T>();
}

/* Load a map of settings with the same type */
template <typename T>
std::map<std::string, T> getSettingsAsMap(std::string const &settingGroup) {
  QSettings settings;
  settings.beginGroup(QString::fromStdString(settingGroup));
  QStringList settingNames = settings.childKeys();
  for (auto &settingName : settingNames)
    if (settings.value(settingName).type() == T) {
      auto settingsMap[settingName.toStdString()] = settings.value<T>(settingName);
    }
  settings.endGroup();

  return settingsMap;
}

template <typename T>
void setSetting(std::string const &settingGroup, std::string const &settingName,
                T const &value) {
  QSettings settings;
  settings.beginGroup(QString::fromStdString(settingGroup));
  settings.setValue(QString::fromStdString(settingName), value);
  settings.endGroup();
}

} // namespace QSettingsHelper
} // namespace MantidWidgets
} // namespace MantidQt
