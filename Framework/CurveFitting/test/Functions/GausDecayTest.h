#ifndef GAUSDECAYTEST_H_
#define GAUSDECAYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/GausDecay.h"

using namespace Mantid::CurveFitting::Functions;

class GausDecayTest : public CxxTest::TestSuite {
public:
  void test_category() {

    GausDecay fn;
    TS_ASSERT(fn.categories().size() == 1);
    TS_ASSERT(fn.category() == "Muon");
  }

  void test_values() {
    GausDecay fn;
    fn.initialize();
    fn.setParameter("A", 0.20);
    fn.setParameter("Sigma", 1.01);

    // Define 1d domain
    Mantid::API::FunctionDomain1DVector x(0, 2, 10);
    Mantid::API::FunctionValues y(x);

    TS_ASSERT_THROWS_NOTHING(fn.function(x, y));
    TS_ASSERT_DELTA(y[0], 0.2000, 1e-4);
    TS_ASSERT_DELTA(y[1], 0.1901, 1e-4);
    TS_ASSERT_DELTA(y[2], 0.1635, 1e-4);
    TS_ASSERT_DELTA(y[3], 0.1270, 1e-4);
    TS_ASSERT_DELTA(y[4], 0.0893, 1e-4);
    TS_ASSERT_DELTA(y[5], 0.0567, 1e-4);
    TS_ASSERT_DELTA(y[6], 0.0326, 1e-4);
    TS_ASSERT_DELTA(y[7], 0.0169, 1e-4);
    TS_ASSERT_DELTA(y[8], 0.0079, 1e-4);
    TS_ASSERT_DELTA(y[9], 0.0033, 1e-4);
  }
};

#endif /*GAUSDECAYTEST_H_*/
