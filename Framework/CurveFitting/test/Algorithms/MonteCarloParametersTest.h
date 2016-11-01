#ifndef MANTID_CURVEFITTING_MONTECARLOPARAMETERSTEST_H_
#define MANTID_CURVEFITTING_MONTECARLOPARAMETERSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Algorithms/MonteCarloParameters.h"
#include "MantidCurveFitting/Algorithms/CalculateCostFunction.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::CurveFitting::Algorithms::CalculateCostFunction;
using Mantid::CurveFitting::Algorithms::MonteCarloParameters;
using namespace Mantid;
using namespace Mantid::API;

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

  void test_no_constraints() {
    auto ws = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(
        [](double x, int i) { return 0.0; }, 1, 0, 1, 0.1);

    MonteCarloParameters alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setPropertyValue("Function", "name=UserFunction,Formula=a*x+b,a=1,ties=(b=0)");
    alg.setProperty("InputWorkspace", ws);
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
  }

  void test_no_lower_bound() {
    auto ws = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(
        [](double x, int i) { return 2.0 + 3.0*x; }, 1, 0, 1, 0.1);

    MonteCarloParameters alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setPropertyValue("Function", "name=UserFunction,Formula=a*x+b,constraints=(a<4, b<4)");
    alg.setProperty("InputWorkspace", ws);
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
  }

  void test_no_upper_bound() {
    auto ws = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(
        [](double x, int i) { return 2.0 + 3.0*x; }, 1, 0, 1, 0.1);

    MonteCarloParameters alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setPropertyValue("Function", "name=UserFunction,Formula=a*x+b,constraints=(a>4, b>4)");
    alg.setProperty("InputWorkspace", ws);
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
  }

  void test_all_free() {
    auto ws = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(
        [](double x, int i) { return 2.0 + 3.0*x; }, 1, 0, 1, 0.1);

    std::string funStr("name=UserFunction,Formula=a*x+b,a=0,b=0,constraints=(1<a<4, 0<b<4)");
    CalculateCostFunction calc;
    calc.initialize();
    calc.setPropertyValue("Function", funStr);
    calc.setProperty("InputWorkspace", ws);
    calc.execute();
    double value = calc.getProperty("Value");

    MonteCarloParameters alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setPropertyValue("Function", funStr);
    alg.setProperty("InputWorkspace", ws);
    alg.setProperty("NIterations", 1000);
    alg.execute();
    IFunction_sptr fun = alg.getProperty("Function");

    CalculateCostFunction calc1;
    calc1.initialize();
    calc1.setProperty("Function", fun);
    calc1.setProperty("InputWorkspace", ws);
    calc1.execute();
    double value1 = calc1.getProperty("Value");
    TS_ASSERT(value1 < value);
  }

  void test_fixed() {
    auto ws = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(
        [](double x, int i) { return 2.0 + 3.0*x; }, 1, 0, 1, 0.1);

    std::string funStr("name=UserFunction,Formula=a*x+b,a=0,ties=(b=1.9),constraints=(1<a<4)");
    CalculateCostFunction calc;
    calc.initialize();
    calc.setPropertyValue("Function", funStr);
    calc.setProperty("InputWorkspace", ws);
    calc.execute();
    double value = calc.getProperty("Value");

    MonteCarloParameters alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setPropertyValue("Function", funStr);
    alg.setProperty("InputWorkspace", ws);
    alg.execute();
    IFunction_sptr fun = alg.getProperty("Function");

    CalculateCostFunction calc1;
    calc1.initialize();
    calc1.setProperty("Function", fun);
    calc1.setProperty("InputWorkspace", ws);
    calc1.execute();
    double value1 = calc1.getProperty("Value");
    TS_ASSERT(value1 < value);
  }

  void test_tied() {
    auto ws = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(
        [](double x, int i) { return 2.0 + 3.0*x; }, 1, 0, 1, 0.1);

    std::string funStr("name=UserFunction,Formula=a*x+b,a=0,ties=(b=a-1),constraints=(1<a<4)");
    CalculateCostFunction calc;
    calc.initialize();
    calc.setPropertyValue("Function", funStr);
    calc.setProperty("InputWorkspace", ws);
    calc.execute();
    double value = calc.getProperty("Value");

    MonteCarloParameters alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setPropertyValue("Function", funStr);
    alg.setProperty("InputWorkspace", ws);
    alg.setProperty("NIterations", 1000);
    alg.execute();
    IFunction_sptr fun = alg.getProperty("Function");
    double a = fun->getParameter("a");
    double b = fun->getParameter("b");
    TS_ASSERT_EQUALS(b, a - 1.0);

    CalculateCostFunction calc1;
    calc1.initialize();
    calc1.setProperty("Function", fun);
    calc1.setProperty("InputWorkspace", ws);
    calc1.execute();
    double value1 = calc1.getProperty("Value");
    TS_ASSERT(value1 < value);
  }
};

#endif /* MANTID_CURVEFITTING_MONTECARLOPARAMETERSTEST_H_ */
