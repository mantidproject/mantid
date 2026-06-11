// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Common/PlotOptions.h"
#include "GUI/Plotting/model/PlottingWorkspace.h"
#include "MantidKernel/System.h"
#include <QString>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/// Capability flags used to enable plotting controls for an output type.
struct PlotOutputTypeCapabilities {
  bool supportsOverplot;
  bool supportsAddToExistingPlot;
  bool excludesPostprocessedGroupOutputs;
  bool requiresWorkspaceGroupsForMultiPlot;
};

/// Describes how one plot output type behaves in the plotting tab.
class MANTIDQT_ISISREFLECTOMETRY_DLL PlotOutputTypeProperties {
public:
  /// Create properties from selectable tree item types, included workspace outputs and capabilities.
  PlotOutputTypeProperties(PlotOutputType plotOutputType,
                           std::vector<PlottingWorkspaceTreeItemType> selectableItemTypes,
                           std::vector<PlottingWorkspaceOutputType> includedWorkspaceOutputTypes,
                           PlotOutputTypeCapabilities capabilities);

  /// Return the label shown in the plot output selector.
  QString const &displayName() const;
  /// Return true if tree items of this type may be selected.
  bool allowsItemType(PlottingWorkspaceTreeItemType itemType) const;
  /// Return true if workspaces with this reduced output type may be selected.
  bool includesWorkspaceOutput(PlottingWorkspaceOutputType outputType) const;
  /// Return true if the output can be plotted over compatible axes.
  bool supportsOverplot() const;
  /// Return true if the output can be added to an existing figure.
  bool supportsAddToExistingPlot() const;
  /// Return true if stitched/group-level reduced outputs should be muted.
  bool excludesPostprocessedGroupOutputs() const;
  /// Return true if multi-plot actions require selecting workspace groups.
  bool requiresWorkspaceGroupsForMultiPlot() const;
  /// Return true if detector-map axis controls should be displayed.
  bool showsDetectorMapProperties() const;
  /// Return true if alignment axis controls should be displayed.
  bool showsAlignmentProperties() const;
  /// Return true if any output-specific property controls should be displayed.
  bool showsPlotProperties() const;

private:
  PlotOutputType m_plotOutputType;
  std::vector<PlottingWorkspaceTreeItemType> m_selectableItemTypes;
  std::vector<PlottingWorkspaceOutputType> m_includedWorkspaceOutputTypes;
  PlotOutputTypeCapabilities m_capabilities;
};

/// Return the configured plotting-tab behavior for an output type.
MANTIDQT_ISISREFLECTOMETRY_DLL PlotOutputTypeProperties const &plotOutputTypeProperties(PlotOutputType outputType);

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
