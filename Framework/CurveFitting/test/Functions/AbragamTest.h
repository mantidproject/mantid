#ifndef ABRAGAMTEST_H_
#define ABRAGAMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/Abragam.h"

using namespace Mantid::CurveFitting::Functions;

class AbragamTest : public CxxTest::TestSuite {
public:
  void test_category() {

    Abragam ab;

    // check its categories
    TS_ASSERT(ab.categories().size() == 1);
    TS_ASSERT(ab.category() == "Muon");
  }

  void test_values() {

    Abragam ab;
    ab.initialize();
    ab.setParameter("A", 0.21);
    ab.setParameter("Omega", 0.51);
    ab.setParameter("Phi", 0.01);
    ab.setParameter("Sigma", 1.01);
    ab.setParameter("Tau", 0.9);

    // Define 1d domain of 20 points in interval [0, 20]
    Mantid::API::FunctionDomain1DVector x(0, 2, 10);
    Mantid::API::FunctionValues y(x);

    TS_ASSERT_THROWS_NOTHING(ab.function(x, y));
    TS_ASSERT_DELTA(y[0], 0.2099, 1e-4);
    TS_ASSERT_DELTA(y[1], 0.2036, 1e-4);
    TS_ASSERT_DELTA(y[2], 0.1873, 1e-4);
    TS_ASSERT_DELTA(y[3], 0.1648, 1e-4);
    TS_ASSERT_DELTA(y[4], 0.1395, 1e-4);
    TS_ASSERT_DELTA(y[5], 0.1140, 1e-4);
    TS_ASSERT_DELTA(y[6], 0.0901, 1e-4);
    TS_ASSERT_DELTA(y[7], 0.0689, 1e-4);
    TS_ASSERT_DELTA(y[8], 0.0508, 1e-4);
    TS_ASSERT_DELTA(y[9], 0.0360, 1e-4);
  }
};

#endif /*ABRAGAMTEST_H_*/
