#ifndef GAUSOSCTEST_H_
#define GAUSOSCTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/GausOsc.h"

using namespace Mantid::CurveFitting::Functions;

class GausOscTest : public CxxTest::TestSuite {
public:
  void test_category() {
    GausOsc fn;
    fn.initialize();

    TS_ASSERT(fn.categories().size() == 1);
    TS_ASSERT(fn.category() == "Muon");
  }

  void test_values() {

    GausOsc fn;
    fn.initialize();
    fn.setParameter("A", 2.5);
    fn.setParameter("Sigma", 0.25);
    fn.setParameter("Frequency", 0.15);
    fn.setParameter("Phi", 0.01);

    // Define 1d domain
    Mantid::API::FunctionDomain1DVector x(0, 2, 10);
    Mantid::API::FunctionValues y(x);

    TS_ASSERT_THROWS_NOTHING(fn.function(x, y));
    TS_ASSERT_DELTA(y[0], 2.4998, 1e-4);
    TS_ASSERT_DELTA(y[1], 2.4325, 1e-4);
    TS_ASSERT_DELTA(y[2], 2.2456, 1e-4);
    TS_ASSERT_DELTA(y[3], 1.9527, 1e-4);
    TS_ASSERT_DELTA(y[4], 1.5744, 1e-4);
    TS_ASSERT_DELTA(y[5], 1.1370, 1e-4);
    TS_ASSERT_DELTA(y[6], 0.6699, 1e-4);
    TS_ASSERT_DELTA(y[7], 0.2032, 1e-4);
    TS_ASSERT_DELTA(y[8], -0.2348, 1e-4);
    TS_ASSERT_DELTA(y[9], -0.6201, 1e-4);
  }
};

#endif /*GAUSOSCTEST_H_*/
