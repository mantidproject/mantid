#ifndef MANTID_ALGORITHMS_COMMENTTEST_H_
#define MANTID_ALGORITHMS_COMMENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/Comment.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;

class CommentTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CommentTest *createSuite() { return new CommentTest(); }
  static void destroySuite(CommentTest *suite) { delete suite; }

  void test_Init() {
    Mantid::Algorithms::Comment alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    std::string wsName = "CommentTest_Exec_workspace";
    // Create test input
    auto ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().add(wsName, ws);
    // and an identical ws for comparison later
    auto ws2 = WorkspaceCreationHelper::create2DWorkspace(10, 10);

    Mantid::Algorithms::Comment alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Workspace", wsName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "Text", "The next algorithm is doing ws equals 1/ws"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr outputWS;
    TS_ASSERT_THROWS_NOTHING(
        outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            wsName));
    TS_ASSERT(outputWS);
    if (!outputWS)
      return;

    IAlgorithm_sptr lastAlgorithm = outputWS->getHistory().lastAlgorithm();

    TS_ASSERT_EQUALS(lastAlgorithm->getPropertyValue("Workspace"),
                     alg.getPropertyValue("Workspace"));
    TS_ASSERT_EQUALS(lastAlgorithm->getPropertyValue("Text"),
                     alg.getPropertyValue("Text"));
    TSM_ASSERT("The workspace has been altered by Comment",
               Mantid::API::equals(ws, ws2));

    AnalysisDataService::Instance().remove(wsName);
  }
};

#endif /* MANTID_ALGORITHMS_COMMENTTEST_H_ */
