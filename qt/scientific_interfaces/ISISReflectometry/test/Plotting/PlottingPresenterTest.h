// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Plotting/presenter/PlottingPresenter.h"
#include "../../../ISISReflectometry/Reduction/RunsTable.h"
#include "../../../ISISReflectometry/TestHelpers/PlottingTestHelpers.h"
#include "../ReflMockObjects.h"
#include "PlottingWorkspaceTreeTestHelpers.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

#include <QWidget>
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <optional>
#include <utility>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using testing::NiceMock;
using testing::Return;

inline std::vector<PlottingWorkspaceTreeItemState>
plottingWorkspaceTreeItemStatesForOutputType(std::vector<PlottingWorkspaceTreeItem> const &items,
                                             PlotOutputType outputType) {
  auto itemsWithContext = items;
  populateTreeItemContexts(itemsWithContext);
  return PlottingViewStateProvider().plottingWorkspaceTreeItemStates(itemsWithContext, outputType);
}

inline std::vector<PlottingWorkspaceTreeItemState>
plottingWorkspaceTreeItemStatesForDefaultOutputType(std::vector<PlottingWorkspaceTreeItem> const &items) {
  return plottingWorkspaceTreeItemStatesForOutputType(items, PlotOutputType::ReflectivityCurve);
}

inline bool treeLabelsEqual(std::vector<PlottingWorkspaceTreeViewItem> const &actual,
                            std::vector<PlottingWorkspaceTreeItemState> const &expected) {
  return actual.size() == expected.size() &&
         std::equal(actual.begin(), actual.end(), expected.begin(), [](auto const &a, auto const &e) {
           return a.label == e.label && a.muted == e.muted && treeLabelsEqual(a.children, e.children);
         });
}

MATCHER_P(PlottingWorkspaceTreeItemStatesEqual, expected, "matches tree labels and muted state") {
  return treeLabelsEqual(arg, expected);
}

class MockWorkspaceTreeView : public PlottingWorkspaceTreeTestHelpers::TestTreeView {
public:
  MockWorkspaceTreeView() {
    ON_CALL(*this, setItems(testing::_)).WillByDefault([this](auto const &items) { TestTreeView::setItems(items); });
  }
  MOCK_METHOD(void, setItems, (std::vector<PlottingWorkspaceTreeViewItem> const &), (override));
};

class MockPlottingView : public IPlottingView {
public:
  MOCK_METHOD(void, subscribe, (PlottingViewSubscriber *), (override));
  MOCK_METHOD(void, setOutputSelectionEnabled, (bool), (override));
  MOCK_METHOD(void, setAvailablePlotOutputTypes, (std::vector<PlotOutputTypeViewItem> const &), (override));
  MOCK_METHOD(void, setPlotOutputControlsState, (PlotOutputControlsState const &), (override));
  MOCK_METHOD(void, setPlotActionState, (PlotActionState const &), (override));
  IPlottingWorkspaceTreeView &workspaceTreeView() override { return tree; }
  MOCK_METHOD(std::string, workspaceFilterText, (), (const, override));
  std::vector<ReducedWorkspaceOutputType> workspaceFilterOutputTypes() const override {
    return {ReducedWorkspaceOutputType::IvsQBinned};
  }
  void selectWorkspaces(std::vector<std::string> const &names) {
    tree.clearSelection();
    auto select = [&](auto const &self, std::vector<PlottingWorkspaceTreeViewItem> const &items,
                      PlottingWorkspaceTreePath path) -> void {
      for (size_t row = 0; row < items.size(); ++row) {
        auto child = path;
        child.emplace_back(static_cast<int>(row));
        if (std::find(names.begin(), names.end(), items[row].label) != names.end()) {
          tree.setSelected(child, true);
        }
        self(self, items[row].children, child);
      }
    };
    select(select, tree.displayedItems, {});
  }
  NiceMock<MockWorkspaceTreeView> tree;
  MOCK_METHOD(std::optional<PlotOutputType>, selectedPlotOutputType, (), (const, override));
  MOCK_METHOD(PlotOutputSelection, selectedPlotOutputSelection, (), (const, override));
  MOCK_METHOD(bool, shouldAddToExistingPlot, (), (const, override));
  MOCK_METHOD(bool, shouldPlotTiledVertically, (), (const, override));
  MOCK_METHOD(bool, shouldShareXAxisWhenTiled, (), (const, override));
  MOCK_METHOD(bool, shouldShareYAxisWhenTiled, (), (const, override));
  MOCK_METHOD(QWidget *, plotParent, (), (override));
  MOCK_METHOD(bool, confirmPlottingMultipleItems, (size_t), (const, override));
};

class MockPlottingModel : public IPlottingModel {
public:
  MOCK_METHOD(std::vector<std::string>, workspacesForPlotting,
              (std::vector<PlottingWorkspace> const &, PlotOutputSelection const &), (const, override));
};

class PlottingPresenterTest : public CxxTest::TestSuite {
public:
  void setUp() override {
    testing::DefaultValue<std::optional<PlotOutputType>>::Set(
        std::optional<PlotOutputType>{PlotOutputType::ReflectivityCurve});
  }

  void tearDown() override {
    testing::DefaultValue<std::optional<PlotOutputType>>::Clear();
    Mantid::API::AnalysisDataService::Instance().clear();
  }

  void testSubscribesToViewOnConstruction() {
    NiceMock<MockPlottingView> view;

    EXPECT_CALL(view, subscribe(testing::_)).Times(1);

    PlottingPresenter presenter(&view);
  }

  void testCurrentWorkspaceFilterIsAppliedAndUpdated() {
    NiceMock<MockPlottingView> view;
    ON_CALL(view, workspaceFilterText()).WillByDefault(Return("does_not_match"));
    PlottingPresenter presenter(&view);
    populatePlottingWorkspaceTree(presenter, view, {"IvsQ_binned_12345"});
    TS_ASSERT(!view.tree.isVisible({0}));

    ON_CALL(view, workspaceFilterText()).WillByDefault(Return("12345"));
    presenter.notifyWorkspaceFilterChanged();

    TS_ASSERT(view.tree.isVisible({0}));
  }

  void testOutputSelectionDisabledWhenReductionResumed() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockBatchPresenter> mainPresenter;
    PlottingPresenter presenter(&view);
    presenter.acceptMainPresenter(&mainPresenter);

    EXPECT_CALL(mainPresenter, isProcessing()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(view, setOutputSelectionEnabled(false)).Times(1);

    presenter.notifyReductionResumed();
  }

  void testAcceptMainPresenterDoesNotQueryInstrumentName() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockBatchPresenter> mainPresenter;
    PlottingPresenter presenter(&view);

    EXPECT_CALL(mainPresenter, instrumentName()).Times(0);

