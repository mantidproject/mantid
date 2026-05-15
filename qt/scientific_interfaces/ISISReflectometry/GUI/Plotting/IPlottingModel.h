// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "GUI/Common/PlotOptions.h"
#include "PlottingWorkspace.h"

#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL IPlottingModel {
public:
  virtual ~IPlottingModel() = default;
  virtual std::vector<std::string> workspacesForPlotting(std::vector<PlottingWorkspaceSelection> const &workspaces,
                                                         PlotOutputOptions const &options) const = 0;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
