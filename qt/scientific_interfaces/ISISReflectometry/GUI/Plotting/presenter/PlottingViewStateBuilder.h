// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "GUI/Common/PlotOptions.h"
#include "GUI/Plotting/view/PlottingViewState.h"

#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/// Builds view state for plotting-tab controls from presenter state.
class MANTIDQT_ISISREFLECTOMETRY_DLL PlottingViewStateBuilder {
public:
  /// Return labelled output types for display in the output selector.
  std::vector<PlotOutputTypeViewItem> outputTypeViewItems(std::vector<PlotOutputType> const &outputTypes) const;
  /// Return output-type specific control visibility.
  PlotOutputControlsState outputControlsState(PlotOutputType outputType) const;
  /// Return enabled and checked state for plotting action controls.
  PlotActionState plotActionState(bool outputSelectionEnabled, size_t selectedWorkspaceCount,
                                  size_t selectedWorkspaceGroupCount, PlotOutputType outputType, bool addToExistingPlot,
                                  bool hasActiveReflectometryFigure, bool activePlotOverplotCompatible) const;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
