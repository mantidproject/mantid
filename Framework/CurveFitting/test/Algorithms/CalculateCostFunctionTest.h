#ifndef MANTID_CURVEFITTING_CALCULATECOSTFUNCTIONTEST_H_
#define MANTID_CURVEFITTING_CALCULATECOSTFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Algorithms/CalculateCostFunction.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <numeric>

using Mantid::CurveFitting::Algorithms::CalculateCostFunction;
using namespace Mantid;
using namespace Mantid::API;

class CalculateCostFunctionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateCostFunctionTest *createSuite() {
    return new CalculateCostFunctionTest();
  }
  static void destroySuite(CalculateCostFunctionTest *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  void test_init() {
    CalculateCostFunction alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_calculate() {
    CalculateCostFunction alg;
    alg.initialize();
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double, int) { return 0.0; }, 1, 0.0, 1.0, 0.1);
    alg.setPropertyValue("Function", "name=UserFunction,Formula=a*x,a=1");
    alg.setProperty("InputWorkspace", ws);
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    double value = alg.getProperty("Value");
    auto sum = std::accumulate(ws->x(0).begin(), ws->x(0).end(), 0.0,
                               [](double s, double a) { return s + a * a; });
    TS_ASSERT_DELTA(value, sum / 2, 1e-15);
  }

  void test_calculate_weighted() {
    CalculateCostFunction alg;
    alg.initialize();
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double, int) { return 0.0; }, 1, 0.0, 1.0, 0.1);
    double w = 0.0;
    std::generate(ws->dataE(0).begin(), ws->dataE(0).end(), [&w] {
      w += 1.0;
      return w;
    });
    alg.setPropertyValue("Function", "name=UserFunction,Formula=a*x,a=1");
    alg.setProperty("InputWorkspace", ws);
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    double value = alg.getProperty("Value");
    w = 0.0;
    auto sum = std::accumulate(ws->x(0).begin(), ws->x(0).end(), 0.0,
                               [&w](double s, double a) {
                                 w += 1.0;
                                 return s + a * a / (w * w);
                               });
    TS_ASSERT_DELTA(value, sum / 2, 1e-15);
  }

  void test_calculate_weighted_unweighted() {
    CalculateCostFunction alg;
    alg.initialize();
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double, int) { return 1.0; }, 1, 0.0, 1.0, 0.1);
    double w = 0.0;
    std::generate(ws->dataE(0).begin(), ws->dataE(0).end(), [&w] {
      w += 1.0;
      return w;
    });
    alg.setPropertyValue("Function", "name=UserFunction,Formula=a*x,a=1");
    alg.setProperty("InputWorkspace", ws);
    alg.setProperty("CostFunction", "Unweighted least squares");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    double value = alg.getProperty("Value");
    auto sum = std::accumulate(
        ws->x(0).begin(), ws->x(0).end(), 0.0,
        [](double s, double a) { return s + (a - 1.0) * (a - 1.0); });
    TS_ASSERT_DELTA(value, sum / 2, 1e-15);
  }
};

#endif /* MANTID_CURVEFITTING_CALCULATECOSTFUNCTIONTEST_H_ */
