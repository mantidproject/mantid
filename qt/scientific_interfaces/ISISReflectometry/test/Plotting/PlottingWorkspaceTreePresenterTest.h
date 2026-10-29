// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Plotting/presenter/PlottingWorkspaceTreePresenter.h"
#include "PlottingWorkspaceTreeTestHelpers.h"

#include <algorithm>
#include <cxxtest/TestSuite.h>
#include <set>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

class PlottingWorkspaceTreePresenterTest : public CxxTest::TestSuite, private PlottingWorkspaceTreeTestHelpers {
public:
  void testTreeItemsHavePreparedDisplayLabels() {
    TestTreeView view;
    PlottingWorkspaceTreePresenter presenter(view);
    auto items = plottingWorkspaceTreeItemStatesWithMutedIvsLambda();
    items[0].children.emplace_back(workspaceGroupItem("workspace_group", {}));
    presenter.setPlottingWorkspaceTreeItemStates(items);

    auto const &group = view.displayedItems[0];
    auto const &run = group.children[0];
    TS_ASSERT_EQUALS(group.itemTypeLabel, "Group");
    TS_ASSERT_EQUALS(group.outputTypeLabel, "");
    TS_ASSERT_EQUALS(group.label, "Group 1");
    TS_ASSERT_EQUALS(run.itemTypeLabel, "Run");
    TS_ASSERT_EQUALS(run.outputTypeLabel, "");
    TS_ASSERT_EQUALS(run.label, "12345");
    TS_ASSERT_EQUALS(run.children[0].itemTypeLabel, "Workspace");
    TS_ASSERT_EQUALS(run.children[0].outputTypeLabel, "IvsLambda");
    TS_ASSERT_EQUALS(run.children[0].label, "IvsLam_12345");
    TS_ASSERT(run.children[0].muted);
    TS_ASSERT_EQUALS(run.children[1].outputTypeLabel, "IvsQ");
    TS_ASSERT_EQUALS(run.children[2].outputTypeLabel, "IvsQBinned");
    TS_ASSERT_EQUALS(group.children[1].itemTypeLabel, "WorkspaceGroup");
    TS_ASSERT_EQUALS(group.children[1].outputTypeLabel, "");
    TS_ASSERT_EQUALS(group.children[1].label, "workspace_group");
  }

  void testSelectedWorkspaceItemsReturnsOnlyWorkspaceItems() {
    TestTreeView view;
    PlottingWorkspaceTreePresenter presenter(view);
    presenter.filterWorkspaces("", {ReducedWorkspaceOutputType::IvsQ, ReducedWorkspaceOutputType::IvsQBinned});
    presenter.setPlottingWorkspaceTreeItemStates(plottingWorkspaceTreeItemStates());

    presenter.notifyItemClicked({0}, false, false);

    auto const selectedWorkspaces = presenter.selectedPlottingWorkspaceNames();
    TS_ASSERT_EQUALS(selectedWorkspaces.size(), 2);
    TS_ASSERT_EQUALS(selectedWorkspaces[0], "IvsQ_12345");
    TS_ASSERT_EQUALS(selectedWorkspaces[1], "IvsQ_binned_12345");
  }

  void testSelectingParentReturnsOnlySelectableChildWorkspaces() {
    TestTreeView view;
    PlottingWorkspaceTreePresenter presenter(view);
    presenter.setPlottingWorkspaceTreeItemStates(plottingWorkspaceTreeItemStatesForSpinAsymmetry());

    presenter.notifyItemClicked({0, 0}, false, false);

    TS_ASSERT(view.isSelected({0, 0}));
    TS_ASSERT_EQUALS(presenter.selectedPlottingWorkspaceNames(), std::vector<std::string>{"IvsQ_binned_12345"});
  }

  void testSelectableWorkspaceGroupReturnsWorkspaceGroupName() {
    TestTreeView view;
    PlottingWorkspaceTreePresenter presenter(view);
    presenter.setPlottingWorkspaceTreeItemStates(plottingWorkspaceTreeItemStatesWithWorkspaceGroupsForSpinAsymmetry());

    presenter.notifyItemClicked({0, 0, 1}, false, false);

    TS_ASSERT(view.isSelected({0, 0, 1}));
    TS_ASSERT_EQUALS(presenter.selectedPlottingWorkspaceNames(), std::vector<std::string>{"IvsQ_binned_group_1"});
  }

  void testSelectedWorkspaceNamesReturnsWorkspaceGroupChildren() {
    TestTreeView view;
    PlottingWorkspaceTreePresenter presenter(view);
    presenter.setPlottingWorkspaceTreeItemStates(plottingWorkspaceTreeItemStatesWithWorkspaceGroups());

    presenter.notifyItemClicked({0, 0, 1}, false, false);

    TS_ASSERT_EQUALS(presenter.selectedPlottingWorkspaceNames(), std::vector<std::string>{"IvsQ_binned_group_1"});
  }

