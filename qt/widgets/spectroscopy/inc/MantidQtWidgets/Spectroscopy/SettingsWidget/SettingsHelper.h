// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "../DllConfig.h"

#include <string>

#include <QStringList>

namespace MantidQt {
namespace CustomInterfaces {
namespace SettingsHelper {

MANTID_SPECTROSCOPY_DLL void setRestrictInputDataByName(bool restricted);
MANTID_SPECTROSCOPY_DLL void setExternalPlotErrorBars(bool errorBars);
MANTID_SPECTROSCOPY_DLL void setLoadHistory(bool loadHistory);
MANTID_SPECTROSCOPY_DLL void setDeveloperFeatureFlags(QStringList const &flags);

MANTID_SPECTROSCOPY_DLL bool restrictInputDataByName();
MANTID_SPECTROSCOPY_DLL bool externalPlotErrorBars();
MANTID_SPECTROSCOPY_DLL bool loadHistory();
MANTID_SPECTROSCOPY_DLL QStringList developerFeatureFlags();
MANTID_SPECTROSCOPY_DLL bool hasDevelopmentFlag(std::string const &flag);
} // namespace SettingsHelper
} // namespace CustomInterfaces
} // namespace MantidQt
