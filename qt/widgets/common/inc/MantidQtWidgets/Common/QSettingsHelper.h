// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_QSETTINGSHELPER_H
#define MANTID_QSETTINGSHELPER_H

#include <map>
#include <string>
#include <typeinfo>
#include <QMetaType>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace MantidQt {
namespace MantidWidgets {
namespace QSettingsHelper {

/* Load an individual setting from disk */
template <typename T>
T getSetting(std::string const &settingGroup, std::string const &settingName) {
  QSettings settings;
  settings.beginGroup(QString::fromStdString(settingGroup));
  auto const settingValue = settings.value(QString::fromStdString(settingName));
  settings.endGroup();

  return settingValue.value<T>();
}

/* Load a map of settings with the same type */
template <typename T>
std::map<std::string, T> getSettingsAsMap(std::string const &settingGroup) {
  std::map<std::string, T> settingsMap;
  QSettings settings;
  settings.beginGroup(QString::fromStdString(settingGroup));
  QStringList settingNames = settings.allKeys();
  std::string templateTypeName = typeid(T).name();
  for (auto &settingName : settingNames) {
    std::string settingTypeName;
    if (settingName.endsWith("/type")) {
      settingTypeName =
          settings.value(settingName).toString().toStdString();
      if (settingTypeName == templateTypeName) {
        auto settingValueName =
            settingName.replace(QString("/type"), QString("/value"));
        auto setting = settings.value(settingValueName);
        auto strippedSettingName = settingName.remove(QString("/value"));
        settingsMap[strippedSettingName.toStdString()] = setting.value<T>();
      }
    }
  }
  settings.endGroup();

  return settingsMap;
}

template <typename T>
void setSetting(std::string const &settingGroup, std::string const &settingName,
                T const &value) {
  QSettings settings;
  settings.beginGroup(QString::fromStdString(settingGroup));
  settings.setValue(QString::fromStdString(settingName).append("/value"), value);
  settings.setValue(QString::fromStdString(settingName).append("/type"), typeid(value).name());
  settings.endGroup();
}

} // namespace QSettingsHelper
} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_INDIRECTSETTINGSHELPER_H */
