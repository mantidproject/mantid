// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PlottingPresenter.h"
#include "GUI/Batch/IBatchPresenter.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "Reduction/ReductionWorkspaces.h"
#include "Reduction/RunsTable.h"
#include <boost/algorithm/string/join.hpp>
#include <exception>
#include <optional>
#include <unordered_map>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
using SelectionMap = std::unordered_map<std::string, PlottingWorkspaceSelection>;

Mantid::API::Workspace_sptr retrieveWorkspaceIfPresent(std::string const &workspaceName) {
  if (workspaceName.empty()) {
    return nullptr;
  }

  return Mantid::API::AnalysisDataService::Instance().doesExist(workspaceName)
             ? Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::Workspace>(workspaceName)
             : nullptr;
}

std::optional<int> periodForWorkspace(Mantid::API::Workspace const &workspace) {
  auto const matrixWorkspace = dynamic_cast<Mantid::API::MatrixWorkspace const *>(&workspace);
  if (!matrixWorkspace) {
    return std::nullopt;
  }

  auto const &run = matrixWorkspace->run();
  try {
    if (run.hasProperty("nperiods") && run.getPropertyAsIntegerValue("nperiods") > 1 &&
        run.hasProperty("current_period")) {
      return run.getPropertyAsIntegerValue("current_period");
    }
  } catch (std::exception const &) {
    return std::nullopt;
  }
  return std::nullopt;
}

std::vector<std::string> runNumbersForWorkspace(Mantid::API::Workspace const &workspace) {
  auto const matrixWorkspace = dynamic_cast<Mantid::API::MatrixWorkspace const *>(&workspace);
  if (!matrixWorkspace) {
    return {};
  }

  auto const &run = matrixWorkspace->run();
  try {
    return run.hasProperty("run_number") ? std::vector<std::string>{run.getProperty("run_number")->value()}
                                         : std::vector<std::string>{};
  } catch (std::exception const &) {
    return {};
  }
}

void addWorkspaceSelection(SelectionMap &workspaceSelections, std::string const &workspaceName,
                           PlottingWorkspaceOutputType outputType, std::string const &groupName,
                           std::vector<std::string> const &runNumbers, std::string workspaceGroupName,
                           std::optional<int> period) {
  workspaceSelections[workspaceName] = PlottingWorkspaceSelection{
      workspaceName, outputType, groupName, runNumbers, std::move(workspaceGroupName), period};
}

void addWorkspaceGroupIfPopulated(PlottingWorkspaceTreeItem &parent, std::string const &workspaceName,
                                  Mantid::API::WorkspaceGroup_const_sptr const &workspaceGroup,
                                  PlottingWorkspaceOutputType outputType, std::string const &groupName,
                                  std::vector<std::string> runNumbers, SelectionMap &workspaceSelections) {
  auto workspaceGroupItem = PlottingWorkspaceTreeItem{workspaceName,
                                                      PlottingWorkspaceTreeItemType::WorkspaceGroup,
                                                      PlottingWorkspaceOutputType::None,
                                                      groupName,
                                                      std::move(runNumbers),
                                                      workspaceName,
                                                      {}};
  for (auto index = 0u; index < workspaceGroup->size(); ++index) {
    auto const memberWorkspace = workspaceGroup->getItem(index);
    if (memberWorkspace && !memberWorkspace->getName().empty()) {
      auto child = PlottingWorkspaceTreeItem{memberWorkspace->getName(),
                                             PlottingWorkspaceTreeItemType::Workspace,
                                             outputType,
                                             groupName,
                                             workspaceGroupItem.runNumbers,
                                             memberWorkspace->getName(),
                                             {}};
      addWorkspaceSelection(workspaceSelections, memberWorkspace->getName(), outputType, groupName,
                            runNumbersForWorkspace(*memberWorkspace), workspaceName,
                            periodForWorkspace(*memberWorkspace));
      workspaceGroupItem.children.emplace_back(std::move(child));
    }
  }

  if (workspaceGroupItem.children.empty()) {
    return;
  }

  parent.children.emplace_back(std::move(workspaceGroupItem));
}

void addWorkspaceIfPresent(PlottingWorkspaceTreeItem &parent, std::string const &workspaceName,
                           PlottingWorkspaceOutputType outputType, std::string const &groupName,
                           std::vector<std::string> runNumbers, SelectionMap &workspaceSelections) {
  auto const workspace = retrieveWorkspaceIfPresent(workspaceName);
  if (!workspace) {
    return;
  }

  if (auto const workspaceGroup = std::dynamic_pointer_cast<Mantid::API::WorkspaceGroup const>(workspace);
      workspaceGroup) {
    addWorkspaceGroupIfPopulated(parent, workspaceName, workspaceGroup, outputType, groupName, std::move(runNumbers),
                                 workspaceSelections);
    return;
  }

  auto item = PlottingWorkspaceTreeItem{workspaceName,
                                        PlottingWorkspaceTreeItemType::Workspace,
                                        outputType,
                                        groupName,
                                        std::move(runNumbers),
                                        workspaceName,
                                        {}};
  addWorkspaceSelection(workspaceSelections, workspaceName, outputType, groupName, runNumbersForWorkspace(*workspace),
                        "", periodForWorkspace(*workspace));
  parent.children.emplace_back(std::move(item));
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
  updateAvailablePlotOutputTypes(instrumentName);
}

