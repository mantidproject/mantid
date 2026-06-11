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
#include <unordered_map>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class RunsTable;

/// Builds the presenter-side workspace tree and resolves selected workspace names.
class MANTIDQT_ISISREFLECTOMETRY_DLL PlottingWorkspaceTree {
public:
  /// Build tree items from successful reduction groups and rows in the runs table.
  std::vector<PlottingWorkspaceTreeItem> makeWorkspaceItems(RunsTable const &runsTable);
  /// Return plot metadata for selected workspace names that are still known to the tree.
  std::vector<PlottingWorkspaceSelection> selectedWorkspacesFor(std::vector<std::string> const &workspaceNames) const;

private:
  std::unordered_map<std::string, PlottingWorkspaceSelection> m_workspaceSelectionsByName;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
