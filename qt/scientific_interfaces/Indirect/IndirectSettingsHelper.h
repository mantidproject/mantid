// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_INDIRECTSETTINGSHELPER_H
#define MANTID_INDIRECTSETTINGSHELPER_H

namespace MantidQt {
namespace CustomInterfaces {
namespace IndirectSettingsHelper {

void setRestrictInputDataByName(bool restricted);
void setExternalPlotErrorBars(bool errorBars);

bool restrictInputDataByName();
bool externalPlotErrorBars();

} // namespace IndirectSettingsHelper
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_INDIRECTSETTINGSHELPER_H */
