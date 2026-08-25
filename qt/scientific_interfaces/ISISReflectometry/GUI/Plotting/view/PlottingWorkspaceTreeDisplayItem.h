// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "GUI/Plotting/model/PlottingWorkspace.h"

#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/// Selection behaviour requested by the presenter for one displayed workspace tree row.
enum class PlottingWorkspaceTreeSelectionMode { None, Direct, ParentOnly, DirectAndParent };

/// View-facing display state for one node in the plotting-tab workspace tree.
struct MANTIDQT_ISISREFLECTOMETRY_DLL PlottingWorkspaceTreeDisplayItem {
  std::string label;
  PlottingWorkspaceTreeItemType itemType;
  ReducedWorkspaceOutputType reducedOutputType;
  std::string workspaceName;
  std::vector<PlottingWorkspaceTreeDisplayItem> children;
  bool muted{false};
  PlottingWorkspaceTreeSelectionMode selectionMode{PlottingWorkspaceTreeSelectionMode::DirectAndParent};
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
