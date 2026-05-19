// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Common/PlotOptions.h"
#include "../../../ISISReflectometry/GUI/Plotting/PlottingWorkspace.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

inline bool operator==(PlotOutputOptions const &lhs, PlotOutputOptions const &rhs) {
  return lhs.outputType == rhs.outputType && lhs.detectorMapXAxis == rhs.detectorMapXAxis &&
         lhs.detectorMapYAxis == rhs.detectorMapYAxis && lhs.alignmentXAxis == rhs.alignmentXAxis &&
         lhs.instrumentName == rhs.instrumentName;
}

inline bool operator!=(PlotOutputOptions const &lhs, PlotOutputOptions const &rhs) { return !(lhs == rhs); }

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
  return lhs.workspaces == rhs.workspaces && lhs.options == rhs.options;
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
