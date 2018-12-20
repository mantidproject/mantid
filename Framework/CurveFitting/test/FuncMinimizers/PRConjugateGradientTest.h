#ifndef CURVEFITTING_PRCONJUGATEGRADIENTTEST_H_
#define CURVEFITTING_PRCONJUGATEGRADIENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ICostFunction.h"
#include "MantidCurveFitting/FuncMinimizers/PRConjugateGradientMinimizer.h"

#include <sstream>

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::FuncMinimisers;
using namespace Mantid::API;

class PRConjugateGradientTestCostFunction : public ICostFunction {
  double a, b;

public:
  PRConjugateGradientTestCostFunction() : a(1), b(1) {}
  std::string name() const override {
    return "PRConjugateGradientTestCostFunction";
  }
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
  void deriv(std::vector<double> &der) const override {
    if (der.size() != 2) {
      der.resize(2);
    }
    double x = a - 1.1;
    double y = b - 2.2;
    der[0] = 2 * x;
    der[1] = 2 * y;
  }
  double valAndDeriv(std::vector<double> &der) const override {
    deriv(der);
    return val();
  }
};

class PRConjugateGradientTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PRConjugateGradientTest *createSuite() {
    return new PRConjugateGradientTest();
  }
  static void destroySuite(PRConjugateGradientTest *suite) { delete suite; }

  void testMinimize() {
    ICostFunction_sptr fun(new PRConjugateGradientTestCostFunction);
    PRConjugateGradientMinimizer s;
    s.initialize(fun);
    TS_ASSERT(s.minimize());
    TS_ASSERT_DELTA(fun->val(), 3.1, 1e-10);
    TS_ASSERT_DELTA(fun->getParameter(0), 1.1, 1e-10);
    TS_ASSERT_DELTA(fun->getParameter(1), 2.2, 1e-10);
    TS_ASSERT_EQUALS(s.getError(), "success");
  }
};

#endif /*CURVEFITTING_PRCONJUGATEGRADIENTTEST_H_*/
