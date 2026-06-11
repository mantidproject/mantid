// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PlottingPresenter.h"
#include "GUI/Batch/IBatchPresenter.h"

#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
auto constexpr multiplePlotItemWarningThreshold = 5;

/// Presenter-local request state collected from the view before model evaluation.
struct PresenterPlotRequest {
  PlotOutputSelection outputSelection;
  PlotLayout layout;
  QWidget *plotParent;
  bool addToExistingPlot;
  bool tiledVertically;
};

/// Return the layout to use when overplotting onto the active plot is not possible.
PlotLayout layoutForAddToExistingOverplot(bool addToExistingPlot, bool activePlotOverplotCompatible) {
  return addToExistingPlot && !activePlotOverplotCompatible ? PlotLayout::Tiled : PlotLayout::Overplot;
}

/// Return true if a request may proceed as an overplot.
bool activePlotOverplotCompatibleForRequest(bool addToExistingPlot, IPlotter const &plotter) {
  return !addToExistingPlot || plotter.canOverplotActiveFigure();
}

/// Collect selected output controls and plotting target options from the view.
PresenterPlotRequest plotRequestFor(IPlottingView &view, std::string const &instrumentName, PlotLayout layout) {
  auto outputSelection = view.selectedPlotOutputSelection();
  outputSelection.instrumentName = instrumentName;
  return {std::move(outputSelection), layout, view.plotParent(), view.addToExistingPlot(),
          layout == PlotLayout::Tiled && view.plotTiledVertically()};
}
} // namespace

PlottingPresenter::PlottingPresenter(IPlottingView *view)
    : m_view(view), m_mainPresenter(nullptr), m_plotter(&m_defaultPlotter),
      m_plotOptionsProvider(&m_defaultPlotOptionsProvider), m_plottingModel(&m_defaultPlottingModel) {
  m_view->subscribe(this);
  updateWidgetEnabledState();
}

PlottingPresenter::PlottingPresenter(IPlottingView *view, IPlotter const &plotter,
                                     IPlotOptionsProvider const &plotOptionsProvider,
                                     IPlottingModel const &plottingModel)
    : m_view(view), m_mainPresenter(nullptr), m_plotter(&plotter), m_plotOptionsProvider(&plotOptionsProvider),
      m_plottingModel(&plottingModel) {
  m_view->subscribe(this);
  updateWidgetEnabledState();
}

void PlottingPresenter::acceptMainPresenter(IBatchPresenter *mainPresenter) {
  m_mainPresenter = mainPresenter;
  updateWidgetEnabledState();
}

void PlottingPresenter::notifyReductionPaused() { updateWidgetEnabledState(); }

void PlottingPresenter::notifyReductionResumed() { updateWidgetEnabledState(); }

void PlottingPresenter::notifyAutoreductionPaused() { updateWidgetEnabledState(); }

void PlottingPresenter::notifyAutoreductionResumed() { updateWidgetEnabledState(); }

void PlottingPresenter::notifyInstrumentChanged(std::string const &instrumentName) {
  m_instrumentName = instrumentName;
  updateAvailablePlotOutputTypes(instrumentName);
}

void PlottingPresenter::notifyRunsTableChanged(RunsTable const &runsTable) {
  m_view->setWorkspaceItems(m_workspaceTree.makeWorkspaceItems(runsTable));
}

void PlottingPresenter::notifyPlotTiledClicked() { plotSelectedWorkspaces(PlotLayout::Tiled); }

void PlottingPresenter::notifyPlotOverplotClicked() {
  auto const addToExistingPlot = m_view->addToExistingPlot();
  plotSelectedWorkspaces(layoutForAddToExistingOverplot(
      addToExistingPlot, activePlotOverplotCompatibleForRequest(addToExistingPlot, *m_plotter)));
}

void PlottingPresenter::notifyPlotIndividualClicked() { plotSelectedWorkspaces(PlotLayout::Individual); }

void PlottingPresenter::notifyAddToExistingPlotChanged() { updateActivePlotAvailability(); }

void PlottingPresenter::notifyActivePlotAvailabilityChanged() { updateActivePlotAvailability(); }

void PlottingPresenter::plotSelectedWorkspaces(PlotLayout layout) const {
  auto const selectedWorkspaces = m_workspaceTree.selectedWorkspacesFor(m_view->selectedWorkspaceNames());
  if (selectedWorkspaces.empty()) {
    return;
  }

  auto const request = plotRequestFor(*m_view, m_instrumentName, layout);
  auto const workspacesToPlot = m_plottingModel->workspacesForPlotting(selectedWorkspaces, request.outputSelection);
  if (workspacesToPlot.empty()) {
    return;
  }

  auto const options = m_plotOptionsProvider->optionsFor(request.outputSelection, request.layout);
  if (workspacesToPlot.size() >= multiplePlotItemWarningThreshold &&
      !m_view->confirmPlottingMultipleItems(workspacesToPlot.size())) {
    return;
  }

  if (request.layout == PlotLayout::Individual) {
    for (auto const &workspace : workspacesToPlot) {
      m_plotter->plot({{workspace}, options, request.plotParent, request.addToExistingPlot, request.tiledVertically});
    }
    updateActivePlotAvailability();
    return;
  }

  m_plotter->plot({workspacesToPlot, options, request.plotParent, request.addToExistingPlot, request.tiledVertically});
  updateActivePlotAvailability();
}

void PlottingPresenter::updateWidgetEnabledState() {
  m_view->setOutputSelectionEnabled(!isProcessing() && !isAutoreducing());
  updateActivePlotAvailability();
}

void PlottingPresenter::updateActivePlotAvailability() const {
  auto const activePlotAvailable = m_plotter->hasActiveFigure();
  m_view->setActivePlotAvailable(activePlotAvailable);
  m_view->setActivePlotOverplotCompatible(activePlotAvailable && m_plotter->canOverplotActiveFigure());
}

void PlottingPresenter::updateAvailablePlotOutputTypes(std::string const &instrumentName) {
  m_view->setAvailablePlotOutputTypes(m_plotOptionsProvider->availableTypes(instrumentName));
}

bool PlottingPresenter::isProcessing() const { return m_mainPresenter && m_mainPresenter->isProcessing(); }

bool PlottingPresenter::isAutoreducing() const { return m_mainPresenter && m_mainPresenter->isAutoreducing(); }

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