    presenter.acceptMainPresenter(&mainPresenter);
  }

  void testOutputSelectionEnabledWhenReductionPaused() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockBatchPresenter> mainPresenter;
    PlottingPresenter presenter(&view);
    presenter.acceptMainPresenter(&mainPresenter);

    EXPECT_CALL(mainPresenter, isProcessing()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(mainPresenter, isAutoreducing()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(view, setOutputSelectionEnabled(true)).Times(1);

    presenter.notifyReductionPaused();
  }

  void testInstrumentChangedUpdatesAvailablePlotOutputTypes() {
    NiceMock<MockPlottingView> view;
    PlottingPresenter presenter(&view);
    auto const expected = std::vector<PlotOutputTypeViewItem>{
        {PlotOutputType::ReflectivityCurve, "Reflectivity Curve"},
        {PlotOutputType::DetectorMap, "Detector Map"},
        {PlotOutputType::SpinAsymmetry, "Spin Asymmetry"},
        {PlotOutputType::Alignment, "Alignment"},
        {PlotOutputType::ReflectivityAndSpinAsymmetry, "Reflectivity + Spin Asymmetry"}};

    EXPECT_CALL(view, setAvailablePlotOutputTypes(expected)).Times(1);

    presenter.notifyInstrumentChanged("POLREF");
  }

  void testPlottingWorkspaceTreeIsClearedWhenNoPlotOutputTypeIsSelected() {
    NiceMock<MockPlottingView> view;
    PlottingPresenter presenter(&view);
    auto runsTable = RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {successfulRow("12345")})}));
    addWorkspaces({"IvsQ_12345"});

    EXPECT_CALL(view, selectedPlotOutputType()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(view.tree,
                setItems(PlottingWorkspaceTreeItemStatesEqual(std::vector<PlottingWorkspaceTreeItemState>{})))
        .Times(1);

    presenter.notifyRunsTableChanged(runsTable);
  }

  void testPlotPassesSelectedInstrumentToModel() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    auto const workspaces = std::vector<std::string>{"IvsQ_12345"};
    auto const selectedPlottingWorkspaces = plottingWorkspaces(workspaces);
    auto const viewOutputSelection = PlotOutputSelection{PlotOutputType::Alignment};
    auto expectedOutputSelection = viewOutputSelection;
    expectedOutputSelection.instrumentName = "POLREF";

    EXPECT_CALL(view, setAvailablePlotOutputTypes(testing::_)).Times(1);
    presenter.notifyInstrumentChanged("POLREF");
    populatePlottingWorkspaceTree(presenter, view, workspaces);
    view.selectWorkspaces(workspaces);
    EXPECT_CALL(view, selectedPlotOutputSelection()).Times(1).WillOnce(Return(viewOutputSelection));
    EXPECT_CALL(plottingModel, workspacesForPlotting(selectedPlottingWorkspaces, expectedOutputSelection))
        .Times(1)
        .WillOnce(Return(std::vector<std::string>{}));
    EXPECT_CALL(plotter, plot(testing::_)).Times(0);

    presenter.notifyPlotIndividualClicked();
  }

  void testRunsTableChangedShowsSuccessfulRowOutputWorkspacesInGroupAndRun() {
    NiceMock<MockPlottingView> view;
    PlottingPresenter presenter(&view);
    auto runsTable = RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {successfulRow("12345")})}));
    addWorkspaces({"IvsLam_12345", "IvsQ_12345", "IvsQ_binned_12345"});

    auto const expected = std::vector<PlottingWorkspaceTreeItem>{groupItem(
        "Group 1", {runItem("12345", {workspaceItem("IvsLam_12345", ReducedWorkspaceOutputType::IvsLambda),
                                      workspaceItem("IvsQ_12345", ReducedWorkspaceOutputType::IvsQ),
                                      workspaceItem("IvsQ_binned_12345", ReducedWorkspaceOutputType::IvsQBinned)})})};

    EXPECT_CALL(view.tree, setItems(PlottingWorkspaceTreeItemStatesEqual(
                               plottingWorkspaceTreeItemStatesForDefaultOutputType(expected))))
        .Times(1);

    presenter.notifyRunsTableChanged(runsTable);
  }

  void testRunsTableChangedOmitsUnsuccessfulRows() {
    NiceMock<MockPlottingView> view;
    PlottingPresenter presenter(&view);
    auto row = successfulRow("12345");
    row.resetState();
    auto runsTable = RunsTable({}, 0.0, ReductionJobs({Group("Group 1", {row})}));
    addWorkspaces({"IvsLam_12345", "IvsQ_12345", "IvsQ_binned_12345"});

    EXPECT_CALL(view.tree,
                setItems(PlottingWorkspaceTreeItemStatesEqual(std::vector<PlottingWorkspaceTreeItemState>{})))
        .Times(1);

    presenter.notifyRunsTableChanged(runsTable);
  }

  void testRunsTableChangedOmitsWorkspacesMissingFromADS() {
    NiceMock<MockPlottingView> view;
    PlottingPresenter presenter(&view);
    auto runsTable = RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {successfulRow("12345")})}));
    addWorkspaces({"IvsQ_12345"});

    auto const expected = std::vector<PlottingWorkspaceTreeItem>{
        groupItem("Group 1", {runItem("12345", {workspaceItem("IvsQ_12345", ReducedWorkspaceOutputType::IvsQ)})})};

    EXPECT_CALL(view.tree, setItems(PlottingWorkspaceTreeItemStatesEqual(
                               plottingWorkspaceTreeItemStatesForDefaultOutputType(expected))))
        .Times(1);

    presenter.notifyRunsTableChanged(runsTable);
  }

  void testRunsTableChangedShowsSuccessfulGroupOutputWorkspace() {
    NiceMock<MockPlottingView> view;
    PlottingPresenter presenter(&view);
    auto group = successfulGroup("Group 1", {successfulRow("12345")}, "stitched_12345");
    auto runsTable = RunsTable({}, 0.0, ReductionJobs({group}));
    addWorkspaces({"stitched_12345"});

    auto const expected = std::vector<PlottingWorkspaceTreeItem>{
        groupItem("Group 1", {workspaceItem("stitched_12345", ReducedWorkspaceOutputType::IvsQBinned,
                                            PlottingWorkspaceOrigin::PostprocessedGroupOutput)})};

    EXPECT_CALL(view.tree, setItems(PlottingWorkspaceTreeItemStatesEqual(
                               plottingWorkspaceTreeItemStatesForDefaultOutputType(expected))))
        .Times(1);

    presenter.notifyRunsTableChanged(runsTable);
  }

  void testRunsTableChangedShowsRowOutputWorkspaceGroupMembers() {
    NiceMock<MockPlottingView> view;
    PlottingPresenter presenter(&view);
    auto runsTable = RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {successfulRow("12345")})}));
    addWorkspaceGroup("IvsQ_12345", {"IvsQ_12345_1", "IvsQ_12345_2"});

    auto const expected = std::vector<PlottingWorkspaceTreeItem>{groupItem(
        "Group 1",
        {runItem("12345", {workspaceGroupItem("IvsQ_12345",
                                              {workspaceItem("IvsQ_12345_1", ReducedWorkspaceOutputType::IvsQ),
                                               workspaceItem("IvsQ_12345_2", ReducedWorkspaceOutputType::IvsQ)})})})};

    EXPECT_CALL(view.tree, setItems(PlottingWorkspaceTreeItemStatesEqual(
                               plottingWorkspaceTreeItemStatesForDefaultOutputType(expected))))
        .Times(1);

    presenter.notifyRunsTableChanged(runsTable);
  }

  void testRunsTableChangedShowsSuccessfulGroupOutputWorkspaceGroupMembers() {
    NiceMock<MockPlottingView> view;
    PlottingPresenter presenter(&view);
    auto group = successfulGroup("Group 1", {successfulRow("12345")}, "stitched_12345");
    auto runsTable = RunsTable({}, 0.0, ReductionJobs({group}));
    addWorkspaceGroup("stitched_12345", {"stitched_12345_1", "stitched_12345_2"});

    auto const expected = std::vector<PlottingWorkspaceTreeItem>{groupItem(
        "Group 1", {workspaceGroupItem("stitched_12345",
                                       {workspaceItem("stitched_12345_1", ReducedWorkspaceOutputType::IvsQBinned),
                                        workspaceItem("stitched_12345_2", ReducedWorkspaceOutputType::IvsQBinned)},
                                       PlottingWorkspaceOrigin::PostprocessedGroupOutput)})};

    EXPECT_CALL(view.tree, setItems(PlottingWorkspaceTreeItemStatesEqual(
                               plottingWorkspaceTreeItemStatesForDefaultOutputType(expected))))
        .Times(1);

    presenter.notifyRunsTableChanged(runsTable);
  }

  void testPlotOutputTypeChangedReevaluatesPlottingWorkspaceTreeItems() {
    NiceMock<MockPlottingView> view;
    PlottingPresenter presenter(&view);
    auto group = successfulGroup("Group 1", {successfulRow("12345")}, "stitched_12345");
    auto runsTable = RunsTable({}, 0.0, ReductionJobs({group}));
    addWorkspaces({"IvsLam_12345", "IvsQ_12345", "IvsQ_binned_12345", "stitched_12345"});
    auto const expected = std::vector<PlottingWorkspaceTreeItem>{groupItem(
        "Group 1", {workspaceItem("stitched_12345", ReducedWorkspaceOutputType::IvsQBinned,
                                  PlottingWorkspaceOrigin::PostprocessedGroupOutput),
                    runItem("12345", {workspaceItem("IvsLam_12345", ReducedWorkspaceOutputType::IvsLambda),
                                      workspaceItem("IvsQ_12345", ReducedWorkspaceOutputType::IvsQ),
                                      workspaceItem("IvsQ_binned_12345", ReducedWorkspaceOutputType::IvsQBinned)})})};

    EXPECT_CALL(view, selectedPlotOutputType())
        .WillOnce(Return(std::optional<PlotOutputType>{PlotOutputType::ReflectivityCurve}))
        .WillRepeatedly(Return(std::optional<PlotOutputType>{PlotOutputType::Alignment}));
    {
      testing::InSequence sequence;
      EXPECT_CALL(view.tree, setItems(PlottingWorkspaceTreeItemStatesEqual(plottingWorkspaceTreeItemStatesForOutputType(
                                 expected, PlotOutputType::ReflectivityCurve))))
          .Times(1);
      EXPECT_CALL(view.tree, setItems(PlottingWorkspaceTreeItemStatesEqual(
                                 plottingWorkspaceTreeItemStatesForOutputType(expected, PlotOutputType::Alignment))))
          .Times(1);
    }

    presenter.notifyRunsTableChanged(runsTable);
    presenter.notifyPlotOutputTypeChanged();
  }

  void testPlotPassesPeriodMetadataToModelForPeriodWorkspaces() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    addWorkspaceWithPeriod("IvsQ_binned_12345_2", 2, 2);
    auto row = successfulRow("12345");
    row.setOutputNames({{""}, {""}, {"IvsQ_binned_12345_2"}});
    auto const expectedPlottingWorkspaces = std::vector<PlottingWorkspace>{
        {"IvsQ_binned_12345_2", {"12345"}, 2, runOutputContext("IvsQ_binned_12345_2", {0, 0, 0})}};
    auto const outputSelection = PlotOutputSelection{PlotOutputType::Alignment};

    EXPECT_CALL(view.tree, setItems(testing::_)).Times(1);
    presenter.notifyRunsTableChanged(RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {row})})));
    view.selectWorkspaces(std::vector<std::string>{"IvsQ_binned_12345_2"});
    EXPECT_CALL(view, selectedPlotOutputSelection()).Times(1).WillOnce(Return(outputSelection));
    EXPECT_CALL(plottingModel, workspacesForPlotting(expectedPlottingWorkspaces, outputSelection))
        .Times(1)
        .WillOnce(Return(std::vector<std::string>{}));
    EXPECT_CALL(plotter, plot(testing::_)).Times(0);

    presenter.notifyPlotIndividualClicked();
  }

  void testPlotPassesPeriodMetadataToModelForWorkspaceGroupChildren() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    addWorkspaceWithRunNumberAndPeriod("IvsQ_binned_12345_1", "12345", 2, 1);
    addWorkspaceWithRunNumberAndPeriod("IvsQ_binned_12345_2", "12345", 2, 2);
    groupExistingWorkspaces("IvsQ_binned_12345", {"IvsQ_binned_12345_1", "IvsQ_binned_12345_2"});
    auto row = successfulRow("12345");
    row.setOutputNames({{""}, {""}, {"IvsQ_binned_12345"}});
    auto const selectedPlottingWorkspaceNames = std::vector<std::string>{"IvsQ_binned_12345_1", "IvsQ_binned_12345_2"};
    auto const expectedPlottingWorkspaces =
        std::vector<PlottingWorkspace>{{"IvsQ_binned_12345_1",
                                        {"12345"},
                                        1,
                                        runOutputContext("IvsQ_binned_12345_1", {0, 0, 0, 0}, "IvsQ_binned_12345")},
                                       {"IvsQ_binned_12345_2",
                                        {"12345"},
                                        2,
                                        runOutputContext("IvsQ_binned_12345_2", {0, 0, 0, 1}, "IvsQ_binned_12345")}};
    auto const outputSelection = PlotOutputSelection{PlotOutputType::Alignment};

    EXPECT_CALL(view.tree, setItems(testing::_)).Times(1);
    presenter.notifyRunsTableChanged(RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {row})})));
    view.selectWorkspaces(selectedPlottingWorkspaceNames);
    EXPECT_CALL(view, selectedPlotOutputSelection()).Times(1).WillOnce(Return(outputSelection));
    EXPECT_CALL(plottingModel, workspacesForPlotting(expectedPlottingWorkspaces, outputSelection))
        .Times(1)
        .WillOnce(Return(std::vector<std::string>{}));
    EXPECT_CALL(plotter, plot(testing::_)).Times(0);

    presenter.notifyPlotIndividualClicked();
  }

  void testPlotDoesNotUsePeriodsLogForPeriodMetadata() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    addWorkspaceWithPeriodsLog("IvsQ_binned_12345_2", 2, 2);
    auto row = successfulRow("12345");
    row.setOutputNames({{""}, {""}, {"IvsQ_binned_12345_2"}});
    auto const expectedPlottingWorkspaces = std::vector<PlottingWorkspace>{
        {"IvsQ_binned_12345_2", {"12345"}, std::nullopt, runOutputContext("IvsQ_binned_12345_2", {0, 0, 0})}};
    auto const outputSelection = PlotOutputSelection{PlotOutputType::Alignment};

    EXPECT_CALL(view.tree, setItems(testing::_)).Times(1);
    presenter.notifyRunsTableChanged(RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {row})})));
    view.selectWorkspaces(std::vector<std::string>{"IvsQ_binned_12345_2"});
    EXPECT_CALL(view, selectedPlotOutputSelection()).Times(1).WillOnce(Return(outputSelection));
    EXPECT_CALL(plottingModel, workspacesForPlotting(expectedPlottingWorkspaces, outputSelection))
        .Times(1)
        .WillOnce(Return(std::vector<std::string>{}));
    EXPECT_CALL(plotter, plot(testing::_)).Times(0);

    presenter.notifyPlotIndividualClicked();
  }

  void testPlotUsesRunNumberSampleLogForPlottingWorkspace() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    addWorkspaceWithRunNumber("IvsQ_POLREF12345", "12345");
    auto row = successfulRow("POLREF12345");
    row.setOutputNames({{""}, {"IvsQ_POLREF12345"}, {""}});
    auto const expectedPlottingWorkspaces = std::vector<PlottingWorkspace>{
        {"IvsQ_POLREF12345", {"12345"}, std::nullopt, runOutputContext("IvsQ_POLREF12345", {0, 0, 0})}};
    auto const outputSelection = PlotOutputSelection{PlotOutputType::Alignment};

    EXPECT_CALL(view.tree, setItems(testing::_)).Times(1);
    presenter.notifyRunsTableChanged(RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {row})})));
    view.selectWorkspaces(std::vector<std::string>{"IvsQ_POLREF12345"});
    EXPECT_CALL(view, selectedPlotOutputSelection()).Times(1).WillOnce(Return(outputSelection));
    EXPECT_CALL(plottingModel, workspacesForPlotting(expectedPlottingWorkspaces, outputSelection))
        .Times(1)
        .WillOnce(Return(std::vector<std::string>{}));
    EXPECT_CALL(plotter, plot(testing::_)).Times(0);

    presenter.notifyPlotIndividualClicked();
  }

  void testPlotDoesNotUseRunsTableRunNumberAsPlottingWorkspaceFallback() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    Mantid::API::AnalysisDataService::Instance().addOrReplace("IvsQ_12345",
                                                              WorkspaceCreationHelper::create2DWorkspace(1, 1));
    auto row = successfulRow("12345");
    row.setOutputNames({{""}, {"IvsQ_12345"}, {""}});
    auto const expectedPlottingWorkspaces =
        std::vector<PlottingWorkspace>{{"IvsQ_12345", {}, std::nullopt, runOutputContext("IvsQ_12345", {0, 0, 0})}};
    auto const outputSelection = PlotOutputSelection{PlotOutputType::Alignment};

    EXPECT_CALL(view.tree, setItems(testing::_)).Times(1);
    presenter.notifyRunsTableChanged(RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {row})})));
    view.selectWorkspaces(std::vector<std::string>{"IvsQ_12345"});
    EXPECT_CALL(view, selectedPlotOutputSelection()).Times(1).WillOnce(Return(outputSelection));
    EXPECT_CALL(plottingModel, workspacesForPlotting(expectedPlottingWorkspaces, outputSelection))
        .Times(1)
        .WillOnce(Return(std::vector<std::string>{}));
    EXPECT_CALL(plotter, plot(testing::_)).Times(0);

    presenter.notifyPlotIndividualClicked();
  }

  void testRunsTableChangedOmitsEmptyWorkspaceGroupOutput() {
    NiceMock<MockPlottingView> view;
    PlottingPresenter presenter(&view);
    auto group = successfulGroup("Group 1", {successfulRow("12345")}, "stitched_12345");
    auto runsTable = RunsTable({}, 0.0, ReductionJobs({group}));
    Mantid::API::AnalysisDataService::Instance().add("stitched_12345", std::make_shared<Mantid::API::WorkspaceGroup>());

    EXPECT_CALL(view.tree,
                setItems(PlottingWorkspaceTreeItemStatesEqual(std::vector<PlottingWorkspaceTreeItemState>{})))
        .Times(1);

    presenter.notifyRunsTableChanged(runsTable);
  }

  void testPlotIndividualPlotsSelectedWorkspaces() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    auto const workspaces = std::vector<std::string>{"IvsQ_12345", "IvsQ_22345"};
    auto const selectedPlottingWorkspaces = plottingWorkspaces(workspaces);
    auto const outputSelection = PlotOutputSelection{PlotOutputType::ReflectivityCurve};
    auto const options = reflectivityCurvePlotOptions(PlotOutputType::ReflectivityCurve, PlotLayout::Individual);

    populatePlottingWorkspaceTree(presenter, view, workspaces);
    view.selectWorkspaces(workspaces);
    EXPECT_CALL(view, selectedPlotOutputSelection()).Times(1).WillOnce(Return(outputSelection));
    EXPECT_CALL(plottingModel, workspacesForPlotting(selectedPlottingWorkspaces, outputSelection))
        .Times(1)
        .WillOnce(Return(workspaces));
    EXPECT_CALL(plotter, plot(PlotRequest{{"IvsQ_12345"}, options})).Times(1);
    EXPECT_CALL(plotter, plot(PlotRequest{{"IvsQ_22345"}, options})).Times(1);

    presenter.notifyPlotIndividualClicked();
  }

  void testPlotIndividualPassesPlotParentToPlotter() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    QWidget plotParent;
    auto const workspaces = std::vector<std::string>{"IvsQ_12345"};
    auto const selectedPlottingWorkspaces = plottingWorkspaces(workspaces);
    auto const outputSelection = PlotOutputSelection{PlotOutputType::ReflectivityCurve};
    auto const options = reflectivityCurvePlotOptions(PlotOutputType::ReflectivityCurve, PlotLayout::Individual);

    populatePlottingWorkspaceTree(presenter, view, workspaces);
    view.selectWorkspaces(workspaces);
    EXPECT_CALL(view, selectedPlotOutputSelection()).Times(1).WillOnce(Return(outputSelection));
    EXPECT_CALL(view, plotParent()).Times(1).WillOnce(Return(&plotParent));
    EXPECT_CALL(plottingModel, workspacesForPlotting(selectedPlottingWorkspaces, outputSelection))
        .Times(1)
        .WillOnce(Return(workspaces));
    EXPECT_CALL(plotter, plot(PlotRequest{{"IvsQ_12345"}, options, &plotParent})).Times(1);

    presenter.notifyPlotIndividualClicked();
  }

  void testPlotIndividualWarnsAndCancelsWhenPlottingFiveItems() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    auto const workspaces =
        std::vector<std::string>{"IvsQ_12345", "IvsQ_22345", "IvsQ_32345", "IvsQ_42345", "IvsQ_52345"};
    auto const selectedPlottingWorkspaces = plottingWorkspaces(workspaces);
    auto const outputSelection = PlotOutputSelection{PlotOutputType::ReflectivityCurve};

    populatePlottingWorkspaceTree(presenter, view, workspaces);
    view.selectWorkspaces(workspaces);
    EXPECT_CALL(view, selectedPlotOutputSelection()).Times(1).WillOnce(Return(outputSelection));
    EXPECT_CALL(view, plotParent()).Times(1).WillOnce(Return(nullptr));
    EXPECT_CALL(plottingModel, workspacesForPlotting(selectedPlottingWorkspaces, outputSelection))
        .Times(1)
        .WillOnce(Return(workspaces));
    EXPECT_CALL(view, confirmPlottingMultipleItems(5)).Times(1).WillOnce(Return(false));
    EXPECT_CALL(plotter, plot(testing::_)).Times(0);

    presenter.notifyPlotIndividualClicked();
  }

  void testPlotIndividualContinuesWhenFiveItemWarningIsAccepted() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    auto const workspaces =
        std::vector<std::string>{"IvsQ_12345", "IvsQ_22345", "IvsQ_32345", "IvsQ_42345", "IvsQ_52345"};
    auto const selectedPlottingWorkspaces = plottingWorkspaces(workspaces);
    auto const outputSelection = PlotOutputSelection{PlotOutputType::ReflectivityCurve};

    populatePlottingWorkspaceTree(presenter, view, workspaces);
    view.selectWorkspaces(workspaces);
    EXPECT_CALL(view, selectedPlotOutputSelection()).Times(1).WillOnce(Return(outputSelection));
    EXPECT_CALL(view, plotParent()).Times(1).WillOnce(Return(nullptr));
    EXPECT_CALL(plottingModel, workspacesForPlotting(selectedPlottingWorkspaces, outputSelection))
        .Times(1)
        .WillOnce(Return(workspaces));
    EXPECT_CALL(view, confirmPlottingMultipleItems(5)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(plotter, plot(testing::_)).Times(5);

    presenter.notifyPlotIndividualClicked();
  }

  void testPlotOverplotPlotsSelectedWorkspaces() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    auto const workspaces = std::vector<std::string>{"IvsQ_12345", "IvsQ_22345"};
    auto const selectedPlottingWorkspaces = plottingWorkspaces(workspaces);
    auto const outputSelection = PlotOutputSelection{PlotOutputType::ReflectivityCurve};

    populatePlottingWorkspaceTree(presenter, view, workspaces);
    view.selectWorkspaces(workspaces);
    EXPECT_CALL(view, selectedPlotOutputSelection()).Times(1).WillOnce(Return(outputSelection));
    EXPECT_CALL(plottingModel, workspacesForPlotting(selectedPlottingWorkspaces, outputSelection))
        .Times(1)
        .WillOnce(Return(workspaces));
    EXPECT_CALL(plotter, plot(PlotRequest{workspaces, reflectivityCurvePlotOptions(PlotOutputType::ReflectivityCurve,
                                                                                   PlotLayout::Overplot)}))
        .Times(1);

    presenter.notifyPlotOverplotClicked();
  }

  void testPlotOverplotAddsSelectedWorkspacesToExistingPlotWhenRequested() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    auto const workspaces = std::vector<std::string>{"IvsQ_12345", "IvsQ_22345"};
    auto const selectedPlottingWorkspaces = plottingWorkspaces(workspaces);
    auto const outputSelection = PlotOutputSelection{PlotOutputType::ReflectivityCurve};

    populatePlottingWorkspaceTree(presenter, view, workspaces);
    EXPECT_CALL(view, shouldAddToExistingPlot()).WillRepeatedly(Return(true));
    view.selectWorkspaces(workspaces);
    EXPECT_CALL(view, selectedPlotOutputSelection()).Times(1).WillOnce(Return(outputSelection));
    EXPECT_CALL(plottingModel, workspacesForPlotting(selectedPlottingWorkspaces, outputSelection))
        .Times(1)
        .WillOnce(Return(workspaces));
    EXPECT_CALL(plotter,
                plot(PlotRequest{workspaces,
                                 reflectivityCurvePlotOptions(PlotOutputType::ReflectivityCurve, PlotLayout::Overplot),
                                 nullptr, true}))
        .Times(1);

    presenter.notifyPlotOverplotClicked();
  }

  void testPlotOverplotWarnsAndCancelsWhenPlottingFiveItems() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    auto const workspaces =
        std::vector<std::string>{"IvsQ_12345", "IvsQ_22345", "IvsQ_32345", "IvsQ_42345", "IvsQ_52345"};
    auto const selectedPlottingWorkspaces = plottingWorkspaces(workspaces);
    auto const outputSelection = PlotOutputSelection{PlotOutputType::ReflectivityCurve};

    populatePlottingWorkspaceTree(presenter, view, workspaces);
    view.selectWorkspaces(workspaces);
    EXPECT_CALL(view, selectedPlotOutputSelection()).Times(1).WillOnce(Return(outputSelection));
    EXPECT_CALL(view, plotParent()).Times(1).WillOnce(Return(nullptr));
    EXPECT_CALL(plottingModel, workspacesForPlotting(selectedPlottingWorkspaces, outputSelection))
        .Times(1)
        .WillOnce(Return(workspaces));
    EXPECT_CALL(view, confirmPlottingMultipleItems(5)).Times(1).WillOnce(Return(false));
    EXPECT_CALL(plotter, plot(testing::_)).Times(0);

    presenter.notifyPlotOverplotClicked();
  }

  void testPlotTiledPlotsSelectedWorkspaces() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    auto const workspaces = std::vector<std::string>{"IvsQ_12345", "IvsQ_22345"};
    auto const selectedPlottingWorkspaces = plottingWorkspaces(workspaces);
    auto const outputSelection = PlotOutputSelection{PlotOutputType::ReflectivityCurve};

    populatePlottingWorkspaceTree(presenter, view, workspaces);
    view.selectWorkspaces(workspaces);
    EXPECT_CALL(view, selectedPlotOutputSelection()).Times(1).WillOnce(Return(outputSelection));
    EXPECT_CALL(plottingModel, workspacesForPlotting(selectedPlottingWorkspaces, outputSelection))
        .Times(1)
        .WillOnce(Return(workspaces));
    EXPECT_CALL(plotter, plot(PlotRequest{workspaces, reflectivityCurvePlotOptions(PlotOutputType::ReflectivityCurve,
                                                                                   PlotLayout::Tiled)}))
        .Times(1);

    presenter.notifyPlotTiledClicked();
  }

  void testPlotTiledAddsSelectedWorkspacesToExistingPlotWhenRequested() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    auto const workspaces = std::vector<std::string>{"IvsQ_12345", "IvsQ_22345"};
    auto const selectedPlottingWorkspaces = plottingWorkspaces(workspaces);
    auto const outputSelection = PlotOutputSelection{PlotOutputType::ReflectivityCurve};

    populatePlottingWorkspaceTree(presenter, view, workspaces);
    EXPECT_CALL(view, shouldAddToExistingPlot()).WillRepeatedly(Return(true));
    view.selectWorkspaces(workspaces);
    EXPECT_CALL(view, selectedPlotOutputSelection()).Times(1).WillOnce(Return(outputSelection));
    EXPECT_CALL(plottingModel, workspacesForPlotting(selectedPlottingWorkspaces, outputSelection))
        .Times(1)
        .WillOnce(Return(workspaces));
    EXPECT_CALL(
        plotter,
        plot(PlotRequest{workspaces, reflectivityCurvePlotOptions(PlotOutputType::ReflectivityCurve, PlotLayout::Tiled),
                         nullptr, true}))
        .Times(1);

    presenter.notifyPlotTiledClicked();
  }

  void testPlotTiledPassesVerticalOptionToPlotter() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    auto const workspaces = std::vector<std::string>{"IvsQ_12345", "IvsQ_22345"};
    auto const selectedPlottingWorkspaces = plottingWorkspaces(workspaces);
    auto const outputSelection = PlotOutputSelection{PlotOutputType::ReflectivityCurve};

    populatePlottingWorkspaceTree(presenter, view, workspaces);
    EXPECT_CALL(view, shouldPlotTiledVertically()).Times(1).WillOnce(Return(true));
    view.selectWorkspaces(workspaces);
    EXPECT_CALL(view, selectedPlotOutputSelection()).Times(1).WillOnce(Return(outputSelection));
    EXPECT_CALL(plottingModel, workspacesForPlotting(selectedPlottingWorkspaces, outputSelection))
        .Times(1)
        .WillOnce(Return(workspaces));
    EXPECT_CALL(
        plotter,
        plot(PlotRequest{workspaces, reflectivityCurvePlotOptions(PlotOutputType::ReflectivityCurve, PlotLayout::Tiled),
                         nullptr, false, true}))
        .Times(1);

    presenter.notifyPlotTiledClicked();
  }

  void testPlotTiledPassesShareAxesOptionsToPlotter() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    auto const workspaces = std::vector<std::string>{"IvsQ_12345", "IvsQ_22345"};
    auto const selectedPlottingWorkspaces = plottingWorkspaces(workspaces);
    auto const outputSelection = PlotOutputSelection{PlotOutputType::ReflectivityCurve};

    populatePlottingWorkspaceTree(presenter, view, workspaces);
    EXPECT_CALL(view, shouldShareXAxisWhenTiled()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(view, shouldShareYAxisWhenTiled()).Times(1).WillOnce(Return(false));
    view.selectWorkspaces(workspaces);
    EXPECT_CALL(view, selectedPlotOutputSelection()).Times(1).WillOnce(Return(outputSelection));
    EXPECT_CALL(plottingModel, workspacesForPlotting(selectedPlottingWorkspaces, outputSelection))
        .Times(1)
        .WillOnce(Return(workspaces));
    EXPECT_CALL(
        plotter,
        plot(PlotRequest{workspaces, reflectivityCurvePlotOptions(PlotOutputType::ReflectivityCurve, PlotLayout::Tiled),
                         nullptr, false, false, true, false}))
        .Times(1);

    presenter.notifyPlotTiledClicked();
  }

  void testAddToExistingPlotChangedUpdatesPlotActionState() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);

    EXPECT_CALL(view, setPlotActionState(testing::_)).Times(1);
    EXPECT_CALL(plotter, hasActiveReflectometryFigure()).Times(0);
    EXPECT_CALL(plotter, canOverplotActiveFigure()).Times(0);

    presenter.notifyAddToExistingPlotChanged();
  }

  void testActiveFigureChangedUpdatesPlotActionState() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);

    EXPECT_CALL(plotter, hasActiveReflectometryFigure()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(plotter, canOverplotActiveFigure()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(view, setPlotActionState(testing::_)).Times(1);

    presenter.notifyActiveFigureChanged();
  }

  void testPlotTiledWarnsAndCancelsWhenPlottingFiveItems() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    auto const workspaces =
        std::vector<std::string>{"IvsQ_12345", "IvsQ_22345", "IvsQ_32345", "IvsQ_42345", "IvsQ_52345"};
    auto const selectedPlottingWorkspaces = plottingWorkspaces(workspaces);
    auto const outputSelection = PlotOutputSelection{PlotOutputType::ReflectivityCurve};

    populatePlottingWorkspaceTree(presenter, view, workspaces);
    view.selectWorkspaces(workspaces);
    EXPECT_CALL(view, selectedPlotOutputSelection()).Times(1).WillOnce(Return(outputSelection));
    EXPECT_CALL(view, plotParent()).Times(1).WillOnce(Return(nullptr));
    EXPECT_CALL(plottingModel, workspacesForPlotting(selectedPlottingWorkspaces, outputSelection))
        .Times(1)
        .WillOnce(Return(workspaces));
    EXPECT_CALL(view, confirmPlottingMultipleItems(5)).Times(1).WillOnce(Return(false));
    EXPECT_CALL(plotter, plot(testing::_)).Times(0);

    presenter.notifyPlotTiledClicked();
  }

  void testPlotDoesNothingWhenNoWorkspacesSelected() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);

    view.selectWorkspaces(std::vector<std::string>{});
    EXPECT_CALL(view, selectedPlotOutputSelection()).Times(0);
    EXPECT_CALL(plottingModel, workspacesForPlotting(testing::_, testing::_)).Times(0);
    EXPECT_CALL(plotter, plot(testing::_)).Times(0);

    presenter.notifyPlotIndividualClicked();
  }

  void testPlotDoesNothingWhenNoOutputTypeIsSelected() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    auto const workspaces = std::vector<std::string>{"IvsQ_12345"};

    populatePlottingWorkspaceTree(presenter, view, workspaces);
    view.selectWorkspaces(workspaces);
    EXPECT_CALL(view, selectedPlotOutputType()).Times(1).WillOnce(Return(std::nullopt));
    EXPECT_CALL(view, selectedPlotOutputSelection()).Times(0);
    EXPECT_CALL(plottingModel, workspacesForPlotting(testing::_, testing::_)).Times(0);
    EXPECT_CALL(plotter, plot(testing::_)).Times(0);

    presenter.notifyPlotIndividualClicked();
  }

  void testPlotDoesNothingWhenModelReturnsNoWorkspaces() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    auto const workspaces = std::vector<std::string>{"IvsQ_binned_group"};
    auto const selectedPlottingWorkspaces = std::vector<PlottingWorkspace>{
        {"IvsQ_binned_group", {"12345"}, std::nullopt, runOutputContext("IvsQ_binned_group", {0, 0, 0})}};
    auto const outputSelection = PlotOutputSelection{PlotOutputType::SpinAsymmetry};

    populatePlottingWorkspaceTreeWithBinnedWorkspace(presenter, view, workspaces.front());
    view.selectWorkspaces(workspaces);
    EXPECT_CALL(view, selectedPlotOutputSelection()).Times(1).WillOnce(Return(outputSelection));
    EXPECT_CALL(plottingModel, workspacesForPlotting(selectedPlottingWorkspaces, outputSelection))
        .Times(1)
        .WillOnce(Return(std::vector<std::string>{}));
    EXPECT_CALL(plotter, plot(testing::_)).Times(0);

    presenter.notifyPlotOverplotClicked();
  }

  void testCombinedOutputSubmitsManualReflectivityAndSpinAsymmetryRequestsInOrder() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider optionsProvider;
    NiceMock<MockPlottingModel> model;
    PlottingPresenter presenter(&view, plotter, optionsProvider, model);
    auto const workspaces = std::vector<std::string>{"IvsQ_binned_12345_1", "IvsQ_binned_12345_2"};
    auto const batches =
        populatePlottingWorkspaceTreeWithBinnedWorkspaceGroups(presenter, view, {{"IvsQ_binned_12345", workspaces}});
    view.selectWorkspaces({"IvsQ_binned_12345"});
    auto const selection = PlotOutputSelection{PlotOutputType::ReflectivityAndSpinAsymmetry};
    EXPECT_CALL(view, selectedPlotOutputSelection()).WillOnce(Return(selection));
    ON_CALL(view, shouldAddToExistingPlot()).WillByDefault(Return(true));
    EXPECT_CALL(model, workspacesForPlotting(batches.front(), PlotOutputSelection{PlotOutputType::ReflectivityCurve}))
        .WillOnce(Return(workspaces));
    EXPECT_CALL(model, workspacesForPlotting(batches.front(), PlotOutputSelection{PlotOutputType::SpinAsymmetry}))
        .WillOnce(Return(std::vector<std::string>{"spin_asymmetry"}));
    testing::InSequence sequence;
    EXPECT_CALL(plotter, plot(PlotRequest{workspaces, reflectivityCurvePlotOptions(PlotOutputType::ReflectivityCurve,
                                                                                   PlotLayout::Overplot)}));
    EXPECT_CALL(
        plotter,
        plot(PlotRequest{
            {"spin_asymmetry"}, spinAsymmetryPlotOptions(PlotLayout::Tiled), nullptr, true, true, true, false}));

    presenter.notifyPlotIndividualClicked();
  }

  void testCombinedOutputDoesNotPlotWhenSpinAsymmetryIsUnavailable() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider optionsProvider;
    NiceMock<MockPlottingModel> model;
    PlottingPresenter presenter(&view, plotter, optionsProvider, model);
    auto const workspaces = std::vector<std::string>{"IvsQ_binned_12345_1", "IvsQ_binned_12345_2"};
    auto const batches =
        populatePlottingWorkspaceTreeWithBinnedWorkspaceGroups(presenter, view, {{"IvsQ_binned_12345", workspaces}});
    view.selectWorkspaces({"IvsQ_binned_12345"});
    auto const selection = PlotOutputSelection{PlotOutputType::ReflectivityAndSpinAsymmetry};
    EXPECT_CALL(view, selectedPlotOutputSelection()).WillOnce(Return(selection));
    EXPECT_CALL(model, workspacesForPlotting(batches.front(), PlotOutputSelection{PlotOutputType::ReflectivityCurve}))
        .WillOnce(Return(workspaces));
    EXPECT_CALL(model, workspacesForPlotting(batches.front(), PlotOutputSelection{PlotOutputType::SpinAsymmetry}))
        .WillOnce(Return(std::vector<std::string>{}));
    EXPECT_CALL(plotter, plot(testing::_)).Times(0);

    presenter.notifyPlotIndividualClicked();
  }

  void testCombinedOutputWarnsForAllComponentsAndCancelsBeforePlotting() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider optionsProvider;
    NiceMock<MockPlottingModel> model;
    PlottingPresenter presenter(&view, plotter, optionsProvider, model);
    auto const workspaces = std::vector<std::string>{"IvsQ_binned_12345_1", "IvsQ_binned_12345_2"};
    auto const batches =
        populatePlottingWorkspaceTreeWithBinnedWorkspaceGroups(presenter, view, {{"IvsQ_binned_12345", workspaces}});
    view.selectWorkspaces({"IvsQ_binned_12345"});
    EXPECT_CALL(view, selectedPlotOutputSelection())
        .WillOnce(Return(PlotOutputSelection{PlotOutputType::ReflectivityAndSpinAsymmetry}));
    EXPECT_CALL(model, workspacesForPlotting(batches.front(), PlotOutputSelection{PlotOutputType::ReflectivityCurve}))
        .WillOnce(Return(std::vector<std::string>{"up_up", "up_down", "down_up", "down_down"}));
    EXPECT_CALL(model, workspacesForPlotting(batches.front(), PlotOutputSelection{PlotOutputType::SpinAsymmetry}))
        .WillOnce(Return(std::vector<std::string>{"spin_asymmetry"}));
    EXPECT_CALL(view, confirmPlottingMultipleItems(5)).WillOnce(Return(false));
    EXPECT_CALL(plotter, plot(testing::_)).Times(0);

    presenter.notifyPlotIndividualClicked();
  }

  void testCombinedOutputRunsACompleteSequencePerWorkspaceGroupInSelectionOrder() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider optionsProvider;
    NiceMock<MockPlottingModel> model;
    PlottingPresenter presenter(&view, plotter, optionsProvider, model);
    auto const membersA = std::vector<std::string>{"a_up", "a_down"};
    auto const membersB = std::vector<std::string>{"b_up", "b_down"};
    auto const batches = populatePlottingWorkspaceTreeWithBinnedWorkspaceGroups(
        presenter, view, {{"group_a", membersA}, {"group_b", membersB}});
    view.tree.setSelected({0, 1, 0}, true);
    view.tree.setSelected({0, 0, 0}, true);
    EXPECT_CALL(view, selectedPlotOutputSelection())
        .WillOnce(Return(PlotOutputSelection{PlotOutputType::ReflectivityAndSpinAsymmetry}));

    testing::InSequence sequence;
    EXPECT_CALL(model, workspacesForPlotting(batches[1], PlotOutputSelection{PlotOutputType::ReflectivityCurve}))
        .WillOnce(Return(membersB));
    EXPECT_CALL(model, workspacesForPlotting(batches[1], PlotOutputSelection{PlotOutputType::SpinAsymmetry}))
        .WillOnce(Return(std::vector<std::string>{"spin_b"}));
    EXPECT_CALL(model, workspacesForPlotting(batches[0], PlotOutputSelection{PlotOutputType::ReflectivityCurve}))
        .WillOnce(Return(membersA));
    EXPECT_CALL(model, workspacesForPlotting(batches[0], PlotOutputSelection{PlotOutputType::SpinAsymmetry}))
        .WillOnce(Return(std::vector<std::string>{"spin_a"}));
    EXPECT_CALL(view, confirmPlottingMultipleItems(6)).WillOnce(Return(true));
    auto const reflectivityOptions =
        reflectivityCurvePlotOptions(PlotOutputType::ReflectivityCurve, PlotLayout::Overplot);
    auto const spinOptions = spinAsymmetryPlotOptions(PlotLayout::Tiled);
    EXPECT_CALL(plotter, plot(PlotRequest{membersB, reflectivityOptions}));
    EXPECT_CALL(plotter, plot(PlotRequest{{"spin_b"}, spinOptions, nullptr, true, true, true, false}));
    EXPECT_CALL(plotter, plot(PlotRequest{membersA, reflectivityOptions}));
    EXPECT_CALL(plotter, plot(PlotRequest{{"spin_a"}, spinOptions, nullptr, true, true, true, false}));

    presenter.notifyPlotIndividualClicked();
  }

  void testOverplotKeepsMultipleWorkspaceGroupsTogether() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider optionsProvider;
    NiceMock<MockPlottingModel> model;
    PlottingPresenter presenter(&view, plotter, optionsProvider, model);
    auto const batches = populatePlottingWorkspaceTreeWithBinnedWorkspaceGroups(
        presenter, view, {{"group_a", {"a_up", "a_down"}}, {"group_b", {"b_up", "b_down"}}});
    view.selectWorkspaces({"group_a", "group_b"});
    auto expectedSelection = batches[0];
    expectedSelection.insert(expectedSelection.end(), batches[1].begin(), batches[1].end());
    auto const selection = PlotOutputSelection{PlotOutputType::ReflectivityCurve};
    auto const outputs = std::vector<std::string>{"a_up", "a_down", "b_up", "b_down"};
    EXPECT_CALL(view, selectedPlotOutputSelection()).WillOnce(Return(selection));
    EXPECT_CALL(model, workspacesForPlotting(expectedSelection, selection)).WillOnce(Return(outputs));
    EXPECT_CALL(plotter, plot(PlotRequest{outputs, optionsProvider.optionsFor(selection, PlotLayout::Overplot)}));

    presenter.notifyPlotOverplotClicked();
  }

