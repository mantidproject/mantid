#ifndef MANTID_CURVEFITTING_STATICKUBOTOYABETIMESGAUSDECAYTEST_H_
#define MANTID_CURVEFITTING_STATICKUBOTOYABETIMESGAUSDECAYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/StaticKuboToyabeTimesGausDecay.h"

using Mantid::CurveFitting::Functions::StaticKuboToyabeTimesGausDecay;

class StaticKuboToyabeTimesGausDecayTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StaticKuboToyabeTimesGausDecayTest *createSuite() {
    return new StaticKuboToyabeTimesGausDecayTest();
  }
  static void destroySuite(StaticKuboToyabeTimesGausDecayTest *suite) {
    delete suite;
  }

  StaticKuboToyabeTimesGausDecayTest() : fn() {}

  void test_Initialize() { TS_ASSERT_THROWS_NOTHING(fn.initialize()); }

  void test_Name() {
    TS_ASSERT_EQUALS(fn.name(), "StaticKuboToyabeTimesGausDecay");
  }

  void test_Params() {
    TS_ASSERT_DELTA(fn.getParameter("A"), 1.0, 0.0001);
    TS_ASSERT_DELTA(fn.getParameter("Delta"), 0.2, 0.0001);
    TS_ASSERT_DELTA(fn.getParameter("Sigma"), 0.2, 0.0001);
  }

  void test_Category() {
    const std::vector<std::string> categories = fn.categories();
    TS_ASSERT(categories.size() == 1);
    TS_ASSERT(categories[0] == "Muon");
  }

  void test_values() {

    fn.setParameter("A", 0.45);
    fn.setParameter("Delta", 1.05);
    fn.setParameter("Sigma", 0.2);

    // Define 1d domain
    Mantid::API::FunctionDomain1DVector x(0, 2, 10);
    Mantid::API::FunctionValues y(x);

    TS_ASSERT_THROWS_NOTHING(fn.function(x, y));
    TS_ASSERT_DELTA(y[0], 0.4500, 1e-4);
    TS_ASSERT_DELTA(y[1], 0.4252, 1e-4);
    TS_ASSERT_DELTA(y[2], 0.3576, 1e-4);
    TS_ASSERT_DELTA(y[3], 0.2650, 1e-4);
    TS_ASSERT_DELTA(y[4], 0.1695, 1e-4);
    TS_ASSERT_DELTA(y[5], 0.0905, 1e-4);
    TS_ASSERT_DELTA(y[6], 0.0390, 1e-4);
    TS_ASSERT_DELTA(y[7], 0.0165, 1e-4);
    TS_ASSERT_DELTA(y[8], 0.0171, 1e-4);
    TS_ASSERT_DELTA(y[9], 0.0317, 1e-4);
  }

  StaticKuboToyabeTimesGausDecay fn;
};

#endif /* MANTID_CURVEFITTING_STATICKUBOTOYABETIMESGAUSDECAYTEST_H_ */