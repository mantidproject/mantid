#ifndef MANTID_CUSTOMINTERFACES_REFLDATAPROCESSORPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLDATAPROCESSORPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflGenericDataProcessorPresenterFactory.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorMockObjects.h"
#include "MantidQtMantidWidgets/DataProcessorUI/ProgressableViewMockObject.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;
using namespace testing;

class ReflDataProcessorPresenterTest : public CxxTest::TestSuite {

private:
  ITableWorkspace_sptr
  createWorkspace(const std::string &wsName,
                  const DataProcessorWhiteList &whitelist) {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();

    const int ncols = static_cast<int>(whitelist.size());

    auto colGroup = ws->addColumn("str", "Group");
    colGroup->setPlotType(0);

    for (int col = 0; col < ncols; col++) {
      auto column = ws->addColumn("str", whitelist.colNameFromColIndex(col));
      column->setPlotType(0);
    }

    if (wsName.length() > 0)
      AnalysisDataService::Instance().addOrReplace(wsName, ws);

    return ws;
  }

  ITableWorkspace_sptr
  createPrefilledWorkspace(const std::string &wsName,
                           const DataProcessorWhiteList &whitelist) {
    auto ws = createWorkspace(wsName, whitelist);
    const std::vector<std::string> group{"0", "0", "1", "1"};
    const std::vector<std::string> run{"13460", "13462", "13469", "13470"};
    const std::vector<std::string> angle{"0.7", "2.3", "0.7", "2.3"};
    const std::string transRun = "13463,13464";
    const std::vector<std::string> qMin{"0.01", "0.035", "0.01", "0.01"};
    const std::vector<std::string> qMax{"0.06", "0.3", "0.06", "0.06"};
    const std::string dqq = "0.04";
    const std::string scale = "1";
    const std::string options = "";
    for (int i = 0; i < 4; ++i) {
      TableRow row = ws->appendRow();
      row << group[i] << run[i] << angle[i] << transRun << qMin[i] << qMax[i]
          << dqq << scale << options;
    }
    return ws;
  }

  ITableWorkspace_sptr
  createPrefilledMixedWorkspace(const std::string &wsName,
                                const DataProcessorWhiteList &whitelist) {
    auto ws = createWorkspace(wsName, whitelist);
    const std::string group = "0";
    const std::vector<std::string> run{"38415", "38417"};
    const std::string angle = "0.5069";
    const std::string transRun = "38393";
    const std::string qMin = "0.0065";
    const std::string qMax = "0.0737";
    const std::vector<std::string> dqq{"0.0148", "0.0198"};
    const std::string scale = "1";
    const std::string options = "";
    for (int i = 0; i < 2; ++i) {
      TableRow row = ws->appendRow();
      row << group << run[i] << angle << transRun << qMin << qMax << dqq[i]
          << scale << options;
    }
    return ws;
  }

  ITableWorkspace_sptr
  createPrefilledMinimalWorkspace(const std::string &wsName,
                                  const DataProcessorWhiteList &whitelist) {

    auto ws = createWorkspace(wsName, whitelist);
    const std::string group = "0";
    const std::string run = "38415";
    const std::string angle = "0.5069";
    const std::string transRun = "";
    const std::string qMin = "0.0065";
    const std::string qMax = "0.0737";
    const std::string dqq = "0.0148";
    const std::string scale = "1";
    const std::string options = "";
    TableRow row = ws->appendRow();
    row << group << run << angle << transRun << qMin << qMax << dqq << scale
        << options;
    return ws;
  }

  void createSampleEventWS(const std::string &wsName) {
    auto tinyWS = WorkspaceCreationHelper::createEventWorkspace2();
    AnalysisDataService::Instance().addOrReplace(wsName, tinyWS);
  }

  ReflGenericDataProcessorPresenterFactory presenterFactory;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflDataProcessorPresenterTest *createSuite() {
    return new ReflDataProcessorPresenterTest();
  }
  static void destroySuite(ReflDataProcessorPresenterTest *suite) {
    delete suite;
  }

