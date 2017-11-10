#ifndef EXPDECAYTEST_H_
#define EXPDECAYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/ExpDecay.h"

using namespace Mantid::CurveFitting::Functions;

class ExpDecayTest : public CxxTest::TestSuite {
public:
  void test_catergory() {
    ExpDecay fn;

    // check it categories
    const std::vector<std::string> categories = fn.categories();
    TS_ASSERT(categories.size() == 1);
    TS_ASSERT(categories[0] == "General");
  }

  void test_values() {

    ExpDecay fn;

    fn.initialize();
    fn.setParameter("Height", 5);
    fn.setParameter("Lifetime", 3);

    // Define 1d domain of 20 points in interval [0, 20]
    Mantid::API::FunctionDomain1DVector x(0, 2, 10);
    Mantid::API::FunctionValues y(x);

    TS_ASSERT_THROWS_NOTHING(fn.function(x, y));

    TS_ASSERT_DELTA(y[0], 5, 1e-4);
    TS_ASSERT_DELTA(y[1], 4.64301, 1e-4);
    TS_ASSERT_DELTA(y[2], 4.31152, 1e-4);
    TS_ASSERT_DELTA(y[3], 4.00369, 1e-4);
    TS_ASSERT_DELTA(y[4], 3.71784, 1e-4);
    TS_ASSERT_DELTA(y[5], 3.45239, 1e-4);
    TS_ASSERT_DELTA(y[6], 3.2059, 1e-4);
    TS_ASSERT_DELTA(y[7], 2.97701, 1e-4);
    TS_ASSERT_DELTA(y[8], 2.76446, 1e-4);
    TS_ASSERT_DELTA(y[9], 2.56709, 1e-4);
  }
};

#endif /*EXPDECAYTEST_H_*/
