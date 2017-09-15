#ifndef MANTID_ALGORITHMS_SIGNALOVERERRORTEST_H_
#define MANTID_ALGORITHMS_SIGNALOVERERRORTEST_H_

#include "MantidAlgorithms/SignalOverError.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using Mantid::DataObjects::Workspace2D_sptr;

class SignalOverErrorTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    SignalOverError alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // Name of the output workspace.
    std::string outWSName("SignalOverErrorTest_OutputWS");

    Workspace2D_sptr inWS = WorkspaceCreationHelper::create2DWorkspace(2, 10);

    SignalOverError alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(
        "InputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(inWS)));
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

    // Check the results
    TS_ASSERT_DELTA(ws->readY(0)[0], M_SQRT2, 1e-5);
    TS_ASSERT_DELTA(ws->readE(0)[0], 0.0, 1e-5);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
};

#endif /* MANTID_ALGORITHMS_SIGNALOVERERRORTEST_H_ */
