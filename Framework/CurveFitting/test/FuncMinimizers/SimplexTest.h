#ifndef CURVEFITTING_SIMPLEXTEST_H_
#define CURVEFITTING_SIMPLEXTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ICostFunction.h"
#include "MantidCurveFitting/FuncMinimizers/SimplexMinimizer.h"

#include <sstream>

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::FuncMinimisers;
using namespace Mantid::API;

class SimplexTestCostFunction : public ICostFunction {
  double a, b;

public:
  SimplexTestCostFunction() : a(1), b(1) {}
  std::string name() const override { return "SimplexTestCostFunction"; }
  double getParameter(size_t i) const override {
    if (i == 0)
      return a;
    return b;
  }
  void setParameter(size_t i, const double &value) override {
    if (i == 0) {
      a = value;
    } else {
      b = value;
    }
  }
  size_t nParams() const override { return 2; }
  double val() const override {
    double x = a - 1.1;
    double y = b - 2.2;
    return 3.1 + x * x + y * y;
  }
  void deriv(std::vector<double> &) const override {}
  double valAndDeriv(std::vector<double> &) const override { return 0.0; }
};

class SimplexTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SimplexTest *createSuite() { return new SimplexTest(); }
  static void destroySuite(SimplexTest *suite) { delete suite; }

  void testSimplex() {
    ICostFunction_sptr fun(new SimplexTestCostFunction);
    SimplexMinimizer s;
    s.initialize(fun);
    TS_ASSERT(s.minimize());
    TS_ASSERT_DELTA(fun->val(), 3.1, 0.0001);
    TS_ASSERT_DELTA(fun->getParameter(0), 1.1, 0.01);
    TS_ASSERT_DELTA(fun->getParameter(1), 2.2, 0.01);
    TS_ASSERT_EQUALS(s.getError(), "success");
  }
};

#endif /*FUNCMINIMIZERFACTORYTEST_H_*/
