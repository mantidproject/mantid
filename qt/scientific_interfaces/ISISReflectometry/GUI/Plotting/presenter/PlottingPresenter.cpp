// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PlottingPresenter.h"
#include "GUI/Batch/IBatchPresenter.h"
#include "PlotOutputTypeProperties.h"

#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
auto constexpr multiplePlotItemWarningThreshold = 5;

/// Presenter-local request state collected from the view before model evaluation.
struct PresenterPlotRequest {
  PlotOutputSelection outputSelection;
  PlotRequestSettings settings;
  QWidget *plotParent;
};

/// Collect selected output controls and plotting target options from the view.
PresenterPlotRequest plotRequestFor(IPlottingView &view, std::string const &instrumentName, PlotLayout layout) {
  auto outputSelection = view.selectedPlotOutputSelection();
  outputSelection.instrumentName = instrumentName;
  return {std::move(outputSelection),
          {layout, view.shouldAddToExistingPlot(), layout == PlotLayout::Tiled && view.shouldPlotTiledVertically(),
           layout == PlotLayout::Tiled && view.shouldShareXAxisWhenTiled(),
           layout == PlotLayout::Tiled && view.shouldShareYAxisWhenTiled()},
          view.plotParent()};
}
} // namespace

PlottingPresenter::PlottingPresenter(IPlottingView *view)
    : m_defaultActiveFigureMonitor(), m_view(view), m_mainPresenter(nullptr), m_plotter(&m_defaultPlotter),
      m_plotOptionsProvider(&m_defaultPlotOptionsProvider), m_plottingModel(&m_defaultPlottingModel),
      m_activeFigureMonitor(&m_defaultActiveFigureMonitor), m_plottingWorkspaceTreePresenter(view->workspaceTreeView()),
      m_outputSelectionEnabled(false), m_hasActiveReflectometryFigure(false), m_activePlotOverplotCompatible(false) {
  initialise();
}

PlottingPresenter::PlottingPresenter(IPlottingView *view, IPlotter const &plotter,
                                     IPlotOptionsProvider const &plotOptionsProvider,
                                     IPlottingModel const &plottingModel)
    : m_defaultActiveFigureMonitor(), m_view(view), m_mainPresenter(nullptr), m_plotter(&plotter),
      m_plotOptionsProvider(&plotOptionsProvider), m_plottingModel(&plottingModel),
      m_activeFigureMonitor(&m_defaultActiveFigureMonitor), m_plottingWorkspaceTreePresenter(view->workspaceTreeView()),
      m_outputSelectionEnabled(false), m_hasActiveReflectometryFigure(false), m_activePlotOverplotCompatible(false) {
  initialise();
}

void PlottingPresenter::initialise() {
  m_view->subscribe(this);
  updateWorkspaceFilter();
  m_activeFigureMonitor->subscribe([this]() { notifyActiveFigureChanged(); });
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
  m_plottingWorkspaceTree.rebuild(runsTable);
  updatePlottingWorkspaceTreeItemStates();
}

void PlottingPresenter::notifyPlotTiledClicked() { plotSelectedWorkspaces(PlotLayout::Tiled); }

void PlottingPresenter::notifyPlotOverplotClicked() { plotSelectedWorkspaces(PlotLayout::Overplot); }

void PlottingPresenter::notifyPlotIndividualClicked() { plotSelectedWorkspaces(PlotLayout::Individual); }

void PlottingPresenter::notifyAddToExistingPlotChanged() { updatePlotActionState(); }

void PlottingPresenter::notifyPlotOutputTypeChanged() {
  m_plottingWorkspaceTreePresenter.clearSelection();
  updatePlottingWorkspaceTreeItemStates();
  updatePlotOutputControlsState();
  updatePlotActionState();
}

void PlottingPresenter::notifyPlottingWorkspaceTreeSelectionChanged() { updatePlotActionState(); }

void PlottingPresenter::notifyWorkspaceFilterChanged() { updateWorkspaceFilter(); }

void PlottingPresenter::updateWorkspaceFilter() {
  m_plottingWorkspaceTreePresenter.filterWorkspaces(m_view->workspaceFilterText(),
                                                    m_view->workspaceFilterOutputTypes());
}

