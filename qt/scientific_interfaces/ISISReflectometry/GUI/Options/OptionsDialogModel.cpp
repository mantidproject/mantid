// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "OptionsDialogModel.h"
#include "MantidQtWidgets/Common/QSettingsHelper.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

using namespace MantidQt::MantidWidgets::QSettingsHelper;

OptionsDialogModel::OptionsDialogModel() {}

/** Applies default options to specified settings
 *
 * @param boolOptions A map to store bool options
 * @param intOptions A map to store int options
 * @return void
 *
 */
void OptionsDialogModel::applyDefaultOptions(
    std::map<std::string, bool> &boolOptions,
    std::map<std::string, int> &intOptions) {
  boolOptions["WarnProcessAll"] = true;
  boolOptions["WarnDiscardChanges"] = true;
  boolOptions["WarnProcessPartialGroup"] = true;
  boolOptions["Round"] = false;
  intOptions["RoundPrecision"] = 3;
}

/** Loads the settings saved by the user
 *
 * @param boolOptions A map to store bool options
 * @param intOptions A map to store int options
 * @return void
 *
 */
void OptionsDialogModel::loadSettings(std::map<std::string, bool> &boolOptions,
                                      std::map<std::string, int> &intOptions) {
  boolOptions = getSettingsAsMap<bool>(REFLECTOMETRY_SETTINGS_GROUP);
  intOptions = getSettingsAsMap<int>(REFLECTOMETRY_SETTINGS_GROUP);
}

/** Saves the settings specified by the user
 *
 * @param boolOptions A map containing values for bool options
 * @param intOptions A map containing values for int options
 * @return void
 *
 */
void OptionsDialogModel::saveSettings(
    const std::map<std::string, bool> &boolOptions,
    const std::map<std::string, int> &intOptions) {
  for (const auto &boolOption : boolOptions)
    setSetting(REFLECTOMETRY_SETTINGS_GROUP, boolOption.first,
               boolOption.second);
  for (const auto &intOption : intOptions)
    setSetting(REFLECTOMETRY_SETTINGS_GROUP, intOption.first, intOption.second);
}

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
