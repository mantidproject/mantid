// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Plotting/QtPlottingView.h"

#include <QApplication>
#include <QItemSelectionModel>
#include <QMouseEvent>
#include <QTreeView>
#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

class QtPlottingViewTest : public CxxTest::TestSuite {
public:
  void testSelectingGroupSelectsChildRunsAndWorkspaces() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItems());
    auto tree = workspaceTree(view);
    auto group = groupIndex(tree);
    auto run = runIndex(tree);
    auto workspace = workspaceIndex(tree);

    tree->selectionModel()->select(group, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

    TS_ASSERT(tree->selectionModel()->isSelected(group));
    TS_ASSERT(tree->selectionModel()->isSelected(run));
    TS_ASSERT(tree->selectionModel()->isSelected(workspace));
  }

  void testSelectingRunSelectsChildWorkspaces() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItems());
    auto tree = workspaceTree(view);
    auto run = runIndex(tree);
    auto workspace = workspaceIndex(tree);

    tree->selectionModel()->select(run, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

    TS_ASSERT(tree->selectionModel()->isSelected(run));
    TS_ASSERT(tree->selectionModel()->isSelected(workspace));
  }

  void testDeselectingGroupDeselectsChildRunsAndWorkspaces() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItems());
    auto tree = workspaceTree(view);
    auto group = groupIndex(tree);
    auto run = runIndex(tree);
    auto workspace = workspaceIndex(tree);

    tree->selectionModel()->select(group, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    tree->selectionModel()->select(group, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);

    TS_ASSERT(!tree->selectionModel()->isSelected(group));
    TS_ASSERT(!tree->selectionModel()->isSelected(run));
    TS_ASSERT(!tree->selectionModel()->isSelected(workspace));
  }

  void testClickingSelectedRunAgainDeselectsRunAndChildren() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItems());
    auto tree = workspaceTree(view);
    auto run = runIndex(tree);
    auto workspace = workspaceIndex(tree);

    click(tree, run);
    click(tree, run);

    TS_ASSERT(!tree->selectionModel()->isSelected(run));
    TS_ASSERT(!tree->selectionModel()->isSelected(workspace));
  }

  void testControlClickSelectsNonAdjacentWorkspaceUnderDifferentParents() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItemsWithGroups(2));
    auto tree = workspaceTree(view);
    auto workspace1 = workspaceIndex(tree, 0, 0, 0);
    auto workspace2 = workspaceIndex(tree, 1, 0, 0);

    click(tree, workspace1);
    click(tree, workspace2, Qt::ControlModifier);

    TS_ASSERT(tree->selectionModel()->isSelected(workspace1));
    TS_ASSERT(tree->selectionModel()->isSelected(workspace2));
  }

  void testControlClickDoesNotSelectIntermediateWorkspaces() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItemsWithGroups(3));
    auto tree = workspaceTree(view);
    auto workspace1 = workspaceIndex(tree, 0, 0, 0);
    auto workspace2 = workspaceIndex(tree, 1, 0, 0);
    auto workspace3 = workspaceIndex(tree, 2, 0, 0);

    click(tree, workspace1);
    click(tree, workspace3, Qt::ControlModifier);

    TS_ASSERT(tree->selectionModel()->isSelected(workspace1));
    TS_ASSERT(!tree->selectionModel()->isSelected(workspace2));
    TS_ASSERT(tree->selectionModel()->isSelected(workspace3));
  }

  void testShiftClickDoesNotSelectIntermediateWorkspaces() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItemsWithGroups(3));
    auto tree = workspaceTree(view);
    auto workspace1 = workspaceIndex(tree, 0, 0, 0);
    auto workspace2 = workspaceIndex(tree, 1, 0, 0);
    auto workspace3 = workspaceIndex(tree, 2, 0, 0);

    click(tree, workspace1);
    click(tree, workspace3, Qt::ShiftModifier);

    TS_ASSERT(tree->selectionModel()->isSelected(workspace1));
    TS_ASSERT(!tree->selectionModel()->isSelected(workspace2));
    TS_ASSERT(tree->selectionModel()->isSelected(workspace3));
  }

  void testShiftClickSelectsClickedRunAndChildren() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItemsWithGroups(3));
    auto tree = workspaceTree(view);
    auto workspace1 = workspaceIndex(tree, 0, 0, 0);
    auto run3 = runIndex(tree, 2, 0);
    auto workspace3 = workspaceIndex(tree, 2, 0, 0);

    click(tree, workspace1);
    click(tree, run3, Qt::ShiftModifier);

    TS_ASSERT(tree->selectionModel()->isSelected(workspace1));
    TS_ASSERT(tree->selectionModel()->isSelected(run3));
    TS_ASSERT(tree->selectionModel()->isSelected(workspace3));
  }

  void testClickingParentGroupAfterSelectedRunSelectsAllDescendants() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItems());
    auto tree = workspaceTree(view);
    auto group = groupIndex(tree);
    auto run = runIndex(tree);
    auto workspace = workspaceIndex(tree);

    click(tree, run);
    click(tree, group);

    TS_ASSERT(tree->selectionModel()->isSelected(group));
    TS_ASSERT(tree->selectionModel()->isSelected(run));
    TS_ASSERT(tree->selectionModel()->isSelected(workspace));
  }

  void testClickingWorkspaceUnderSelectedRunSwitchesSelectionToWorkspace() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItems());
    auto tree = workspaceTree(view);
    auto run = runIndex(tree);
    auto workspace = workspaceIndex(tree);

    click(tree, run);
    click(tree, workspace);

    TS_ASSERT(!tree->selectionModel()->isSelected(run));
    TS_ASSERT(tree->selectionModel()->isSelected(workspace));
  }

  void testClickingRunUnderSelectedGroupSwitchesSelectionToRunAndWorkspaces() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItems());
    auto tree = workspaceTree(view);
    auto group = groupIndex(tree);
    auto run = runIndex(tree);
    auto workspace = workspaceIndex(tree);

    click(tree, group);
    click(tree, run);

    TS_ASSERT(!tree->selectionModel()->isSelected(group));
    TS_ASSERT(tree->selectionModel()->isSelected(run));
    TS_ASSERT(tree->selectionModel()->isSelected(workspace));
  }

  void testShiftClickingWorkspaceUnderSelectedRunDoesNotChangeSelection() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItems());
    auto tree = workspaceTree(view);
    auto run = runIndex(tree);
    auto workspace = workspaceIndex(tree);

    click(tree, run);
    click(tree, workspace, Qt::ShiftModifier);

    TS_ASSERT(tree->selectionModel()->isSelected(run));
    TS_ASSERT(tree->selectionModel()->isSelected(workspace));
  }

  void testShiftClickingRunUnderSelectedGroupDoesNotChangeSelection() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItems());
    auto tree = workspaceTree(view);
    auto group = groupIndex(tree);
    auto run = runIndex(tree);
    auto workspace = workspaceIndex(tree);

    click(tree, group);
    click(tree, run, Qt::ShiftModifier);

    TS_ASSERT(tree->selectionModel()->isSelected(group));
    TS_ASSERT(tree->selectionModel()->isSelected(run));
    TS_ASSERT(tree->selectionModel()->isSelected(workspace));
  }

  void testDoubleClickDoesNotChangeSelection() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItemsWithGroups(2));
    auto tree = workspaceTree(view);
    auto workspace1 = workspaceIndex(tree, 0, 0, 0);
    auto workspace2 = workspaceIndex(tree, 1, 0, 0);

    click(tree, workspace1);
    doubleClick(tree, workspace2);

    TS_ASSERT(tree->selectionModel()->isSelected(workspace1));
    TS_ASSERT(!tree->selectionModel()->isSelected(workspace2));
  }

  void testClickAndDragDoesNotSelectDraggedOverWorkspace() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItemsWithGroups(2));
    auto tree = workspaceTree(view);
    auto workspace1 = workspaceIndex(tree, 0, 0, 0);
    auto workspace2 = workspaceIndex(tree, 1, 0, 0);

    clickAndDrag(tree, workspace1, workspace2);

    TS_ASSERT(tree->selectionModel()->isSelected(workspace1));
    TS_ASSERT(!tree->selectionModel()->isSelected(workspace2));
  }

