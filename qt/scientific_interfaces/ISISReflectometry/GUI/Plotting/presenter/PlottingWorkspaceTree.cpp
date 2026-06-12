// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PlottingWorkspaceTree.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "Reduction/ReductionWorkspaces.h"
#include "Reduction/RunsTable.h"

#include <boost/algorithm/string/join.hpp>

#include <exception>
#include <optional>
#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
using WorkspaceSelectionMap = std::unordered_map<std::string, PlottingWorkspaceSelection>;

/// Return the ADS workspace if the name is non-empty and exists.
Mantid::API::Workspace_sptr retrieveWorkspaceIfPresent(std::string const &workspaceName) {
  if (workspaceName.empty()) {
    return nullptr;
  }

  return Mantid::API::AnalysisDataService::Instance().doesExist(workspaceName)
             ? Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::Workspace>(workspaceName)
             : nullptr;
}

/// Return the current period metadata for multi-period matrix workspaces.
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

/// Return run-number metadata stored on a matrix workspace.
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

/// Store plot metadata for a selectable workspace tree item.
void recordWorkspaceSelection(WorkspaceSelectionMap &workspaceSelectionsByName, std::string const &workspaceName,
                              PlottingWorkspaceOutputType outputType, std::string const &groupName,
                              std::vector<std::string> const &runNumbers, std::string workspaceGroupName,
                              std::optional<int> period) {
  workspaceSelectionsByName[workspaceName] = PlottingWorkspaceSelection{
      workspaceName, outputType, groupName, runNumbers, std::move(workspaceGroupName), period};
}

/// Add a workspace-group tree item if it has named child workspaces.
void addWorkspaceGroupItemIfPopulated(PlottingWorkspaceTreeItem &parent, std::string const &workspaceName,
                                      Mantid::API::WorkspaceGroup_const_sptr const &workspaceGroup,
                                      PlottingWorkspaceOutputType outputType, std::string const &groupName,
                                      std::vector<std::string> runNumbers,
                                      WorkspaceSelectionMap &workspaceSelectionsByName) {
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
      recordWorkspaceSelection(workspaceSelectionsByName, memberWorkspace->getName(), outputType, groupName,
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

/// Add a workspace or workspace-group item for a named ADS workspace if it exists.
void addWorkspaceItemIfPresent(PlottingWorkspaceTreeItem &parent, std::string const &workspaceName,
                               PlottingWorkspaceOutputType outputType, std::string const &groupName,
                               std::vector<std::string> runNumbers, WorkspaceSelectionMap &workspaceSelectionsByName) {
  auto const workspace = retrieveWorkspaceIfPresent(workspaceName);
  if (!workspace) {
    return;
  }

  if (auto const workspaceGroup = std::dynamic_pointer_cast<Mantid::API::WorkspaceGroup const>(workspace);
      workspaceGroup) {
    addWorkspaceGroupItemIfPopulated(parent, workspaceName, workspaceGroup, outputType, groupName,
                                     std::move(runNumbers), workspaceSelectionsByName);
    return;
  }

  auto item = PlottingWorkspaceTreeItem{workspaceName,
                                        PlottingWorkspaceTreeItemType::Workspace,
                                        outputType,
                                        groupName,
                                        std::move(runNumbers),
                                        workspaceName,
                                        {}};
  recordWorkspaceSelection(workspaceSelectionsByName, workspaceName, outputType, groupName,
                           runNumbersForWorkspace(*workspace), "", periodForWorkspace(*workspace));
  parent.children.emplace_back(std::move(item));
}
} // namespace

std::vector<PlottingWorkspaceTreeItem> PlottingWorkspaceTree::makeWorkspaceItems(RunsTable const &runsTable) {
  auto workspaceItems = std::vector<PlottingWorkspaceTreeItem>{};
  m_workspaceSelectionsByName.clear();
  for (auto const &group : runsTable.reductionJobs().groups()) {
    auto groupItem = PlottingWorkspaceTreeItem{group.name(),
                                               PlottingWorkspaceTreeItemType::Group,
                                               PlottingWorkspaceOutputType::None,
                                               group.name(),
                                               {},
                                               "",
                                               {}};
    if (group.success()) {
      addWorkspaceItemIfPresent(groupItem, group.postprocessedWorkspaceName(), PlottingWorkspaceOutputType::IvsQBinned,
                                group.name(), {}, m_workspaceSelectionsByName);
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
      addWorkspaceItemIfPresent(runItem, outputs.iVsLambda(), PlottingWorkspaceOutputType::IvsLambda, group.name(),
                                row.runNumbers(), m_workspaceSelectionsByName);
      addWorkspaceItemIfPresent(runItem, outputs.iVsQ(), PlottingWorkspaceOutputType::IvsQ, group.name(),
                                row.runNumbers(), m_workspaceSelectionsByName);
      addWorkspaceItemIfPresent(runItem, outputs.iVsQBinned(), PlottingWorkspaceOutputType::IvsQBinned, group.name(),
                                row.runNumbers(), m_workspaceSelectionsByName);

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
PlottingWorkspaceTree::selectedWorkspacesFor(std::vector<std::string> const &workspaceNames) const {
  auto selections = std::vector<PlottingWorkspaceSelection>{};
  selections.reserve(workspaceNames.size());
  for (auto const &workspaceName : workspaceNames) {
    auto const workspaceSelection = m_workspaceSelectionsByName.find(workspaceName);
    if (workspaceSelection != m_workspaceSelectionsByName.cend()) {
      selections.emplace_back(workspaceSelection->second);
    }
  }
  return selections;
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
