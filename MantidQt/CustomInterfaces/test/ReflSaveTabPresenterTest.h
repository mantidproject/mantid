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