private:
  std::vector<PlottingWorkspaceTreeItem> workspaceItems() const {
    return {{"Group 1", {{"12345", {{"IvsLam_12345", {}}, {"IvsQ_12345", {}}}}}}};
  }

  std::vector<PlottingWorkspaceTreeItem> workspaceItemsWithGroups(int groups) const {
    std::vector<PlottingWorkspaceTreeItem> items;
    for (auto group = 1; group <= groups; ++group) {
      auto const run = std::to_string(group) + "2345";
      items.emplace_back(PlottingWorkspaceTreeItem{"Group " + std::to_string(group), {{run, {{"IvsLam_" + run, {}}}}}});
    }
    return items;
  }

  QTreeView *workspaceTree(QtPlottingView &view) const { return view.findChild<QTreeView *>("workspaceTree"); }

  QModelIndex groupIndex(QTreeView *tree) const { return tree->model()->index(0, 0); }

  QModelIndex runIndex(QTreeView *tree) const { return tree->model()->index(0, 0, groupIndex(tree)); }

  QModelIndex runIndex(QTreeView *tree, int group, int run) const {
    return tree->model()->index(run, 0, tree->model()->index(group, 0));
  }

  QModelIndex workspaceIndex(QTreeView *tree) const { return tree->model()->index(0, 0, runIndex(tree)); }

  QModelIndex workspaceIndex(QTreeView *tree, int group, int run, int workspace) const {
    auto const groupModelIndex = tree->model()->index(group, 0);
    auto const runModelIndex = tree->model()->index(run, 0, groupModelIndex);
    return tree->model()->index(workspace, 0, runModelIndex);
  }

  void click(QTreeView *tree, QModelIndex const &index, Qt::KeyboardModifiers modifiers = Qt::NoModifier) const {
    auto const position = tree->visualRect(index).center();
    auto event = QMouseEvent(QEvent::MouseButtonPress, position, Qt::LeftButton, Qt::LeftButton, modifiers);
    QApplication::sendEvent(tree->viewport(), &event);
  }

  void doubleClick(QTreeView *tree, QModelIndex const &index, Qt::KeyboardModifiers modifiers = Qt::NoModifier) const {
    auto const position = tree->visualRect(index).center();
    auto event = QMouseEvent(QEvent::MouseButtonDblClick, position, Qt::LeftButton, Qt::LeftButton, modifiers);
    QApplication::sendEvent(tree->viewport(), &event);
  }

  void clickAndDrag(QTreeView *tree, QModelIndex const &from, QModelIndex const &to) const {
    auto const fromPosition = tree->visualRect(from).center();
    auto const toPosition = tree->visualRect(to).center();
    auto pressEvent = QMouseEvent(QEvent::MouseButtonPress, fromPosition, Qt::LeftButton, Qt::LeftButton, {});
    auto moveEvent = QMouseEvent(QEvent::MouseMove, toPosition, Qt::NoButton, Qt::LeftButton, {});
    auto releaseEvent = QMouseEvent(QEvent::MouseButtonRelease, toPosition, Qt::LeftButton, Qt::NoButton, {});

    QApplication::sendEvent(tree->viewport(), &pressEvent);
    QApplication::sendEvent(tree->viewport(), &moveEvent);
    QApplication::sendEvent(tree->viewport(), &releaseEvent);
  }
};
