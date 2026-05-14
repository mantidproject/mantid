// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Plotting/QtPlottingView.h"

#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QItemSelectionModel>
#include <QMouseEvent>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <cxxtest/TestSuite.h>
#include <utility>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

class QtPlottingViewTest : public CxxTest::TestSuite {
public:
  void testOnlyReflectivityCurvePresetIsAvailable() {
    QtPlottingView view;
    auto plotPreset = view.findChild<QComboBox *>("plotPreset");

    TS_ASSERT_EQUALS(plotPreset->count(), 1);
    TS_ASSERT_EQUALS(plotPreset->currentText().toStdString(), "Reflectivity Curve");
    TS_ASSERT_EQUALS(view.selectedPlotOutputType(), PlotOutputType::ReflectivityCurve);
  }

  void testPlotButtonsHaveExpectedLabelsAndOrder() {
    QtPlottingView view;
    auto const optionsLayout = view.findChild<QVBoxLayout *>("optionsLayout");

    assertButton(optionsLayout->itemAt(3)->widget(), "plotIndividual", "Plot");
    assertButton(optionsLayout->itemAt(4)->widget(), "plotOverplot", "Plot over");
    assertButton(optionsLayout->itemAt(5)->widget(), "plotTiled", "Plot tiled");
  }

  void testWorkspaceTreeHasExpectedColumnHeaders() {
    QtPlottingView view;
    auto tree = workspaceTree(view);

    TS_ASSERT_EQUALS(tree->model()->headerData(0, Qt::Horizontal).toString().toStdString(), "Item type");
    TS_ASSERT_EQUALS(tree->model()->headerData(1, Qt::Horizontal).toString().toStdString(), "Output type");
    TS_ASSERT_EQUALS(tree->model()->headerData(2, Qt::Horizontal).toString().toStdString(), "Item");
  }

  void testWorkspaceTreeShowsItemTypeOutputTypeAndItemColumns() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItems());
    auto tree = workspaceTree(view);

    TS_ASSERT_EQUALS(tree->model()->data(groupIndex(tree)).toString().toStdString(), "Group");
    TS_ASSERT_EQUALS(tree->model()->data(groupOutputTypeIndex(tree)).toString().toStdString(), "");
    TS_ASSERT_EQUALS(tree->model()->data(groupItemIndex(tree)).toString().toStdString(), "Group 1");
    TS_ASSERT_EQUALS(tree->model()->data(workspaceIndex(tree)).toString().toStdString(), "Workspace");
    TS_ASSERT_EQUALS(tree->model()->data(workspaceOutputTypeIndex(tree)).toString().toStdString(), "IvsLambda");
    TS_ASSERT_EQUALS(tree->model()->data(workspaceItemIndex(tree)).toString().toStdString(), "IvsLam_12345");
  }

  void testWorkspaceTreeMetadataColumnsUseMutedText() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItems());
    auto tree = workspaceTree(view);

    TS_ASSERT_EQUALS(foregroundColour(tree, workspaceIndex(tree)), QColor(112, 112, 112));
    TS_ASSERT_EQUALS(foregroundColour(tree, workspaceOutputTypeIndex(tree)), QColor(112, 112, 112));
    TS_ASSERT_DIFFERS(foregroundColour(tree, workspaceItemIndex(tree)), QColor(112, 112, 112));
  }

  void testWorkspaceTreeUsesDelegateForSubtleColumnDivider() {
    QtPlottingView view;
    auto tree = workspaceTree(view);

    TS_ASSERT_EQUALS(tree->itemDelegate()->objectName().toStdString(), "workspaceTreeItemDelegate");
    TS_ASSERT(tree->styleSheet().isEmpty());
  }

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

  void testClickingOutputTypeColumnSelectsWorkspaceRow() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItems());
    auto tree = workspaceTree(view);
    auto workspace = workspaceIndex(tree);

    click(tree, workspaceOutputTypeIndex(tree));

    TS_ASSERT(tree->selectionModel()->isSelected(workspace));
  }

  void testClickingItemColumnForGroupSelectsChildRunsAndWorkspaces() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItems());
    auto tree = workspaceTree(view);
    auto group = groupIndex(tree);
    auto run = runIndex(tree);
    auto workspace = workspaceIndex(tree);

    click(tree, groupItemIndex(tree));

    TS_ASSERT(tree->selectionModel()->isSelected(group));
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

  void testSelectedWorkspacesReturnsOnlyWorkspaceItems() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItems());
    auto tree = workspaceTree(view);

    click(tree, groupIndex(tree));

    auto const selectedWorkspaces = view.selectedWorkspaces();
    TS_ASSERT_EQUALS(selectedWorkspaces.size(), 2);
    TS_ASSERT_EQUALS(selectedWorkspaces[0], "IvsLam_12345");
    TS_ASSERT_EQUALS(selectedWorkspaces[1], "IvsQ_12345");
  }

  void testPlotButtonsNotifySubscriber() {
    QtPlottingView view;
    TestPlottingViewSubscriber subscriber;
    view.subscribe(&subscriber);
    view.setOutputOptionsEnabled(true);

    view.findChild<QPushButton *>("plotTiled")->click();
    view.findChild<QPushButton *>("plotOverplot")->click();
    view.findChild<QPushButton *>("plotIndividual")->click();

    TS_ASSERT_EQUALS(subscriber.tiledClicked, 1);
    TS_ASSERT_EQUALS(subscriber.overplotClicked, 1);
    TS_ASSERT_EQUALS(subscriber.individualClicked, 1);
  }

