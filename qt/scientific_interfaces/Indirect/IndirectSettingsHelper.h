// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "DllConfig.h"
namespace MantidQt {
namespace CustomInterfaces {
namespace IndirectSettingsHelper {

MANTIDQT_INDIRECT_DLL void setRestrictInputDataByName(bool restricted);
MANTIDQT_INDIRECT_DLL void setExternalPlotErrorBars(bool errorBars);

MANTIDQT_INDIRECT_DLL bool restrictInputDataByName();
MANTIDQT_INDIRECT_DLL bool externalPlotErrorBars();

} // namespace IndirectSettingsHelper
} // namespace CustomInterfaces
} // namespace MantidQt
