// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "GUI/Common/PlotOptions.h"
#include "GUI/Plotting/model/PlottingWorkspace.h"
#include <string>
#include <vector>

class QWidget;

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/// Presenter callbacks emitted by the plotting tab view.
class MANTIDQT_ISISREFLECTOMETRY_DLL PlottingViewSubscriber {
public:
  virtual ~PlottingViewSubscriber() = default;
  /// Notify that the user requested a tiled plot.
  virtual void notifyPlotTiledClicked() = 0;
  /// Notify that the user requested an overplot.
  virtual void notifyPlotOverplotClicked() = 0;
  /// Notify that the user requested individual plots.
  virtual void notifyPlotIndividualClicked() = 0;
  /// Notify that the add-to-existing checkbox changed state.
  virtual void notifyAddToExistingPlotChanged() = 0;
  /// Notify that controls depending on the active plot should be refreshed.
  virtual void notifyActivePlotAvailabilityChanged() = 0;
};

/// Interface for the ISIS Reflectometry plotting tab view.
class MANTIDQT_ISISREFLECTOMETRY_DLL IPlottingView {
public:
  virtual ~IPlottingView() = default;
  /// Register the presenter that receives view notifications.
  virtual void subscribe(PlottingViewSubscriber *notifyee) = 0;
  /// Enable or disable the plot-output selection controls.
  virtual void setOutputSelectionEnabled(bool enabled) = 0;
  /// Set output types available for the current instrument.
  virtual void setAvailablePlotOutputTypes(std::vector<PlotOutputType> const &outputTypes) = 0;
  /// Replace the displayed workspace tree.
  virtual void setWorkspaceItems(std::vector<PlottingWorkspaceTreeItem> const &items) = 0;
  /// Return selected workspace leaf names.
  virtual std::vector<std::string> selectedWorkspaceNames() const = 0;
  /// Return the selected output type.
  virtual PlotOutputType selectedPlotOutputType() const = 0;
  /// Return the full output selection, including output-specific axes.
  virtual PlotOutputSelection selectedPlotOutputSelection() const = 0;
  /// Return true if plotting should target the active figure.
  virtual bool addToExistingPlot() const = 0;
  /// Return true if tiled plots should be arranged vertically first.
  virtual bool plotTiledVertically() const = 0;
  /// Update controls that require an active figure.
  virtual void setActivePlotAvailable(bool available) = 0;
  /// Update controls that require the active figure to support overplotting.
  virtual void setActivePlotOverplotCompatible(bool compatible) = 0;
  /// Return the parent widget for plot windows.
  virtual QWidget *plotParent() = 0;
  /// Confirm large plot requests with the user.
  virtual bool confirmPlottingMultipleItems(size_t plotCount) const = 0;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
