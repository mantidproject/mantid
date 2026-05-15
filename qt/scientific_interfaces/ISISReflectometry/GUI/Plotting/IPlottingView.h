// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "GUI/Common/PlotOptions.h"
#include "PlottingWorkspace.h"
#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL PlottingViewSubscriber {
public:
  virtual ~PlottingViewSubscriber() = default;
  virtual void notifyPlotTiledClicked() = 0;
  virtual void notifyPlotOverplotClicked() = 0;
  virtual void notifyPlotIndividualClicked() = 0;
};

class MANTIDQT_ISISREFLECTOMETRY_DLL IPlottingView {
public:
  virtual ~IPlottingView() = default;
  virtual void subscribe(PlottingViewSubscriber *notifyee) = 0;
  virtual void setOutputOptionsEnabled(bool enabled) = 0;
  virtual void setAvailablePlotOutputTypes(std::vector<PlotOutputType> const &outputTypes) = 0;
  virtual void setWorkspaceItems(std::vector<PlottingWorkspaceTreeItem> const &items) = 0;
  virtual std::vector<PlottingWorkspaceSelection> selectedWorkspaceItems() const = 0;
  virtual PlotOutputType selectedPlotOutputType() const = 0;
  virtual PlotOutputOptions selectedPlotOutputOptions() const = 0;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
