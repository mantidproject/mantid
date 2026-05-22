// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Plotting/PlottingPresenter.h"
#include "../../../ISISReflectometry/Reduction/RunsTable.h"
#include "../../../ISISReflectometry/TestHelpers/PlottingTestHelpers.h"
#include "../ReflMockObjects.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <optional>
#include <utility>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using testing::NiceMock;
using testing::Return;

class MockPlottingView : public IPlottingView {
public:
  MOCK_METHOD1(subscribe, void(PlottingViewSubscriber *));
  MOCK_METHOD1(setOutputOptionsEnabled, void(bool));
  MOCK_METHOD1(setAvailablePlotOutputTypes, void(std::vector<PlotOutputType> const &));
  MOCK_METHOD1(setWorkspaceItems, void(std::vector<PlottingWorkspaceTreeItem> const &));
  MOCK_CONST_METHOD0(selectedWorkspaceNames, std::vector<std::string>());
  MOCK_CONST_METHOD0(selectedPlotOutputType, PlotOutputType());
  MOCK_CONST_METHOD0(selectedPlotOutputOptions, PlotOutputOptions());
};

class MockPlottingModel : public IPlottingModel {
public:
  MOCK_CONST_METHOD2(workspacesForPlotting, std::vector<std::string>(std::vector<PlottingWorkspaceSelection> const &,
                                                                     PlotOutputOptions const &));
};

class PlottingPresenterTest : public CxxTest::TestSuite {
public:
  void tearDown() override { Mantid::API::AnalysisDataService::Instance().clear(); }

  void testSubscribesToViewOnConstruction() {
    NiceMock<MockPlottingView> view;

    EXPECT_CALL(view, subscribe(testing::_)).Times(1);

    PlottingPresenter presenter(&view);
  }

  void testOutputOptionsDisabledWhenReductionResumed() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockBatchPresenter> mainPresenter;
    PlottingPresenter presenter(&view);
    presenter.acceptMainPresenter(&mainPresenter);

    EXPECT_CALL(mainPresenter, isProcessing()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(view, setOutputOptionsEnabled(false)).Times(1);

    presenter.notifyReductionResumed();
  }

  void testAcceptMainPresenterDoesNotQueryInstrumentName() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockBatchPresenter> mainPresenter;
    PlottingPresenter presenter(&view);

    EXPECT_CALL(mainPresenter, instrumentName()).Times(0);

