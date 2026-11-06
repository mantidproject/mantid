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
using PlottingWorkspacesByName = std::unordered_map<std::string, PlottingWorkspace>;

/// Inherit the parent's scope items and origin, then record the next child's actual position in the tree.
PlottingWorkspaceContext contextForChildItem(PlottingWorkspaceTreeItem const &parentItem,
                                             PlottingWorkspaceTreeItemType childItemType,
                                             std::string const &childWorkspaceName) {
  auto childPath = parentItem.context.scopeItems.at(parentItem.itemType).path;
  childPath.emplace_back(static_cast<int>(parentItem.children.size()));

  auto childContext = PlottingWorkspaceContext{};
  childContext.scopeItems = parentItem.context.scopeItems;
  childContext.scopeItems[childItemType] = PlottingWorkspaceScopeItem{std::move(childPath), childWorkspaceName};
  childContext.origin = parentItem.context.origin;
  return childContext;
}

/// Return the ADS workspace if the name is non-empty and exists.
Mantid::API::Workspace_sptr findWorkspace(std::string const &workspaceName) {
  if (workspaceName.empty()) {
    return nullptr;
  }

  return Mantid::API::AnalysisDataService::Instance().doesExist(workspaceName)
             ? Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::Workspace>(workspaceName)
             : nullptr;
}

/// Return the current period metadata for multi-period matrix workspaces.
std::optional<int> periodNumberForWorkspace(Mantid::API::Workspace const &workspace) {
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

/// Store a plotting workspace by its ADS name.
void storePlottingWorkspace(PlottingWorkspacesByName &plottingWorkspacesByName, std::string const &workspaceName,
                            std::vector<std::string> runNumbers, std::optional<int> periodNumber,
                            PlottingWorkspaceContext const &context) {
  plottingWorkspacesByName[workspaceName] =
      PlottingWorkspace{workspaceName, std::move(runNumbers), periodNumber, context};
}

/// Add a workspace-group tree item if it has named child workspaces.
void appendWorkspaceGroupItem(PlottingWorkspaceTreeItem &parent, std::string const &workspaceName,
                              Mantid::API::WorkspaceGroup_const_sptr const &workspaceGroup,
                              ReducedWorkspaceOutputType reducedOutputType, PlottingWorkspaceOrigin outputOrigin,
                              PlottingWorkspacesByName &plottingWorkspacesByName) {
  auto workspaceGroupItem = PlottingWorkspaceTreeItem{workspaceName,
                                                      PlottingWorkspaceTreeItemType::WorkspaceGroup,
                                                      ReducedWorkspaceOutputType::None,
                                                      workspaceName,
                                                      {}};
  workspaceGroupItem.context =
      contextForChildItem(parent, PlottingWorkspaceTreeItemType::WorkspaceGroup, workspaceName);
  workspaceGroupItem.context.origin = outputOrigin;
  for (auto index = 0u; index < workspaceGroup->size(); ++index) {
    auto const memberWorkspace = workspaceGroup->getItem(index);
    if (memberWorkspace && !memberWorkspace->getName().empty()) {
      auto child = PlottingWorkspaceTreeItem{memberWorkspace->getName(),
                                             PlottingWorkspaceTreeItemType::Workspace,
                                             reducedOutputType,
                                             memberWorkspace->getName(),
                                             {}};
      child.context =
          contextForChildItem(workspaceGroupItem, PlottingWorkspaceTreeItemType::Workspace, child.workspaceName);
      storePlottingWorkspace(plottingWorkspacesByName, memberWorkspace->getName(),
                             runNumbersForWorkspace(*memberWorkspace), periodNumberForWorkspace(*memberWorkspace),
                             child.context);
      workspaceGroupItem.children.emplace_back(std::move(child));
    }
  }

  if (workspaceGroupItem.children.empty()) {
    return;
  }

  parent.children.emplace_back(std::move(workspaceGroupItem));
}

/// Add a workspace or workspace-group item for a named ADS workspace if it exists.
void appendWorkspaceItemIfPresent(PlottingWorkspaceTreeItem &parent, std::string const &workspaceName,
                                  ReducedWorkspaceOutputType reducedOutputType, PlottingWorkspaceOrigin outputOrigin,
                                  PlottingWorkspacesByName &plottingWorkspacesByName) {
  auto const workspace = findWorkspace(workspaceName);
  if (!workspace) {
    return;
  }

  if (auto const workspaceGroup = std::dynamic_pointer_cast<Mantid::API::WorkspaceGroup const>(workspace);
      workspaceGroup) {
    appendWorkspaceGroupItem(parent, workspaceName, workspaceGroup, reducedOutputType, outputOrigin,
                             plottingWorkspacesByName);
    return;
  }

  auto item = PlottingWorkspaceTreeItem{
      workspaceName, PlottingWorkspaceTreeItemType::Workspace, reducedOutputType, workspaceName, {}};
  item.context = contextForChildItem(parent, PlottingWorkspaceTreeItemType::Workspace, workspaceName);
  item.context.origin = outputOrigin;
  storePlottingWorkspace(plottingWorkspacesByName, workspaceName, runNumbersForWorkspace(*workspace),
                         periodNumberForWorkspace(*workspace), item.context);
  parent.children.emplace_back(std::move(item));
}

} // namespace

