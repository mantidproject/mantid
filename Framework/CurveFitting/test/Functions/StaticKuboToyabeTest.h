#ifndef STATICKUBOTOYABETEST_H_
#define STATICKUBOTOYABETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/StaticKuboToyabe.h"

using namespace Mantid::CurveFitting::Functions;

class StaticKuboToyabeTest : public CxxTest::TestSuite {
public:
  void test_category() {

    StaticKuboToyabe fn;
    fn.initialize();

    TS_ASSERT(fn.categories().size() == 1);
    TS_ASSERT(fn.category() == "Muon");
  }

  void test_values() {

    StaticKuboToyabe fn;
    fn.initialize();
    fn.setParameter("A", 0.45);
    fn.setParameter("Delta", 1.05);

    // Define 1d domain
    Mantid::API::FunctionDomain1DVector x(0, 2, 10);
    Mantid::API::FunctionValues y(x);

    TS_ASSERT_THROWS_NOTHING(fn.function(x, y));
    TS_ASSERT_DELTA(y[0], 0.4500, 1e-4);
    TS_ASSERT_DELTA(y[1], 0.4260, 1e-4);
    TS_ASSERT_DELTA(y[2], 0.3604, 1e-4);
    TS_ASSERT_DELTA(y[3], 0.2697, 1e-4);
    TS_ASSERT_DELTA(y[4], 0.1750, 1e-4);
    TS_ASSERT_DELTA(y[5], 0.0951, 1e-4);
    TS_ASSERT_DELTA(y[6], 0.0419, 1e-4);
    TS_ASSERT_DELTA(y[7], 0.0181, 1e-4);
    TS_ASSERT_DELTA(y[8], 0.0194, 1e-4);
    TS_ASSERT_DELTA(y[9], 0.0372, 1e-4);
  }
};

#endif /*STATICKUBOTOYABETEST_H_*/
