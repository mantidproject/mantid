#ifndef CURVEFITTING_PRCONJUGATEGRADIENTTEST_H_
#define CURVEFITTING_PRCONJUGATEGRADIENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/PRConjugateGradientMinimizer.h"
#include "MantidAPI/ICostFunction.h"

#include <sstream>

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::API;

class PRConjugateGradientTestCostFunction : public ICostFunction {
  double a, b;

public:
  PRConjugateGradientTestCostFunction() : a(1), b(1) {}
  virtual std::string name() const {
    return "PRConjugateGradientTestCostFunction";
  }
  virtual double getParameter(size_t i) const {
    if (i == 0)
      return a;
    return b;
  }
  virtual void setParameter(size_t i, const double &value) {
    if (i == 0) {
      a = value;
    } else {
      b = value;
    }
  }
  virtual size_t nParams() const { return 2; }
  virtual double val() const {
    double x = a - 1.1;
    double y = b - 2.2;
    return 3.1 + x * x + y * y;
  }
  virtual void deriv(std::vector<double> &der) const {
    if (der.size() != 2) {
      der.resize(2);
    }
    double x = a - 1.1;
    double y = b - 2.2;
    der[0] = 2 * x;
    der[1] = 2 * y;
  }
  virtual double valAndDeriv(std::vector<double> &der) const {
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
