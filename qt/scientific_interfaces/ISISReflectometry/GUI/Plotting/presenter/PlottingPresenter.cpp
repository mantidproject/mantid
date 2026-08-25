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

/// Collect selected output controls and plotting target options from the view.
PresenterPlotRequest plotRequestFor(IPlottingView &view, std::string const &instrumentName, PlotLayout layout) {
  auto outputSelection = view.selectedPlotOutputSelection();
  outputSelection.instrumentName = instrumentName;
  return {std::move(outputSelection), layout, view.plotParent(), view.addToExistingPlot(),
          layout == PlotLayout::Tiled && view.plotTiledVertically()};
}
} // namespace

PlottingPresenter::PlottingPresenter(IPlottingView *view)
    : m_defaultActiveFigureMonitor(), m_view(view), m_mainPresenter(nullptr), m_plotter(&m_defaultPlotter),
      m_plotOptionsProvider(&m_defaultPlotOptionsProvider), m_plottingModel(&m_defaultPlottingModel),
      m_activeFigureMonitor(&m_defaultActiveFigureMonitor), m_outputSelectionEnabled(false),
      m_hasActiveReflectometryFigure(false), m_activePlotOverplotCompatible(false) {
  initialise();
}

PlottingPresenter::PlottingPresenter(IPlottingView *view, IPlotter const &plotter,
                                     IPlotOptionsProvider const &plotOptionsProvider,
                                     IPlottingModel const &plottingModel)
    : m_defaultActiveFigureMonitor(), m_view(view), m_mainPresenter(nullptr), m_plotter(&plotter),
      m_plotOptionsProvider(&plotOptionsProvider), m_plottingModel(&plottingModel),
      m_activeFigureMonitor(&m_defaultActiveFigureMonitor), m_outputSelectionEnabled(false),
      m_hasActiveReflectometryFigure(false), m_activePlotOverplotCompatible(false) {
  initialise();
}

void PlottingPresenter::initialise() {
  m_view->subscribe(this);
  m_activeFigureMonitor->subscribe([this]() { notifyActiveFigureChanged(); });
  m_activeFigureMonitor->start();
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
  m_workspaceTree.rebuild(runsTable);
  updateWorkspaceItemsForCurrentOutputType();
}

void PlottingPresenter::notifyPlotTiledClicked() { plotSelectedWorkspaces(PlotLayout::Tiled); }

void PlottingPresenter::notifyPlotOverplotClicked() { plotSelectedWorkspaces(PlotLayout::Overplot); }

void PlottingPresenter::notifyPlotIndividualClicked() { plotSelectedWorkspaces(PlotLayout::Individual); }

void PlottingPresenter::notifyAddToExistingPlotChanged() { updatePlotActionState(); }

void PlottingPresenter::notifyPlotOutputTypeChanged() {
  updateWorkspaceItemsForCurrentOutputType();
  updatePlotOutputControlsState();
  updatePlotActionState();
}

void PlottingPresenter::notifyWorkspaceSelectionChanged() { updatePlotActionState(); }

void PlottingPresenter::notifyActiveFigureChanged() { updateActivePlotCompatibility(); }

void PlottingPresenter::plotSelectedWorkspaces(PlotLayout layout) {
  auto const selectedPlottingWorkspaces = m_workspaceTree.plottingWorkspacesForNames(m_view->selectedWorkspaceNames());
  if (selectedPlottingWorkspaces.empty()) {
    return;
  }
  if (!m_view->selectedPlotOutputType()) {
    return;
  }

  auto const request = plotRequestFor(*m_view, m_instrumentName, layout);
  auto const workspacesToPlot =
      m_plottingModel->workspacesForPlotting(selectedPlottingWorkspaces, request.outputSelection);
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
    updateActivePlotCompatibility();
    return;
  }

  m_plotter->plot({workspacesToPlot, options, request.plotParent, request.addToExistingPlot, request.tiledVertically});
  updateActivePlotCompatibility();
}

void PlottingPresenter::updateWidgetEnabledState() {
  m_outputSelectionEnabled = !isProcessing() && !isAutoreducing();
  m_view->setOutputSelectionEnabled(m_outputSelectionEnabled);
  updateActivePlotCompatibility();
}

void PlottingPresenter::updateActivePlotCompatibility() {
  m_hasActiveReflectometryFigure = m_plotter->hasActiveReflectometryFigure();
  m_activePlotOverplotCompatible = m_plotter->canOverplotActiveFigure();
  updatePlotActionState();
}

void PlottingPresenter::updateAvailablePlotOutputTypes(std::string const &instrumentName) {
  m_view->setAvailablePlotOutputTypes(
      m_viewStateBuilder.outputTypeViewItems(m_plotOptionsProvider->availableTypes(instrumentName)));
  updatePlotOutputControlsState();
  updateWorkspaceItemsForCurrentOutputType();
  updatePlotActionState();
}

void PlottingPresenter::updateWorkspaceItemsForCurrentOutputType() {
  auto const selectedOutputType = m_view->selectedPlotOutputType();
  if (!selectedOutputType) {
    m_view->setWorkspaceItems({});
    return;
  }

  m_view->setWorkspaceItems(m_workspaceTreeDisplayStateBuilder.build(m_workspaceTree.items(), *selectedOutputType));
}

void PlottingPresenter::updatePlotActionState() const {
  auto const selectedOutputType = m_view->selectedPlotOutputType();
  if (!selectedOutputType) {
    m_view->setPlotActionState({});
    return;
  }

  m_view->setPlotActionState(m_viewStateBuilder.plotActionState(
      m_outputSelectionEnabled, m_view->selectedWorkspaceNames().size(), m_view->selectedWorkspaceGroupCount(),
      *selectedOutputType, m_view->addToExistingPlot(), m_hasActiveReflectometryFigure,
      m_activePlotOverplotCompatible));
}

void PlottingPresenter::updatePlotOutputControlsState() const {
  auto const selectedOutputType = m_view->selectedPlotOutputType();
  m_view->setPlotOutputControlsState(selectedOutputType ? m_viewStateBuilder.outputControlsState(*selectedOutputType)
                                                        : PlotOutputControlsState{});
}

bool PlottingPresenter::isProcessing() const { return m_mainPresenter && m_mainPresenter->isProcessing(); }

bool PlottingPresenter::isAutoreducing() const { return m_mainPresenter && m_mainPresenter->isAutoreducing(); }

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
