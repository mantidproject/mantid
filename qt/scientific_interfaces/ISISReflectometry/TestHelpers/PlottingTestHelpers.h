// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../ISISReflectometry/GUI/Common/PlotOptions.h"
#include "../../ISISReflectometry/GUI/Plotting/model/PlottingWorkspace.h"
#include "../../ISISReflectometry/GUI/Plotting/view/PlottingViewState.h"
#include "../../ISISReflectometry/GUI/Plotting/view/PlottingWorkspaceTreeItemState.h"

#include <cstddef>
#include <ostream>
#include <string>
#include <type_traits>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

inline char const *toString(PlottingWorkspaceTreeItemType type) {
  switch (type) {
  case PlottingWorkspaceTreeItemType::ReductionGroup:
    return "ReductionGroup";
  case PlottingWorkspaceTreeItemType::Run:
    return "Run";
  case PlottingWorkspaceTreeItemType::WorkspaceGroup:
    return "WorkspaceGroup";
  case PlottingWorkspaceTreeItemType::Workspace:
    return "Workspace";
  }
  return "Unknown";
}

inline char const *toString(ReducedWorkspaceOutputType type) {
  switch (type) {
  case ReducedWorkspaceOutputType::None:
    return "None";
  case ReducedWorkspaceOutputType::IvsQ:
    return "IvsQ";
  case ReducedWorkspaceOutputType::IvsLambda:
    return "IvsLambda";
  case ReducedWorkspaceOutputType::IvsQBinned:
    return "IvsQBinned";
  }
  return "Unknown";
}

inline void PrintTo(PlottingWorkspaceTreeItemType type, std::ostream *os) { *os << toString(type); }

inline void PrintTo(ReducedWorkspaceOutputType type, std::ostream *os) { *os << toString(type); }

inline char const *toString(PlottingWorkspaceTreeSelectionMode mode) {
  switch (mode) {
  case PlottingWorkspaceTreeSelectionMode::None:
    return "None";
  case PlottingWorkspaceTreeSelectionMode::Direct:
    return "Direct";
  case PlottingWorkspaceTreeSelectionMode::ParentOnly:
    return "ParentOnly";
  case PlottingWorkspaceTreeSelectionMode::DirectAndParent:
    return "DirectAndParent";
  }
  return "Unknown";
}

inline void PrintTo(PlottingWorkspaceTreeSelectionMode mode, std::ostream *os) { *os << toString(mode); }

inline void PrintTo(PlotOutputType outputType, std::ostream *os) {
  *os << static_cast<std::underlying_type_t<PlotOutputType>>(outputType);
}

inline void printStringVector(std::vector<std::string> const &values, std::ostream *os) {
  *os << "[";
  for (std::size_t index = 0; index < values.size(); ++index) {
    if (index != 0)
      *os << ", ";
    *os << "\"" << values[index] << "\"";
  }
  *os << "]";
}

inline void PrintTo(PlottingWorkspace const &workspace, std::ostream *os) {
  *os << "{workspaceName: \"" << workspace.workspaceName << "\", runNumbers: ";
  printStringVector(workspace.runNumbers, os);
  *os << ", containingWorkspaceGroupName: \"" << workspace.containingWorkspaceGroupName << "\", periodNumber: ";
  if (workspace.periodNumber) {
    *os << *workspace.periodNumber;
  } else {
    *os << "none";
  }
  *os << "}";
}

inline void PrintTo(PlottingWorkspaceTreeItem const &item, std::ostream *os) {
  *os << "{label: \"" << item.label << "\", itemType: ";
  PrintTo(item.itemType, os);
  *os << ", reducedOutputType: ";
  PrintTo(item.reducedOutputType, os);
  *os << ", workspaceName: \"" << item.workspaceName << "\", children: [";
  for (std::size_t index = 0; index < item.children.size(); ++index) {
    if (index != 0)
      *os << ", ";
    PrintTo(item.children[index], os);
  }
  *os << "]}";
}

inline void PrintTo(PlottingWorkspaceTreeItemState const &state, std::ostream *os) {
  *os << "{label: \"" << state.label << "\", itemType: ";
  PrintTo(state.itemType, os);
  *os << ", reducedOutputType: ";
  PrintTo(state.reducedOutputType, os);
  *os << ", workspaceName: \"" << state.workspaceName << "\", children: [";
  for (std::size_t index = 0; index < state.children.size(); ++index) {
    if (index != 0)
      *os << ", ";
    PrintTo(state.children[index], os);
  }
  *os << "], muted: " << state.muted << ", selectionMode: ";
  PrintTo(state.selectionMode, os);
  *os << "}";
}

inline bool operator==(PlotOutputSelection const &lhs, PlotOutputSelection const &rhs) {
  return lhs.outputType == rhs.outputType && lhs.detectorMapXAxis == rhs.detectorMapXAxis &&
         lhs.detectorMapYAxis == rhs.detectorMapYAxis && lhs.alignmentXAxis == rhs.alignmentXAxis &&
         lhs.instrumentName == rhs.instrumentName;
}

