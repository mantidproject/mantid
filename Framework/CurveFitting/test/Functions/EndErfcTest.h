#ifndef ENDERFCTEST_H_
#define ENDERFCTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/EndErfc.h"

using namespace Mantid::CurveFitting::Functions;

class EndErfcTest : public CxxTest::TestSuite {
public:
  void test_category() {

    EndErfc fn;
    fn.initialize();

    TS_ASSERT(fn.categories().size() == 1);
    TS_ASSERT(fn.category() == "Calibrate");
  }

  void test_values() {

    EndErfc fn;
    fn.initialize();
    fn.setParameter("A", 1.0);
    fn.setParameter("B", 2.0);
    fn.setParameter("C", 5.0);
    fn.setParameter("D", 1.0);

    Mantid::API::FunctionDomain1DVector x(0, 2, 10);
    Mantid::API::FunctionValues y(x);

    TS_ASSERT_THROWS_NOTHING(fn.function(x, y));
    TS_ASSERT_DELTA(y[0], 1.5716, 1e-4);
    TS_ASSERT_DELTA(y[1], 1.6150, 1e-4);
    TS_ASSERT_DELTA(y[2], 1.6599, 1e-4);
    TS_ASSERT_DELTA(y[3], 1.7060, 1e-4);
    TS_ASSERT_DELTA(y[4], 1.7533, 1e-4);
    TS_ASSERT_DELTA(y[5], 1.8014, 1e-4);
    TS_ASSERT_DELTA(y[6], 1.8504, 1e-4);
    TS_ASSERT_DELTA(y[7], 1.8999, 1e-4);
    TS_ASSERT_DELTA(y[8], 1.9498, 1e-4);
    TS_ASSERT_DELTA(y[9], 2.0000, 1e-4);
  }
};

#endif /*ENDERFCTEST_H_*/
