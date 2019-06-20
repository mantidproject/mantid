// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_REFLDATAPROCESSORPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLDATAPROCESSORPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../ISISReflectometry/ReflGenericDataProcessorPresenterFactory.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidQtWidgets/Common/DataProcessorUI/MockObjects.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ProgressableViewMockObject.h"
#include "MantidTestHelpers/DataProcessorTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace DataProcessorTestHelper;
using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;
using namespace testing;

std::ostream &operator<<(std::ostream &os, QString const &str) {
  os << str.toStdString();
  return os;
}

class ReflDataProcessorPresenterTest : public CxxTest::TestSuite {

private:
  ITableWorkspace_sptr createWorkspace(const QString &wsName,
                                       const WhiteList &whitelist) {
    auto ws = WorkspaceFactory::Instance().createTable();

    auto colGroup = ws->addColumn("str", "Group");
    colGroup->setPlotType(0);

    for (auto const &column : whitelist) {
      auto newWorkspaceColumn =
          ws->addColumn("str", column.name().toStdString());
      newWorkspaceColumn->setPlotType(0);
    }

    if (wsName.length() > 0)
      AnalysisDataService::Instance().addOrReplace(wsName.toStdString(), ws);

    return ws;
  }

  ITableWorkspace_sptr createPrefilledWorkspace(const QString &wsName,
                                                const WhiteList &whitelist) {
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
  createPrefilledMixedWorkspace(const QString &wsName,
                                const WhiteList &whitelist) {
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
  createPrefilledMinimalWorkspace(const QString &wsName,
                                  const WhiteList &whitelist) {

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

  void createSampleEventWS(const QString &wsName) {
    auto tinyWS = WorkspaceCreationHelper::createEventWorkspace2();
    AnalysisDataService::Instance().addOrReplace(wsName.toStdString(), tinyWS);
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

  static bool workspaceExists(std::string const &name) {
    return AnalysisDataService::Instance().doesExist(name);
  }

  void setUp() override {
    DefaultValue<QString>::Set(QString());
    DefaultValue<ColumnOptionsQMap>::Set(ColumnOptionsQMap());
  }

  void tearDown() override {
    DefaultValue<QString>::Clear();
    DefaultValue<ColumnOptionsQMap>::Clear();
  }

  static auto constexpr DEFAULT_GROUP_NUMBER = 1;

  void assertSliceExists(const std::string &run, const size_t i,
                         const std::vector<std::string> &slices) {
    const auto runName = run + "_slice_" + slices[i] + "_to_" + slices[i + 1];
    TS_ASSERT(!workspaceExists("IvsLam_" + runName));
    TS_ASSERT(workspaceExists("IvsQ_" + runName));
    TS_ASSERT(workspaceExists("IvsQ_binned_" + runName));
    TS_ASSERT(workspaceExists("TOF_" + runName));
  }

  void testProcessEventWorkspacesUniformEvenSlicing() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;

    EXPECT_CALL(mockMainPresenter,
                getPreprocessingOptions(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    EXPECT_CALL(mockMainPresenter, getProcessingOptions(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    EXPECT_CALL(mockMainPresenter,
                getPostprocessingOptionsAsString(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return(""));

    auto presenter = presenterFactory.create(DEFAULT_GROUP_NUMBER);
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
        .Times(AtLeast(1))
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(groupList));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingValues(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return("3"));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingType(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return("UniformEven"));
    EXPECT_CALL(mockDataProcessorView, getEnableNotebook())
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(mockDataProcessorView, getProcessInstrument())
        .Times(6)
        .WillRepeatedly(Return("INTER"));
    EXPECT_CALL(mockDataProcessorView, requestNotebookPath()).Times(0);

    TS_ASSERT_THROWS_NOTHING(
        presenter->notify(DataProcessorPresenter::ProcessFlag));

    // Check output workspaces were created as expected
    std::vector<std::string> slices13460 = {"0", "461.333", "922.667", "1384"};
    std::vector<std::string> slices13462 = {"0", "770.333", "1540.67", "2311"};
    for (size_t i = 0; i < 3; i++) {
      assertSliceExists("13460", i, slices13460);
      assertSliceExists("13462", i, slices13462);
    }
    TS_ASSERT(workspaceExists("TOF_13460"));
    TS_ASSERT(workspaceExists("TOF_13462"));
    TS_ASSERT(workspaceExists("TOF_13460_monitors"));
    TS_ASSERT(workspaceExists("TOF_13462_monitors"));
    TS_ASSERT(workspaceExists("TRANS_13463"));
    TS_ASSERT(workspaceExists("TRANS_13464"));
    TS_ASSERT(workspaceExists("TRANS_13463_13464"));

    // Tidy up
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testProcessEventWorkspacesUniformSlicing() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    EXPECT_CALL(mockMainPresenter,
                getPreprocessingOptions(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    EXPECT_CALL(mockMainPresenter, getProcessingOptions(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    EXPECT_CALL(mockMainPresenter,
                getPostprocessingOptionsAsString(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return(""));

    auto presenter = presenterFactory.create(DEFAULT_GROUP_NUMBER);
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
        .Times(AtLeast(1))
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(groupList));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingValues(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return("500"));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingType(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return("Uniform"));
    EXPECT_CALL(mockDataProcessorView, getEnableNotebook())
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(mockDataProcessorView, getProcessInstrument())
        .Times(6)
        .WillRepeatedly(Return("INTER"));
    EXPECT_CALL(mockDataProcessorView, requestNotebookPath()).Times(0);

    TS_ASSERT_THROWS_NOTHING(
        presenter->notify(DataProcessorPresenter::ProcessFlag));

    // Check output workspaces were created as expected
    std::vector<std::string> slices13460 = {"0", "500", "1000", "1500"};
    std::vector<std::string> slices13462 = {"0",    "500",  "1000",
                                            "1500", "2000", "2500"};
    for (size_t i = 0; i < 3; i++) {
      assertSliceExists("13460", i, slices13460);
      assertSliceExists("13462", i, slices13462);
    }
    // Uniform slicing allows for different runs to have different numbers
    // of output slices
    for (size_t i = 3; i < 4; i++) {
      assertSliceExists("13462", i, slices13462);
    }
    TS_ASSERT(workspaceExists("TOF_13460"));
    TS_ASSERT(workspaceExists("TOF_13462"));
    TS_ASSERT(workspaceExists("TOF_13460_monitors"));
    TS_ASSERT(workspaceExists("TOF_13462_monitors"));
    TS_ASSERT(workspaceExists("TRANS_13463"));
    TS_ASSERT(workspaceExists("TRANS_13464"));
    TS_ASSERT(workspaceExists("TRANS_13463_13464"));

    // Tidy up
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testProcessEventWorkspacesCustomSlicing() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    EXPECT_CALL(mockMainPresenter,
                getPreprocessingOptions(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    EXPECT_CALL(mockMainPresenter, getProcessingOptions(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    EXPECT_CALL(mockMainPresenter,
                getPostprocessingOptionsAsString(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return(""));

    auto presenter = presenterFactory.create(DEFAULT_GROUP_NUMBER);
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
        .Times(AtLeast(1))
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(groupList));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingValues(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return("0,10,20,30"));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingType(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return("Custom"));
    EXPECT_CALL(mockDataProcessorView, getEnableNotebook())
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(mockDataProcessorView, getProcessInstrument())
        .Times(6)
        .WillRepeatedly(Return("INTER"));
    EXPECT_CALL(mockDataProcessorView, requestNotebookPath()).Times(0);

    TS_ASSERT_THROWS_NOTHING(
        presenter->notify(DataProcessorPresenter::ProcessFlag));

    // Check output workspaces were created as expected
    std::vector<std::string> slices13460 = {"0", "10", "20", "30"};
    std::vector<std::string> slices13462 = {"0", "10", "20", "30"};
    for (size_t i = 0; i < 3; i++) {
      assertSliceExists("13460", i, slices13460);
      assertSliceExists("13462", i, slices13462);
    }
    TS_ASSERT(workspaceExists("TOF_13460"));
    TS_ASSERT(workspaceExists("TOF_13462"));
    TS_ASSERT(workspaceExists("TOF_13460_monitors"));
    TS_ASSERT(workspaceExists("TOF_13462_monitors"));
    TS_ASSERT(workspaceExists("TRANS_13463"));
    TS_ASSERT(workspaceExists("TRANS_13464"));
    TS_ASSERT(workspaceExists("TRANS_13463_13464"));

    // Tidy up
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testProcessEventWorkspacesLogValueSlicing() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    EXPECT_CALL(mockMainPresenter,
                getPreprocessingOptions(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    EXPECT_CALL(mockMainPresenter, getProcessingOptions(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    EXPECT_CALL(mockMainPresenter,
                getPostprocessingOptionsAsString(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return(""));
    auto presenter = presenterFactory.create(DEFAULT_GROUP_NUMBER);
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
        .Times(AtLeast(1))
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(groupList));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingValues(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return("Slicing=\"0,10,20,30\",LogFilter=proton_charge"));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingType(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return("LogValue"));
    EXPECT_CALL(mockDataProcessorView, getEnableNotebook())
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(mockDataProcessorView, getProcessInstrument())
        .Times(6)
        .WillRepeatedly(Return("INTER"));
    EXPECT_CALL(mockDataProcessorView, requestNotebookPath()).Times(0);

    TS_ASSERT_THROWS_NOTHING(
        presenter->notify(DataProcessorPresenter::ProcessFlag));

    // Check output workspaces were created as expected
    std::vector<std::string> slices13460 = {"0", "10", "20", "30"};
    std::vector<std::string> slices13462 = {"0", "10", "20", "30"};
    for (size_t i = 0; i < 3; i++) {
      assertSliceExists("13460", i, slices13460);
      assertSliceExists("13462", i, slices13462);
    }
    TS_ASSERT(workspaceExists("TOF_13460"));
    TS_ASSERT(workspaceExists("TOF_13462"));
    TS_ASSERT(workspaceExists("TOF_13460_monitors"));
    TS_ASSERT(workspaceExists("TOF_13462_monitors"));
    TS_ASSERT(workspaceExists("TRANS_13463"));
    TS_ASSERT(workspaceExists("TRANS_13464"));
    TS_ASSERT(workspaceExists("TRANS_13463_13464"));

    // Tidy up
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testProcessWithNotebookWarn() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    EXPECT_CALL(mockMainPresenter,
                getPreprocessingOptions(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    EXPECT_CALL(mockMainPresenter, getProcessingOptions(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    EXPECT_CALL(mockMainPresenter,
                getPostprocessingOptionsAsString(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return(QString()));
    EXPECT_CALL(mockDataProcessorView, getProcessInstrument())
        .Times(2)
        .WillRepeatedly(Return("INTER"));
    EXPECT_CALL(mockDataProcessorView, getEnableNotebook())
        .Times(1)
        .WillOnce(Return(true));

    auto presenter = presenterFactory.create(DEFAULT_GROUP_NUMBER);
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
    EXPECT_CALL(mockDataProcessorView, giveUserWarning(_, _)).Times(1);

    // The user hits the "process" button with the first group selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(groupList));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingValues(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return("0,10"));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingType(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return("Custom"));
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
    EXPECT_CALL(mockMainPresenter,
                getPreprocessingOptions(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    EXPECT_CALL(mockMainPresenter, getProcessingOptions(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    EXPECT_CALL(mockMainPresenter,
                getPostprocessingOptionsAsString(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return(""));

    auto presenter = presenterFactory.create(DEFAULT_GROUP_NUMBER);
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

    // We should get a single warning about the workspaces being processed as
    // non-event data and no other warnings.

    /// @todo This was broken in v.3.12.0 where we still got a single warning
    /// here so the test passed, but it was actually an error about the
    /// reduction failing rather than the expected warning. Since then better
    /// error handling has been added so we now get the original expected error
    /// again, but we also still get the reduction error. I'm disabling this
    /// for now until the bug is fixed.

    // EXPECT_CALL(mockDataProcessorView, giveUserWarning(_, _)).Times(1);

    // The user hits the "process" button with the first group selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(groupList));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingValues(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return("0,10,20,30"));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingType(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return("Custom"));

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
    auto mockTreeManager = std::make_unique<MockTreeManager>();
    auto *mockTreeManager_ptr = mockTreeManager.get();
    auto presenter = presenterFactory.create(DEFAULT_GROUP_NUMBER);
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);
    presenter->acceptTreeManager(std::move(mockTreeManager));

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    TS_ASSERT_THROWS_NOTHING(
        presenter->notify(DataProcessorPresenter::OpenTableFlag));

    // The following code sets up the desired workspaces without having to
    // process any runs to obtain them
    const size_t numSlices = 3;
    presenter->addNumGroupSlicesEntry(0, numSlices);
    presenter->addNumGroupSlicesEntry(1, numSlices);
    auto row0 = makeRowData({"13460"}, {}, numSlices);
    auto row1 = makeRowData({"13462"}, {}, numSlices);
    GroupData group = {{0, row0}, {1, row1}};
    TreeData tree = {{0, group}};

    createSampleEventWS("IvsQ_binned_13460_slice_0");
    createSampleEventWS("IvsQ_binned_13460_slice_1");
    createSampleEventWS("IvsQ_binned_13460_slice_2");
    createSampleEventWS("IvsQ_binned_13462_slice_0");
    createSampleEventWS("IvsQ_binned_13462_slice_1");
    createSampleEventWS("IvsQ_binned_13462_slice_2");

    // We should not be warned
    EXPECT_CALL(mockDataProcessorView, giveUserWarning(_, _)).Times(0);

    // The user hits "plot rows" with the first group selected
    EXPECT_CALL(*mockTreeManager_ptr, selectedData(false))
        .Times(1)
        .WillOnce(Return(tree));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingValues(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return("0,10,20,30"));

    auto const pythonCode =
        QString("base_graph = None\nbase_graph = "
                "plotSpectrum(\"IvsQ_binned_13460_slice_0\", "
                "0, True, window = base_graph)\n"
                "base_graph = plotSpectrum(\"IvsQ_binned_13460_slice_1\", 0, "
                "True, window = "
                "base_graph)\n"
                "base_graph = plotSpectrum(\"IvsQ_binned_13460_slice_2\", 0, "
                "True, window = "
                "base_graph)\n"
                "base_graph = plotSpectrum(\"IvsQ_binned_13462_slice_0\", 0, "
                "True, window = "
                "base_graph)\n"
                "base_graph = plotSpectrum(\"IvsQ_binned_13462_slice_1\", 0, "
                "True, window = "
                "base_graph)\n"
                "base_graph = plotSpectrum(\"IvsQ_binned_13462_slice_2\", 0, "
                "True, window = "
                "base_graph)\nbase_graph.activeLayer().logLogAxes()\n");

    EXPECT_CALL(mockDataProcessorView, runPythonAlgorithm(pythonCode)).Times(1);
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
    auto mockTreeManager = std::make_unique<MockTreeManager>();
    auto *mockTreeManager_ptr = mockTreeManager.get();
    auto presenter = presenterFactory.create(DEFAULT_GROUP_NUMBER);
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);
    presenter->acceptTreeManager(std::move(mockTreeManager));

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    TS_ASSERT_THROWS_NOTHING(
        presenter->notify(DataProcessorPresenter::OpenTableFlag));

    // The following code sets up the desired workspaces without having to
    // process any runs to obtain them
    const size_t numSlices = 3;
    presenter->addNumGroupSlicesEntry(0, numSlices);
    presenter->addNumGroupSlicesEntry(1, numSlices);
    auto row0 = makeRowData({"13460"}, {}, numSlices);
    auto row1 = makeRowData({"13462"}, {}, numSlices);
    GroupData group = {{0, row0}, {1, row1}};
    TreeData tree = {{0, group}};

    createSampleEventWS("IvsQ_binned_13460_slice_0");
    createSampleEventWS("IvsQ_binned_13460_slice_1");
    createSampleEventWS("IvsQ_binned_13460_slice_2");
    createSampleEventWS("IvsQ_binned_13462_slice_0");
    createSampleEventWS("IvsQ_binned_13462_slice_1");
    createSampleEventWS("IvsQ_binned_13462_slice_2");

    // We should not be warned
    EXPECT_CALL(mockDataProcessorView, giveUserWarning(_, _)).Times(0);

    // The user hits "plot rows" with the first group selected
    EXPECT_CALL(*mockTreeManager_ptr, selectedData(false))
        .Times(1)
        .WillOnce(Return(tree));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingValues(DEFAULT_GROUP_NUMBER))
        .Times(1)
        .WillOnce(Return("0,10,20,30"));

    auto const pythonCode =
        QString("base_graph = None\nbase_graph = "
                "plotSpectrum(\"IvsQ_binned_13460_slice_0\", "
                "0, True, window = base_graph)\n"
                "base_graph = plotSpectrum(\"IvsQ_binned_13460_slice_1\", 0, "
                "True, window = "
                "base_graph)\n"
                "base_graph = plotSpectrum(\"IvsQ_binned_13460_slice_2\", 0, "
                "True, window = "
                "base_graph)\n"
                "base_graph = plotSpectrum(\"IvsQ_binned_13462_slice_0\", 0, "
                "True, window = "
                "base_graph)\n"
                "base_graph = plotSpectrum(\"IvsQ_binned_13462_slice_1\", 0, "
                "True, window = "
                "base_graph)\n"
                "base_graph = plotSpectrum(\"IvsQ_binned_13462_slice_2\", 0, "
                "True, window = "
                "base_graph)\nbase_graph.activeLayer().logLogAxes()\n");

    EXPECT_CALL(mockDataProcessorView, runPythonAlgorithm(pythonCode)).Times(1);
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
    auto mockTreeManager = std::make_unique<MockTreeManager>();
    auto *mockTreeManager_ptr = mockTreeManager.get();
    auto presenter = presenterFactory.create(DEFAULT_GROUP_NUMBER);
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);
    presenter->acceptTreeManager(std::move(mockTreeManager));

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    TS_ASSERT_THROWS_NOTHING(
        presenter->notify(DataProcessorPresenter::OpenTableFlag));

    // The following code sets up the desired workspaces without having to
    // process any runs to obtain them
    const size_t numSlices = 1;
    presenter->addNumGroupSlicesEntry(0, numSlices);
    auto row0 = makeRowData({"13460"}, {}, numSlices);
    GroupData group = {{0, row0}};
    TreeData tree = {{0, group}};

    createSampleEventWS("13460");

    // We should be warned
    EXPECT_CALL(mockDataProcessorView, giveUserWarning(_, _)).Times(1);

    // The user hits "plot rows" with the first row selected
    EXPECT_CALL(*mockTreeManager_ptr, selectedData(false))
        .Times(1)
        .WillOnce(Return(tree));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingValues(DEFAULT_GROUP_NUMBER))
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
    auto presenter = presenterFactory.create(DEFAULT_GROUP_NUMBER);
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    TS_ASSERT_THROWS_NOTHING(
        presenter->notify(DataProcessorPresenter::OpenTableFlag));

    presenter->addNumGroupSlicesEntry(0, 1);
    createSampleEventWS("13460");
    createSampleEventWS("13462");

    std::set<int> groupList;
    groupList.insert(0);

    // We should be warned
    EXPECT_CALL(mockDataProcessorView, giveUserWarning(_, _)).Times(1);

    // The user hits "plot rows" with the first row selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(groupList));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingValues(DEFAULT_GROUP_NUMBER))
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
