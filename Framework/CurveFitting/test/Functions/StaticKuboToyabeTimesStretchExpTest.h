#ifndef MANTID_CURVEFITTING_STATICKUBOTOYABETIMESSTRETCHEXPTEST_H_
#define MANTID_CURVEFITTING_STATICKUBOTOYABETIMESSTRETCHEXPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/StaticKuboToyabeTimesStretchExp.h"

using Mantid::CurveFitting::Functions::StaticKuboToyabeTimesStretchExp;

class StaticKuboToyabeTimesStretchExpTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StaticKuboToyabeTimesStretchExpTest *createSuite() {
    return new StaticKuboToyabeTimesStretchExpTest();
  }
  static void destroySuite(StaticKuboToyabeTimesStretchExpTest *suite) {
    delete suite;
  }

  StaticKuboToyabeTimesStretchExpTest() : fn() {}

  void test_Initialize() { TS_ASSERT_THROWS_NOTHING(fn.initialize()); }

  void test_Name() {
    TS_ASSERT_EQUALS(fn.name(), "StaticKuboToyabeTimesStretchExp");
  }

  void test_Params() {
    TS_ASSERT_DELTA(fn.getParameter("A"), 0.2, 0.0001);
    TS_ASSERT_DELTA(fn.getParameter("Delta"), 0.2, 0.0001);
    TS_ASSERT_DELTA(fn.getParameter("Lambda"), 0.2, 0.0001);
    TS_ASSERT_DELTA(fn.getParameter("Beta"), 0.2, 0.0001)
  }

  void test_category() {
    TS_ASSERT_EQUALS(fn.categories().size(), 1);
    TS_ASSERT_EQUALS(fn.category(), "Muon");
  }

  void test_values() {

    fn.setParameter("A", 2.0);
    fn.setParameter("Delta", 1.0);
    fn.setParameter("Lambda", 0.9);
    fn.setParameter("Beta", 4.0);

    // Define 1d domain
    Mantid::API::FunctionDomain1DVector x(0, 2, 10);
    Mantid::API::FunctionValues y(x);

    TS_ASSERT_THROWS_NOTHING(fn.function(x, y));
    TS_ASSERT_DELTA(y[0], 2.0000, 1e-4);
    TS_ASSERT_DELTA(y[1], 1.9002, 1e-4);
    TS_ASSERT_DELTA(y[2], 1.5946, 1e-4);
    TS_ASSERT_DELTA(y[3], 1.1066, 1e-4);
    TS_ASSERT_DELTA(y[4], 0.5677, 1e-4);
    TS_ASSERT_DELTA(y[5], 0.1831, 1e-4);
    TS_ASSERT_DELTA(y[6], 0.0302, 1e-4);
    TS_ASSERT_DELTA(y[7], 0.0021, 1e-4);
    TS_ASSERT_DELTA(y[8], 0.0001, 1e-4);
    TS_ASSERT_DELTA(y[9], 0.0000, 1e-4);
  }

  StaticKuboToyabeTimesStretchExp fn;
};

#endif /* MANTID_CURVEFITTING_STATICKUBOTOYABETIMESSTRETCHEXPTEST_H_ */