inline bool operator!=(PlotOutputSelection const &lhs, PlotOutputSelection const &rhs) { return !(lhs == rhs); }

inline bool operator==(PlotAxis const &lhs, PlotAxis const &rhs) {
  return lhs.label == rhs.label && lhs.unit == rhs.unit && lhs.scale == rhs.scale;
}

inline bool operator!=(PlotAxis const &lhs, PlotAxis const &rhs) { return !(lhs == rhs); }

inline bool operator==(PlotOptions const &lhs, PlotOptions const &rhs) {
  return lhs.outputType == rhs.outputType && lhs.plotStyle == rhs.plotStyle && lhs.layout == rhs.layout &&
         lhs.xAxis == rhs.xAxis && lhs.yAxis == rhs.yAxis && lhs.zAxis == rhs.zAxis &&
         lhs.showErrors == rhs.showErrors && lhs.horizontalMarker == rhs.horizontalMarker &&
         lhs.windowTitle == rhs.windowTitle;
}

inline bool operator!=(PlotOptions const &lhs, PlotOptions const &rhs) { return !(lhs == rhs); }

inline bool operator==(PlotRequest const &lhs, PlotRequest const &rhs) {
  return lhs.workspaces == rhs.workspaces && lhs.options == rhs.options && lhs.parentWidget == rhs.parentWidget &&
         lhs.addToExistingPlot == rhs.addToExistingPlot && lhs.tiledVertically == rhs.tiledVertically;
}

inline bool operator!=(PlotRequest const &lhs, PlotRequest const &rhs) { return !(lhs == rhs); }

inline bool operator==(PlotOutputTypeViewItem const &lhs, PlotOutputTypeViewItem const &rhs) {
  return lhs.outputType == rhs.outputType && lhs.label == rhs.label;
}

inline bool operator!=(PlotOutputTypeViewItem const &lhs, PlotOutputTypeViewItem const &rhs) { return !(lhs == rhs); }

inline bool operator==(PlotOutputControlsState const &lhs, PlotOutputControlsState const &rhs) {
  return lhs.plotPropertiesVisible == rhs.plotPropertiesVisible &&
         lhs.detectorMapControlsVisible == rhs.detectorMapControlsVisible &&
         lhs.alignmentControlsVisible == rhs.alignmentControlsVisible;
}

inline bool operator!=(PlotOutputControlsState const &lhs, PlotOutputControlsState const &rhs) { return !(lhs == rhs); }

inline bool operator==(PlotActionState const &lhs, PlotActionState const &rhs) {
  return lhs.plotIndividualEnabled == rhs.plotIndividualEnabled && lhs.plotOverplotEnabled == rhs.plotOverplotEnabled &&
         lhs.plotTiledEnabled == rhs.plotTiledEnabled &&
         lhs.plotTiledVerticallyEnabled == rhs.plotTiledVerticallyEnabled &&
         lhs.addToExistingPlotEnabled == rhs.addToExistingPlotEnabled &&
         lhs.addToExistingPlotChecked == rhs.addToExistingPlotChecked;
}

inline bool operator!=(PlotActionState const &lhs, PlotActionState const &rhs) { return !(lhs == rhs); }

inline bool operator==(PlottingWorkspaceTreeItem const &lhs, PlottingWorkspaceTreeItem const &rhs) {
  return lhs.label == rhs.label && lhs.itemType == rhs.itemType && lhs.reducedOutputType == rhs.reducedOutputType &&
         lhs.workspaceName == rhs.workspaceName && lhs.children == rhs.children;
}

inline bool operator!=(PlottingWorkspaceTreeItem const &lhs, PlottingWorkspaceTreeItem const &rhs) {
  return !(lhs == rhs);
}

inline bool operator==(PlottingWorkspaceTreeItemState const &lhs, PlottingWorkspaceTreeItemState const &rhs) {
  return lhs.label == rhs.label && lhs.itemType == rhs.itemType && lhs.reducedOutputType == rhs.reducedOutputType &&
         lhs.workspaceName == rhs.workspaceName && lhs.children == rhs.children && lhs.muted == rhs.muted &&
         lhs.selectionMode == rhs.selectionMode;
}

inline bool operator!=(PlottingWorkspaceTreeItemState const &lhs, PlottingWorkspaceTreeItemState const &rhs) {
  return !(lhs == rhs);
}

inline bool operator==(PlottingWorkspace const &lhs, PlottingWorkspace const &rhs) {
  return lhs.workspaceName == rhs.workspaceName && lhs.runNumbers == rhs.runNumbers &&
         lhs.containingWorkspaceGroupName == rhs.containingWorkspaceGroupName && lhs.periodNumber == rhs.periodNumber;
}

inline bool operator!=(PlottingWorkspace const &lhs, PlottingWorkspace const &rhs) { return !(lhs == rhs); }

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
