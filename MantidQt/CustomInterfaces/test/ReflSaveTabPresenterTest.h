#ifndef MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtCustomInterfaces/Reflectometry/ReflSaveTabPresenter.h"
#include "ReflMockObjects.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace testing;

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

    std::vector<std::string> wsNames = { "ws1", "ws2", "ws3" };
    createWS(wsNames[0]);
    createWS(wsNames[1]);
    createWS(wsNames[2]);

    // Group workspaces 1 and 2 together
    IAlgorithm_sptr groupAlg = AlgorithmManager::Instance().create("GroupWorkspaces");
    groupAlg->setProperty("InputWorkspaces", {"ws1", "ws2"});
    groupAlg->setProperty("OutputWorkspace", "groupWs");
    groupAlg->execute();
    
    EXPECT_CALL(mockView, clearWorkspaceList()).Times(Exactly(1));
    // Workspace 'groupWs' should not be included in the workspace list
    EXPECT_CALL(mockView, setWorkspaceList(wsNames)).Times(Exactly(1));
    presenter.populateWorkspaceList();
    AnalysisDataService::Instance().clear();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testRefreshWorkspaceList() {
    MockSaveTabView mockView;
    ReflSaveTabPresenter presenter(&mockView);

    createWS("ws1");

    EXPECT_CALL(mockView, clearWorkspaceList()).Times(Exactly(2));
    EXPECT_CALL(mockView, setWorkspaceList(std::vector<std::string> { "ws1" })).
      Times(Exactly(1));
    EXPECT_CALL(mockView, 
      setWorkspaceList(std::vector<std::string> { "ws1", "ws2" })).
      Times(Exactly(1));
    presenter.populateWorkspaceList();
    createWS("ws2");
    presenter.populateWorkspaceList();
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
    EXPECT_CALL(mockView, setWorkspaceList(
      std::vector<std::string> { "anotherWs", "different", "someWsName" }))
      .Times(Exactly(1));
    EXPECT_CALL(mockView, setWorkspaceList(
      std::vector<std::string> { "anotherWs", "someWsName" }))
      .Times(Exactly(1));
    presenter.populateWorkspaceList();
    presenter.filterWorkspaceNames("Ws", false);
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
    EXPECT_CALL(mockView, setWorkspaceList(
      std::vector<std::string> { "_42", "apple_113", "grape_", "pear_cut"}))
      .Times(Exactly(1));
    EXPECT_CALL(mockView, setWorkspaceList(
      std::vector<std::string> { "_42", "apple_113" }))
      .Times(Exactly(1));
    presenter.populateWorkspaceList();
    presenter.filterWorkspaceNames("[a-zA-Z]*_[0-9]+", true);
    AnalysisDataService::Instance().clear();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testPopulateParametersList() {
    MockSaveTabView mockView;
    ReflSaveTabPresenter presenter(&mockView);

    createWS("ws1");
    std::vector<std::string> logs;
    const auto &properties = AnalysisDataService::Instance().retrieveWS
      <MatrixWorkspace>("ws1")->run().getProperties();
    for (auto it = properties.begin(); it != properties.end(); it++) {
      logs.push_back((*it)->name());
    }

    EXPECT_CALL(mockView, clearWorkspaceList()).Times(Exactly(1));
    EXPECT_CALL(mockView, setWorkspaceList(std::vector<std::string> { "ws1" }))
      .Times(Exactly(1));
    EXPECT_CALL(mockView, clearParametersList()).Times(Exactly(1));
    EXPECT_CALL(mockView, setParametersList(logs)).Times(Exactly(1));
    presenter.populateWorkspaceList();
    presenter.populateParametersList("ws1");
    AnalysisDataService::Instance().clear();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

private:
  void createWS(std::string name) {
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("CreateWorkspace");
    alg->setProperty("DataX", std::vector<double> { 1, 2, 3 });
    alg->setProperty("DataY", std::vector<double> { 1, 2 });
    alg->setProperty("OutputWorkspace", name);
    alg->execute();
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTERTEST_H */
