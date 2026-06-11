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
#include "GUI/Plotting/presenter/IPlottingPresenter.h"
#include "GUI/Plotting/presenter/PlottingWorkspaceTree.h"
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
  /// Refresh active-plot state after the add-to-existing option changes.
  void notifyAddToExistingPlotChanged() override;
  /// Refresh active-plot state for controls that depend on open figures.
  void notifyActivePlotAvailabilityChanged() override;

private:
  /// Evaluate selected workspaces and dispatch a plot request for the chosen layout.
  void plotSelectedWorkspaces(PlotLayout layout) const;
  /// Update view state for active-figure availability and overplot compatibility.
  void updateActivePlotAvailability() const;
  /// Update output types for the selected instrument.
  void updateAvailablePlotOutputTypes(std::string const &instrumentName);
  /// Update controls that depend on reduction and autoreduction state.
  void updateWidgetEnabledState();
  bool isProcessing() const;
  bool isAutoreducing() const;

  Plotter m_defaultPlotter;
  PlotOptionsProvider m_defaultPlotOptionsProvider;
  PlottingModel m_defaultPlottingModel;
  IPlottingView *m_view;
  IBatchPresenter *m_mainPresenter;
  IPlotter const *m_plotter;
  IPlotOptionsProvider const *m_plotOptionsProvider;
  IPlottingModel const *m_plottingModel;
  PlottingWorkspaceTree m_workspaceTree;
  std::string m_instrumentName;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
