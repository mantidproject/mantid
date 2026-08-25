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
  return PlottingViewStateBuilder().plottingWorkspaceTreeItemStates(items, outputType);
}

inline std::vector<PlottingWorkspaceTreeItemState>
plottingWorkspaceTreeItemStatesForDefaultOutputType(std::vector<PlottingWorkspaceTreeItem> const &items) {
  return plottingWorkspaceTreeItemStatesForOutputType(items, PlotOutputType::ReflectivityCurve);
}

MATCHER_P(PlottingWorkspaceTreeItemStatesEqual, expected, "matches plotting workspace tree item states") {
  if (arg == expected)
    return true;

  *result_listener << "\nexpected: " << testing::PrintToString(expected) << "\nactual: " << testing::PrintToString(arg);
  return false;
}

class MockPlottingView : public IPlottingView {
public:
  MOCK_METHOD(void, subscribe, (PlottingViewSubscriber *), (override));
  MOCK_METHOD(void, setOutputSelectionEnabled, (bool), (override));
  MOCK_METHOD(void, setAvailablePlotOutputTypes, (std::vector<PlotOutputTypeViewItem> const &), (override));
  MOCK_METHOD(void, setPlotOutputControlsState, (PlotOutputControlsState const &), (override));
  MOCK_METHOD(void, setPlotActionState, (PlotActionState const &), (override));
  MOCK_METHOD(void, setPlottingWorkspaceTreeItemStates, (std::vector<PlottingWorkspaceTreeItemState> const &),
              (override));
  MOCK_METHOD(std::vector<std::string>, selectedPlottingWorkspaceNames, (), (const, override));
  MOCK_METHOD(size_t, selectedPlottingWorkspaceGroupCount, (), (const, override));
  MOCK_METHOD(std::optional<PlotOutputType>, selectedPlotOutputType, (), (const, override));
  MOCK_METHOD(PlotOutputSelection, selectedPlotOutputSelection, (), (const, override));
  MOCK_METHOD(bool, addToExistingPlot, (), (const, override));
  MOCK_METHOD(bool, plotTiledVertically, (), (const, override));
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
    auto const expected = std::vector<PlotOutputTypeViewItem>{{PlotOutputType::ReflectivityCurve, "Reflectivity Curve"},
                                                              {PlotOutputType::DetectorMap, "Detector Map"},
                                                              {PlotOutputType::SpinAsymmetry, "Spin Asymmetry"},
                                                              {PlotOutputType::Alignment, "Alignment"}};

    EXPECT_CALL(view, setAvailablePlotOutputTypes(expected)).Times(1);