  void testWorkspaceOutputTypeAndRegexFiltersHideRowsAndEmptyParents() {
    TestTreeView view;
    PlottingWorkspaceTreePresenter presenter(view);
    presenter.setPlottingWorkspaceTreeItemStates(
        {groupItem("sample", {runItem("12345", {workspaceItem("lambda", ReducedWorkspaceOutputType::IvsLambda),
                                                workspaceItem("q", ReducedWorkspaceOutputType::IvsQ),
                                                workspaceItem("binned", ReducedWorkspaceOutputType::IvsQBinned)}),
                              runItem("67890", {workspaceItem("other", ReducedWorkspaceOutputType::IvsQBinned)})}),
         groupItem("empty", {})});

    TS_ASSERT(!view.isVisible({0, 0, 0}));
    TS_ASSERT(!view.isVisible({0, 0, 1}));
    TS_ASSERT(view.isVisible({0, 0, 2}));
    TS_ASSERT(!view.isVisible({1}));

    presenter.filterWorkspaces("^12345$", {ReducedWorkspaceOutputType::IvsQBinned, ReducedWorkspaceOutputType::IvsQ});

    TS_ASSERT(view.isVisible({0}));
    TS_ASSERT(view.isVisible({0, 0}));
    TS_ASSERT(!view.isVisible({0, 0, 0}));
    TS_ASSERT(view.isVisible({0, 0, 1}));
    TS_ASSERT(view.isVisible({0, 0, 2}));
    TS_ASSERT(!view.isVisible({0, 1}));
  }

  void testInvalidRegexRetainsLastValidExpressionWhileOutputTypesStillUpdate() {
    TestTreeView view;
    PlottingWorkspaceTreePresenter presenter(view);
    presenter.setPlottingWorkspaceTreeItemStates(plottingWorkspaceTreeItemStatesWithBinnedOutput());
    presenter.filterWorkspaces("binned", {ReducedWorkspaceOutputType::IvsQBinned});
    TS_ASSERT(view.isVisible({0, 0, 2}));

    presenter.filterWorkspaces("[", {ReducedWorkspaceOutputType::IvsQBinned, ReducedWorkspaceOutputType::IvsQ});
    TS_ASSERT(!view.isVisible({0, 0, 1}));
    presenter.filterWorkspaces("[", {ReducedWorkspaceOutputType::IvsQ});
    TS_ASSERT(!view.isVisible({0, 0, 2}));
    presenter.filterWorkspaces("[", {ReducedWorkspaceOutputType::IvsQBinned, ReducedWorkspaceOutputType::IvsQ});
    TS_ASSERT(view.isVisible({0, 0, 2}));
  }

  void testWorkspaceFilterIsReappliedWhenTreeIsRebuilt() {
    TestTreeView view;
    PlottingWorkspaceTreePresenter presenter(view);
    presenter.setPlottingWorkspaceTreeItemStates(
        {groupItem("original", {runItem("12345", {workspaceItem("first", ReducedWorkspaceOutputType::IvsQBinned),
                                                  workspaceItem("second", ReducedWorkspaceOutputType::IvsQBinned)})})});
    presenter.filterWorkspaces("second", {ReducedWorkspaceOutputType::IvsQBinned});
    presenter.setPlottingWorkspaceTreeItemStates({groupItem(
        "replacement", {runItem("67890", {workspaceItem("second", ReducedWorkspaceOutputType::IvsQBinned),
                                          workspaceItem("first", ReducedWorkspaceOutputType::IvsQBinned)})})});

    TS_ASSERT(view.isVisible({0, 0, 0}));
    TS_ASSERT(!view.isVisible({0, 0, 1}));
  }

  void testFilteringPreservesHiddenSelectionsAndAllowsSelectionToAccumulate() {
    TestTreeView view;
    PlottingWorkspaceTreePresenter presenter(view);
    presenter.filterWorkspaces("", {ReducedWorkspaceOutputType::IvsQBinned, ReducedWorkspaceOutputType::IvsQ});
    presenter.setPlottingWorkspaceTreeItemStates(plottingWorkspaceTreeItemStates());
    presenter.notifyItemClicked({0, 0, 0}, false, false);

    presenter.filterWorkspaces("", {ReducedWorkspaceOutputType::IvsQBinned});
    presenter.notifyItemClicked({0, 0, 1}, true, true);

    TS_ASSERT(!view.isVisible({0, 0, 0}));
    TS_ASSERT(view.isSelected({0, 0, 0}));
    TS_ASSERT(view.isSelected({0, 0, 1}));
    TS_ASSERT_EQUALS(presenter.selectedPlottingWorkspaceNames().size(), 2);
  }

