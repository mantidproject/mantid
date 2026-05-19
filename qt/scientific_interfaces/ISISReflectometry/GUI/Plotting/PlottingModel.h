// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IPlottingModel.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL PlottingModel : public IPlottingModel {
public:
  std::vector<std::string> workspacesForPlotting(std::vector<PlottingWorkspaceSelection> const &workspaces,
                                                 PlotOutputOptions const &options) const override;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