private:
  class TestPlottingViewSubscriber : public PlottingViewSubscriber {
  public:
    void notifyPlotTiledClicked() override { ++tiledClicked; }
    void notifyPlotOverplotClicked() override { ++overplotClicked; }
    void notifyPlotIndividualClicked() override { ++individualClicked; }

    int tiledClicked{0};
    int overplotClicked{0};
    int individualClicked{0};
  };

  void assertButton(QWidget const *widget, std::string const &objectName, std::string const &text) const {
    auto const button = dynamic_cast<QPushButton const *>(widget);
    TS_ASSERT(button);
    TS_ASSERT_EQUALS(button->objectName().toStdString(), objectName);
    TS_ASSERT_EQUALS(button->text().toStdString(), text);
  }

  std::vector<PlottingWorkspaceTreeItem> workspaceItems() const {
    return {
        groupItem("Group 1", {runItem("12345", {workspaceItem("IvsLam_12345", PlottingWorkspaceOutputType::IvsLambda),
                                                workspaceItem("IvsQ_12345", PlottingWorkspaceOutputType::IvsQ)})})};
  }

  std::vector<PlottingWorkspaceTreeItem> workspaceItemsWithGroups(int groups) const {
    std::vector<PlottingWorkspaceTreeItem> items;
    for (auto group = 1; group <= groups; ++group) {
      auto const run = std::to_string(group) + "2345";
      items.emplace_back(
          groupItem("Group " + std::to_string(group),
                    {runItem(run, {workspaceItem("IvsLam_" + run, PlottingWorkspaceOutputType::IvsLambda)})}));
    }
    return items;
  }

  PlottingWorkspaceTreeItem groupItem(std::string label, std::vector<PlottingWorkspaceTreeItem> children) const {
    return {std::move(label),   PlottingWorkspaceTreeItemType::Group, PlottingWorkspaceOutputType::None, "", {}, "",
            std::move(children)};
  }

  PlottingWorkspaceTreeItem runItem(std::string label, std::vector<PlottingWorkspaceTreeItem> children) const {
    return {std::move(label),   PlottingWorkspaceTreeItemType::Run, PlottingWorkspaceOutputType::None, "", {}, "",
            std::move(children)};
  }

  PlottingWorkspaceTreeItem workspaceItem(std::string label, PlottingWorkspaceOutputType outputType) const {
    auto const workspaceName = label;
    return {std::move(label), PlottingWorkspaceTreeItemType::Workspace, outputType, "", {}, workspaceName, {}};
  }

  QTreeView *workspaceTree(QtPlottingView &view) const { return view.findChild<QTreeView *>("workspaceTree"); }

  QColor foregroundColour(QTreeView *tree, QModelIndex const &index) const {
    return tree->model()->data(index, Qt::ForegroundRole).value<QBrush>().color();
  }

  QModelIndex groupIndex(QTreeView *tree) const { return tree->model()->index(0, 0); }

  QModelIndex groupOutputTypeIndex(QTreeView *tree) const { return tree->model()->index(0, 1); }

  QModelIndex groupItemIndex(QTreeView *tree) const { return tree->model()->index(0, 2); }

  QModelIndex runIndex(QTreeView *tree) const { return tree->model()->index(0, 0, groupIndex(tree)); }

  QModelIndex runIndex(QTreeView *tree, int group, int run) const {
    return tree->model()->index(run, 0, tree->model()->index(group, 0));
  }

  QModelIndex workspaceIndex(QTreeView *tree) const { return tree->model()->index(0, 0, runIndex(tree)); }

  QModelIndex workspaceOutputTypeIndex(QTreeView *tree) const { return tree->model()->index(0, 1, runIndex(tree)); }

  QModelIndex workspaceItemIndex(QTreeView *tree) const { return tree->model()->index(0, 2, runIndex(tree)); }

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
