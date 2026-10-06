// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"

#include <string>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

struct MANTIDQT_ISISREFLECTOMETRY_DLL InstrumentSettingsViewState {
  bool detectorCorrectionControlsVisible;
  bool specularPixelVisible;
};

MANTIDQT_ISISREFLECTOMETRY_DLL InstrumentSettingsViewState
instrumentSettingsViewState(std::string const &instrumentName);

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
