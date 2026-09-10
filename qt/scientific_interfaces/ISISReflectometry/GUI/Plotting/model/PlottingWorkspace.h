// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"

#include <optional>
#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/// Node type used by the plotting-tab workspace tree.
enum class PlottingWorkspaceTreeItemType { ReductionGroup, Run, WorkspaceGroup, Workspace };

/// Reduction output represented by a workspace tree item.
enum class ReducedWorkspaceOutputType { None, IvsQ, IvsLambda, IvsQBinned };

/// Domain data for one node in the plotting-tab workspace tree.
struct MANTIDQT_ISISREFLECTOMETRY_DLL PlottingWorkspaceTreeItem {
  std::string label;
  PlottingWorkspaceTreeItemType itemType;
  ReducedWorkspaceOutputType reducedOutputType;
  std::string workspaceName;
  std::vector<PlottingWorkspaceTreeItem> children;
};

/// Workspace from which the plotting model can produce one or more plot-ready workspaces.
struct MANTIDQT_ISISREFLECTOMETRY_DLL PlottingWorkspace {
  std::string workspaceName;
  std::vector<std::string> runNumbers;
  std::string containingWorkspaceGroupName;
  std::optional<int> periodNumber;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
