// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/// Node type used by the plotting-tab workspace tree.
/// Ordered from broadest to most specific: selection scopes allow nodes at the same or broader level.
enum class PlottingWorkspaceTreeItemType { ReductionGroup, Run, WorkspaceGroup, Workspace };

/// The tree level that provides one input unit for a plot output.
using PlotSelectionScope = PlottingWorkspaceTreeItemType;

/// Row indices from the root to a workspace tree node. Empty denotes the root.
using PlottingWorkspaceTreePath = std::vector<int>;

/// The reduction stage that produced a workspace or workspace group.
enum class PlottingWorkspaceOrigin { None, RunOutput, PostprocessedGroupOutput };

/// Root-to-item path and ADS name of the tree item associated with one selection scope.
struct PlottingWorkspaceScopeItem {
  PlottingWorkspaceTreePath path;
  /// Empty for Run and ReductionGroup nodes, which do not represent ADS workspaces.
  std::string workspaceName;
};

/// Scope paths and output origin recorded during construction, shared by tree items and plotting inputs.
struct PlottingWorkspaceContext {
  /// Contains this item and its ancestors, keyed by tree level.
  std::map<PlotSelectionScope, PlottingWorkspaceScopeItem> scopeItems;
  PlottingWorkspaceOrigin origin{PlottingWorkspaceOrigin::None};
};

/// Reduction output represented by a workspace tree item.
enum class ReducedWorkspaceOutputType { None, IvsQ, IvsLambda, IvsQBinned };

/// Domain data for one node in the plotting-tab workspace tree.
struct MANTIDQT_ISISREFLECTOMETRY_DLL PlottingWorkspaceTreeItem {
  std::string label;
  PlottingWorkspaceTreeItemType itemType;
  ReducedWorkspaceOutputType reducedOutputType;
  std::string workspaceName;
  std::vector<PlottingWorkspaceTreeItem> children;
  PlottingWorkspaceContext context{};
};

/// Workspace from which the plotting model can produce one or more plot-ready workspaces.
struct MANTIDQT_ISISREFLECTOMETRY_DLL PlottingWorkspace {
  std::string workspaceName;
  std::vector<std::string> runNumbers;
  std::optional<int> periodNumber;
  PlottingWorkspaceContext context{};
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