void PlottingPresenter::notifyRunsTableChanged(RunsTable const &runsTable) {
  m_view->setWorkspaceItems(makeWorkspaceItems(runsTable));
}

void PlottingPresenter::notifyPlotTiledClicked() { plotSelectedWorkspaces(PlotLayout::Tiled); }

void PlottingPresenter::notifyPlotOverplotClicked() { plotSelectedWorkspaces(PlotLayout::Overplot); }

void PlottingPresenter::notifyPlotIndividualClicked() { plotSelectedWorkspaces(PlotLayout::Individual); }

std::vector<PlottingWorkspaceTreeItem> PlottingPresenter::makeWorkspaceItems(RunsTable const &runsTable) {
  auto workspaceItems = std::vector<PlottingWorkspaceTreeItem>{};
  m_workspaceSelections.clear();
  for (auto const &group : runsTable.reductionJobs().groups()) {
    auto groupItem = PlottingWorkspaceTreeItem{group.name(),
                                               PlottingWorkspaceTreeItemType::Group,
                                               PlottingWorkspaceOutputType::None,
                                               group.name(),
                                               {},
                                               "",
                                               {}};
    if (group.success()) {
      addWorkspaceIfPresent(groupItem, group.postprocessedWorkspaceName(), PlottingWorkspaceOutputType::IvsQBinned,
                            group.name(), {}, m_workspaceSelections);
    }

    for (auto const &maybeRow : group.rows()) {
      if (!maybeRow.has_value() || !maybeRow->success()) {
        continue;
      }

      auto const &row = maybeRow.value();
      auto runItem = PlottingWorkspaceTreeItem{boost::algorithm::join(row.runNumbers(), "+"),
                                               PlottingWorkspaceTreeItemType::Run,
                                               PlottingWorkspaceOutputType::None,
                                               group.name(),
                                               row.runNumbers(),
                                               "",
                                               {}};
      auto const &outputs = row.reducedWorkspaceNames();
      addWorkspaceIfPresent(runItem, outputs.iVsLambda(), PlottingWorkspaceOutputType::IvsLambda, group.name(),
                            row.runNumbers(), m_workspaceSelections);
      addWorkspaceIfPresent(runItem, outputs.iVsQ(), PlottingWorkspaceOutputType::IvsQ, group.name(), row.runNumbers(),
                            m_workspaceSelections);
      addWorkspaceIfPresent(runItem, outputs.iVsQBinned(), PlottingWorkspaceOutputType::IvsQBinned, group.name(),
                            row.runNumbers(), m_workspaceSelections);

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

std::vector<PlottingWorkspaceSelection>
PlottingPresenter::selectedWorkspacesFor(std::vector<std::string> const &workspaceNames) const {
  auto selections = std::vector<PlottingWorkspaceSelection>{};
  selections.reserve(workspaceNames.size());
  for (auto const &workspaceName : workspaceNames) {
    auto const selection = m_workspaceSelections.find(workspaceName);
    if (selection != m_workspaceSelections.cend()) {
      selections.emplace_back(selection->second);
    }
  }
  return selections;
}

void PlottingPresenter::plotSelectedWorkspaces(PlotLayout layout) const {
  auto const selectedWorkspaces = selectedWorkspacesFor(m_view->selectedWorkspaceNames());
  if (selectedWorkspaces.empty()) {
    return;
  }

  auto const outputOptions = m_view->selectedPlotOutputOptions();
  auto const workspacesToPlot = m_plottingModel->workspacesForPlotting(selectedWorkspaces, outputOptions);
  if (workspacesToPlot.empty()) {
    return;
  }

  auto const options = m_plotOptionsProvider->optionsFor(outputOptions, layout);
  if (layout == PlotLayout::Individual) {
    for (auto const &workspace : workspacesToPlot) {
      m_plotter->plot({{workspace}, options});
    }
    return;
  }

  m_plotter->plot({workspacesToPlot, options});
}

void PlottingPresenter::updateWidgetEnabledState() {
  m_view->setOutputOptionsEnabled(!isProcessing() && !isAutoreducing());
}

void PlottingPresenter::updateAvailablePlotOutputTypes(std::string const &instrumentName) {
  m_view->setAvailablePlotOutputTypes(m_plotOptionsProvider->availableTypes(instrumentName));
}

bool PlottingPresenter::isProcessing() const { return m_mainPresenter && m_mainPresenter->isProcessing(); }

bool PlottingPresenter::isAutoreducing() const { return m_mainPresenter && m_mainPresenter->isAutoreducing(); }

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