  ReflDataProcessorPresenterTest() { FrameworkManager::Instance(); }

  void testProcessEventWorkspacesCustomSlicing() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    auto presenter = presenterFactory.create();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    TS_ASSERT_THROWS_NOTHING(
        presenter->notify(DataProcessorPresenter::OpenTableFlag));

    std::set<int> groupList;
    groupList.insert(0);

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    // The user hits the "process" button with the first group selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(groupList));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingOptions())
        .Times(1)
        .WillOnce(Return("0,10,20,30"));
    EXPECT_CALL(mockMainPresenter, getPreprocessingOptions())
        .Times(6)
        .WillRepeatedly(Return(std::map<std::string, std::string>()));
    EXPECT_CALL(mockMainPresenter, getProcessingOptions())
        .Times(6)
        .WillRepeatedly(Return(""));
    EXPECT_CALL(mockMainPresenter, getPostprocessingOptions())
        .Times(3)
        .WillRepeatedly(Return(""));
    EXPECT_CALL(mockDataProcessorView, getEnableNotebook())
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(mockDataProcessorView, getProcessInstrument())
        .Times(14)
        .WillRepeatedly(Return("INTER"));
    EXPECT_CALL(mockDataProcessorView, requestNotebookPath()).Times(0);

    TS_ASSERT_THROWS_NOTHING(
        presenter->notify(DataProcessorPresenter::ProcessFlag));

    // Check output workspaces were created as expected
    for (size_t i = 0; i < 30; i += 10) {
      TS_ASSERT(AnalysisDataService::Instance().doesExist(
          "IvsLam_13460_" + std::to_string(i) + "_" + std::to_string(i + 10)));
      TS_ASSERT(AnalysisDataService::Instance().doesExist(
          "IvsLam_13462_" + std::to_string(i) + "_" + std::to_string(i + 10)));
      TS_ASSERT(AnalysisDataService::Instance().doesExist(
          "IvsQ_13460_" + std::to_string(i) + "_" + std::to_string(i + 10)));
      TS_ASSERT(AnalysisDataService::Instance().doesExist(
          "IvsQ_13462_" + std::to_string(i) + "_" + std::to_string(i + 10)));
      TS_ASSERT(AnalysisDataService::Instance().doesExist(
          "IvsQ_13460_" + std::to_string(i) + "_" + std::to_string(i + 10) +
          "_13462_" + std::to_string(i) + "_" + std::to_string(i + 10)));
      TS_ASSERT(AnalysisDataService::Instance().doesExist(
          "IvsQ_binned_13460_" + std::to_string(i) + "_" +
          std::to_string(i + 10)));
      TS_ASSERT(AnalysisDataService::Instance().doesExist(
          "IvsQ_binned_13462_" + std::to_string(i) + "_" +
          std::to_string(i + 10)));
      TS_ASSERT(AnalysisDataService::Instance().doesExist(
          "TOF_13460_" + std::to_string(i) + "_" + std::to_string(i + 10)));
      TS_ASSERT(AnalysisDataService::Instance().doesExist(
          "TOF_13462_" + std::to_string(i) + "_" + std::to_string(i + 10)));
    }
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_13460"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_13462"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_13460_monitors"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_13462_monitors"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TRANS_13463"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TRANS_13464"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TRANS_13463_13464"));