    presenter.acceptMainPresenter(&mainPresenter);
  }

  void testOutputOptionsEnabledWhenReductionPaused() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockBatchPresenter> mainPresenter;
    PlottingPresenter presenter(&view);
    presenter.acceptMainPresenter(&mainPresenter);

    EXPECT_CALL(mainPresenter, isProcessing()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(mainPresenter, isAutoreducing()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(view, setOutputOptionsEnabled(true)).Times(1);

    presenter.notifyReductionPaused();
  }

  void testInstrumentChangedUpdatesAvailablePlotOutputTypes() {
    NiceMock<MockPlottingView> view;
    PlottingPresenter presenter(&view);
    auto const expected = std::vector<PlotOutputType>{PlotOutputType::ReflectivityCurve, PlotOutputType::DetectorMap,
                                                      PlotOutputType::SpinAsymmetry, PlotOutputType::Alignment};

    EXPECT_CALL(view, setAvailablePlotOutputTypes(expected)).Times(1);

    presenter.notifyInstrumentChanged("POLREF");
  }

  void testPlotPassesSelectedInstrumentToModel() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    auto const workspaces = std::vector<std::string>{"IvsQ_12345"};
    auto const selectedWorkspaces = workspaceSelections(workspaces);
    auto const viewOutputOptions = PlotOutputOptions{PlotOutputType::Alignment};
    auto expectedOutputOptions = viewOutputOptions;
    expectedOutputOptions.instrumentName = "POLREF";

    EXPECT_CALL(view, setAvailablePlotOutputTypes(testing::_)).Times(1);
    presenter.notifyInstrumentChanged("POLREF");
    populateSelections(presenter, view, workspaces);
    EXPECT_CALL(view, selectedWorkspaceNames()).Times(1).WillOnce(Return(workspaces));
    EXPECT_CALL(view, selectedPlotOutputOptions()).Times(1).WillOnce(Return(viewOutputOptions));
    EXPECT_CALL(plottingModel, workspacesForPlotting(selectedWorkspaces, expectedOutputOptions))
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
        "Group 1", {runItem("12345", {workspaceItem("IvsLam_12345", PlottingWorkspaceOutputType::IvsLambda),
                                      workspaceItem("IvsQ_12345", PlottingWorkspaceOutputType::IvsQ),
                                      workspaceItem("IvsQ_binned_12345", PlottingWorkspaceOutputType::IvsQBinned)})})};

    EXPECT_CALL(view, setWorkspaceItems(expected)).Times(1);

    presenter.notifyRunsTableChanged(runsTable);
  }

  void testRunsTableChangedOmitsUnsuccessfulRows() {
    NiceMock<MockPlottingView> view;
    PlottingPresenter presenter(&view);
    auto row = successfulRow("12345");
    row.resetState();
    auto runsTable = RunsTable({}, 0.0, ReductionJobs({Group("Group 1", {row})}));
    addWorkspaces({"IvsLam_12345", "IvsQ_12345", "IvsQ_binned_12345"});

    EXPECT_CALL(view, setWorkspaceItems(std::vector<PlottingWorkspaceTreeItem>{})).Times(1);

    presenter.notifyRunsTableChanged(runsTable);
  }

  void testRunsTableChangedOmitsWorkspacesMissingFromADS() {
    NiceMock<MockPlottingView> view;
    PlottingPresenter presenter(&view);
    auto runsTable = RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {successfulRow("12345")})}));
    addWorkspaces({"IvsQ_12345"});

    auto const expected = std::vector<PlottingWorkspaceTreeItem>{
        groupItem("Group 1", {runItem("12345", {workspaceItem("IvsQ_12345", PlottingWorkspaceOutputType::IvsQ)})})};

    EXPECT_CALL(view, setWorkspaceItems(expected)).Times(1);

    presenter.notifyRunsTableChanged(runsTable);
  }

  void testRunsTableChangedShowsSuccessfulGroupOutputWorkspace() {
    NiceMock<MockPlottingView> view;
    PlottingPresenter presenter(&view);
    auto group = successfulGroup("Group 1", {successfulRow("12345")}, "stitched_12345");
    auto runsTable = RunsTable({}, 0.0, ReductionJobs({group}));
    addWorkspaces({"stitched_12345"});

    auto const expected = std::vector<PlottingWorkspaceTreeItem>{groupItem(
        "Group 1", {workspaceItem("Group 1", {}, "stitched_12345", PlottingWorkspaceOutputType::IvsQBinned)})};

    EXPECT_CALL(view, setWorkspaceItems(expected)).Times(1);

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
                                              {workspaceItem("IvsQ_12345_1", PlottingWorkspaceOutputType::IvsQ),
                                               workspaceItem("IvsQ_12345_2", PlottingWorkspaceOutputType::IvsQ)})})})};

    EXPECT_CALL(view, setWorkspaceItems(expected)).Times(1);

    presenter.notifyRunsTableChanged(runsTable);
  }

  void testRunsTableChangedShowsSuccessfulGroupOutputWorkspaceGroupMembers() {
    NiceMock<MockPlottingView> view;
    PlottingPresenter presenter(&view);
    auto group = successfulGroup("Group 1", {successfulRow("12345")}, "stitched_12345");
    auto runsTable = RunsTable({}, 0.0, ReductionJobs({group}));
    addWorkspaceGroup("stitched_12345", {"stitched_12345_1", "stitched_12345_2"});

    auto const expected = std::vector<PlottingWorkspaceTreeItem>{groupItem(
        "Group 1", {workspaceGroupItem(
                       "Group 1", {}, "stitched_12345",
                       {workspaceItem("Group 1", {}, "stitched_12345_1", PlottingWorkspaceOutputType::IvsQBinned),
                        workspaceItem("Group 1", {}, "stitched_12345_2", PlottingWorkspaceOutputType::IvsQBinned)})})};

    EXPECT_CALL(view, setWorkspaceItems(expected)).Times(1);

    presenter.notifyRunsTableChanged(runsTable);
  }

  void testRunsTableChangedIncludesSelectionContextMetadata() {
    NiceMock<MockPlottingView> view;
    PlottingPresenter presenter(&view);
    auto runsTable = RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {successfulRow("12345")})}));
    addWorkspaces({"IvsQ_binned_12345"});

    auto const expected = std::vector<PlottingWorkspaceTreeItem>{
        groupItem("Group 1", {runItem("Group 1", {"12345"}, "12345",
                                      {workspaceItem("Group 1", {"12345"}, "IvsQ_binned_12345",
                                                     PlottingWorkspaceOutputType::IvsQBinned)})})};

    EXPECT_CALL(view, setWorkspaceItems(expected)).Times(1);

    presenter.notifyRunsTableChanged(runsTable);
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
    auto const expectedSelections = std::vector<PlottingWorkspaceSelection>{
        {"IvsQ_binned_12345_2", PlottingWorkspaceOutputType::IvsQBinned, "Group 1", {"12345"}, "", 2}};
    auto const outputOptions = PlotOutputOptions{PlotOutputType::Alignment};

    EXPECT_CALL(view, setWorkspaceItems(testing::_)).Times(1);
    presenter.notifyRunsTableChanged(RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {row})})));
    EXPECT_CALL(view, selectedWorkspaceNames())
        .Times(1)
        .WillOnce(Return(std::vector<std::string>{"IvsQ_binned_12345_2"}));
    EXPECT_CALL(view, selectedPlotOutputOptions()).Times(1).WillOnce(Return(outputOptions));
    EXPECT_CALL(plottingModel, workspacesForPlotting(expectedSelections, outputOptions))
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
    auto const selectedWorkspaceNames = std::vector<std::string>{"IvsQ_binned_12345_1", "IvsQ_binned_12345_2"};
    auto const expectedSelections = std::vector<PlottingWorkspaceSelection>{
        {"IvsQ_binned_12345_1", PlottingWorkspaceOutputType::IvsQBinned, "Group 1", {"12345"}, "IvsQ_binned_12345", 1},
        {"IvsQ_binned_12345_2", PlottingWorkspaceOutputType::IvsQBinned, "Group 1", {"12345"}, "IvsQ_binned_12345", 2}};
    auto const outputOptions = PlotOutputOptions{PlotOutputType::Alignment};

    EXPECT_CALL(view, setWorkspaceItems(testing::_)).Times(1);
    presenter.notifyRunsTableChanged(RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {row})})));
    EXPECT_CALL(view, selectedWorkspaceNames()).Times(1).WillOnce(Return(selectedWorkspaceNames));
    EXPECT_CALL(view, selectedPlotOutputOptions()).Times(1).WillOnce(Return(outputOptions));
    EXPECT_CALL(plottingModel, workspacesForPlotting(expectedSelections, outputOptions))
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
    auto const expectedSelections = std::vector<PlottingWorkspaceSelection>{
        {"IvsQ_binned_12345_2", PlottingWorkspaceOutputType::IvsQBinned, "Group 1", {"12345"}, "", std::nullopt}};
    auto const outputOptions = PlotOutputOptions{PlotOutputType::Alignment};

    EXPECT_CALL(view, setWorkspaceItems(testing::_)).Times(1);
    presenter.notifyRunsTableChanged(RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {row})})));
    EXPECT_CALL(view, selectedWorkspaceNames())
        .Times(1)
        .WillOnce(Return(std::vector<std::string>{"IvsQ_binned_12345_2"}));
    EXPECT_CALL(view, selectedPlotOutputOptions()).Times(1).WillOnce(Return(outputOptions));
    EXPECT_CALL(plottingModel, workspacesForPlotting(expectedSelections, outputOptions))
        .Times(1)
        .WillOnce(Return(std::vector<std::string>{}));
    EXPECT_CALL(plotter, plot(testing::_)).Times(0);

    presenter.notifyPlotIndividualClicked();
  }

  void testPlotUsesRunNumberSampleLogForSelectionMetadata() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    addWorkspaceWithRunNumber("IvsQ_POLREF12345", "12345");
    auto row = successfulRow("POLREF12345");
    row.setOutputNames({"", "IvsQ_POLREF12345", ""});
    auto const expectedSelections = std::vector<PlottingWorkspaceSelection>{
        {"IvsQ_POLREF12345", PlottingWorkspaceOutputType::IvsQ, "Group 1", {"12345"}, "", std::nullopt}};
    auto const outputOptions = PlotOutputOptions{PlotOutputType::Alignment};

    EXPECT_CALL(view, setWorkspaceItems(testing::_)).Times(1);
    presenter.notifyRunsTableChanged(RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {row})})));
    EXPECT_CALL(view, selectedWorkspaceNames()).Times(1).WillOnce(Return(std::vector<std::string>{"IvsQ_POLREF12345"}));
    EXPECT_CALL(view, selectedPlotOutputOptions()).Times(1).WillOnce(Return(outputOptions));
    EXPECT_CALL(plottingModel, workspacesForPlotting(expectedSelections, outputOptions))
        .Times(1)
        .WillOnce(Return(std::vector<std::string>{}));
    EXPECT_CALL(plotter, plot(testing::_)).Times(0);

    presenter.notifyPlotIndividualClicked();
  }

  void testPlotDoesNotUseRunsTableRunNumberAsSelectionMetadataFallback() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    Mantid::API::AnalysisDataService::Instance().addOrReplace("IvsQ_12345",
                                                              WorkspaceCreationHelper::create2DWorkspace(1, 1));
    auto row = successfulRow("12345");
    row.setOutputNames({"", "IvsQ_12345", ""});
    auto const expectedSelections = std::vector<PlottingWorkspaceSelection>{
        {"IvsQ_12345", PlottingWorkspaceOutputType::IvsQ, "Group 1", {}, "", std::nullopt}};
    auto const outputOptions = PlotOutputOptions{PlotOutputType::Alignment};

    EXPECT_CALL(view, setWorkspaceItems(testing::_)).Times(1);
    presenter.notifyRunsTableChanged(RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {row})})));
    EXPECT_CALL(view, selectedWorkspaceNames()).Times(1).WillOnce(Return(std::vector<std::string>{"IvsQ_12345"}));
    EXPECT_CALL(view, selectedPlotOutputOptions()).Times(1).WillOnce(Return(outputOptions));
    EXPECT_CALL(plottingModel, workspacesForPlotting(expectedSelections, outputOptions))
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

    EXPECT_CALL(view, setWorkspaceItems(std::vector<PlottingWorkspaceTreeItem>{})).Times(1);

    presenter.notifyRunsTableChanged(runsTable);
  }

  void testPlotIndividualPlotsSelectedWorkspaces() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    auto const workspaces = std::vector<std::string>{"IvsQ_12345", "IvsQ_22345"};
    auto const selectedWorkspaces = workspaceSelections(workspaces);
    auto const outputOptions = PlotOutputOptions{PlotOutputType::ReflectivityCurve};
    auto const options = reflectivityCurvePlotOptions(PlotOutputType::ReflectivityCurve, PlotLayout::Individual);

    populateSelections(presenter, view, workspaces);
    EXPECT_CALL(view, selectedWorkspaceNames()).Times(1).WillOnce(Return(workspaces));
    EXPECT_CALL(view, selectedPlotOutputOptions()).Times(1).WillOnce(Return(outputOptions));
    EXPECT_CALL(plottingModel, workspacesForPlotting(selectedWorkspaces, outputOptions))
        .Times(1)
        .WillOnce(Return(workspaces));
    EXPECT_CALL(plotter, plot(PlotRequest{{"IvsQ_12345"}, options})).Times(1);
    EXPECT_CALL(plotter, plot(PlotRequest{{"IvsQ_22345"}, options})).Times(1);

    presenter.notifyPlotIndividualClicked();
  }

  void testPlotOverplotPlotsSelectedWorkspaces() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    auto const workspaces = std::vector<std::string>{"IvsQ_12345", "IvsQ_22345"};
    auto const selectedWorkspaces = workspaceSelections(workspaces);
    auto const outputOptions = PlotOutputOptions{PlotOutputType::ReflectivityCurve};

    populateSelections(presenter, view, workspaces);
    EXPECT_CALL(view, selectedWorkspaceNames()).Times(1).WillOnce(Return(workspaces));
    EXPECT_CALL(view, selectedPlotOutputOptions()).Times(1).WillOnce(Return(outputOptions));
    EXPECT_CALL(plottingModel, workspacesForPlotting(selectedWorkspaces, outputOptions))
        .Times(1)
        .WillOnce(Return(workspaces));
    EXPECT_CALL(plotter, plot(PlotRequest{workspaces, reflectivityCurvePlotOptions(PlotOutputType::ReflectivityCurve,
                                                                                   PlotLayout::Overplot)}))
        .Times(1);

    presenter.notifyPlotOverplotClicked();
  }

  void testPlotTiledPlotsSelectedWorkspaces() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);
    auto const workspaces = std::vector<std::string>{"IvsQ_12345", "IvsQ_22345"};
    auto const selectedWorkspaces = workspaceSelections(workspaces);
    auto const outputOptions = PlotOutputOptions{PlotOutputType::ReflectivityCurve};

    populateSelections(presenter, view, workspaces);
    EXPECT_CALL(view, selectedWorkspaceNames()).Times(1).WillOnce(Return(workspaces));
    EXPECT_CALL(view, selectedPlotOutputOptions()).Times(1).WillOnce(Return(outputOptions));
    EXPECT_CALL(plottingModel, workspacesForPlotting(selectedWorkspaces, outputOptions))
        .Times(1)
        .WillOnce(Return(workspaces));
    EXPECT_CALL(plotter, plot(PlotRequest{workspaces, reflectivityCurvePlotOptions(PlotOutputType::ReflectivityCurve,
                                                                                   PlotLayout::Tiled)}))
        .Times(1);

    presenter.notifyPlotTiledClicked();
  }

  void testPlotDoesNothingWhenNoWorkspacesSelected() {
    NiceMock<MockPlottingView> view;
    NiceMock<MockPlotter> plotter;
    PlotOptionsProvider plotOptionsProvider;
    NiceMock<MockPlottingModel> plottingModel;
    PlottingPresenter presenter(&view, plotter, plotOptionsProvider, plottingModel);

    EXPECT_CALL(view, selectedWorkspaceNames()).Times(1).WillOnce(Return(std::vector<std::string>{}));
    EXPECT_CALL(view, selectedPlotOutputOptions()).Times(0);
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
    auto const selectedWorkspaces = std::vector<PlottingWorkspaceSelection>{
        {"IvsQ_binned_group", PlottingWorkspaceOutputType::IvsQBinned, "Group 1", {"12345"}, "", std::nullopt}};
    auto const outputOptions = PlotOutputOptions{PlotOutputType::SpinAsymmetry};

    populateSelectionsForBinnedWorkspace(presenter, view, workspaces.front());
    EXPECT_CALL(view, selectedWorkspaceNames()).Times(1).WillOnce(Return(workspaces));
    EXPECT_CALL(view, selectedPlotOutputOptions()).Times(1).WillOnce(Return(outputOptions));
    EXPECT_CALL(plottingModel, workspacesForPlotting(selectedWorkspaces, outputOptions))
        .Times(1)
        .WillOnce(Return(std::vector<std::string>{}));
    EXPECT_CALL(plotter, plot(testing::_)).Times(0);

    presenter.notifyPlotOverplotClicked();
  }

