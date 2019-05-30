// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_ESTIMATEFITPARAMETERSTEST_H_
#define MANTID_CURVEFITTING_ESTIMATEFITPARAMETERSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/IFunction.h"
#include "MantidCurveFitting/Algorithms/CalculateCostFunction.h"
#include "MantidCurveFitting/Algorithms/EstimateFitParameters.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::CurveFitting::Algorithms::CalculateCostFunction;
using Mantid::CurveFitting::Algorithms::EstimateFitParameters;
using namespace Mantid;
using namespace Mantid::API;

class EstimateFitParametersTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EstimateFitParametersTest *createSuite() {
    return new EstimateFitParametersTest();
  }
  static void destroySuite(EstimateFitParametersTest *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  void test_init() {
    EstimateFitParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_no_constraints() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double, int) { return 0.0; }, 1, 0, 1, 0.1);

    EstimateFitParameters alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setPropertyValue("Function",
                         "name=UserFunction,Formula=a*x+b,a=1,ties=(b=0)");
    alg.setProperty("InputWorkspace", ws);
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }

  void test_no_lower_bound() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double x, int) { return 2.0 + 3.0 * x; }, 1, 0, 1, 0.1);

    EstimateFitParameters alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setPropertyValue(
        "Function", "name=UserFunction,Formula=a*x+b,constraints=(a<4, b<4)");
    alg.setProperty("InputWorkspace", ws);
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }

  void test_no_upper_bound() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double x, int) { return 2.0 + 3.0 * x; }, 1, 0, 1, 0.1);

    EstimateFitParameters alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setPropertyValue(
        "Function", "name=UserFunction,Formula=a*x+b,constraints=(a>4, b>4)");
    alg.setProperty("InputWorkspace", ws);
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }

  void test_all_free() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double x, int) { return 2.0 + 3.0 * x; }, 1, 0, 1, 0.1);

    std::string funStr(
        "name=UserFunction,Formula=a*x+b,a=0,b=0,constraints=(1<a<4, 0<b<4)");
    CalculateCostFunction calc;
    calc.initialize();
    calc.setPropertyValue("Function", funStr);
    calc.setProperty("InputWorkspace", ws);
    calc.execute();
    double value = calc.getProperty("Value");

    EstimateFitParameters alg;
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
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double x, int) { return 2.0 + 3.0 * x; }, 1, 0, 1, 0.1);

    std::string funStr(
        "name=UserFunction,Formula=a*x+b,a=0,ties=(b=1.9),constraints=(1<a<4)");
    CalculateCostFunction calc;
    calc.initialize();
    calc.setPropertyValue("Function", funStr);
    calc.setProperty("InputWorkspace", ws);
    calc.execute();
    double value = calc.getProperty("Value");

    EstimateFitParameters alg;
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
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double x, int) { return 2.0 + 3.0 * x; }, 1, 0, 1, 0.1);

    std::string funStr(
        "name=UserFunction,Formula=a*x+b,a=0,ties=(b=a-1),constraints=(1<a<4)");
    CalculateCostFunction calc;
    calc.initialize();
    calc.setPropertyValue("Function", funStr);
    calc.setProperty("InputWorkspace", ws);
    calc.execute();
    double value = calc.getProperty("Value");

    EstimateFitParameters alg;
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

  void test_fix_bad_parameters() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double x, int) { return exp(-x * x / 4.0); }, 1, -8.5, 8.5, 1.0);

    std::string funStr("name=BackToBackExponential,S=1.1,constraints=(0.01<I<"
                       "200,0.001<A<300,0.001<B<300,-5<X0<5,0.001<S<4)");

    EstimateFitParameters alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setPropertyValue("Function", funStr);
    alg.setProperty("InputWorkspace", ws);
    alg.setProperty("NSamples", 100);
    alg.setProperty("Selection", 10);
    alg.setProperty("NIterations", 10);
    alg.setProperty("Type", "Cross Entropy");
    alg.setProperty("FixBadParameters", true);
    alg.setProperty("Seed", 11);
    alg.execute();
    IFunction_sptr fun = alg.getProperty("Function");
    double A = fun->getParameter("A");
    double B = fun->getParameter("B");
    double I = fun->getParameter("I");
    double S = fun->getParameter("S");
    TS_ASSERT_DELTA(A, 131.2747, 1e-4);
    TS_ASSERT_DELTA(B, 145.7469, 1e-4);
    TS_ASSERT_DELTA(I, 3.7114, 1e-4);
    TS_ASSERT_DELTA(S, 1.5160, 1e-4);
    TS_ASSERT(fun->isFixed(fun->parameterIndex("A")));
    TS_ASSERT(fun->isFixed(fun->parameterIndex("B")));
    TS_ASSERT(!fun->isFixed(fun->parameterIndex("I")));
    TS_ASSERT(!fun->isFixed(fun->parameterIndex("S")));
  }

  void test_fix_bad_parameters_doesnt_change_values() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double x, int) { return exp(-x * x / 4.0); }, 1, -8.5, 8.5, 1.0);

    std::string funStr("name=BackToBackExponential,S=1.1,constraints=(0.01<I<"
                       "200,0.001<A<300,0.001<B<300,-5<X0<5,0.001<S<4)");

    EstimateFitParameters alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setPropertyValue("Function", funStr);
    alg.setProperty("InputWorkspace", ws);
    alg.setProperty("NSamples", 100);
    alg.setProperty("Selection", 10);
    alg.setProperty("NIterations", 10);
    alg.setProperty("Type", "Cross Entropy");
    alg.setProperty("FixBadParameters", false);
    alg.setProperty("Seed", 11);
    alg.execute();
    IFunction_sptr fun = alg.getProperty("Function");
    double A = fun->getParameter("A");
    double B = fun->getParameter("B");
    double I = fun->getParameter("I");
    double S = fun->getParameter("S");
    TS_ASSERT_DELTA(A, 131.2747, 1e-4);
    TS_ASSERT_DELTA(B, 145.7469, 1e-4);
    TS_ASSERT_DELTA(I, 3.7114, 1e-4);
    TS_ASSERT_DELTA(S, 1.5160, 1e-4);
    TS_ASSERT(!fun->isFixed(fun->parameterIndex("A")));
    TS_ASSERT(!fun->isFixed(fun->parameterIndex("B")));
    TS_ASSERT(!fun->isFixed(fun->parameterIndex("I")));
    TS_ASSERT(!fun->isFixed(fun->parameterIndex("S")));
  }

  void test_output() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double x, int) { return 2.0 + 3.0 * x; }, 1, 0, 1, 0.1);

    std::string funStr(
        "name=UserFunction,Formula=a*x+b,a=0,b=0,constraints=(1<a<4, 0<b<4)");
    EstimateFitParameters alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setPropertyValue("Function", funStr);
    alg.setProperty("InputWorkspace", ws);
    alg.setProperty("OutputWorkspace", "out");
    alg.execute();
    IFunction_sptr fun = alg.getProperty("Function");
    auto params =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("out");
    TS_ASSERT(params);
    TS_ASSERT_EQUALS(params->rowCount(), 2);
    TS_ASSERT_EQUALS(params->columnCount(), 11);

    double costValue = 0.0;
    auto names = params->getColumn(0);
    for (size_t col = 1; col < params->columnCount(); ++col) {
      auto column = params->getColumn(col);
      for (size_t row = 0; row < column->size(); ++row) {
        fun->setParameter(names->cell<std::string>(row),
                          column->cell<double>(row));
      }
      CalculateCostFunction calc;
      calc.initialize();
      calc.setProperty("Function", fun);
      calc.setProperty("InputWorkspace", ws);
      calc.execute();
      double value = calc.getProperty("Value");
      TSM_ASSERT_LESS_THAN(
          "Parameter sets aren't sorted by cost function value.", costValue,
          value);
    }
    AnalysisDataService::Instance().clear();
  }
};

#endif /* MANTID_CURVEFITTING_ESTIMATEFITPARAMETERSTEST_H_ */
