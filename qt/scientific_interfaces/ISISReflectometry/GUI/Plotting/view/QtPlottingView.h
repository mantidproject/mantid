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
#include <optional>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class QtPlottingWorkspaceTreeViewAdapter;

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
  void setAvailablePlotOutputTypes(std::vector<PlotOutputTypeViewItem> const &outputTypes) override final;
  /// Apply visibility for output-specific controls.
  void setPlotOutputControlsState(PlotOutputControlsState const &state) override;
  /// Apply enabled and checked state for plotting action controls.
  void setPlotActionState(PlotActionState const &state) override;
  /// Replace the plotting workspace tree contents.
  void setPlottingWorkspaceTreeItemStates(std::vector<PlottingWorkspaceTreeItemState> const &itemStates) override;
  /// Return names of selected workspace leaf nodes.
  std::vector<std::string> selectedPlottingWorkspaceNames() const override;
  /// Return the number of selected workspace-group rows.
  size_t selectedPlottingWorkspaceGroupCount() const override;
  /// Return the selected plot output type.
  std::optional<PlotOutputType> selectedPlotOutputType() const override;
  /// Return the full output selection, including output-specific axis controls.
  PlotOutputSelection selectedPlotOutputSelection() const override;
  /// Return true if the next plot should be added to the active plot.
  bool addToExistingPlot() const override;
  /// Return true if tiled plots should fill vertically before horizontally.
  bool plotTiledVertically() const override;
  /// Return the widget to use as the parent for plot windows.
  QWidget *plotParent() override;
  /// Ask the user to confirm plotting a large number of items.
  bool confirmPlottingMultipleItems(size_t plotCount) const override;

private:
  /// Set up controls, tree controller and signal-slot connections.
  void initLayout();
  /// Enable or disable output selectors.
  void setOutputSelectionControlsEnabled(bool enabled);
  /// Clear selected tree rows.
  void clearPlottingWorkspaceTreeSelection();

  Ui::PlottingWidget m_ui;
  std::unique_ptr<QtPlottingWorkspaceTreeViewAdapter> m_plottingWorkspaceTreeViewAdapter;
  PlottingViewSubscriber *m_notifyee;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
