#ifndef MANTID_CURVEFITTING_COSTFUNCPOISSONTEST_H_
#define MANTID_CURVEFITTING_COSTFUNCPOISSONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/CostFunctions/CostFuncPoisson.h"

using Mantid::CurveFitting::CostFunctions::CostFuncPoisson;

class CostFuncPoissonTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CostFuncPoissonTest *createSuite() {
    return new CostFuncPoissonTest();
  }
  static void destroySuite(CostFuncPoissonTest *suite) { delete suite; }

  void test_Something() { TS_FAIL("You forgot to write a test!"); }
};

#endif /* MANTID_CURVEFITTING_COSTFUNCPOISSONTEST_H_ */