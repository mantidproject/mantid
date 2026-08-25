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

/// Owns the plotting workspaces and hierarchy derived from the runs table.
class MANTIDQT_ISISREFLECTOMETRY_DLL PlottingWorkspaceTree {
public:
  /// Replace the tree with plotting workspaces from successful reductions in the runs table.
  void rebuild(RunsTable const &runsTable);
  /// Return the current plotting workspace hierarchy.
  std::vector<PlottingWorkspaceTreeItem> const &items() const;
  /// Return the known plotting workspaces matching the supplied ADS workspace names.
  std::vector<PlottingWorkspace> plottingWorkspacesForNames(std::vector<std::string> const &workspaceNames) const;

private:
  std::vector<PlottingWorkspaceTreeItem> m_items;
  std::unordered_map<std::string, PlottingWorkspace> m_plottingWorkspacesByName;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