void PlottingPresenter::notifyActiveFigureChanged() { updateActivePlotCompatibility(); }

void PlottingPresenter::plotSelectedWorkspaces(PlotLayout layout) {
  auto const selectedPlottingWorkspaces = m_plottingWorkspaceTree.plottingWorkspacesForNames(
      m_plottingWorkspaceTreePresenter.selectedPlottingWorkspaceNames());
  if (selectedPlottingWorkspaces.empty()) {
    return;
  }
  if (!m_view->selectedPlotOutputType()) {
    return;
  }

  auto const request = plotRequestFor(*m_view, m_instrumentName, layout);
  auto preparedRequests = std::vector<PlotRequest>{};
  auto plotItemCount = size_t{0};
  for (auto const &definition : plotOutputTypeProperties(request.outputSelection.outputType).plotRequests()) {
    auto outputSelection = request.outputSelection;
    outputSelection.outputType = definition.outputType;
    auto const settings = definition.settingsOverride.value_or(request.settings);
    auto workspaces = m_plottingModel->workspacesForPlotting(selectedPlottingWorkspaces, outputSelection);
    if (workspaces.empty()) {
      return;
    }
    plotItemCount += workspaces.size();
    auto const options = m_plotOptionsProvider->optionsFor(outputSelection, settings.layout);
    auto plotRequest = PlotRequest{{},
                                   options,
                                   request.plotParent,
                                   settings.addToExistingPlot,
                                   settings.tiledVertically,
                                   settings.shareXAxisWhenTiled,
                                   settings.shareYAxisWhenTiled};
    if (settings.layout == PlotLayout::Individual) {
      for (auto const &workspace : workspaces) {
        plotRequest.workspaces = {workspace};
        preparedRequests.emplace_back(plotRequest);
      }
    } else {
      plotRequest.workspaces = std::move(workspaces);
      preparedRequests.emplace_back(std::move(plotRequest));
    }
  }

  if (plotItemCount >= multiplePlotItemWarningThreshold && !m_view->confirmPlottingMultipleItems(plotItemCount)) {
    return;
  }
  for (auto const &plotRequest : preparedRequests) {
    m_plotter->plot(plotRequest);
  }
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
      m_viewStateProvider.outputTypeViewItems(m_plotOptionsProvider->availableTypes(instrumentName)));
  updatePlotOutputControlsState();
  updatePlottingWorkspaceTreeItemStates();
  updatePlotActionState();
}

void PlottingPresenter::updatePlottingWorkspaceTreeItemStates() {
  auto const selectedOutputType = m_view->selectedPlotOutputType();
  if (!selectedOutputType) {
    m_plottingWorkspaceTreePresenter.setPlottingWorkspaceTreeItemStates({});
    return;
  }

  m_plottingWorkspaceTreePresenter.setPlottingWorkspaceTreeItemStates(
      m_viewStateProvider.plottingWorkspaceTreeItemStates(m_plottingWorkspaceTree.items(), *selectedOutputType));
}

void PlottingPresenter::updatePlotActionState() const {
  auto const selectedOutputType = m_view->selectedPlotOutputType();
  if (!selectedOutputType) {
    m_view->setPlotActionState({});
    return;
  }

  m_view->setPlotActionState(m_viewStateProvider.plotActionState(
      m_outputSelectionEnabled, m_plottingWorkspaceTreePresenter.selectedPlottingWorkspaceNames().size(),
      m_plottingWorkspaceTreePresenter.selectedPlottingWorkspaceGroupCount(), *selectedOutputType,
      m_view->shouldAddToExistingPlot(), m_hasActiveReflectometryFigure, m_activePlotOverplotCompatible));
}

void PlottingPresenter::updatePlotOutputControlsState() const {
  auto const selectedOutputType = m_view->selectedPlotOutputType();
  m_view->setPlotOutputControlsState(selectedOutputType ? m_viewStateProvider.outputControlsState(*selectedOutputType)
                                                        : PlotOutputControlsState{});
}

bool PlottingPresenter::isProcessing() const { return m_mainPresenter && m_mainPresenter->isProcessing(); }

bool PlottingPresenter::isAutoreducing() const { return m_mainPresenter && m_mainPresenter->isAutoreducing(); }

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
