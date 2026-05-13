// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PlottingPresenter.h"
#include "GUI/Batch/IBatchPresenter.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "Reduction/ReductionWorkspaces.h"
#include "Reduction/RunsTable.h"
#include <boost/algorithm/string/join.hpp>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
Mantid::API::Workspace_sptr retrieveWorkspaceIfPresent(std::string const &workspaceName) {
  if (workspaceName.empty()) {
    return nullptr;
  }

  return Mantid::API::AnalysisDataService::Instance().doesExist(workspaceName)
             ? Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::Workspace>(workspaceName)
             : nullptr;
}

void addWorkspaceGroupIfPopulated(PlottingWorkspaceTreeItem &parent, std::string const &workspaceName,
                                  Mantid::API::WorkspaceGroup_const_sptr const &workspaceGroup) {
  auto workspaceGroupItem = PlottingWorkspaceTreeItem{workspaceName, {}};
  for (auto index = 0u; index < workspaceGroup->size(); ++index) {
    auto const memberWorkspace = workspaceGroup->getItem(index);
    if (memberWorkspace && !memberWorkspace->getName().empty()) {
      workspaceGroupItem.children.emplace_back(PlottingWorkspaceTreeItem{memberWorkspace->getName(), {}});
    }
  }

  if (workspaceGroupItem.children.empty()) {
    return;
  }

  parent.children.emplace_back(std::move(workspaceGroupItem));
}

void addWorkspaceIfPresent(PlottingWorkspaceTreeItem &parent, std::string const &workspaceName) {
  auto const workspace = retrieveWorkspaceIfPresent(workspaceName);
  if (!workspace) {
    return;
  }

  if (auto const workspaceGroup = std::dynamic_pointer_cast<Mantid::API::WorkspaceGroup const>(workspace);
      workspaceGroup) {
    addWorkspaceGroupIfPopulated(parent, workspaceName, workspaceGroup);
    return;
  }

  parent.children.emplace_back(PlottingWorkspaceTreeItem{workspaceName, {}});
}
} // namespace

bool operator==(PlottingWorkspaceTreeItem const &lhs, PlottingWorkspaceTreeItem const &rhs) {
  return lhs.label == rhs.label && lhs.children == rhs.children;
}

bool operator!=(PlottingWorkspaceTreeItem const &lhs, PlottingWorkspaceTreeItem const &rhs) { return !(lhs == rhs); }

PlottingPresenter::PlottingPresenter(IPlottingView *view)
    : m_view(view), m_mainPresenter(nullptr), m_plotter(&m_defaultPlotter),
      m_plotOptionsProvider(&m_defaultPlotOptionsProvider) {
  m_view->subscribe(this);
  updateWidgetEnabledState();
}

PlottingPresenter::PlottingPresenter(IPlottingView *view, IPlotter const &plotter,
                                     IPlotOptionsProvider const &plotOptionsProvider)
    : m_view(view), m_mainPresenter(nullptr), m_plotter(&plotter), m_plotOptionsProvider(&plotOptionsProvider) {
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

void PlottingPresenter::notifyRunsTableChanged(RunsTable const &runsTable) {
  m_view->setWorkspaceItems(makeWorkspaceItems(runsTable));
}

void PlottingPresenter::notifyPlotTiledClicked() { plotSelectedWorkspaces(PlotLayout::Tiled); }

void PlottingPresenter::notifyPlotOverplotClicked() { plotSelectedWorkspaces(PlotLayout::Overplot); }

void PlottingPresenter::notifyPlotIndividualClicked() { plotSelectedWorkspaces(PlotLayout::Individual); }

std::vector<PlottingWorkspaceTreeItem> PlottingPresenter::makeWorkspaceItems(RunsTable const &runsTable) const {
  auto workspaceItems = std::vector<PlottingWorkspaceTreeItem>{};
  for (auto const &group : runsTable.reductionJobs().groups()) {
    auto groupItem = PlottingWorkspaceTreeItem{group.name(), {}};
    if (group.success()) {
      addWorkspaceIfPresent(groupItem, group.postprocessedWorkspaceName());
    }

    for (auto const &maybeRow : group.rows()) {
      if (!maybeRow.has_value() || !maybeRow->success()) {
        continue;
      }

      auto const &row = maybeRow.value();
      auto runItem = PlottingWorkspaceTreeItem{boost::algorithm::join(row.runNumbers(), "+"), {}};
      auto const &outputs = row.reducedWorkspaceNames();
      addWorkspaceIfPresent(runItem, outputs.iVsLambda());
      addWorkspaceIfPresent(runItem, outputs.iVsQ());
      addWorkspaceIfPresent(runItem, outputs.iVsQBinned());

      if (!runItem.children.empty()) {
        groupItem.children.emplace_back(std::move(runItem));
      }
    }

    if (!groupItem.children.empty()) {
      workspaceItems.emplace_back(std::move(groupItem));
    }
  }
  return workspaceItems;
}

void PlottingPresenter::plotSelectedWorkspaces(PlotLayout layout) const {
  auto const workspaces = m_view->selectedWorkspaces();
  if (workspaces.empty()) {
    return;
  }

  auto const options = m_plotOptionsProvider->optionsFor(m_view->selectedPlotOutputType(), layout);
  if (layout == PlotLayout::Individual) {
    for (auto const &workspace : workspaces) {
      m_plotter->plot({{workspace}, options});
    }
    return;
  }

  m_plotter->plot({workspaces, options});
}

void PlottingPresenter::updateWidgetEnabledState() {
  m_view->setOutputOptionsEnabled(!isProcessing() && !isAutoreducing());
}

bool PlottingPresenter::isProcessing() const { return m_mainPresenter && m_mainPresenter->isProcessing(); }

bool PlottingPresenter::isAutoreducing() const { return m_mainPresenter && m_mainPresenter->isAutoreducing(); }

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