    // Tidy up
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testProcessWithNotebookWarn() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    auto presenter = presenterFactory.create();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);

    createPrefilledMinimalWorkspace("TestWorkspace", presenter->getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    TS_ASSERT_THROWS_NOTHING(
        presenter->notify(DataProcessorPresenter::OpenTableFlag));

    std::set<int> groupList;
    groupList.insert(0);

    // We should be warned
    EXPECT_CALL(mockMainPresenter, giveUserWarning(_, _)).Times(1);

    // The user hits the "process" button with the first group selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(groupList));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingOptions())
        .Times(1)
        .WillOnce(Return("0,10"));
    EXPECT_CALL(mockMainPresenter, getPreprocessingOptions())
        .Times(1)
        .WillRepeatedly(Return(std::map<std::string, std::string>()));
    EXPECT_CALL(mockMainPresenter, getProcessingOptions())
        .Times(1)
        .WillRepeatedly(Return(""));
    EXPECT_CALL(mockDataProcessorView, getProcessInstrument())
        .Times(2)
        .WillRepeatedly(Return("INTER"));
    EXPECT_CALL(mockDataProcessorView, getEnableNotebook())
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(mockDataProcessorView, requestNotebookPath()).Times(0);

    TS_ASSERT_THROWS_NOTHING(
        presenter->notify(DataProcessorPresenter::ProcessFlag));

    // Tidy up
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testProcessMixedWorkspacesWarn() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    auto presenter = presenterFactory.create();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);

    createPrefilledMixedWorkspace("TestWorkspace", presenter->getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    TS_ASSERT_THROWS_NOTHING(
        presenter->notify(DataProcessorPresenter::OpenTableFlag));

    std::set<int> groupList;
    groupList.insert(0);

    // We should be warned
    EXPECT_CALL(mockMainPresenter, giveUserWarning(_, _)).Times(1);

    // The user hits the "process" button with the first group selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(groupList));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingOptions())
        .Times(1)
        .WillOnce(Return("0,10,20,30"));
    EXPECT_CALL(mockMainPresenter, getPreprocessingOptions())
        .Times(2)
        .WillRepeatedly(Return(std::map<std::string, std::string>()));
    EXPECT_CALL(mockMainPresenter, getProcessingOptions())
        .Times(2)
        .WillRepeatedly(Return(""));
    EXPECT_CALL(mockMainPresenter, getPostprocessingOptions())
        .Times(1)
        .WillRepeatedly(Return(""));
    EXPECT_CALL(mockDataProcessorView, getProcessInstrument())
        .Times(8)
        .WillRepeatedly(Return("INTER"));

    TS_ASSERT_THROWS_NOTHING(
        presenter->notify(DataProcessorPresenter::ProcessFlag));

    // Tidy up
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPlotRowPythonCode() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    auto presenter = presenterFactory.create();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    TS_ASSERT_THROWS_NOTHING(
        presenter->notify(DataProcessorPresenter::OpenTableFlag));

    createSampleEventWS("IvsQ_13460_0_10");
    createSampleEventWS("IvsQ_13460_10_20");
    createSampleEventWS("IvsQ_13460_20_30");
    createSampleEventWS("IvsQ_13462_0_10");
    createSampleEventWS("IvsQ_13462_10_20");
    createSampleEventWS("IvsQ_13462_20_30");

    std::map<int, std::set<int>> rowlist;
    rowlist[0].insert(0);
    rowlist[0].insert(1);

    // We should not be warned
    EXPECT_CALL(mockMainPresenter, giveUserWarning(_, _)).Times(0);

    // The user hits "plot rows" with the first row selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(rowlist));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(std::set<int>()));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingOptions())
        .Times(1)
        .WillOnce(Return("0,10,20,30"));

    std::string pythonCode =
        "base_graph = None\nbase_graph = plotSpectrum(\"IvsQ_13460_0_10\", 0, "
        "True, window = base_graph)\n"
        "base_graph = plotSpectrum(\"IvsQ_13460_10_20\", 0, True, "
        "window = base_graph)\n"
        "base_graph = plotSpectrum(\"IvsQ_13460_20_30\", 0, True, "
        "window = base_graph)\n"
        "base_graph = plotSpectrum(\"IvsQ_13462_0_10\", 0, True, "
        "window = base_graph)\n"
        "base_graph = plotSpectrum(\"IvsQ_13462_10_20\", 0, True, "
        "window = base_graph)\n"
        "base_graph = plotSpectrum(\"IvsQ_13462_20_30\", 0, True, "
        "window = base_graph)\nbase_graph.activeLayer().logLogAxes()\n";

    EXPECT_CALL(mockMainPresenter, runPythonAlgorithm(pythonCode)).Times(1);
    TS_ASSERT_THROWS_NOTHING(
        presenter->notify(DataProcessorPresenter::PlotRowFlag));

    // Tidy up
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPlotGroupPythonCode() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    auto presenter = presenterFactory.create();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    TS_ASSERT_THROWS_NOTHING(
        presenter->notify(DataProcessorPresenter::OpenTableFlag));

    createSampleEventWS("IvsQ_13460_0_10");
    createSampleEventWS("IvsQ_13460_10_20");
    createSampleEventWS("IvsQ_13460_20_30");
    createSampleEventWS("IvsQ_13462_0_10");
    createSampleEventWS("IvsQ_13462_10_20");
    createSampleEventWS("IvsQ_13462_20_30");

    std::set<int> groupList;
    groupList.insert(0);

    // We should not be warned
    EXPECT_CALL(mockMainPresenter, giveUserWarning(_, _)).Times(0);

    // The user hits "plot rows" with the first row selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(groupList));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingOptions())
        .Times(1)
        .WillOnce(Return("0,10,20,30"));

    std::string pythonCode =
        "base_graph = None\nbase_graph = plotSpectrum(\"IvsQ_13460_0_10\", 0, "
        "True, window = base_graph)\n"
        "base_graph = plotSpectrum(\"IvsQ_13460_10_20\", 0, True, "
        "window = base_graph)\n"
        "base_graph = plotSpectrum(\"IvsQ_13460_20_30\", 0, True, "
        "window = base_graph)\n"
        "base_graph = plotSpectrum(\"IvsQ_13462_0_10\", 0, True, "
        "window = base_graph)\n"
        "base_graph = plotSpectrum(\"IvsQ_13462_10_20\", 0, True, "
        "window = base_graph)\n"
        "base_graph = plotSpectrum(\"IvsQ_13462_20_30\", 0, True, "
        "window = base_graph)\nbase_graph.activeLayer().logLogAxes()\n";

    EXPECT_CALL(mockMainPresenter, runPythonAlgorithm(pythonCode)).Times(1);
    TS_ASSERT_THROWS_NOTHING(
        presenter->notify(DataProcessorPresenter::PlotRowFlag));

    // Tidy up
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPlotRowWarn() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    auto presenter = presenterFactory.create();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    TS_ASSERT_THROWS_NOTHING(
        presenter->notify(DataProcessorPresenter::OpenTableFlag));

    createSampleEventWS("13460");

    std::map<int, std::set<int>> rowlist;
    rowlist[0].insert(0);

    // We should be warned
    EXPECT_CALL(mockMainPresenter, giveUserWarning(_, _)).Times(1);

    // The user hits "plot rows" with the first row selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(rowlist));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(std::set<int>()));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingOptions())
        .Times(1)
        .WillOnce(Return("0,10,20,30"));
    TS_ASSERT_THROWS_NOTHING(
        presenter->notify(DataProcessorPresenter::PlotRowFlag));

    // Tidy up
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPlotGroupWarn() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    auto presenter = presenterFactory.create();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    TS_ASSERT_THROWS_NOTHING(
        presenter->notify(DataProcessorPresenter::OpenTableFlag));

    createSampleEventWS("13460");
    createSampleEventWS("13462");

    std::set<int> groupList;
    groupList.insert(0);

    // We should be warned
    EXPECT_CALL(mockMainPresenter, giveUserWarning(_, _)).Times(1);

    // The user hits "plot rows" with the first row selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(groupList));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingOptions())
        .Times(1)
        .WillOnce(Return("0,10,20,30"));
    TS_ASSERT_THROWS_NOTHING(
        presenter->notify(DataProcessorPresenter::PlotGroupFlag));

    // Tidy up
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLDATAPROCESSORPRESENTERTEST_H */