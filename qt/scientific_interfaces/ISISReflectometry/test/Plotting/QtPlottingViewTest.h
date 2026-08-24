// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Plotting/view/QtPlottingView.h"
#include "../../../ISISReflectometry/GUI/Plotting/view/WorkspaceTreeView.h"

#include <QApplication>
#include <QBrush>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QItemSelectionModel>
#include <QMouseEvent>
#include <QPalette>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <cxxtest/TestSuite.h>
#include <utility>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

class QtPlottingViewTest : public CxxTest::TestSuite {
public:
  void testNoPlotOutputTypesAreAvailableBeforePresenterSetsThem() {
    QtPlottingView view;
    auto plotPreset = view.findChild<QComboBox *>("plotPreset");

    TS_ASSERT_EQUALS(plotPreset->count(), 0);
    TS_ASSERT(!view.selectedPlotOutputType());
  }

  void testAvailablePlotOutputTypesUpdatesPresetOptions() {
    QtPlottingView view;
    auto plotPreset = view.findChild<QComboBox *>("plotPreset");

    view.setAvailablePlotOutputTypes(
        outputTypeViewItems({PlotOutputType::ReflectivityCurve, PlotOutputType::DetectorMap,
                             PlotOutputType::SpinAsymmetry, PlotOutputType::Alignment}));

    TS_ASSERT_EQUALS(plotPreset->count(), 4);
    TS_ASSERT_EQUALS(plotPreset->itemText(0).toStdString(), "Reflectivity Curve");
    TS_ASSERT_EQUALS(plotPreset->itemText(1).toStdString(), "Detector Map");
    TS_ASSERT_EQUALS(plotPreset->itemText(2).toStdString(), "Spin Asymmetry");
    TS_ASSERT_EQUALS(plotPreset->itemText(3).toStdString(), "Alignment");
  }

  void testChangingPlotOutputTypeClearsWorkspaceSelection() {
    QtPlottingView view;
    auto plotPreset = view.findChild<QComboBox *>("plotPreset");
    view.setAvailablePlotOutputTypes(
        outputTypeViewItems({PlotOutputType::ReflectivityCurve, PlotOutputType::SpinAsymmetry}));
    view.setWorkspaceItems(workspaceItems());
    auto tree = workspaceTree(view);

    click(tree, groupIndex(tree));
    plotPreset->setCurrentIndex(1);

    TS_ASSERT(tree->selectionModel()->selectedRows().empty());
  }

  void testUpdatingAvailablePlotOutputTypesPreservesSelectionWhenOutputTypeDoesNotChange() {
    QtPlottingView view;
    view.setAvailablePlotOutputTypes(outputTypeViewItems({PlotOutputType::ReflectivityCurve}));
    view.setWorkspaceItems(workspaceItems());
    auto tree = workspaceTree(view);

    click(tree, groupIndex(tree));
    view.setAvailablePlotOutputTypes(
        outputTypeViewItems({PlotOutputType::ReflectivityCurve, PlotOutputType::DetectorMap}));

    TS_ASSERT(tree->selectionModel()->isSelected(groupIndex(tree)));
    TS_ASSERT(tree->selectionModel()->isSelected(runIndex(tree)));
    TS_ASSERT(tree->selectionModel()->isSelected(workspaceIndex(tree)));
  }