    presenter.notifyInstrumentChanged("POLREF");
  }

  void testPlottingWorkspaceTreeIsClearedWhenNoPlotOutputTypeIsSelected() {
    NiceMock<MockPlottingView> view;
    PlottingPresenter presenter(&view);
    auto runsTable = RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {successfulRow("12345")})}));
    addWorkspaces({"IvsQ_12345"});

    EXPECT_CALL(view, selectedPlotOutputType()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(view, setPlottingWorkspaceTreeItemStates(
                          PlottingWorkspaceTreeItemStatesEqual(std::vector<PlottingWorkspaceTreeItemState>{})))
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
    EXPECT_CALL(view, selectedPlottingWorkspaceNames()).Times(1).WillOnce(Return(workspaces));
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

    EXPECT_CALL(view, setPlottingWorkspaceTreeItemStates(PlottingWorkspaceTreeItemStatesEqual(
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

    EXPECT_CALL(view, setPlottingWorkspaceTreeItemStates(
                          PlottingWorkspaceTreeItemStatesEqual(std::vector<PlottingWorkspaceTreeItemState>{})))
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

    EXPECT_CALL(view, setPlottingWorkspaceTreeItemStates(PlottingWorkspaceTreeItemStatesEqual(
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
        groupItem("Group 1", {workspaceItem("stitched_12345", ReducedWorkspaceOutputType::IvsQBinned)})};

    EXPECT_CALL(view, setPlottingWorkspaceTreeItemStates(PlottingWorkspaceTreeItemStatesEqual(
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

    EXPECT_CALL(view, setPlottingWorkspaceTreeItemStates(PlottingWorkspaceTreeItemStatesEqual(
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
                                        workspaceItem("stitched_12345_2", ReducedWorkspaceOutputType::IvsQBinned)})})};

    EXPECT_CALL(view, setPlottingWorkspaceTreeItemStates(PlottingWorkspaceTreeItemStatesEqual(
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
        "Group 1", {workspaceItem("stitched_12345", ReducedWorkspaceOutputType::IvsQBinned),
                    runItem("12345", {workspaceItem("IvsLam_12345", ReducedWorkspaceOutputType::IvsLambda),
                                      workspaceItem("IvsQ_12345", ReducedWorkspaceOutputType::IvsQ),
                                      workspaceItem("IvsQ_binned_12345", ReducedWorkspaceOutputType::IvsQBinned)})})};

    EXPECT_CALL(view, selectedPlotOutputType())
        .WillOnce(Return(std::optional<PlotOutputType>{PlotOutputType::ReflectivityCurve}))
        .WillRepeatedly(Return(std::optional<PlotOutputType>{PlotOutputType::Alignment}));
    {
      testing::InSequence sequence;
      EXPECT_CALL(view, setPlottingWorkspaceTreeItemStates(PlottingWorkspaceTreeItemStatesEqual(
                            plottingWorkspaceTreeItemStatesForOutputType(expected, PlotOutputType::ReflectivityCurve))))
          .Times(1);
      EXPECT_CALL(view, setPlottingWorkspaceTreeItemStates(PlottingWorkspaceTreeItemStatesEqual(
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
    row.setOutputNames({"", "", "IvsQ_binned_12345_2"});
    auto const expectedPlottingWorkspaces = std::vector<PlottingWorkspace>{{"IvsQ_binned_12345_2", {"12345"}, "", 2}};
    auto const outputSelection = PlotOutputSelection{PlotOutputType::Alignment};

    EXPECT_CALL(view, setPlottingWorkspaceTreeItemStates(testing::_)).Times(1);
    presenter.notifyRunsTableChanged(RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {row})})));
    EXPECT_CALL(view, selectedPlottingWorkspaceNames())
        .Times(1)
        .WillOnce(Return(std::vector<std::string>{"IvsQ_binned_12345_2"}));
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
    row.setOutputNames({"", "", "IvsQ_binned_12345"});
    auto const selectedPlottingWorkspaceNames = std::vector<std::string>{"IvsQ_binned_12345_1", "IvsQ_binned_12345_2"};
    auto const expectedPlottingWorkspaces =
        std::vector<PlottingWorkspace>{{"IvsQ_binned_12345_1", {"12345"}, "IvsQ_binned_12345", 1},
                                       {"IvsQ_binned_12345_2", {"12345"}, "IvsQ_binned_12345", 2}};
    auto const outputSelection = PlotOutputSelection{PlotOutputType::Alignment};

    EXPECT_CALL(view, setPlottingWorkspaceTreeItemStates(testing::_)).Times(1);
    presenter.notifyRunsTableChanged(RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {row})})));
    EXPECT_CALL(view, selectedPlottingWorkspaceNames()).Times(1).WillOnce(Return(selectedPlottingWorkspaceNames));
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
    row.setOutputNames({"", "", "IvsQ_binned_12345_2"});
    auto const expectedPlottingWorkspaces =
        std::vector<PlottingWorkspace>{{"IvsQ_binned_12345_2", {"12345"}, "", std::nullopt}};
    auto const outputSelection = PlotOutputSelection{PlotOutputType::Alignment};

    EXPECT_CALL(view, setPlottingWorkspaceTreeItemStates(testing::_)).Times(1);
    presenter.notifyRunsTableChanged(RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {row})})));
    EXPECT_CALL(view, selectedPlottingWorkspaceNames())
        .Times(1)
        .WillOnce(Return(std::vector<std::string>{"IvsQ_binned_12345_2"}));
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
    row.setOutputNames({"", "IvsQ_POLREF12345", ""});
    auto const expectedPlottingWorkspaces =
        std::vector<PlottingWorkspace>{{"IvsQ_POLREF12345", {"12345"}, "", std::nullopt}};
    auto const outputSelection = PlotOutputSelection{PlotOutputType::Alignment};

    EXPECT_CALL(view, setPlottingWorkspaceTreeItemStates(testing::_)).Times(1);
    presenter.notifyRunsTableChanged(RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {row})})));
    EXPECT_CALL(view, selectedPlottingWorkspaceNames())
        .Times(1)
        .WillOnce(Return(std::vector<std::string>{"IvsQ_POLREF12345"}));
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
    row.setOutputNames({"", "IvsQ_12345", ""});
    auto const expectedPlottingWorkspaces = std::vector<PlottingWorkspace>{{"IvsQ_12345", {}, "", std::nullopt}};
    auto const outputSelection = PlotOutputSelection{PlotOutputType::Alignment};

    EXPECT_CALL(view, setPlottingWorkspaceTreeItemStates(testing::_)).Times(1);
    presenter.notifyRunsTableChanged(RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {row})})));
    EXPECT_CALL(view, selectedPlottingWorkspaceNames())
        .Times(1)
        .WillOnce(Return(std::vector<std::string>{"IvsQ_12345"}));
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

    EXPECT_CALL(view, setPlottingWorkspaceTreeItemStates(
                          PlottingWorkspaceTreeItemStatesEqual(std::vector<PlottingWorkspaceTreeItemState>{})))
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
    EXPECT_CALL(view, selectedPlottingWorkspaceNames()).Times(2).WillRepeatedly(Return(workspaces));
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
    EXPECT_CALL(view, selectedPlottingWorkspaceNames()).Times(2).WillRepeatedly(Return(workspaces));
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
    EXPECT_CALL(view, selectedPlottingWorkspaceNames()).Times(1).WillOnce(Return(workspaces));
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
    EXPECT_CALL(view, selectedPlottingWorkspaceNames()).Times(2).WillRepeatedly(Return(workspaces));
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
    EXPECT_CALL(view, selectedPlottingWorkspaceNames()).Times(2).WillRepeatedly(Return(workspaces));
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
    EXPECT_CALL(view, addToExistingPlot()).WillRepeatedly(Return(true));
    EXPECT_CALL(view, selectedPlottingWorkspaceNames()).Times(2).WillRepeatedly(Return(workspaces));
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
    EXPECT_CALL(view, selectedPlottingWorkspaceNames()).Times(1).WillOnce(Return(workspaces));
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
    EXPECT_CALL(view, selectedPlottingWorkspaceNames()).Times(2).WillRepeatedly(Return(workspaces));
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
    EXPECT_CALL(view, addToExistingPlot()).WillRepeatedly(Return(true));
    EXPECT_CALL(view, selectedPlottingWorkspaceNames()).Times(2).WillRepeatedly(Return(workspaces));
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
    EXPECT_CALL(view, plotTiledVertically()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(view, selectedPlottingWorkspaceNames()).Times(2).WillRepeatedly(Return(workspaces));
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
    EXPECT_CALL(view, selectedPlottingWorkspaceNames()).Times(1).WillOnce(Return(workspaces));
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

    EXPECT_CALL(view, selectedPlottingWorkspaceNames()).Times(1).WillOnce(Return(std::vector<std::string>{}));
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
    EXPECT_CALL(view, selectedPlottingWorkspaceNames()).Times(1).WillOnce(Return(workspaces));
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
    auto const selectedPlottingWorkspaces =
        std::vector<PlottingWorkspace>{{"IvsQ_binned_group", {"12345"}, "", std::nullopt}};
    auto const outputSelection = PlotOutputSelection{PlotOutputType::SpinAsymmetry};

    populatePlottingWorkspaceTreeWithBinnedWorkspace(presenter, view, workspaces.front());
    EXPECT_CALL(view, selectedPlottingWorkspaceNames()).Times(1).WillOnce(Return(workspaces));
    EXPECT_CALL(view, selectedPlotOutputSelection()).Times(1).WillOnce(Return(outputSelection));
    EXPECT_CALL(plottingModel, workspacesForPlotting(selectedPlottingWorkspaces, outputSelection))
        .Times(1)
        .WillOnce(Return(std::vector<std::string>{}));
    EXPECT_CALL(plotter, plot(testing::_)).Times(0);

    presenter.notifyPlotOverplotClicked();
  }

