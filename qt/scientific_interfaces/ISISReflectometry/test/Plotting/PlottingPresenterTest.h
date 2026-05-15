// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Plotting/PlottingPresenter.h"
#include "../../../ISISReflectometry/Reduction/RunsTable.h"
#include "../ReflMockObjects.h"
#include "PlottingTestHelpers.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
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
  MOCK_CONST_METHOD0(selectedWorkspaceItems, std::vector<PlottingWorkspaceSelection>());
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

    auto const expected = std::vector<PlottingWorkspaceTreeItem>{
        groupItem("Group 1", {workspaceItem("Group 1", {}, "stitched_12345", PlottingWorkspaceOutputType::IvsQ)})};

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
        "Group 1",
        {workspaceGroupItem("Group 1", {}, "stitched_12345",
                            {workspaceItem("Group 1", {}, "stitched_12345_1", PlottingWorkspaceOutputType::IvsQ),
                             workspaceItem("Group 1", {}, "stitched_12345_2", PlottingWorkspaceOutputType::IvsQ)})})};

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

    EXPECT_CALL(view, selectedWorkspaceItems()).Times(1).WillOnce(Return(selectedWorkspaces));
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

    EXPECT_CALL(view, selectedWorkspaceItems()).Times(1).WillOnce(Return(selectedWorkspaces));
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

    EXPECT_CALL(view, selectedWorkspaceItems()).Times(1).WillOnce(Return(selectedWorkspaces));
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

    EXPECT_CALL(view, selectedWorkspaceItems()).Times(1).WillOnce(Return(std::vector<PlottingWorkspaceSelection>{}));
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
    auto const selectedWorkspaces = workspaceSelections(workspaces);
    auto const outputOptions = PlotOutputOptions{PlotOutputType::SpinAsymmetry};

    EXPECT_CALL(view, selectedWorkspaceItems()).Times(1).WillOnce(Return(selectedWorkspaces));
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
      selections.push_back({workspaceName, PlottingWorkspaceOutputType::IvsQ, "Group 1", {"12345"}, ""});
    }
    return selections;
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
      Mantid::API::AnalysisDataService::Instance().addOrReplace(name, WorkspaceCreationHelper::create2DWorkspace(1, 1));
    }
  }

  void addWorkspaceGroup(std::string const &groupName, std::vector<std::string> const &workspaceNames) {
    addWorkspaces(workspaceNames);
    Mantid::API::AnalysisDataService::Instance().add(groupName, std::make_shared<Mantid::API::WorkspaceGroup>());
    for (auto const &name : workspaceNames) {
      Mantid::API::AnalysisDataService::Instance().addToGroup(groupName, name);
    }
  }
};
