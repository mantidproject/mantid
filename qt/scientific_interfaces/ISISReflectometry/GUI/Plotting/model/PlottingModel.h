// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IPlottingModel.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/// Creates or selects the workspaces required by each plotting-tab output type.
class MANTIDQT_ISISREFLECTOMETRY_DLL PlottingModel : public IPlottingModel {
public:
  /// Return existing or generated workspace names to plot for the selected output type.
  std::vector<std::string> workspacesForPlotting(std::vector<PlottingWorkspaceSelection> const &workspaces,
                                                 PlotOutputSelection const &selection) const override;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
