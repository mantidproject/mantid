// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QMetaType>
#include <QSettings>
#include <QString>
#include <QVariant>
#include <map>
#include <string>
#include <typeinfo>

namespace MantidQt {
namespace MantidWidgets {
namespace QSettingsHelper {

namespace Detail {
inline QString qualifiedName(std::string const &settingGroup, QString const &settingName) {
  auto group = QString::fromStdString(settingGroup);
  if (!group.isEmpty() && !group.endsWith('/'))
    group.append('/');
  return group.append(settingName);
}
} // namespace Detail

/** Loads an individual setting from disk
 *
 * @param settings The settings store to read from
 * @param settingGroup The name of the setting group
 * @param settingName The name of the setting
 * @return The value stored for the requested setting
 *
 */
template <typename T>
T getSetting(QSettings const &settings, std::string const &settingGroup, std::string const &settingName) {
  return settings.value(Detail::qualifiedName(settingGroup, QString::fromStdString(settingName))).template value<T>();
}

template <typename T> T getSetting(std::string const &settingGroup, std::string const &settingName) {
  QSettings settings;
  return getSetting<T>(settings, settingGroup, settingName);
}

/** Loads a map of settings with the same type. This comparison is required
 *  as QVariant types are not properly encoded in ini files
 *
 * @param settings The settings store to read from
 * @param settingGroup The name of the setting group
 * @return A map of the values stored for all settings matching the given type
 *
 */
template <typename T>
std::map<std::string, T> getSettingsAsMap(QSettings const &settings, std::string const &settingGroup) {
  std::map<std::string, T> settingsMap;
  auto groupPrefix = QString::fromStdString(settingGroup);
  if (!groupPrefix.isEmpty() && !groupPrefix.endsWith('/'))
    groupPrefix.append('/');
  std::string templateTypeName = typeid(T).name();
  for (auto const &qualifiedName : settings.allKeys()) {
    if (qualifiedName.startsWith(groupPrefix) && qualifiedName.endsWith("/type")) {
      std::string settingTypeName = settings.value(qualifiedName).toString().toStdString();
      if (settingTypeName == templateTypeName) {
        auto settingName = qualifiedName.mid(groupPrefix.size());
        settingName.chop(5);
        auto setting = settings.value(groupPrefix + settingName + "/value");
        settingsMap[settingName.toStdString()] = setting.template value<T>();
      }
    }
  }
  return settingsMap;
}

template <typename T> std::map<std::string, T> getSettingsAsMap(std::string const &settingGroup) {
  QSettings settings;
  return getSettingsAsMap<T>(settings, settingGroup);
}

/** Sets the value of a specified setting
 *
 * @param settings The settings store to write to
 * @param settingGroup The name of the setting group
 * @param settingName The name of the setting
 * @param value The value of the named setting
 */
template <typename T>
void setSetting(QSettings &settings, std::string const &settingGroup, std::string const &settingName, T const &value) {
  auto const qualifiedName = Detail::qualifiedName(settingGroup, QString::fromStdString(settingName));
  settings.setValue(qualifiedName + "/value", value);
  settings.setValue(qualifiedName + "/type", typeid(value).name());
}

template <typename T> void setSetting(std::string const &settingGroup, std::string const &settingName, T const &value) {
  QSettings settings;
  setSetting(settings, settingGroup, settingName, value);
}

} // namespace QSettingsHelper
} // namespace MantidWidgets
} // namespace MantidQt
