// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "DllConfig.h"

#include <string>

#include <QStringList>

namespace MantidQt {
namespace CustomInterfaces {
namespace IndirectSettingsHelper {

MANTIDQT_INELASTIC_DLL void setRestrictInputDataByName(bool restricted);
MANTIDQT_INELASTIC_DLL void setExternalPlotErrorBars(bool errorBars);
MANTIDQT_INELASTIC_DLL void setDeveloperFeatureFlags(QStringList const &flags);

MANTIDQT_INELASTIC_DLL bool restrictInputDataByName();
MANTIDQT_INELASTIC_DLL bool externalPlotErrorBars();
MANTIDQT_INELASTIC_DLL QStringList developerFeatureFlags();
MANTIDQT_INELASTIC_DLL bool hasDevelopmentFlag(std::string const &flag);

} // namespace IndirectSettingsHelper
} // namespace CustomInterfaces
} // namespace MantidQt
