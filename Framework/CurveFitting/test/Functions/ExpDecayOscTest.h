#ifndef EXPDECAYOSCTEST_H_
#define EXPDECAYOSCTEST_H_

#include <cmath>
#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/ExpDecayOsc.h"

using namespace Mantid::CurveFitting::Functions;

class ExpDecayOscTest : public CxxTest::TestSuite {
public:
  void test_category() {

    ExpDecayOsc fn;

    TS_ASSERT(fn.categories().size() == 1);
    TS_ASSERT(fn.category() == "Muon");
  }

  void test_values() {

    ExpDecayOsc fn;
    fn.initialize();
    fn.setParameter("A", 0.25);
    fn.setParameter("Lambda", 0.15);
    fn.setParameter("Frequency", 0.1);
    fn.setParameter("Phi", 0.15);

    // Define 1d domain of 20 points in interval [0, 20]
    Mantid::API::FunctionDomain1DVector x(0, 4, 10);
    Mantid::API::FunctionValues y(x);

    TS_ASSERT_THROWS_NOTHING(fn.function(x, y));
    TS_ASSERT_DELTA(y[0], 0.2471, 1e-4);
    TS_ASSERT_DELTA(y[1], 0.2126, 1e-4);
    TS_ASSERT_DELTA(y[2], 0.1661, 1e-4);
    TS_ASSERT_DELTA(y[3], 0.1126, 1e-4);
    TS_ASSERT_DELTA(y[4], 0.0572, 1e-4);
    TS_ASSERT_DELTA(y[5], 0.0043, 1e-4);
    TS_ASSERT_DELTA(y[6], -0.0422, 1e-4);
    TS_ASSERT_DELTA(y[7], -0.0797, 1e-4);
    TS_ASSERT_DELTA(y[8], -0.1065, 1e-4);
    TS_ASSERT_DELTA(y[9], -0.1218, 1e-4);
  }
};

#endif /*EXPDECAYOSCTEST_H_*/