private:
  std::vector<PlottingWorkspaceSelection> workspaceSelections(std::vector<std::string> const &workspaceNames) {
    auto selections = std::vector<PlottingWorkspaceSelection>{};
    selections.reserve(workspaceNames.size());
    for (auto const &workspaceName : workspaceNames) {
      selections.push_back({workspaceName,
                            PlottingWorkspaceOutputType::IvsQ,
                            "Group 1",
                            {runNumberFromWorkspaceName(workspaceName)},
                            "",
                            std::nullopt});
    }
    return selections;
  }

  std::string runNumberFromWorkspaceName(std::string const &workspaceName) {
    auto const separator = workspaceName.find_last_of('_');
    return separator == std::string::npos ? workspaceName : workspaceName.substr(separator + 1);
  }

  void populateSelections(PlottingPresenter &presenter, MockPlottingView &view,
                          std::vector<std::string> const &workspaceNames) {
    addWorkspaces(workspaceNames);
    auto rows = std::vector<std::optional<Row>>{};
    for (auto const &workspaceName : workspaceNames) {
      rows.emplace_back(successfulRow(runNumberFromWorkspaceName(workspaceName)));
    }

    EXPECT_CALL(view, setWorkspaceItems(testing::_)).Times(1);
    presenter.notifyRunsTableChanged(RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", std::move(rows))})));
  }

  void populateSelectionsForBinnedWorkspace(PlottingPresenter &presenter, MockPlottingView &view,
                                            std::string const &workspaceName) {
    addWorkspaceWithRunNumber(workspaceName, "12345");
    auto row = successfulRow("12345");
    row.setOutputNames({"", "", workspaceName});

    EXPECT_CALL(view, setWorkspaceItems(testing::_)).Times(1);
    presenter.notifyRunsTableChanged(RunsTable({}, 0.0, ReductionJobs({successfulGroup("Group 1", {row})})));
  }

  PlottingWorkspaceTreeItem groupItem(std::string label, std::vector<PlottingWorkspaceTreeItem> children) {
    return {
        std::move(label),   PlottingWorkspaceTreeItemType::Group, PlottingWorkspaceOutputType::None, "Group 1", {}, "",
        std::move(children)};
  }

  PlottingWorkspaceTreeItem runItem(std::string label, std::vector<PlottingWorkspaceTreeItem> children) {
    return runItem("Group 1", {label}, std::move(label), std::move(children));
  }

  PlottingWorkspaceTreeItem workspaceGroupItem(std::string label, std::vector<PlottingWorkspaceTreeItem> children) {
    return workspaceGroupItem("Group 1", {"12345"}, std::move(label), std::move(children));
  }

  PlottingWorkspaceTreeItem workspaceGroupItem(std::string groupName, std::vector<std::string> runNumbers,
                                               std::string label, std::vector<PlottingWorkspaceTreeItem> children) {
    auto const workspaceName = label;
    return {std::move(label),
            PlottingWorkspaceTreeItemType::WorkspaceGroup,
            PlottingWorkspaceOutputType::None,
            std::move(groupName),
            std::move(runNumbers),
            workspaceName,
            std::move(children)};
  }

  PlottingWorkspaceTreeItem workspaceItem(std::string label, PlottingWorkspaceOutputType outputType) {
    return workspaceItem("Group 1", {"12345"}, std::move(label), outputType);
  }

  PlottingWorkspaceTreeItem runItem(std::string groupName, std::vector<std::string> runNumbers, std::string label,
                                    std::vector<PlottingWorkspaceTreeItem> children) {
    return {std::move(label),
            PlottingWorkspaceTreeItemType::Run,
            PlottingWorkspaceOutputType::None,
            std::move(groupName),
            std::move(runNumbers),
            "",
            std::move(children)};
  }

  PlottingWorkspaceTreeItem workspaceItem(std::string groupName, std::vector<std::string> runNumbers, std::string label,
                                          PlottingWorkspaceOutputType outputType) {
    auto const workspaceName = label;
    return {std::move(label),
            PlottingWorkspaceTreeItemType::Workspace,
            outputType,
            std::move(groupName),
            std::move(runNumbers),
            workspaceName,
            {}};
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
