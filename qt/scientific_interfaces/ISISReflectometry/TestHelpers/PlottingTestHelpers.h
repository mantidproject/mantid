// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../ISISReflectometry/GUI/Common/PlotOptions.h"
#include "../../ISISReflectometry/GUI/Plotting/model/PlottingWorkspace.h"

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

inline char const *toString(PlottingWorkspaceTreeItemType type) {
  switch (type) {
  case PlottingWorkspaceTreeItemType::Group:
    return "Group";
  case PlottingWorkspaceTreeItemType::Run:
    return "Run";
  case PlottingWorkspaceTreeItemType::WorkspaceGroup:
    return "WorkspaceGroup";
  case PlottingWorkspaceTreeItemType::Workspace:
    return "Workspace";
  }
  return "Unknown";
}

inline char const *toString(PlottingWorkspaceOutputType type) {
  switch (type) {
  case PlottingWorkspaceOutputType::None:
    return "None";
  case PlottingWorkspaceOutputType::IvsQ:
    return "IvsQ";
  case PlottingWorkspaceOutputType::IvsLambda:
    return "IvsLambda";
  case PlottingWorkspaceOutputType::IvsQBinned:
    return "IvsQBinned";
  }
  return "Unknown";
}

inline void PrintTo(PlottingWorkspaceTreeItemType type, std::ostream *os) { *os << toString(type); }

inline void PrintTo(PlottingWorkspaceOutputType type, std::ostream *os) { *os << toString(type); }

inline void printStringVector(std::vector<std::string> const &values, std::ostream *os) {
  *os << "[";
  for (std::size_t index = 0; index < values.size(); ++index) {
    if (index != 0)
      *os << ", ";
    *os << "\"" << values[index] << "\"";
  }
  *os << "]";
}

inline void PrintTo(PlottingWorkspaceTreeItem const &item, std::ostream *os) {
  *os << "{label: \"" << item.label << "\", itemType: ";
  PrintTo(item.itemType, os);
  *os << ", outputType: ";
  PrintTo(item.outputType, os);
  *os << ", groupName: \"" << item.groupName << "\", runNumbers: ";
  printStringVector(item.runNumbers, os);
  *os << ", workspaceName: \"" << item.workspaceName << "\", children: [";
  for (std::size_t index = 0; index < item.children.size(); ++index) {
    if (index != 0)
      *os << ", ";
    PrintTo(item.children[index], os);
  }
  *os << "]}";
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

inline bool operator==(PlottingWorkspaceTreeItem const &lhs, PlottingWorkspaceTreeItem const &rhs) {
  return lhs.label == rhs.label && lhs.itemType == rhs.itemType && lhs.outputType == rhs.outputType &&
         lhs.groupName == rhs.groupName && lhs.runNumbers == rhs.runNumbers && lhs.workspaceName == rhs.workspaceName &&
         lhs.children == rhs.children;
}

inline bool operator!=(PlottingWorkspaceTreeItem const &lhs, PlottingWorkspaceTreeItem const &rhs) {
  return !(lhs == rhs);
}

inline bool operator==(PlottingWorkspaceSelection const &lhs, PlottingWorkspaceSelection const &rhs) {
  return lhs.workspaceName == rhs.workspaceName && lhs.outputType == rhs.outputType && lhs.groupName == rhs.groupName &&
         lhs.runNumbers == rhs.runNumbers && lhs.workspaceGroupName == rhs.workspaceGroupName &&
         lhs.period == rhs.period;
}

inline bool operator!=(PlottingWorkspaceSelection const &lhs, PlottingWorkspaceSelection const &rhs) {
  return !(lhs == rhs);
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
