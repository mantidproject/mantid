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

template <typename T>
T getSetting(std::string const& settingGroup, std::string const& settingName) {
  return getSettingAsQVariant(std::string const &settingGroup,
                              std::string const &settingName)
      .value<T>();
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