void PlottingWorkspaceTree::rebuild(RunsTable const &runsTable) {
  m_items.clear();
  m_plottingWorkspacesByName.clear();
  for (auto const &group : runsTable.reductionJobs().groups()) {
    auto groupItem = PlottingWorkspaceTreeItem{
        group.name(), PlottingWorkspaceTreeItemType::ReductionGroup, ReducedWorkspaceOutputType::None, "", {}};
    groupItem.context.scopeItems[PlotSelectionScope::ReductionGroup] =
        PlottingWorkspaceScopeItem{{static_cast<int>(m_items.size())}, groupItem.workspaceName};
    if (group.success()) {
      for (auto const &workspaceName : group.postprocessedWorkspaceNames()) {
        appendWorkspaceItemIfPresent(groupItem, workspaceName, ReducedWorkspaceOutputType::IvsQBinned,
                                     PlottingWorkspaceOrigin::PostprocessedGroupOutput, m_plottingWorkspacesByName);
      }
    }

    for (auto const &maybeRow : group.rows()) {
      if (!maybeRow.has_value() || !maybeRow->success()) {
        continue;
      }

      auto const &row = maybeRow.value();
      auto runItem = PlottingWorkspaceTreeItem{boost::algorithm::join(row.runNumbers(), "+"),
                                               PlottingWorkspaceTreeItemType::Run,
                                               ReducedWorkspaceOutputType::None,
                                               "",
                                               {}};
      runItem.context = contextForChildItem(groupItem, PlottingWorkspaceTreeItemType::Run, runItem.workspaceName);
      auto const &outputs = row.reducedWorkspaceNames();

      for (size_t i = 0; i < outputs.iVsQ().size(); ++i) {
        appendWorkspaceItemIfPresent(runItem, outputs.iVsLambda()[i], ReducedWorkspaceOutputType::IvsLambda,
                                     PlottingWorkspaceOrigin::RunOutput, m_plottingWorkspacesByName);
        appendWorkspaceItemIfPresent(runItem, outputs.iVsQ()[i], ReducedWorkspaceOutputType::IvsQ,
                                     PlottingWorkspaceOrigin::RunOutput, m_plottingWorkspacesByName);
        appendWorkspaceItemIfPresent(runItem, outputs.iVsQBinned()[i], ReducedWorkspaceOutputType::IvsQBinned,
                                     PlottingWorkspaceOrigin::RunOutput, m_plottingWorkspacesByName);
      }

      if (!runItem.children.empty()) {
        groupItem.children.emplace_back(std::move(runItem));
      }
    }

    if (!groupItem.children.empty()) {
      m_items.emplace_back(std::move(groupItem));
    }
  }
}

std::vector<PlottingWorkspaceTreeItem> const &PlottingWorkspaceTree::items() const { return m_items; }

std::vector<PlottingWorkspace>
PlottingWorkspaceTree::plottingWorkspacesForNames(std::vector<std::string> const &workspaceNames) const {
  auto plottingWorkspaces = std::vector<PlottingWorkspace>{};
  plottingWorkspaces.reserve(workspaceNames.size());
  for (auto const &workspaceName : workspaceNames) {
    auto const plottingWorkspace = m_plottingWorkspacesByName.find(workspaceName);
    if (plottingWorkspace != m_plottingWorkspacesByName.cend()) {
      plottingWorkspaces.emplace_back(plottingWorkspace->second);
    }
  }
  return plottingWorkspaces;
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
