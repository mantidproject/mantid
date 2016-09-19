#ifndef EXPDECAYMUONTEST_H_
#define EXPDECAYMUONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/ExpDecayMuon.h"

using namespace Mantid::CurveFitting::Functions;

class ExpDecayMuonTest : public CxxTest::TestSuite {
public:
  void test_category() {

    ExpDecayMuon fn;
    fn.initialize();

    TS_ASSERT(fn.categories().size() == 1);
    TS_ASSERT(fn.category() == "Muon");
  }

  void test_values() {

    ExpDecayMuon fn;
    fn.initialize();
    fn.setParameter("A", 0.21);
    fn.setParameter("Lambda", 0.61);

    // Define 1d domain of 20 points in interval [0, 20]
    Mantid::API::FunctionDomain1DVector x(0, 2, 10);
    Mantid::API::FunctionValues y(x);

    TS_ASSERT_THROWS_NOTHING(fn.function(x, y));
    TS_ASSERT_DELTA(y[0], 0.2100, 1e-4);
    TS_ASSERT_DELTA(y[1], 0.183378, 1e-4);
    TS_ASSERT_DELTA(y[2], 0.1601, 1e-4);
    TS_ASSERT_DELTA(y[3], 0.1398, 1e-4);
    TS_ASSERT_DELTA(y[4], 0.1221, 1e-4);
    TS_ASSERT_DELTA(y[5], 0.1066, 1e-4);
    TS_ASSERT_DELTA(y[6], 0.0931, 1e-4);
    TS_ASSERT_DELTA(y[7], 0.0813, 1e-4);
    TS_ASSERT_DELTA(y[8], 0.0709, 1e-4);
    TS_ASSERT_DELTA(y[9], 0.0619, 1e-4);
  }
};

#endif /*EXPDECAYMUONTEST_H_*/
