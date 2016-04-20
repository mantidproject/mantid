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
    auto inWksp = WorkspaceCreationHelper::Create1DWorkspaceRand(30);
    auto E = inWksp->dataE(0);
    E[0] = 0.; // stress oneIfZero
    auto Y = inWksp->dataY(0);
    Y[1] = 0.; // stress sqrtOrOne

    std::string outWSname = "SetUncertainties_" + mode;

    SetUncertainties alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    alg.setProperty("InputWorkspace", inWksp);
    alg.setProperty("SetError", mode);
    alg.setProperty("OutputWorkspace", outWSname);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    auto outWS =
        API::AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>(
            outWSname);
    TS_ASSERT(bool(outWS)); // non-null pointer
    return outWS;
  }

  void test_zero() {
    auto outWS = runAlg("zero");

    const auto E = outWS->readE(0);
    for (auto item : E) {
      TS_ASSERT_EQUALS(item, 0.);
    }

    API::AnalysisDataService::Instance().remove(outWS->name());
  }

  void test_sqrt() {
    auto outWS = runAlg("sqrt");

    const auto E = outWS->readE(0);
    const auto Y = outWS->readY(0);
    for (size_t i = 0; i < E.size(); ++i) {
      TS_ASSERT_DELTA(Y[i], E[i] * E[i], .001);
    }

    API::AnalysisDataService::Instance().remove(outWS->name());
  }

  void test_oneIfZero() {
    auto outWS = runAlg("oneIfZero");

    const auto E = outWS->readE(0);
    for (auto item : E) {
      TS_ASSERT(item > 0.);
    }
    API::AnalysisDataService::Instance().remove(outWS->name());
  }

  void test_sqrtOrOne() {
    auto outWS = runAlg("sqrtOrOne");

    const auto E = outWS->readE(0);
    const auto Y = outWS->readY(0);
    for (size_t i = 0; i < E.size(); ++i) {
      if (Y[i] == 0.) {
        TS_ASSERT_EQUALS(E[i], 1.);
      } else {
        TS_ASSERT_DELTA(Y[i], E[i] * E[i], .001);
      }
    }

    API::AnalysisDataService::Instance().remove(outWS->name());
  }
};

#endif /* MANTID_ALGORITHMS_SETUNCERTAINTIESTEST_H_ */
