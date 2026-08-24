// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "GUI/Common/PlotOptionsProvider.h"
#include "GUI/Common/Plotter.h"
#include "GUI/Plotting/model/IPlottingModel.h"
#include "GUI/Plotting/model/PlottingModel.h"
#include "GUI/Plotting/model/PlottingWorkspaceTree.h"
#include "GUI/Plotting/presenter/IPlottingPresenter.h"
#include "GUI/Plotting/presenter/PlottingViewStateBuilder.h"
#include "GUI/Plotting/presenter/PlottingWorkspaceTreeDisplayStateBuilder.h"
#include "GUI/Plotting/presenter/QtActiveFigureMonitor.h"
#include "GUI/Plotting/view/IPlottingView.h"

#include <string>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/// Coordinates plotting-tab selections, plot option creation and plotting requests.
class MANTIDQT_ISISREFLECTOMETRY_DLL PlottingPresenter : public IPlottingPresenter, public PlottingViewSubscriber {
public:
  /// Create a presenter with default model, option provider and plotter.
  explicit PlottingPresenter(IPlottingView *view);
  /// Create a presenter with injected collaborators for testing.
  PlottingPresenter(IPlottingView *view, IPlotter const &plotter, IPlotOptionsProvider const &plotOptionsProvider,
                    IPlottingModel const &plottingModel);

  void acceptMainPresenter(IBatchPresenter *mainPresenter) override;
  void notifyReductionPaused() override;
  void notifyReductionResumed() override;
  void notifyAutoreductionPaused() override;
  void notifyAutoreductionResumed() override;
  void notifyInstrumentChanged(std::string const &instrumentName) override;
  void notifyRunsTableChanged(RunsTable const &runsTable) override;
  /// Plot selected workspaces as tiled axes.
  void notifyPlotTiledClicked() override;
  /// Plot selected workspaces over compatible axes or tiled if overplotting is not possible.
  void notifyPlotOverplotClicked() override;
  /// Plot selected workspaces as separate figures.
  void notifyPlotIndividualClicked() override;
  /// Refresh active-plot compatibility after the add-to-existing option changes.
  void notifyAddToExistingPlotChanged() override;
  /// Refresh tree state after the selected output type changes.
  void notifyPlotOutputTypeChanged() override;
  /// Refresh action state after the workspace tree selection changes.
  void notifyWorkspaceSelectionChanged() override;
  /// Refresh action state for controls that depend on the active figure.
  void notifyActiveFigureChanged();

private:
  /// Connect the presenter to view and active-figure notifications.
  void initialise();
  /// Evaluate selected workspaces and dispatch a plot request for the chosen layout.
  void plotSelectedWorkspaces(PlotLayout layout);
  /// Update view state for active-figure overplot compatibility.
  void updateActivePlotCompatibility();
  /// Recalculate and apply plotting action state.
  void updatePlotActionState() const;
  /// Recalculate and apply output-specific control visibility.
  void updatePlotOutputControlsState() const;
  /// Update output types for the selected instrument.
  void updateAvailablePlotOutputTypes(std::string const &instrumentName);
  /// Reapply output-type specific tree state and display it in the view.
  void updateWorkspaceItemsForCurrentOutputType();
  /// Update controls that depend on reduction and autoreduction state.
  void updateWidgetEnabledState();
  bool isProcessing() const;
  bool isAutoreducing() const;

  Plotter m_defaultPlotter;
  PlotOptionsProvider m_defaultPlotOptionsProvider;
  PlottingModel m_defaultPlottingModel;
  QtActiveFigureMonitor m_defaultActiveFigureMonitor;
  IPlottingView *m_view;
  IBatchPresenter *m_mainPresenter;
  IPlotter const *m_plotter;
  IPlotOptionsProvider const *m_plotOptionsProvider;
  IPlottingModel const *m_plottingModel;
  IActiveFigureMonitor *m_activeFigureMonitor;
  PlottingWorkspaceTree m_workspaceTree;
  PlottingViewStateBuilder m_viewStateBuilder;
  PlottingWorkspaceTreeDisplayStateBuilder m_workspaceTreeDisplayStateBuilder;
  std::vector<PlottingWorkspaceTreeItem> m_workspaceItems;
  std::string m_instrumentName;
  bool m_outputSelectionEnabled;
  bool m_hasActiveReflectometryFigure;
  bool m_activePlotOverplotCompatible;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