  void testUpdatingAvailablePlotOutputTypesClearsSelectionWhenCurrentOutputTypeIsRemoved() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItems());
    auto tree = workspaceTree(view);

    click(tree, groupIndex(tree));
    view.setAvailablePlotOutputTypes(outputTypeViewItems({PlotOutputType::DetectorMap}));

    TS_ASSERT(tree->selectionModel()->selectedRows().empty());
  }

  void testPlotButtonsHaveExpectedLabelsAndOrder() {
    QtPlottingView view;
    auto const optionsLayout = view.findChild<QVBoxLayout *>("optionsLayout");

    assertButton(optionsLayout->itemAt(10)->widget(), "plotIndividual", "Plot");
    assertButton(optionsLayout->itemAt(11)->widget(), "plotOverplot", "Plot over");
    assertButton(optionsLayout->itemAt(12)->widget(), "plotTiled", "Plot tiled");
    assertCheckBox(optionsLayout->itemAt(13)->widget(), "addToExistingPlot", "Add to existing plot");
    assertCheckBox(optionsLayout->itemAt(14)->widget(), "plotTiledVertically", "Plot tiled vertically");
  }

  void testPlotActionStateDisablesPlotButtons() {
    QtPlottingView view;

    view.setPlotActionState({false, false, false, false, false, false});

    assertPlotButtonsEnabled(view, false, false, false);
  }

  void testPlotActionStateEnablesPlotButtons() {
    QtPlottingView view;

    view.setPlotActionState({true, true, true, true, true, true});

    assertPlotButtonsEnabled(view, true, true, true);
    TS_ASSERT(view.findChild<QCheckBox *>("plotTiledVertically")->isEnabled());
    TS_ASSERT(view.findChild<QCheckBox *>("addToExistingPlot")->isEnabled());
    TS_ASSERT(view.findChild<QCheckBox *>("addToExistingPlot")->isChecked());
  }

  void testPlotActionStateCanUncheckAddToExistingPlotWithoutNotifyingSubscriber() {
    QtPlottingView view;
    TestPlottingViewSubscriber subscriber;
    view.subscribe(&subscriber);
    view.setPlotActionState({true, true, true, true, true, true});

    view.setPlotActionState({true, true, true, true, true, false});

    TS_ASSERT(!view.findChild<QCheckBox *>("addToExistingPlot")->isChecked());
    TS_ASSERT_EQUALS(subscriber.addToExistingPlotChanged, 0);
  }

  void testOutputControlsStateHidesPlotOutputProperties() {
    QtPlottingView view;

    view.setPlotOutputControlsState({false, false, false});

    TS_ASSERT(view.findChild<QWidget *>("plotPropertiesTopSeparator")->isHidden());
    TS_ASSERT(view.findChild<QWidget *>("plotPropertiesBottomSeparator")->isHidden());
    TS_ASSERT(view.findChild<QWidget *>("detectorMapYAxis")->isHidden());
    TS_ASSERT(view.findChild<QWidget *>("detectorMapXAxis")->isHidden());
    TS_ASSERT(view.findChild<QWidget *>("alignmentXAxis")->isHidden());
  }

  void testOutputControlsStateShowsDetectorMapAxisUnitProperties() {
    QtPlottingView view;

    view.setPlotOutputControlsState({true, true, false});

    TS_ASSERT(!view.findChild<QWidget *>("plotPropertiesTopSeparator")->isHidden());
    TS_ASSERT(!view.findChild<QWidget *>("plotPropertiesBottomSeparator")->isHidden());
    TS_ASSERT(!view.findChild<QWidget *>("detectorMapYAxis")->isHidden());
    TS_ASSERT(!view.findChild<QWidget *>("detectorMapXAxis")->isHidden());
    TS_ASSERT(view.findChild<QWidget *>("alignmentXAxis")->isHidden());
  }

  void testDetectorMapSelectedAxisUnitPropertiesAreReturned() {
    QtPlottingView view;
    auto plotPreset = view.findChild<QComboBox *>("plotPreset");

    view.setAvailablePlotOutputTypes(outputTypeViewItems({PlotOutputType::DetectorMap}));
    plotPreset->setCurrentIndex(0);
    view.findChild<QComboBox *>("detectorMapYAxis")->setCurrentIndex(1);
    view.findChild<QComboBox *>("detectorMapXAxis")->setCurrentIndex(1);

    auto const selection = view.selectedPlotOutputSelection();
    TS_ASSERT_EQUALS(selection.detectorMapYAxis, DetectorMapYAxis::Theta);
    TS_ASSERT_EQUALS(selection.detectorMapXAxis, DetectorMapXAxis::Lambda);
  }

  void testOutputControlsStateShowsAlignmentXAxisUnitProperty() {
    QtPlottingView view;

    view.setPlotOutputControlsState({true, false, true});

    TS_ASSERT(!view.findChild<QWidget *>("plotPropertiesTopSeparator")->isHidden());
    TS_ASSERT(!view.findChild<QWidget *>("plotPropertiesBottomSeparator")->isHidden());
    TS_ASSERT(view.findChild<QWidget *>("detectorMapYAxis")->isHidden());
    TS_ASSERT(view.findChild<QWidget *>("detectorMapXAxis")->isHidden());
    TS_ASSERT(!view.findChild<QWidget *>("alignmentXAxis")->isHidden());
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
    TS_ASSERT_EQUALS(tree->model()->data(workspaceOutputTypeIndex(tree)).toString().toStdString(), "IvsQ");
    TS_ASSERT_EQUALS(tree->model()->data(workspaceItemIndex(tree)).toString().toStdString(), "IvsQ_12345");
  }

  void testWorkspaceTreeItemsAreEnabledByDefault() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItems());
    auto tree = workspaceTree(view);

    TS_ASSERT(rowIsEnabled(tree, workspaceIndex(tree)));
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

  void testSelectedWorkspaceItemsReturnsOnlyWorkspaceItems() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItems());
    auto tree = workspaceTree(view);

    click(tree, groupIndex(tree));

    auto const selectedWorkspaces = view.selectedWorkspaceNames();
    TS_ASSERT_EQUALS(selectedWorkspaces.size(), 2);
    TS_ASSERT_EQUALS(selectedWorkspaces[0], "IvsQ_12345");
    TS_ASSERT_EQUALS(selectedWorkspaces[1], "IvsQ_binned_12345");
  }

  void testMutedWorkspaceItemsUseMutedRowPresentation() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItemsWithMutedIvsLambda());
    auto tree = workspaceTree(view);

    TS_ASSERT(rowIsMuted(tree, workspaceIndex(tree, 0, 0, 0)));
    TS_ASSERT(rowIsEnabled(tree, workspaceIndex(tree, 0, 0, 0)));
    TS_ASSERT(!rowIsMuted(tree, workspaceIndex(tree, 0, 0, 1)));
    TS_ASSERT(!rowIsMuted(tree, workspaceIndex(tree, 0, 0, 2)));
    TS_ASSERT_EQUALS(backgroundColour(tree, workspaceIndex(tree, 0, 0, 0)), mutedBackgroundColour(tree));
    TS_ASSERT_EQUALS(backgroundColour(tree, workspaceOutputTypeIndex(tree, 0, 0, 0)), mutedBackgroundColour(tree));
    TS_ASSERT_EQUALS(backgroundColour(tree, workspaceItemIndex(tree, 0, 0, 0)), mutedBackgroundColour(tree));
    TS_ASSERT_DIFFERS(backgroundColour(tree, workspaceItemIndex(tree, 0, 0, 1)), mutedBackgroundColour(tree));
  }

  void testNonSelectableWorkspaceItemsCannotBeSelectedDirectly() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItemsWithMutedIvsLambda());
    auto tree = workspaceTree(view);
    auto workspace = workspaceIndex(tree, 0, 0, 0);

    click(tree, workspace);

    TS_ASSERT(!tree->selectionModel()->isSelected(workspace));
  }

  void testNonSelectableWorkspaceGroupsCannotBeSelectedDirectly() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItemsWithMutedIvsLambdaWorkspaceGroup());
    auto tree = workspaceTree(view);
    auto workspaceGroup = workspaceIndex(tree, 0, 0, 0);

    click(tree, workspaceGroup);

    TS_ASSERT(!tree->selectionModel()->isSelected(workspaceGroup));
  }

  void testMutedWorkspaceItemsCanRemainSelectableThroughParent() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItemsForSpinAsymmetry());
    auto tree = workspaceTree(view);

    TS_ASSERT(rowIsMuted(tree, workspaceIndex(tree, 0, 0, 0)));
    TS_ASSERT(rowIsMuted(tree, workspaceIndex(tree, 0, 0, 1)));
    TS_ASSERT(rowIsMuted(tree, workspaceIndex(tree, 0, 0, 2)));
    TS_ASSERT(rowIsEnabled(tree, workspaceIndex(tree, 0, 0, 0)));
    TS_ASSERT(rowIsEnabled(tree, workspaceIndex(tree, 0, 0, 1)));
    TS_ASSERT(rowIsEnabled(tree, workspaceIndex(tree, 0, 0, 2)));
  }

  void testParentOnlyWorkspaceItemsCannotBeSelectedDirectly() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItemsForSpinAsymmetry());
    auto tree = workspaceTree(view);
    auto workspace = workspaceIndex(tree, 0, 0, 2);

    click(tree, workspace);

    TS_ASSERT(!tree->selectionModel()->isSelected(workspace));
  }

  void testSelectingParentReturnsOnlySelectableChildWorkspaces() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItemsForSpinAsymmetry());
    auto tree = workspaceTree(view);

    click(tree, runIndex(tree));

    auto const selectedWorkspaces = view.selectedWorkspaceNames();
    TS_ASSERT(tree->selectionModel()->isSelected(runIndex(tree)));
    TS_ASSERT_EQUALS(selectedWorkspaces.size(), 1);
    TS_ASSERT_EQUALS(selectedWorkspaces[0], "IvsQ_binned_12345");
  }

  void testSelectableWorkspaceGroupReturnsWorkspaceGroupName() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItemsWithWorkspaceGroupsForSpinAsymmetry());
    auto tree = workspaceTree(view);
    auto workspaceGroup = workspaceIndex(tree, 0, 0, 1);

    click(tree, workspaceGroup);

    auto const selectedWorkspaces = view.selectedWorkspaceNames();
    TS_ASSERT(tree->selectionModel()->isSelected(workspaceGroup));
    TS_ASSERT_EQUALS(selectedWorkspaces.size(), 1);
    TS_ASSERT_EQUALS(selectedWorkspaces[0], "IvsQ_binned_group_1");
  }

  void testMutedStitchedDisplayItemsUseMutedRowPresentation() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItemsWithMutedStitchedOutput());
    auto tree = workspaceTree(view);

    TS_ASSERT(rowIsMuted(tree, groupChildIndex(tree, 0)));
    TS_ASSERT(rowIsMuted(tree, groupChildIndex(tree, 1)));
    TS_ASSERT(rowIsMuted(tree, groupChildIndex(tree, 1, 0)));
    TS_ASSERT(rowIsEnabled(tree, groupChildIndex(tree, 0)));
    TS_ASSERT(rowIsEnabled(tree, groupChildIndex(tree, 1)));
    TS_ASSERT(rowIsEnabled(tree, groupChildIndex(tree, 1, 0)));
    TS_ASSERT_EQUALS(backgroundColour(tree, groupChildIndex(tree, 0)), mutedBackgroundColour(tree));
    TS_ASSERT_EQUALS(backgroundColour(tree, groupChildIndex(tree, 1)), mutedBackgroundColour(tree));
    TS_ASSERT_EQUALS(backgroundColour(tree, groupChildIndex(tree, 1, 0)), mutedBackgroundColour(tree));
  }

  void testNonSelectableMutedWorkspaceGroupCannotBeSelectedDirectly() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItemsWithMutedStitchedOutput());
    auto tree = workspaceTree(view);
    auto workspaceGroup = groupChildIndex(tree, 1);

    click(tree, workspaceGroup);

    TS_ASSERT(!tree->selectionModel()->isSelected(workspaceGroup));
    TS_ASSERT(view.selectedWorkspaceNames().empty());
  }

  void testMutedStitchedWorkspaceAndWorkspaceGroupRowsUseMutedPresentation() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItemsWithMutedStitchedOutput());
    auto tree = workspaceTree(view);

    TS_ASSERT(rowIsMuted(tree, groupChildIndex(tree, 0)));
    TS_ASSERT(rowIsMuted(tree, groupChildIndex(tree, 1)));
    TS_ASSERT(rowIsMuted(tree, groupChildIndex(tree, 1, 0)));
    TS_ASSERT(rowIsEnabled(tree, groupChildIndex(tree, 0)));
    TS_ASSERT(rowIsEnabled(tree, groupChildIndex(tree, 1)));
    TS_ASSERT(rowIsEnabled(tree, groupChildIndex(tree, 1, 0)));
    TS_ASSERT_EQUALS(backgroundColour(tree, groupChildIndex(tree, 0)), mutedBackgroundColour(tree));
    TS_ASSERT_EQUALS(backgroundColour(tree, groupChildIndex(tree, 1)), mutedBackgroundColour(tree));
    TS_ASSERT_EQUALS(backgroundColour(tree, groupChildIndex(tree, 1, 0)), mutedBackgroundColour(tree));
  }

  void testNonSelectableMutedWorkspaceCannotBeSelectedDirectly() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItemsWithMutedStitchedOutput());
    auto tree = workspaceTree(view);
    auto workspace = groupChildIndex(tree, 0);

    click(tree, workspace);

    TS_ASSERT(!tree->selectionModel()->isSelected(workspace));
    TS_ASSERT(view.selectedWorkspaceNames().empty());
  }

  void testNonSelectableMutedWorkspaceGroupCannotBeSelectedDirectlyFromDetectorMapFixture() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItemsWithMutedStitchedOutput());
    auto tree = workspaceTree(view);
    auto workspaceGroup = groupChildIndex(tree, 1);

    click(tree, workspaceGroup);

    TS_ASSERT(!tree->selectionModel()->isSelected(workspaceGroup));
    TS_ASSERT(view.selectedWorkspaceNames().empty());
  }

  void testSelectedWorkspaceNamesReturnsWorkspaceGroupChildren() {
    QtPlottingView view;
    view.setWorkspaceItems(workspaceItemsWithWorkspaceGroups());
    auto tree = workspaceTree(view);
    auto workspaceGroup = workspaceIndex(tree, 0, 0, 1);

    click(tree, workspaceGroup);

    auto const selectedWorkspaces = view.selectedWorkspaceNames();
    TS_ASSERT_EQUALS(selectedWorkspaces.size(), 1);
    TS_ASSERT_EQUALS(selectedWorkspaces[0], "IvsQ_binned_group_1");
  }

  void testPlotButtonsNotifySubscriber() {
    QtPlottingView view;
    TestPlottingViewSubscriber subscriber;
    view.subscribe(&subscriber);
    view.setWorkspaceItems(workspaceItems());
    view.setOutputSelectionEnabled(true);
    view.setPlotActionState({true, true, true, true, true, false});
    auto tree = workspaceTree(view);
    click(tree, groupIndex(tree));

    view.findChild<QPushButton *>("plotTiled")->click();
    view.findChild<QPushButton *>("plotOverplot")->click();
    view.findChild<QPushButton *>("plotIndividual")->click();
    view.findChild<QCheckBox *>("addToExistingPlot")->setChecked(true);

    TS_ASSERT_EQUALS(subscriber.tiledClicked, 1);
    TS_ASSERT_EQUALS(subscriber.overplotClicked, 1);
    TS_ASSERT_EQUALS(subscriber.individualClicked, 1);
    TS_ASSERT_EQUALS(subscriber.addToExistingPlotChanged, 1);
  }

