#ifndef WEIGHTEDMEANOFWORKSPACETEST_H_
#define WEIGHTEDMEANOFWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/WeightedMeanOfWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <limits>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class WeightedMeanOfWorkspaceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WeightedMeanOfWorkspaceTest *createSuite() {
    return new WeightedMeanOfWorkspaceTest();
  }
  static void destroySuite(WeightedMeanOfWorkspaceTest *suite) { delete suite; }

  void testInit() {
    WeightedMeanOfWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void testExec() {
    // Get input workspace
    MatrixWorkspace_sptr inputWS = createWorkspace();
    // Name of the output workspace.
    std::string outWSName("WeightedMeanOfWorkspaceTest_OutputWS");

    WeightedMeanOfWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(ws->y(0)[0], 2.0);
    TS_ASSERT_EQUALS(ws->e(0)[0], 1.0);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void testBadValues() {
    // Get input workspace
    MatrixWorkspace_sptr inputWS = createWorkspace(false);

    // Put bad values into workspace
    inputWS->mutableY(1)[0] = std::numeric_limits<double>::quiet_NaN();
    inputWS->mutableE(1)[1] = std::numeric_limits<double>::quiet_NaN();
    inputWS->mutableY(1)[2] = std::numeric_limits<double>::infinity();

    // Name of the output workspace.
    std::string outWSName("WeightedMeanOfWorkspaceTest_OutputWS");

    WeightedMeanOfWorkspace alg;
    alg.initialize();
    alg.isInitialized();
    alg.setProperty("InputWorkspace", inputWS);
    alg.setPropertyValue("OutputWorkspace", outWSName);
    alg.execute();
    alg.isExecuted();

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws;
    ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(ws->y(0)[0], 2.0);
    TS_ASSERT_EQUALS(ws->e(0)[0], 1.0);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void testEventWs() {
    // Get input workspace
    EventWorkspace_sptr inputWS = createEventWorkspace();
    // Name of the output workspace.
    std::string outWSName("WeightedMeanOfWorkspaceTest_OutputWS");

    WeightedMeanOfWorkspace alg;
    alg.initialize();
    alg.isInitialized();
    alg.setProperty("InputWorkspace", inputWS);
    alg.setPropertyValue("OutputWorkspace", outWSName);
    alg.execute();
    TS_ASSERT(!alg.isExecuted());
  }

private:
  MatrixWorkspace_sptr createWorkspace(bool doMasked = true) {
    std::set<int64_t> masked;
    if (doMasked) {
      masked.insert(0);
    }
    return WorkspaceCreationHelper::create2DWorkspace123(4, 3, true, masked);
  }

  EventWorkspace_sptr createEventWorkspace() {
    return WorkspaceCreationHelper::createEventWorkspace();
  }
};

#endif /* WEIGHTEDMEANOFWORKSPACETEST_H_ */
