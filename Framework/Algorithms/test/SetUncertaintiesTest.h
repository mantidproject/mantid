#ifndef MANTID_ALGORITHMS_SETUNCERTAINTIESTEST_H_
#define MANTID_ALGORITHMS_SETUNCERTAINTIESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SetUncertainties.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Algorithms;
using namespace Mantid::Kernel;
using namespace Mantid;

class SetUncertaintiesTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    SetUncertainties alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  /**
    * Create and execute the algorithm in the specified mode.
  * @param mode The name of the SetError property SetUncertainties
  * @return The name that the output workspace will be registered as.
  */
  API::MatrixWorkspace_sptr runAlg(const std::string &mode) {
    // random data mostly works
    auto inWksp = WorkspaceCreationHelper::create1DWorkspaceRand(30, true);
    // Ensure first elements of random workspace are zero so test don't
    // pass randomly
    auto &E = inWksp->mutableE(0);
    E[0] = 0.;
    auto &Y = inWksp->mutableY(0);
    Y[1] = 0.; // stress sqrtOrOne

    std::string outWSname = "SetUncertainties_" + mode;

    SetUncertainties alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    alg.setProperty("InputWorkspace", inWksp);
    alg.setProperty("SetError", mode);
    alg.setProperty("OutputWorkspace", outWSname);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    const auto outWS =
        API::AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>(
            outWSname);
    TS_ASSERT(bool(outWS)); // non-null pointer
    return outWS;
  }

  void test_zero() {
    const auto outWS = runAlg("zero");

    const auto E = outWS->e(0);
    for (const auto item : E) {
      TS_ASSERT_EQUALS(item, 0.);
    }

    API::AnalysisDataService::Instance().remove(outWS->getName());
  }

  void test_sqrt() {
    const auto outWS = runAlg("sqrt");

    const auto E = outWS->e(0);
    const auto Y = outWS->y(0);
    for (size_t i = 0; i < E.size(); ++i) {
      TS_ASSERT_DELTA(Y[i], E[i] * E[i], .001);
    }

    API::AnalysisDataService::Instance().remove(outWS->getName());
  }

  void test_oneIfZero() {
    const auto outWS = runAlg("oneIfZero");

    const auto E = outWS->e(0);
    for (const auto item : E) {
      TS_ASSERT(item > 0.);
    }
    API::AnalysisDataService::Instance().remove(outWS->getName());
  }

  void test_sqrtOrOne() {
    const auto outWS = runAlg("sqrtOrOne");

    const auto &E = outWS->e(0);
    const auto &Y = outWS->y(0);
    for (size_t i = 0; i < E.size(); ++i) {
      if (Y[i] == 0.) {
        TS_ASSERT_EQUALS(E[i], 1.);
      } else {
        TS_ASSERT_DELTA(Y[i], E[i] * E[i], .001);
      }
    }

    API::AnalysisDataService::Instance().remove(outWS->getName());
  }
};

class SetUncertaintiesTestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    algZero.initialize();
    algCalc.initialize();

    // This size controls the test time - and aims for
    // 0.1-0.2 seconds for the algorithm execution time
    constexpr size_t wsSize(1000000);

    // random data mostly works
    inputWs = WorkspaceCreationHelper::create1DWorkspaceRand(wsSize, true);
    algZero.setProperty("InputWorkspace", inputWs);
    algZero.setProperty("SetError", "zero");
    algZero.setProperty("OutputWorkspace", wsName);

    algCalc.setProperty("InputWorkspace", inputWs);
    algCalc.setProperty("SetError", "zero");
    algCalc.setProperty("OutputWorkspace", wsName);

    algZero.setRethrows(true);
    algCalc.setRethrows(true);
  }

  void testSetUncertaintiesPerformance() {
    // First run zeroing all errors
    TS_ASSERT_THROWS_NOTHING(algZero.execute());
    TS_ASSERT_THROWS_NOTHING(algCalc.execute());
  }

private:
  SetUncertainties algZero;
  SetUncertainties algCalc;
  boost::shared_ptr<Mantid::DataObjects::Workspace2D> inputWs;
  const std::string wsName = "outputWs";
};

#endif /* MANTID_ALGORITHMS_SETUNCERTAINTIESTEST_H_ */