private:
  class TestPlottingViewSubscriber : public PlottingViewSubscriber {
  public:
    void notifyPlotTiledClicked() override { ++tiledClicked; }
    void notifyPlotOverplotClicked() override { ++overplotClicked; }
    void notifyPlotIndividualClicked() override { ++individualClicked; }
    void notifyAddToExistingPlotChanged() override { ++addToExistingPlotChanged; }
    void notifyPlotOutputTypeChanged() override {}
    void notifyWorkspaceSelectionChanged() override {}

    int tiledClicked{0};
    int overplotClicked{0};
    int individualClicked{0};
    int addToExistingPlotChanged{0};
  };

  void assertButton(QWidget const *widget, std::string const &objectName, std::string const &text) const {
    auto const button = dynamic_cast<QPushButton const *>(widget);
    TS_ASSERT(button);
    TS_ASSERT_EQUALS(button->objectName().toStdString(), objectName);
    TS_ASSERT_EQUALS(button->text().toStdString(), text);
  }

  void assertCheckBox(QWidget const *widget, std::string const &objectName, std::string const &text) const {
    auto const checkBox = dynamic_cast<QCheckBox const *>(widget);
    TS_ASSERT(checkBox);
    TS_ASSERT_EQUALS(checkBox->objectName().toStdString(), objectName);
    TS_ASSERT_EQUALS(checkBox->text().toStdString(), text);
  }

  void assertPlotButtonsEnabled(QtPlottingView &view, bool individual, bool overplot, bool tiled) const {
    TS_ASSERT_EQUALS(view.findChild<QPushButton *>("plotIndividual")->isEnabled(), individual);
    TS_ASSERT_EQUALS(view.findChild<QPushButton *>("plotOverplot")->isEnabled(), overplot);
    TS_ASSERT_EQUALS(view.findChild<QPushButton *>("plotTiled")->isEnabled(), tiled);
  }

  std::vector<PlotOutputTypeViewItem> outputTypeViewItems(std::initializer_list<PlotOutputType> outputTypes) const {
    auto items = std::vector<PlotOutputTypeViewItem>{};
    for (auto const outputType : outputTypes) {
      switch (outputType) {
      case PlotOutputType::ReflectivityCurve:
        items.push_back({outputType, "Reflectivity Curve"});
        break;
      case PlotOutputType::DetectorMap:
        items.push_back({outputType, "Detector Map"});
        break;
      case PlotOutputType::SpinAsymmetry:
        items.push_back({outputType, "Spin Asymmetry"});
        break;
      case PlotOutputType::Alignment:
        items.push_back({outputType, "Alignment"});
        break;
      }
    }
    return items;
  }

  std::vector<PlottingWorkspaceTreeDisplayItem> workspaceItems() const {
    return {groupItem(
        "Group 1", {runItem("12345", {workspaceItem("IvsQ_12345", PlottingWorkspaceOutputType::IvsQ),
                                      workspaceItem("IvsQ_binned_12345", PlottingWorkspaceOutputType::IvsQBinned)})})};
  }

  std::vector<PlottingWorkspaceTreeDisplayItem> workspaceItemsWithGroups(int groups) const {
    std::vector<PlottingWorkspaceTreeDisplayItem> items;
    for (auto group = 1; group <= groups; ++group) {
      auto const run = std::to_string(group) + "2345";
      items.emplace_back(groupItem("Group " + std::to_string(group),
                                   {runItem(run, {workspaceItem("IvsQ_" + run, PlottingWorkspaceOutputType::IvsQ)})}));
    }
    return items;
  }

  std::vector<PlottingWorkspaceTreeDisplayItem> workspaceItemsWithBinnedOutput() const {
    return {groupItem(
        "Group 1", {runItem("12345", {workspaceItem("IvsLam_12345", PlottingWorkspaceOutputType::IvsLambda),
                                      workspaceItem("IvsQ_12345", PlottingWorkspaceOutputType::IvsQ),
                                      workspaceItem("IvsQ_binned_12345", PlottingWorkspaceOutputType::IvsQBinned)})})};
  }

  std::vector<PlottingWorkspaceTreeDisplayItem> workspaceItemsWithMutedIvsLambda() const {
    auto items = workspaceItemsWithBinnedOutput();
    items[0].children[0].children[0] = mutedItem(std::move(items[0].children[0].children[0]), false);
    return items;
  }

  std::vector<PlottingWorkspaceTreeDisplayItem> workspaceItemsWithBinnedWorkspaces(int workspaceCount) const {
    auto workspaces = std::vector<PlottingWorkspaceTreeDisplayItem>{};
    for (auto workspace = 1; workspace <= workspaceCount; ++workspace) {
      workspaces.emplace_back(
          workspaceItem("IvsQ_binned_" + std::to_string(workspace), PlottingWorkspaceOutputType::IvsQBinned));
    }
    return {groupItem("Group 1", {runItem("12345", std::move(workspaces))})};
  }

  std::vector<PlottingWorkspaceTreeDisplayItem> workspaceItemsWithBinnedWorkspaceGroups(int workspaceGroupCount) const {
    auto workspaceGroups = std::vector<PlottingWorkspaceTreeDisplayItem>{};
    for (auto workspaceGroup = 1; workspaceGroup <= workspaceGroupCount; ++workspaceGroup) {
      workspaceGroups.emplace_back(
          workspaceGroupItem("IvsQ_binned_group_" + std::to_string(workspaceGroup),
                             {workspaceItem("IvsQ_binned_group_" + std::to_string(workspaceGroup) + "_1",
                                            PlottingWorkspaceOutputType::IvsQBinned)}));
    }
    return {groupItem("Group 1", {runItem("12345", std::move(workspaceGroups))})};
  }

  std::vector<PlottingWorkspaceTreeDisplayItem> workspaceItemsWithWorkspaceGroups() const {
    return {groupItem(
        "Group 1",
        {runItem("12345", {workspaceGroupItem("IvsLam_group", {workspaceItem("IvsLam_group_1",
                                                                             PlottingWorkspaceOutputType::IvsLambda)}),
                           workspaceGroupItem(
                               "IvsQ_binned_group",
                               {workspaceItem("IvsQ_binned_group_1", PlottingWorkspaceOutputType::IvsQBinned)})})})};
  }

  std::vector<PlottingWorkspaceTreeDisplayItem> workspaceItemsWithMutedIvsLambdaWorkspaceGroup() const {
    auto items = workspaceItemsWithWorkspaceGroups();
    items[0].children[0].children[0] = mutedItem(std::move(items[0].children[0].children[0]), false);
    items[0].children[0].children[0].children[0] =
        mutedItem(std::move(items[0].children[0].children[0].children[0]), false);
    return items;
  }

  std::vector<PlottingWorkspaceTreeDisplayItem> workspaceItemsForSpinAsymmetry() const {
    auto items = workspaceItemsWithBinnedOutput();
    auto &workspaces = items[0].children[0].children;
    workspaces[0] = mutedItem(std::move(workspaces[0]), false);
    workspaces[1] = mutedItem(std::move(workspaces[1]), false);
    workspaces[2] = mutedItem(std::move(workspaces[2]), true);
    return items;
  }

  std::vector<PlottingWorkspaceTreeDisplayItem> workspaceItemsWithWorkspaceGroupsForSpinAsymmetry() const {
    auto items = workspaceItemsWithWorkspaceGroups();
    auto &workspaceGroups = items[0].children[0].children;
    workspaceGroups[0] = mutedItem(std::move(workspaceGroups[0]), false);
    workspaceGroups[0].children[0] = mutedItem(std::move(workspaceGroups[0].children[0]), false);
    workspaceGroups[1].children[0] = mutedItem(std::move(workspaceGroups[1].children[0]), true);
    return items;
  }

  std::vector<PlottingWorkspaceTreeDisplayItem> workspaceItemsWithStitchedOutput() const {
    return {groupItem(
        "Group 1", {workspaceItem("stitched_12345", PlottingWorkspaceOutputType::IvsQBinned),
                    workspaceGroupItem("stitched_group",
                                       {workspaceItem("stitched_group_1", PlottingWorkspaceOutputType::IvsQBinned)})})};
  }

  std::vector<PlottingWorkspaceTreeDisplayItem> workspaceItemsWithMutedStitchedOutput() const {
    auto items = workspaceItemsWithStitchedOutput();
    items[0].children[0] = mutedItem(std::move(items[0].children[0]), false);
    items[0].children[1] = mutedItem(std::move(items[0].children[1]), false);
    items[0].children[1].children[0] = mutedItem(std::move(items[0].children[1].children[0]), false);
    return items;
  }

  PlottingWorkspaceTreeDisplayItem groupItem(std::string label,
                                             std::vector<PlottingWorkspaceTreeDisplayItem> children) const {
    return {std::move(label), PlottingWorkspaceTreeItemType::Group, PlottingWorkspaceOutputType::None, "",
            std::move(children)};
  }

  PlottingWorkspaceTreeDisplayItem runItem(std::string label,
                                           std::vector<PlottingWorkspaceTreeDisplayItem> children) const {
    return {std::move(label), PlottingWorkspaceTreeItemType::Run, PlottingWorkspaceOutputType::None, "",
            std::move(children)};
  }

  PlottingWorkspaceTreeDisplayItem workspaceGroupItem(std::string label,
                                                      std::vector<PlottingWorkspaceTreeDisplayItem> children) const {
    auto const workspaceName = label;
    return {std::move(label), PlottingWorkspaceTreeItemType::WorkspaceGroup, PlottingWorkspaceOutputType::None,
            workspaceName, std::move(children)};
  }

  PlottingWorkspaceTreeDisplayItem workspaceItem(std::string label, PlottingWorkspaceOutputType outputType) const {
    auto const workspaceName = label;
    return {std::move(label), PlottingWorkspaceTreeItemType::Workspace, outputType, workspaceName, {}};
  }

  PlottingWorkspaceTreeDisplayItem mutedItem(PlottingWorkspaceTreeDisplayItem item, bool selectableAsChild) const {
    item.muted = true;
    item.selectionMode =
        selectableAsChild ? PlottingWorkspaceTreeSelectionMode::ParentOnly : PlottingWorkspaceTreeSelectionMode::None;
    return item;
  }

  QTreeView *workspaceTree(QtPlottingView &view) const { return view.findChild<QTreeView *>("workspaceTree"); }

  QColor mutedBackgroundColour(QTreeView *tree) const { return tree->palette().brush(QPalette::AlternateBase).color(); }

  bool rowIsMuted(QTreeView *tree, QModelIndex const &index) const {
    for (auto column = 0; column < tree->model()->columnCount(index.parent()); ++column) {
      if (!tree->model()->index(index.row(), column, index.parent()).data(WorkspaceTree::mutedRole).toBool()) {
        return false;
      }
    }
    return true;
  }

  bool rowIsEnabled(QTreeView *tree, QModelIndex const &index) const {
    for (auto column = 0; column < tree->model()->columnCount(index.parent()); ++column) {
      if (!tree->model()
               ->flags(tree->model()->index(index.row(), column, index.parent()))
               .testFlag(Qt::ItemIsEnabled)) {
        return false;
      }
    }
    return true;
  }

  QColor backgroundColour(QTreeView *tree, QModelIndex const &index) const {
    return tree->model()->data(index, Qt::BackgroundRole).value<QBrush>().color();
  }

  QModelIndex groupIndex(QTreeView *tree) const { return tree->model()->index(0, 0); }

  QModelIndex groupIndex(QTreeView *tree, int group) const { return tree->model()->index(group, 0); }

  QModelIndex groupOutputTypeIndex(QTreeView *tree) const { return tree->model()->index(0, 1); }

  QModelIndex groupItemIndex(QTreeView *tree) const { return tree->model()->index(0, 2); }

  QModelIndex runIndex(QTreeView *tree) const { return tree->model()->index(0, 0, groupIndex(tree)); }

  QModelIndex runIndex(QTreeView *tree, int group, int run) const {
    return tree->model()->index(run, 0, tree->model()->index(group, 0));
  }

  QModelIndex workspaceIndex(QTreeView *tree) const { return tree->model()->index(0, 0, runIndex(tree)); }

  QModelIndex workspaceOutputTypeIndex(QTreeView *tree) const { return tree->model()->index(0, 1, runIndex(tree)); }

  QModelIndex workspaceOutputTypeIndex(QTreeView *tree, int group, int run, int workspace) const {
    auto const groupModelIndex = tree->model()->index(group, 0);
    auto const runModelIndex = tree->model()->index(run, 0, groupModelIndex);
    return tree->model()->index(workspace, 1, runModelIndex);
  }

  QModelIndex workspaceItemIndex(QTreeView *tree) const { return tree->model()->index(0, 2, runIndex(tree)); }

  QModelIndex workspaceItemIndex(QTreeView *tree, int group, int run, int workspace) const {
    auto const groupModelIndex = tree->model()->index(group, 0);
    auto const runModelIndex = tree->model()->index(run, 0, groupModelIndex);
    return tree->model()->index(workspace, 2, runModelIndex);
  }

  QModelIndex workspaceIndex(QTreeView *tree, int group, int run, int workspace) const {
    auto const groupModelIndex = tree->model()->index(group, 0);
    auto const runModelIndex = tree->model()->index(run, 0, groupModelIndex);
    return tree->model()->index(workspace, 0, runModelIndex);
  }

  QModelIndex groupChildIndex(QTreeView *tree, int child, int grandchild = -1) const {
    auto const groupModelIndex = groupIndex(tree);
    auto const childIndex = tree->model()->index(child, 0, groupModelIndex);
    return grandchild < 0 ? childIndex : tree->model()->index(grandchild, 0, childIndex);
  }

  QModelIndex groupChildItemIndex(QTreeView *tree, int child, int grandchild = -1) const {
    auto const groupModelIndex = groupIndex(tree);
    auto const childIndex = tree->model()->index(child, 0, groupModelIndex);
    return grandchild < 0 ? tree->model()->index(child, 2, groupModelIndex)
                          : tree->model()->index(grandchild, 2, childIndex);
  }

  void click(QTreeView *tree, QModelIndex const &index, Qt::KeyboardModifiers modifiers = Qt::NoModifier) const {
    auto const position = tree->visualRect(index).center();
    auto event = QMouseEvent(QEvent::MouseButtonPress, position, position, Qt::LeftButton, Qt::LeftButton, modifiers);
    QApplication::sendEvent(tree->viewport(), &event);
  }

  void selectSpinAsymmetry(QtPlottingView &view) const {
    view.setAvailablePlotOutputTypes(
        outputTypeViewItems({PlotOutputType::ReflectivityCurve, PlotOutputType::SpinAsymmetry}));
    view.findChild<QComboBox *>("plotPreset")->setCurrentIndex(1);
  }

  void selectAlignment(QtPlottingView &view) const {
    view.setAvailablePlotOutputTypes(
        outputTypeViewItems({PlotOutputType::ReflectivityCurve, PlotOutputType::Alignment}));
    view.findChild<QComboBox *>("plotPreset")->setCurrentIndex(1);
  }

  void selectDetectorMap(QtPlottingView &view) const {
    view.setAvailablePlotOutputTypes(
        outputTypeViewItems({PlotOutputType::ReflectivityCurve, PlotOutputType::DetectorMap}));
    view.findChild<QComboBox *>("plotPreset")->setCurrentIndex(1);
  }

  void doubleClick(QTreeView *tree, QModelIndex const &index, Qt::KeyboardModifiers modifiers = Qt::NoModifier) const {
    auto const position = tree->visualRect(index).center();
    auto event =
        QMouseEvent(QEvent::MouseButtonDblClick, position, position, Qt::LeftButton, Qt::LeftButton, modifiers);
    QApplication::sendEvent(tree->viewport(), &event);
  }

  void clickAndDrag(QTreeView *tree, QModelIndex const &from, QModelIndex const &to) const {
    auto const fromPosition = tree->visualRect(from).center();
    auto const toPosition = tree->visualRect(to).center();
    auto pressEvent =
        QMouseEvent(QEvent::MouseButtonPress, fromPosition, fromPosition, Qt::LeftButton, Qt::LeftButton, {});
    auto moveEvent = QMouseEvent(QEvent::MouseMove, toPosition, toPosition, Qt::NoButton, Qt::LeftButton, {});
    auto releaseEvent =
        QMouseEvent(QEvent::MouseButtonRelease, toPosition, toPosition, Qt::LeftButton, Qt::NoButton, {});

    QApplication::sendEvent(tree->viewport(), &pressEvent);
    QApplication::sendEvent(tree->viewport(), &moveEvent);
    QApplication::sendEvent(tree->viewport(), &releaseEvent);
  }
};
