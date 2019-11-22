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
  QStringList settingNames = settings.childKeys();
  static const char *templateTypeName = typeid(T).name();
  for (auto &settingName : settingNames) {
    auto setting = settings.value(settingName);
    if (QVariant::typeToName(setting.type()) ==
        templateTypeName) {
      settingsMap[settingName.toStdString()] =
          setting.value<T>();
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
  settings.setValue(QString::fromStdString(settingName), value);
  settings.endGroup();
}

} // namespace QSettingsHelper
} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_INDIRECTSETTINGSHELPER_H */