  void testPartiallyFilteredCompleteWorkspaceGroupSelectionReturnsAllWorkspaceNames() {
    TestTreeView view;
    PlottingWorkspaceTreePresenter presenter(view);
    auto spinGroup =
        workspaceGroupItem("spin_group", {workspaceItem("spin_up", ReducedWorkspaceOutputType::IvsQBinned),
                                          workspaceItem("spin_down", ReducedWorkspaceOutputType::IvsQBinned)});
    for (auto &child : spinGroup.children) {
      child = mutedItem(std::move(child), true);
    }
    presenter.setPlottingWorkspaceTreeItemStates({groupItem("Group 1", {runItem("12345", {std::move(spinGroup)})})});
    presenter.filterWorkspaces("^spin_up$", {ReducedWorkspaceOutputType::IvsQBinned});

    presenter.notifyItemClicked({0, 0, 0}, false, false);

    TS_ASSERT(view.isSelected({0, 0, 0}));
    TS_ASSERT(view.isSelected({0, 0, 0, 0}));
    TS_ASSERT(!view.isSelected({0, 0, 0, 1}));
    TS_ASSERT_EQUALS(presenter.selectedPlottingWorkspaceNames(), std::vector<std::string>({"spin_up", "spin_down"}));
  }

  void testDeselectingAndReselectingCompleteWorkspaceGroupCannotReturnPartialOrReorderedSpinStates() {
    TestTreeView view;
    PlottingWorkspaceTreePresenter presenter(view);
    auto spinGroup =
        workspaceGroupItem("spin_group", {workspaceItem("spin_1", ReducedWorkspaceOutputType::IvsQBinned),
                                          workspaceItem("spin_2", ReducedWorkspaceOutputType::IvsQBinned),
                                          workspaceItem("spin_3", ReducedWorkspaceOutputType::IvsQBinned),
                                          workspaceItem("spin_4", ReducedWorkspaceOutputType::IvsQBinned)});
    for (auto &child : spinGroup.children) {
      child = mutedItem(std::move(child), true);
    }
    presenter.setPlottingWorkspaceTreeItemStates({groupItem("Group 1", {runItem("12345", {std::move(spinGroup)})})});
    presenter.notifyItemClicked({0, 0, 0}, false, false);
    presenter.filterWorkspaces("^spin_[12]$", {ReducedWorkspaceOutputType::IvsQBinned});

    presenter.notifyItemClicked({0, 0, 0}, false, false);

    TS_ASSERT(view.isSelected({0, 0, 0, 2}));
    TS_ASSERT(view.isSelected({0, 0, 0, 3}));
    TS_ASSERT(presenter.selectedPlottingWorkspaceNames().empty());

    presenter.notifyItemClicked({0, 0, 0}, true, true);

    TS_ASSERT_EQUALS(presenter.selectedPlottingWorkspaceNames(),
                     std::vector<std::string>({"spin_1", "spin_2", "spin_3", "spin_4"}));
  }

  void testPartiallyFilteredOrdinaryWorkspaceGroupReturnsOnlyVisibleSelectedWorkspaceNames() {
    TestTreeView view;
    PlottingWorkspaceTreePresenter presenter(view);
    presenter.setPlottingWorkspaceTreeItemStates({groupItem(
        "Group 1",
        {runItem("12345", {workspaceGroupItem("workspace_group",
                                              {workspaceItem("first", ReducedWorkspaceOutputType::IvsQBinned),
                                               workspaceItem("second", ReducedWorkspaceOutputType::IvsQBinned)})})})});
    presenter.filterWorkspaces("^first$", {ReducedWorkspaceOutputType::IvsQBinned});

    presenter.notifyItemClicked({0, 0, 0}, false, false);

    TS_ASSERT(view.isSelected({0, 0, 0, 0}));
    TS_ASSERT(!view.isSelected({0, 0, 0, 1}));
    TS_ASSERT_EQUALS(presenter.selectedPlottingWorkspaceNames(), std::vector<std::string>({"first"}));
  }

  void testHiddenSelectedWorkspaceGroupStillContributesToSelectionCount() {
    TestTreeView view;
    PlottingWorkspaceTreePresenter presenter(view);
    presenter.setPlottingWorkspaceTreeItemStates(plottingWorkspaceTreeItemStatesWithWorkspaceGroups());
    presenter.notifyItemClicked({0, 0, 1}, false, false);

    presenter.filterWorkspaces("", {});

    TS_ASSERT(view.isSelected({0, 0, 1}));
    TS_ASSERT_EQUALS(presenter.selectedPlottingWorkspaceGroupCount(), 1);
    TS_ASSERT_EQUALS(presenter.selectedPlottingWorkspaceNames(), std::vector<std::string>{"IvsQ_binned_group_1"});
  }

private:
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
};
