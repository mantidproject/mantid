#ifndef STRETCHEXPMUONTEST_H_
#define STRETCHEXPMUONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/StretchExpMuon.h"

using namespace Mantid::CurveFitting::Functions;

class StretchExpMuonTest : public CxxTest::TestSuite {
public:
  void test_category() {

    StretchExpMuon fn;
    TS_ASSERT(fn.categories().size() == 1);
    TS_ASSERT(fn.category() == "Muon");
  }

  void test_values() {

    StretchExpMuon fn;
    fn.initialize();
    fn.setParameter("A", 1.00);
    fn.setParameter("Lambda", 2.5);
    fn.setParameter("Beta", 0.50);

    // Define 1d domain
    Mantid::API::FunctionDomain1DVector x(0, 2, 10);
    Mantid::API::FunctionValues y(x);

    TS_ASSERT_THROWS_NOTHING(fn.function(x, y));
    TS_ASSERT_DELTA(y[0], 1.0000, 1e-4);
    TS_ASSERT_DELTA(y[1], 0.4745, 1e-4);
    TS_ASSERT_DELTA(y[2], 0.3485, 1e-4);
    TS_ASSERT_DELTA(y[3], 0.2749, 1e-4);
    TS_ASSERT_DELTA(y[4], 0.2252, 1e-4);
    TS_ASSERT_DELTA(y[5], 0.1888, 1e-4);
    TS_ASSERT_DELTA(y[6], 0.1610, 1e-4);
    TS_ASSERT_DELTA(y[7], 0.1391, 1e-4);
    TS_ASSERT_DELTA(y[8], 0.1214, 1e-4);
    TS_ASSERT_DELTA(y[9], 0.1068, 1e-4);
  }
};

#endif /*STRETCHEXPTEST_H_*/
