#ifndef MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflSaveTabPresenter.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "ReflMockObjects.h"
#include "Poco/File.h"
#include "Poco/Path.h"

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace testing;

using Mantid::DataObjects::Workspace2D_sptr;

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflSaveTabPresenterTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflSaveTabPresenterTest *createSuite() {
    return new ReflSaveTabPresenterTest();
  }
  static void destroySuite(ReflSaveTabPresenterTest *suite) { delete suite; }

  ReflSaveTabPresenterTest() { FrameworkManager::Instance(); }

  void testPopulateWorkspaceList() {
    MockSaveTabView mockView;
    ReflSaveTabPresenter presenter(&mockView);

    std::vector<std::string> wsNames = {"ws1", "ws2", "ws3"};
    createWS(wsNames[0]);
    createWS(wsNames[1]);
    createWS(wsNames[2]);
    createTableWS("tableWS");

    // Group workspaces 1 and 2 together
    IAlgorithm_sptr groupAlg =
        AlgorithmManager::Instance().create("GroupWorkspaces");
    groupAlg->setProperty("InputWorkspaces", {"ws1", "ws2"});
    groupAlg->setProperty("OutputWorkspace", "groupWs");
    groupAlg->execute();

    EXPECT_CALL(mockView, clearWorkspaceList()).Times(Exactly(1));
    // Workspaces 'groupWs' and 'tableWS' should not be included in the
    // workspace list
    EXPECT_CALL(mockView, setWorkspaceList(wsNames)).Times(Exactly(1));
    presenter.notify(IReflSaveTabPresenter::populateWorkspaceListFlag);
    AnalysisDataService::Instance().clear();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testRefreshWorkspaceList() {
    MockSaveTabView mockView;
    ReflSaveTabPresenter presenter(&mockView);

    createWS("ws1");

    EXPECT_CALL(mockView, clearWorkspaceList()).Times(Exactly(2));
    EXPECT_CALL(mockView, setWorkspaceList(std::vector<std::string>{"ws1"}))
        .Times(Exactly(1));
    EXPECT_CALL(mockView,
                setWorkspaceList(std::vector<std::string>{"ws1", "ws2"}))
        .Times(Exactly(1));
    presenter.notify(IReflSaveTabPresenter::populateWorkspaceListFlag);
    createWS("ws2");
    presenter.notify(IReflSaveTabPresenter::populateWorkspaceListFlag);
    AnalysisDataService::Instance().clear();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testFilterWorkspaceNoRegex() {
    MockSaveTabView mockView;
    ReflSaveTabPresenter presenter(&mockView);

    createWS("anotherWs");
    createWS("different");
    createWS("someWsName");

    EXPECT_CALL(mockView, clearWorkspaceList()).Times(Exactly(2));
    EXPECT_CALL(mockView, setWorkspaceList(std::vector<std::string>{
                              "anotherWs", "different", "someWsName"}))
        .Times(Exactly(1));
    EXPECT_CALL(mockView, getFilter()).Times(Exactly(1)).WillOnce(Return("Ws"));
    EXPECT_CALL(mockView, getRegexCheck())
        .Times(Exactly(1))
        .WillOnce(Return(false));
    EXPECT_CALL(mockView, setWorkspaceList(std::vector<std::string>{
                              "anotherWs", "someWsName"})).Times(Exactly(1));
    presenter.notify(IReflSaveTabPresenter::populateWorkspaceListFlag);
    presenter.notify(IReflSaveTabPresenter::filterWorkspaceListFlag);
    AnalysisDataService::Instance().clear();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testFilterWorkspaceWithRegex() {
    MockSaveTabView mockView;
    ReflSaveTabPresenter presenter(&mockView);

    createWS("_42");
    createWS("apple_113");
    createWS("grape_");
    createWS("pear_cut");

    EXPECT_CALL(mockView, clearWorkspaceList()).Times(Exactly(2));
    EXPECT_CALL(mockView, setWorkspaceList(std::vector<std::string>{
                              "_42", "apple_113", "grape_", "pear_cut"}))
        .Times(Exactly(1));
    EXPECT_CALL(mockView, getFilter())
        .Times(Exactly(1))
        .WillOnce(Return("[a-zA-Z]*_[0-9]+"));
    EXPECT_CALL(mockView, getRegexCheck())
        .Times(Exactly(1))
        .WillOnce(Return(true));
    EXPECT_CALL(mockView,
                setWorkspaceList(std::vector<std::string>{"_42", "apple_113"}))
        .Times(Exactly(1));
    presenter.notify(IReflSaveTabPresenter::populateWorkspaceListFlag);
    presenter.notify(IReflSaveTabPresenter::filterWorkspaceListFlag);
    AnalysisDataService::Instance().clear();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testPopulateParametersList() {
    MockSaveTabView mockView;
    ReflSaveTabPresenter presenter(&mockView);

    createWS("ws1");
    std::vector<std::string> logs;
    const auto &properties = AnalysisDataService::Instance()
                                 .retrieveWS<MatrixWorkspace>("ws1")
                                 ->run()
                                 .getProperties();
    for (auto it = properties.begin(); it != properties.end(); it++) {
      logs.push_back((*it)->name());
    }

    EXPECT_CALL(mockView, clearParametersList()).Times(Exactly(1));
    EXPECT_CALL(mockView, getCurrentWorkspaceName())
        .Times(Exactly(1))
        .WillOnce(Return("ws1"));
    EXPECT_CALL(mockView, setParametersList(logs)).Times(Exactly(1));
    presenter.notify(IReflSaveTabPresenter::workspaceParamsFlag);
    AnalysisDataService::Instance().clear();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testSaveWorkspaces() {
    MockSaveTabView mockView;
    ReflSaveTabPresenter presenter(&mockView);

    std::string savePath = createSavePath();
    std::vector<std::string> wsNames = {"ws1", "ws2", "ws3"};
    createWS(wsNames[0]);
    createWS(wsNames[1]);
    createWS(wsNames[2]);

    EXPECT_CALL(mockView, getSavePath())
        .Times(Exactly(1))
        .WillOnce(Return(savePath));
    EXPECT_CALL(mockView, getTitleCheck())
        .Times(Exactly(1))
        .WillOnce(Return(false));
    EXPECT_CALL(mockView, getSelectedParameters())
        .Times(Exactly(1))
        .WillOnce(Return(std::vector<std::string>()));
    EXPECT_CALL(mockView, getQResolutionCheck())
        .Times(Exactly(1))
        .WillOnce(Return(false));
    EXPECT_CALL(mockView, getSeparator())
        .Times(Exactly(1))
        .WillOnce(Return("comma"));
    EXPECT_CALL(mockView, getPrefix()).Times(Exactly(1)).WillOnce(Return(""));
    EXPECT_CALL(mockView, getFileFormatIndex())
        .Times(Exactly(1))
        .WillOnce(Return(0));
    EXPECT_CALL(mockView, getSelectedWorkspaces())
        .Times(Exactly(1))
        .WillOnce(Return(wsNames));
    presenter.notify(IReflSaveTabPresenter::saveWorkspacesFlag);
    for (auto it = wsNames.begin(); it != wsNames.end(); it++) {
      Poco::File(savePath + *it + ".dat").remove();
    }
    AnalysisDataService::Instance().clear();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testSuggestSaveDir() {
    MockSaveTabView mockView;
    ReflSaveTabPresenter presenter(&mockView);

    std::string saveDir = Mantid::Kernel::ConfigService::Instance().getString(
        "defaultsave.directory");

    EXPECT_CALL(mockView, setSavePath(saveDir));
    presenter.notify(IReflSaveTabPresenter::suggestSaveDirFlag);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

private:
  void createWS(std::string name) {
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace(name, ws);
  }

  void createTableWS(std::string name) {
    ITableWorkspace_sptr ws =
        WorkspaceFactory::Instance().createTable("TableWorkspace");
    AnalysisDataService::Instance().addOrReplace(name, ws);
  }

  std::string createSavePath() {
    // First attempt to obtain path from default save directory
    std::string savePath = Mantid::Kernel::ConfigService::Instance().getString(
        "defaultsave.directory");
    if (savePath.empty())
      // Otherwise use current path as save directory
      savePath = Poco::Path::current();

    return savePath;
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTERTEST_H */