private:
  std::vector<PlottingWorkspace> plottingWorkspaces(std::vector<std::string> const &workspaceNames) {
    auto plottingWorkspaces = std::vector<PlottingWorkspace>{};
    plottingWorkspaces.reserve(workspaceNames.size());
    for (auto const &workspaceName : workspaceNames) {
      plottingWorkspaces.push_back({workspaceName, {runNumberFromWorkspaceName(workspaceName)}, "", std::nullopt});
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

    EXPECT_CALL(view, setPlottingWorkspaceTreeItemStates(testing::_)).Times(1);
    presenter.notifyRunsTableChanged(RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", std::move(rows))})));
  }

  void populatePlottingWorkspaceTreeWithBinnedWorkspace(PlottingPresenter &presenter, MockPlottingView &view,
                                                        std::string const &workspaceName) {
    addWorkspaceWithRunNumber(workspaceName, "12345");
    auto row = successfulRow("12345");
    row.setOutputNames({"", "", workspaceName});

    EXPECT_CALL(view, setPlottingWorkspaceTreeItemStates(testing::_)).Times(1);
    presenter.notifyRunsTableChanged(RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {row})})));
  }

  PlottingWorkspaceTreeItem groupItem(std::string label, std::vector<PlottingWorkspaceTreeItem> children) {
    return {std::move(label), PlottingWorkspaceTreeItemType::ReductionGroup, ReducedWorkspaceOutputType::None, "",
            std::move(children)};
  }

  PlottingWorkspaceTreeItem runItem(std::string label, std::vector<PlottingWorkspaceTreeItem> children) {
    return {std::move(label), PlottingWorkspaceTreeItemType::Run, ReducedWorkspaceOutputType::None, "",
            std::move(children)};
  }

  PlottingWorkspaceTreeItem workspaceGroupItem(std::string label, std::vector<PlottingWorkspaceTreeItem> children) {
    auto const workspaceName = label;
    return {std::move(label), PlottingWorkspaceTreeItemType::WorkspaceGroup, ReducedWorkspaceOutputType::None,
            workspaceName, std::move(children)};
  }

  PlottingWorkspaceTreeItem workspaceItem(std::string label, ReducedWorkspaceOutputType reducedOutputType) {
    auto const workspaceName = label;
    return {std::move(label), PlottingWorkspaceTreeItemType::Workspace, reducedOutputType, workspaceName, {}};
  }

  Row successfulRow(std::string const &run) {
    auto row = Row({run}, 0.5, TransmissionRunPair(), RangeInQ(), std::nullopt, ReductionOptionsMap(),
                   ReductionWorkspaces({run}, TransmissionRunPair()));
    row.setOutputNames({"IvsLam_" + run, "IvsQ_" + run, "IvsQ_binned_" + run});
    row.setSuccess();
    return row;
  }

  Group successfulGroup(std::string const &groupName, std::vector<std::optional<Row>> rows,
                        std::string const &outputWorkspace = "") {
    auto group = Group(groupName, std::move(rows));
    if (!outputWorkspace.empty()) {
      group.setOutputNames({outputWorkspace});
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