private:
  std::vector<PlottingWorkspace> plottingWorkspaces(std::vector<std::string> const &workspaceNames) {
    auto plottingWorkspaces = std::vector<PlottingWorkspace>{};
    plottingWorkspaces.reserve(workspaceNames.size());
    for (size_t row = 0; row < workspaceNames.size(); ++row) {
      auto const &workspaceName = workspaceNames[row];
      plottingWorkspaces.push_back({workspaceName,
                                    {runNumberFromWorkspaceName(workspaceName)},
                                    std::nullopt,
                                    runOutputContext(workspaceName, {0, static_cast<int>(row), 0})});
    }
    return plottingWorkspaces;
  }

  std::string runNumberFromWorkspaceName(std::string const &workspaceName) {
    auto const separator = workspaceName.find_last_of('_');
    return separator == std::string::npos ? workspaceName : workspaceName.substr(separator + 1);
  }

  void populatePlottingWorkspaceTree(PlottingPresenter &presenter, MockPlottingView &view,
                                     std::vector<std::string> const &workspaceNames) {
    addWorkspaces(workspaceNames);
    auto rows = std::vector<std::optional<Row>>{};
    for (auto const &workspaceName : workspaceNames) {
      rows.emplace_back(successfulRow(runNumberFromWorkspaceName(workspaceName)));
    }

    EXPECT_CALL(view.tree, setItems(testing::_)).Times(1);
    presenter.notifyRunsTableChanged(RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", std::move(rows))})));
  }

  void populatePlottingWorkspaceTreeWithBinnedWorkspace(PlottingPresenter &presenter, MockPlottingView &view,
                                                        std::string const &workspaceName) {
    addWorkspaceWithRunNumber(workspaceName, "12345");
    auto row = successfulRow("12345");
    row.setOutputNames({{""}, {""}, {workspaceName}});

    EXPECT_CALL(view.tree, setItems(testing::_)).Times(1);
    presenter.notifyRunsTableChanged(RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {row})})));
  }

  std::vector<std::vector<PlottingWorkspace>> populatePlottingWorkspaceTreeWithBinnedWorkspaceGroups(
      PlottingPresenter &presenter, MockPlottingView &view,
      std::vector<std::pair<std::string, std::vector<std::string>>> const &groups) {
    auto rows = std::vector<std::optional<Row>>{};
    auto expectedBatches = std::vector<std::vector<PlottingWorkspace>>{};
    for (size_t rowIndex = 0; rowIndex < groups.size(); ++rowIndex) {
      auto const &[groupName, members] = groups[rowIndex];
      auto const runNumber = std::to_string(12345 + rowIndex);
      auto expectedMembers = std::vector<PlottingWorkspace>{};
      for (size_t memberIndex = 0; memberIndex < members.size(); ++memberIndex) {
        auto const &memberName = members[memberIndex];
        addWorkspaceWithRunNumber(memberName, runNumber);
        expectedMembers.push_back(
            {memberName,
             {runNumber},
             std::nullopt,
             runOutputContext(memberName, {0, static_cast<int>(rowIndex), 0, static_cast<int>(memberIndex)},
                              groupName)});
      }
      groupExistingWorkspaces(groupName, members);
      auto row = successfulRow(runNumber);
      row.setOutputNames({{""}, {""}, {groupName}});
      rows.emplace_back(std::move(row));
      expectedBatches.emplace_back(std::move(expectedMembers));
    }
    EXPECT_CALL(view.tree, setItems(testing::_)).Times(1);
    presenter.notifyRunsTableChanged(RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", std::move(rows))})));
    return expectedBatches;
  }

  PlottingWorkspaceTreeItem groupItem(std::string label, std::vector<PlottingWorkspaceTreeItem> children) {
    return {std::move(label), PlottingWorkspaceTreeItemType::ReductionGroup, ReducedWorkspaceOutputType::None, "",
            std::move(children)};
  }

  PlottingWorkspaceTreeItem runItem(std::string label, std::vector<PlottingWorkspaceTreeItem> children) {
    auto item = PlottingWorkspaceTreeItem{std::move(label), PlottingWorkspaceTreeItemType::Run,
                                          ReducedWorkspaceOutputType::None, "", std::move(children)};
    item.context.origin = PlottingWorkspaceOrigin::RunOutput;
    return item;
  }

  PlottingWorkspaceTreeItem workspaceGroupItem(std::string label, std::vector<PlottingWorkspaceTreeItem> children,
                                               PlottingWorkspaceOrigin origin = PlottingWorkspaceOrigin::None) {
    auto const workspaceName = label;
    auto item = PlottingWorkspaceTreeItem{std::move(label), PlottingWorkspaceTreeItemType::WorkspaceGroup,
                                          ReducedWorkspaceOutputType::None, workspaceName, std::move(children)};
    item.context.origin = origin;
    return item;
  }

  PlottingWorkspaceTreeItem workspaceItem(std::string label, ReducedWorkspaceOutputType reducedOutputType,
                                          PlottingWorkspaceOrigin origin = PlottingWorkspaceOrigin::None) {
    auto const workspaceName = label;
    auto item = PlottingWorkspaceTreeItem{
        std::move(label), PlottingWorkspaceTreeItemType::Workspace, reducedOutputType, workspaceName, {}};
    item.context.origin = origin;
    return item;
  }

  Row successfulRow(std::string const &run) {
    auto row = Row({run}, 0.5, TransmissionRunPair(), RangeInQ(), std::nullopt, ReductionOptionsMap(),
                   ReductionWorkspaces({run}, TransmissionRunPair()));
    row.setOutputNames({{"IvsLam_" + run}, {"IvsQ_" + run}, {"IvsQ_binned_" + run}});
    row.setSuccess();
    return row;
  }

  Group successfulGroup(std::string const &groupName, std::vector<std::optional<Row>> rows,
                        std::string const &outputWorkspace = "") {
    auto group = Group(groupName, std::move(rows));
    if (!outputWorkspace.empty()) {
      group.setOutputNames({{outputWorkspace}});
    }
    group.setSuccess();
    return group;
  }

  void addWorkspaces(std::vector<std::string> const &workspaceNames) {
    for (auto const &name : workspaceNames) {
      addWorkspaceWithRunNumber(name, runNumberFromWorkspaceName(name));
    }
  }

  void addWorkspaceWithPeriod(std::string const &workspaceName, int const periods, int const currentPeriod) {
    auto workspace = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    workspace->mutableRun().addProperty("run_number", std::string{"12345"});
    workspace->mutableRun().addProperty("nperiods", periods);
    workspace->mutableRun().addProperty("current_period", currentPeriod);
    Mantid::API::AnalysisDataService::Instance().addOrReplace(workspaceName, workspace);
  }

  void addWorkspaceWithRunNumberAndPeriod(std::string const &workspaceName, std::string const &runNumber,
                                          int const periods, int const currentPeriod) {
    auto workspace = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    workspace->mutableRun().addProperty("run_number", runNumber);
    workspace->mutableRun().addProperty("nperiods", periods);
    workspace->mutableRun().addProperty("current_period", currentPeriod);
    Mantid::API::AnalysisDataService::Instance().addOrReplace(workspaceName, workspace);
  }

  void addWorkspaceWithPeriodsLog(std::string const &workspaceName, int const periods, int const currentPeriod) {
    auto workspace = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    workspace->mutableRun().addProperty("run_number", std::string{"12345"});
    workspace->mutableRun().addProperty("periods", periods);
    workspace->mutableRun().addProperty("current_period", currentPeriod);
    Mantid::API::AnalysisDataService::Instance().addOrReplace(workspaceName, workspace);
  }

  void addWorkspaceWithRunNumber(std::string const &workspaceName, std::string const &runNumber) {
    auto workspace = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    workspace->mutableRun().addProperty("run_number", runNumber);
    Mantid::API::AnalysisDataService::Instance().addOrReplace(workspaceName, workspace);
  }

  void groupExistingWorkspaces(std::string const &groupName, std::vector<std::string> const &workspaceNames) {
    Mantid::API::AnalysisDataService::Instance().add(groupName, std::make_shared<Mantid::API::WorkspaceGroup>());
    for (auto const &name : workspaceNames) {
      Mantid::API::AnalysisDataService::Instance().addToGroup(groupName, name);
    }
  }

  void addWorkspaceGroup(std::string const &groupName, std::vector<std::string> const &workspaceNames) {
    addWorkspaces(workspaceNames);
    groupExistingWorkspaces(groupName, workspaceNames);
  }
};
