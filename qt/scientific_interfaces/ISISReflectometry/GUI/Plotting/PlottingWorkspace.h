// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"

#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

enum class PlottingWorkspaceTreeItemType { Group, Run, WorkspaceGroup, Workspace };

enum class PlottingWorkspaceOutputType { None, IvsQ, IvsLambda, IvsQBinned };

struct MANTIDQT_ISISREFLECTOMETRY_DLL PlottingWorkspaceTreeItem {
  std::string label;
  PlottingWorkspaceTreeItemType itemType;
  PlottingWorkspaceOutputType outputType;
  std::string groupName;
  std::vector<std::string> runNumbers;
  std::string workspaceName;
  std::vector<PlottingWorkspaceTreeItem> children;
};

struct MANTIDQT_ISISREFLECTOMETRY_DLL PlottingWorkspaceSelection {
  std::string workspaceName;
  PlottingWorkspaceOutputType outputType;
  std::string groupName;
  std::vector<std::string> runNumbers;
  std::string workspaceGroupName;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
