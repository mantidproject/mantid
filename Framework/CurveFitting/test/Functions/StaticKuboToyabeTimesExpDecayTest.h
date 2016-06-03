#ifndef MANTID_CURVEFITTING_STATICKUBOTOYABETIMESEXPDECAYTEST_H_
#define MANTID_CURVEFITTING_STATICKUBOTOYABETIMESEXPDECAYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/StaticKuboToyabeTimesExpDecay.h"

using Mantid::CurveFitting::Functions::StaticKuboToyabeTimesExpDecay;

class StaticKuboToyabeTimesExpDecayTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StaticKuboToyabeTimesExpDecayTest *createSuite() {
    return new StaticKuboToyabeTimesExpDecayTest();
  }
  static void destroySuite(StaticKuboToyabeTimesExpDecayTest *suite) {
    delete suite;
  }

  StaticKuboToyabeTimesExpDecayTest() : fn() {}

  void test_Initialize() { TS_ASSERT_THROWS_NOTHING(fn.initialize()); }

  void test_Name() {
    TS_ASSERT_EQUALS(fn.name(), "StaticKuboToyabeTimesExpDecay");
  }

  void test_Params() {
    TS_ASSERT_DELTA(fn.getParameter("A"), 0.2, 0.0001);
    TS_ASSERT_DELTA(fn.getParameter("Delta"), 0.2, 0.0001);
    TS_ASSERT_DELTA(fn.getParameter("Lambda"), 0.2, 0.0001);
  }

  void test_Category() {
    const std::vector<std::string> categories = fn.categories();
    TS_ASSERT(categories.size() == 1);
    TS_ASSERT(categories[0] == "Muon");
  }

  void test_values() {

    StaticKuboToyabeTimesExpDecay fn;
    fn.initialize();
    fn.setParameter("A", 0.45);
    fn.setParameter("Delta", 1.05);
    fn.setParameter("Lambda", 0.23);

    // Define 1d domain
    Mantid::API::FunctionDomain1DVector x(0, 2, 10);
    Mantid::API::FunctionValues y(x);

    TS_ASSERT_THROWS_NOTHING(fn.function(x, y));
    TS_ASSERT_DELTA(y[0], 0.4500, 1e-4);
    TS_ASSERT_DELTA(y[1], 0.4048, 1e-4);
    TS_ASSERT_DELTA(y[2], 0.3254, 1e-4);
    TS_ASSERT_DELTA(y[3], 0.2314, 1e-4);
    TS_ASSERT_DELTA(y[4], 0.1426, 1e-4);
    TS_ASSERT_DELTA(y[5], 0.0736, 1e-4);
    TS_ASSERT_DELTA(y[6], 0.0308, 1e-4);
    TS_ASSERT_DELTA(y[7], 0.0127, 1e-4);
    TS_ASSERT_DELTA(y[8], 0.0129, 1e-4);
    TS_ASSERT_DELTA(y[9], 0.0234, 1e-4);
  }

  StaticKuboToyabeTimesExpDecay fn;
};

#endif /* MANTID_CURVEFITTING_STATICKUBOTOYABETIMESEXPDECAYTEST_H_ */