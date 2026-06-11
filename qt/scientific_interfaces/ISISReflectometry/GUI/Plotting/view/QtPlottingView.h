// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IPlottingView.h"
#include "ui_PlottingWidget.h"

#include <memory>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class WorkspaceTreeController;

/// Qt implementation of the ISIS Reflectometry plotting tab.
class MANTIDQT_ISISREFLECTOMETRY_DLL QtPlottingView : public QWidget, public IPlottingView {
  Q_OBJECT
public:
  /// Create and initialise the plotting tab widget.
  explicit QtPlottingView(QWidget *parent = nullptr);
  ~QtPlottingView() override;

  /// Register the presenter that receives plotting-tab notifications.
  void subscribe(PlottingViewSubscriber *notifyee) override;
  /// Enable or disable plot output controls while reduction state changes.
  void setOutputSelectionEnabled(bool enabled) override;
  /// Replace the available plot output types in the output selector.
  void setAvailablePlotOutputTypes(std::vector<PlotOutputType> const &outputTypes) override;
  /// Replace the workspace tree contents.
  void setWorkspaceItems(std::vector<PlottingWorkspaceTreeItem> const &items) override;
  /// Return names of selected workspace leaf nodes.
  std::vector<std::string> selectedWorkspaceNames() const override;
  /// Return the selected plot output type.
  PlotOutputType selectedPlotOutputType() const override;
  /// Return the full output selection, including output-specific axis controls.
  PlotOutputSelection selectedPlotOutputSelection() const override;
  /// Return true if the next plot should be added to the active plot.
  bool addToExistingPlot() const override;
  /// Return true if tiled plots should fill vertically before horizontally.
  bool plotTiledVertically() const override;
  /// Update controls that depend on an active matplotlib figure existing.
  void setActivePlotAvailable(bool available) override;
  /// Update controls that depend on the active figure accepting overplots.
  void setActivePlotOverplotCompatible(bool compatible) override;
  /// Return the widget to use as the parent for plot windows.
  QWidget *plotParent() override;
  /// Ask the user to confirm plotting a large number of items.
  bool confirmPlottingMultipleItems(size_t plotCount) const override;

private:
  /// Set up controls, tree controller and signal-slot connections.
  void initLayout();
  /// Enable or disable output selectors.
  void setOutputSelectionControlsEnabled(bool enabled);
  /// Recalculate enabled states for all plotting action controls.
  void updatePlotButtonEnabledStates();
  /// Show, hide and apply controls for the selected output type.
  void updatePlotOutputProperties();
  /// Clear selected tree rows and refresh action states.
  void clearWorkspaceSelection();

  Ui::PlottingWidget m_ui;
  std::unique_ptr<WorkspaceTreeController> m_workspaceTree;
  PlottingViewSubscriber *m_notifyee;
  bool m_outputSelectionEnabled;
  bool m_activePlotAvailable;
  bool m_activePlotOverplotCompatible;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
