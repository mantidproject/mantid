#ifndef LOGNORMALTEST_H_
#define LOGNORMALTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/LogNormal.h"

using namespace Mantid::CurveFitting::Functions;

class LogNormalTest : public CxxTest::TestSuite {
public:
  void test_category() {

    LogNormal fn;
    TS_ASSERT(fn.categories().size() == 1);
    TS_ASSERT(fn.category() == "Peak");
  }

  void test_values() {

    LogNormal fn;
    fn.setParameter("Height", 10.);
    fn.setParameter("Location", 1.5);
    fn.setParameter("Scale", 0.9);

    Mantid::API::FunctionDomain1DVector x(0, 2, 10);
    Mantid::API::FunctionValues y(x);

    TS_ASSERT_THROWS_NOTHING(fn.function(x, y));
    TS_ASSERT_DELTA(y[0], 0.0000, 1e-4);
    TS_ASSERT_DELTA(y[1], 0.1713, 1e-4);
    TS_ASSERT_DELTA(y[2], 0.8327, 1e-4);
    TS_ASSERT_DELTA(y[3], 1.5949, 1e-4);
    TS_ASSERT_DELTA(y[4], 2.2362, 1e-4);
    TS_ASSERT_DELTA(y[5], 2.7090, 1e-4);
    TS_ASSERT_DELTA(y[6], 3.0273, 1e-4);
    TS_ASSERT_DELTA(y[7], 3.2206, 1e-4);
    TS_ASSERT_DELTA(y[8], 3.3183, 1e-4);
    TS_ASSERT_DELTA(y[9], 3.3453, 1e-4);
  }
};

#endif /*LOGNORMALTEST_H_*/