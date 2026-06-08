// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Common/PlotOptions.h"
#include "GUI/Plotting/PlottingWorkspace.h"
#include "MantidKernel/System.h"
#include <QString>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

struct PlotOutputTypeCapabilities {
  bool supportsOverplot;
  bool supportsAddToExistingPlot;
  bool excludesPostprocessedGroupOutputs;
  bool requiresWorkspaceGroupsForMultiPlot;
};

struct PlotOutputTypePropertyControls {
  bool detectorMap;
  bool alignment;
};

class MANTIDQT_ISISREFLECTOMETRY_DLL PlotOutputTypeProperties {
public:
  PlotOutputTypeProperties(QString displayName, std::vector<PlottingWorkspaceTreeItemType> selectableItemTypes,
                           std::vector<PlottingWorkspaceOutputType> includedWorkspaceOutputTypes,
                           PlotOutputTypeCapabilities capabilities, PlotOutputTypePropertyControls propertyControls);

  QString const &displayName() const;
  bool allowsItemType(PlottingWorkspaceTreeItemType itemType) const;
  bool includesWorkspaceOutput(PlottingWorkspaceOutputType outputType) const;
  bool supportsOverplot() const;
  bool supportsAddToExistingPlot() const;
  bool excludesPostprocessedGroupOutputs() const;
  bool requiresWorkspaceGroupsForMultiPlot() const;
  bool showsDetectorMapProperties() const;
  bool showsAlignmentProperties() const;
  bool showsPlotProperties() const;

private:
  QString m_displayName;
  std::vector<PlottingWorkspaceTreeItemType> m_selectableItemTypes;
  std::vector<PlottingWorkspaceOutputType> m_includedWorkspaceOutputTypes;
  PlotOutputTypeCapabilities m_capabilities;
  PlotOutputTypePropertyControls m_propertyControls;
};

MANTIDQT_ISISREFLECTOMETRY_DLL PlotOutputTypeProperties const &plotOutputTypeProperties(PlotOutputType outputType);

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
