#ifndef MANTID_CURVEFITTING_MONTECARLOPARAMETERSTEST_H_
#define MANTID_CURVEFITTING_MONTECARLOPARAMETERSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Algorithms/MonteCarloParameters.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/MersenneTwister.h"

using Mantid::CurveFitting::Algorithms::MonteCarloParameters;
using namespace Mantid;
using namespace Mantid::API;

class RandomEngine {
public:
  typedef double result_type;
  double operator()() {return 0.5;}
  double min() {return 0.0;}
  double max() {return 1.0;}
};

class MonteCarloParametersTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MonteCarloParametersTest *createSuite() {
    return new MonteCarloParametersTest();
  }
  static void destroySuite(MonteCarloParametersTest *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  void test_init() {
    MonteCarloParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

};

#endif /* MANTID_CURVEFITTING_MONTECARLOPARAMETERSTEST_H_ */
