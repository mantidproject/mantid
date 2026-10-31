// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Plotting/view/IPlottingWorkspaceTreeView.h"
#include "GUI/Plotting/view/PlottingViewState.h"
#include <algorithm>
#include <set>
#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/// Workspace tree fixtures shared by presenter and Qt integration tests.
class PlottingWorkspaceTreeTestHelpers {
public:
  class TestTreeView : public IPlottingWorkspaceTreeView {
  public:
    void subscribe(PlottingWorkspaceTreeViewSubscriber *subscriber) override { m_subscriber = subscriber; }
    void setItems(std::vector<PlottingWorkspaceTreeViewItem> const &items) override {
      displayedItems = items;
      clearSelection();
      m_hidden.clear();
    }
    void expandAll() override {}
    void setVisible(PlottingWorkspaceTreePath const &path, bool visible) override {
      if (visible) {
        m_hidden.erase(path);
      } else {
        m_hidden.insert(path);
      }
    }
    bool isVisible(PlottingWorkspaceTreePath const &path) const override { return m_hidden.count(path) == 0; }
    void clearSelection() override {
      auto previous = std::move(m_selected);
      m_selected.clear();
      if (m_subscriber && !previous.empty()) {
        m_subscriber->notifySelectionChanged(previous, {});
      }
    }
    void setSelected(PlottingWorkspaceTreePath const &path, bool selected) override {
      auto found = std::find(m_selected.begin(), m_selected.end(), path);
      if (selected == (found != m_selected.end())) {
        return;
      }
      if (selected) {
        m_selected.emplace_back(path);
      } else {
        m_selected.erase(found);
      }
      if (m_subscriber) {
        m_subscriber->notifySelectionChanged(
            selected ? std::vector<PlottingWorkspaceTreePath>{} : std::vector<PlottingWorkspaceTreePath>{path},
            selected ? std::vector<PlottingWorkspaceTreePath>{path} : std::vector<PlottingWorkspaceTreePath>{});
      }
    }
    bool isSelected(PlottingWorkspaceTreePath const &path) const override {
      return std::find(m_selected.cbegin(), m_selected.cend(), path) != m_selected.cend();
    }
    std::vector<PlottingWorkspaceTreePath> selectedItems() const override { return m_selected; }

    std::vector<PlottingWorkspaceTreeViewItem> displayedItems;

  private:
    PlottingWorkspaceTreeViewSubscriber *m_subscriber{nullptr};
    std::vector<PlottingWorkspaceTreePath> m_selected;
    std::set<PlottingWorkspaceTreePath> m_hidden;
  };
  std::vector<PlottingWorkspaceTreeItemState> plottingWorkspaceTreeItemStates() const {
    return {groupItem(
        "Group 1", {runItem("12345", {workspaceItem("IvsQ_12345", ReducedWorkspaceOutputType::IvsQ),
                                      workspaceItem("IvsQ_binned_12345", ReducedWorkspaceOutputType::IvsQBinned)})})};
  }

  std::vector<PlottingWorkspaceTreeItemState> plottingWorkspaceTreeItemStatesWithGroups(int groups) const {
    std::vector<PlottingWorkspaceTreeItemState> items;
    for (auto group = 1; group <= groups; ++group) {
      auto const run = std::to_string(group) + "2345";
      items.emplace_back(
          groupItem("Group " + std::to_string(group),
                    {runItem(run, {workspaceItem("IvsQ_binned_" + run, ReducedWorkspaceOutputType::IvsQBinned)})}));
    }
    return items;
  }

  std::vector<PlottingWorkspaceTreeItemState> plottingWorkspaceTreeItemStatesWithBinnedOutput() const {
    return {groupItem(
        "Group 1", {runItem("12345", {workspaceItem("IvsLam_12345", ReducedWorkspaceOutputType::IvsLambda),
                                      workspaceItem("IvsQ_12345", ReducedWorkspaceOutputType::IvsQ),
                                      workspaceItem("IvsQ_binned_12345", ReducedWorkspaceOutputType::IvsQBinned)})})};
  }

  std::vector<PlottingWorkspaceTreeItemState> plottingWorkspaceTreeItemStatesWithMutedIvsLambda() const {
    auto items = plottingWorkspaceTreeItemStatesWithBinnedOutput();
    items[0].children[0].children[0] = mutedItem(std::move(items[0].children[0].children[0]), false);
    return items;
  }

  std::vector<PlottingWorkspaceTreeItemState>
  plottingWorkspaceTreeItemStatesWithBinnedWorkspaces(int workspaceCount) const {
    auto workspaces = std::vector<PlottingWorkspaceTreeItemState>{};
    for (auto workspace = 1; workspace <= workspaceCount; ++workspace) {
      workspaces.emplace_back(
          workspaceItem("IvsQ_binned_" + std::to_string(workspace), ReducedWorkspaceOutputType::IvsQBinned));
    }
    return {groupItem("Group 1", {runItem("12345", std::move(workspaces))})};
  }

