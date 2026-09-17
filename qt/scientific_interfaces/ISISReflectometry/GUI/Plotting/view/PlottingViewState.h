// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "GUI/Common/PlotOptions.h"
#include "GUI/Plotting/model/PlottingWorkspace.h"

#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/// One plot output type option to display in the output selector.
struct MANTIDQT_ISISREFLECTOMETRY_DLL PlotOutputTypeViewItem {
  PlotOutputType outputType;
  std::string label;
};

/// Visibility state for output-type specific controls in the plotting tab.
struct MANTIDQT_ISISREFLECTOMETRY_DLL PlotOutputControlsState {
  bool plotPropertiesVisible{false};
  bool detectorMapControlsVisible{false};
  bool alignmentControlsVisible{false};
};

/// Enabled and checked state for plotting action controls.
struct MANTIDQT_ISISREFLECTOMETRY_DLL PlotActionState {
  bool plotIndividualEnabled{false};
  bool plotOverplotEnabled{false};
  bool plotTiledEnabled{false};
  bool plotTiledVerticallyEnabled{false};
  bool addToExistingPlotEnabled{false};
  bool addToExistingPlotChecked{false};
};

/// Selection behaviour requested by the presenter for one displayed workspace tree row.
enum class PlottingWorkspaceTreeSelectionMode { None, Direct, ParentOnly, DirectAndParent };

/// View-facing state for one node in the plotting workspace tree.
struct MANTIDQT_ISISREFLECTOMETRY_DLL PlottingWorkspaceTreeItemState {
  std::string label;
  PlottingWorkspaceTreeItemType itemType;
  ReducedWorkspaceOutputType reducedOutputType;
  std::string workspaceName;
  std::vector<PlottingWorkspaceTreeItemState> children;
  bool muted{false};
  PlottingWorkspaceTreeSelectionMode selectionMode{PlottingWorkspaceTreeSelectionMode::DirectAndParent};
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
