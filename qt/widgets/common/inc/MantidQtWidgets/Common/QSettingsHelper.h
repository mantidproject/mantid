// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_QSETTINGSHELPER_H
#define MANTID_QSETTINGSHELPER_H

#include <string>

namespace MantidQt {
namespace MantidWidgets {
namespace QSettingsHelper {

template <typename T>
T getSetting(std::string const &settingGroup, std::string const &settingName);

template <typename T>
void setSetting(std::string const &settingGroup, std::string const &settingName,
                T const &value);

} // namespace QSettingsHelper
} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_INDIRECTSETTINGSHELPER_H */
