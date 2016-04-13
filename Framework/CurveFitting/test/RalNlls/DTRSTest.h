#ifndef CURVEFITTING_RAL_NLLS_DTRSTEST_H_
#define CURVEFITTING_RAL_NLLS_DTRSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/RalNlls/DTRS.h"
#include "MantidCurveFitting/Functions/SimpleChebfun.h"

#include <iostream>

using namespace Mantid::CurveFitting::RalNlls;
using namespace Mantid::CurveFitting::Functions;

class DTRSTest : public CxxTest::TestSuite {
public:
  void test_sign() {
    TS_ASSERT_EQUALS(sign(12.0, 1.0), 12.0);
    TS_ASSERT_EQUALS(sign(12.0, 0.0), 12.0);
    TS_ASSERT_EQUALS(sign(12.0, -1.0), -12.0);
    TS_ASSERT_EQUALS(sign(-12.0, 1.0), 12.0);
    TS_ASSERT_EQUALS(sign(-12.0, 0.0), 12.0);
    TS_ASSERT_EQUALS(sign(-12.0, -1.0), -12.0);
  }

  void test_roots_quadratic() {

  }
};

#endif // CURVEFITTING_RAL_NLLS_DTRSTEST_H_