  std::vector<PlottingWorkspaceTreeItemState>
  plottingWorkspaceTreeItemStatesWithBinnedWorkspaceGroups(int workspaceGroupCount) const {
    auto workspaceGroups = std::vector<PlottingWorkspaceTreeItemState>{};
    for (auto workspaceGroup = 1; workspaceGroup <= workspaceGroupCount; ++workspaceGroup) {
      workspaceGroups.emplace_back(
          workspaceGroupItem("IvsQ_binned_group_" + std::to_string(workspaceGroup),
                             {workspaceItem("IvsQ_binned_group_" + std::to_string(workspaceGroup) + "_1",
                                            ReducedWorkspaceOutputType::IvsQBinned)}));
    }
    return {groupItem("Group 1", {runItem("12345", std::move(workspaceGroups))})};
  }

  std::vector<PlottingWorkspaceTreeItemState> plottingWorkspaceTreeItemStatesWithWorkspaceGroups() const {
    return {groupItem(
        "Group 1",
        {runItem("12345",
                 {workspaceGroupItem("IvsLam_group",
                                     {workspaceItem("IvsLam_group_1", ReducedWorkspaceOutputType::IvsLambda)}),
                  workspaceGroupItem("IvsQ_binned_group", {workspaceItem("IvsQ_binned_group_1",
                                                                         ReducedWorkspaceOutputType::IvsQBinned)})})})};
  }

  std::vector<PlottingWorkspaceTreeItemState> plottingWorkspaceTreeItemStatesWithMutedIvsLambdaWorkspaceGroup() const {
    auto items = plottingWorkspaceTreeItemStatesWithWorkspaceGroups();
    items[0].children[0].children[0] = mutedItem(std::move(items[0].children[0].children[0]), false);
    items[0].children[0].children[0].children[0] =
        mutedItem(std::move(items[0].children[0].children[0].children[0]), false);
    return items;
  }

  std::vector<PlottingWorkspaceTreeItemState> plottingWorkspaceTreeItemStatesForSpinAsymmetry() const {
    auto items = plottingWorkspaceTreeItemStatesWithBinnedOutput();
    auto &workspaces = items[0].children[0].children;
    workspaces[0] = mutedItem(std::move(workspaces[0]), false);
    workspaces[1] = mutedItem(std::move(workspaces[1]), false);
    workspaces[2] = mutedItem(std::move(workspaces[2]), true);
    return items;
  }

  std::vector<PlottingWorkspaceTreeItemState>
  plottingWorkspaceTreeItemStatesWithWorkspaceGroupsForSpinAsymmetry() const {
    auto items = plottingWorkspaceTreeItemStatesWithWorkspaceGroups();
    auto &workspaceGroups = items[0].children[0].children;
    workspaceGroups[0] = mutedItem(std::move(workspaceGroups[0]), false);
    workspaceGroups[0].children[0] = mutedItem(std::move(workspaceGroups[0].children[0]), false);
    workspaceGroups[1].children[0] = mutedItem(std::move(workspaceGroups[1].children[0]), true);
    return items;
  }

  std::vector<PlottingWorkspaceTreeItemState> plottingWorkspaceTreeItemStatesWithStitchedOutput() const {
    return {groupItem(
        "Group 1", {workspaceItem("stitched_12345", ReducedWorkspaceOutputType::IvsQBinned),
                    workspaceGroupItem("stitched_group",
                                       {workspaceItem("stitched_group_1", ReducedWorkspaceOutputType::IvsQBinned)})})};
  }

  std::vector<PlottingWorkspaceTreeItemState> plottingWorkspaceTreeItemStatesWithMutedStitchedOutput() const {
    auto items = plottingWorkspaceTreeItemStatesWithStitchedOutput();
    items[0].children[0] = mutedItem(std::move(items[0].children[0]), false);
    items[0].children[1] = mutedItem(std::move(items[0].children[1]), false);
    items[0].children[1].children[0] = mutedItem(std::move(items[0].children[1].children[0]), false);
    return items;
  }

  PlottingWorkspaceTreeItemState groupItem(std::string label,
                                           std::vector<PlottingWorkspaceTreeItemState> children) const {
    return {std::move(label), PlottingWorkspaceTreeItemType::ReductionGroup, ReducedWorkspaceOutputType::None, "",
            std::move(children)};
  }

  PlottingWorkspaceTreeItemState runItem(std::string label,
                                         std::vector<PlottingWorkspaceTreeItemState> children) const {
    return {std::move(label), PlottingWorkspaceTreeItemType::Run, ReducedWorkspaceOutputType::None, "",
            std::move(children)};
  }

  PlottingWorkspaceTreeItemState workspaceGroupItem(std::string label,
                                                    std::vector<PlottingWorkspaceTreeItemState> children) const {
    auto const workspaceName = label;
    return {std::move(label), PlottingWorkspaceTreeItemType::WorkspaceGroup, ReducedWorkspaceOutputType::None,
            workspaceName, std::move(children)};
  }

  PlottingWorkspaceTreeItemState workspaceItem(std::string label, ReducedWorkspaceOutputType outputType) const {
    auto const workspaceName = label;
    return {std::move(label), PlottingWorkspaceTreeItemType::Workspace, outputType, workspaceName, {}};
  }

  PlottingWorkspaceTreeItemState mutedItem(PlottingWorkspaceTreeItemState item, bool selectableAsChild) const {
    item.muted = true;
    item.selectionMode =
        selectableAsChild ? PlottingWorkspaceTreeSelectionMode::ParentOnly : PlottingWorkspaceTreeSelectionMode::None;
    return item;
  }
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
