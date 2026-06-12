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
enum class PlottingWorkspaceTreeItemType { Group, Run, WorkspaceGroup, Workspace };

/// Reduced workspace output represented by a selectable workspace tree item.
enum class PlottingWorkspaceOutputType { None, IvsQ, IvsLambda, IvsQBinned };

/// Display and metadata for one node in the plotting-tab workspace tree.
struct MANTIDQT_ISISREFLECTOMETRY_DLL PlottingWorkspaceTreeItem {
  std::string label;
  PlottingWorkspaceTreeItemType itemType;
  PlottingWorkspaceOutputType outputType;
  std::string groupName;
  std::vector<std::string> runNumbers;
  std::string workspaceName;
  std::vector<PlottingWorkspaceTreeItem> children;
};

/// Metadata required by the model to produce plot-ready workspaces from a tree selection.
struct MANTIDQT_ISISREFLECTOMETRY_DLL PlottingWorkspaceSelection {
  std::string workspaceName;
  PlottingWorkspaceOutputType outputType;
  std::string groupName;
  std::vector<std::string> runNumbers;
  std::string workspaceGroupName;
  std::optional<int> period;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
