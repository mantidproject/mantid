#ifndef MANTID_CURVEFITTING_KERENTEST_H_
#define MANTID_CURVEFITTING_KERENTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/Keren.h"
#include "MantidKernel/PhysicalConstants.h"

using Mantid::CurveFitting::Functions::Keren;

namespace {
constexpr double TWOPI = 2.0 * M_PI;
}

/// Test class to test protected methods
class TestFunction : public Keren {
public:
  double wrapRelaxation(const double delta, const double larmor,
                        const double fluct, const double time) const {
    return relaxation(delta, larmor, fluct, time);
  }
  double wrapPolarization(const double delta, const double larmor,
                          const double fluct, const double time) const {
    return polarization(delta, larmor, fluct, time);
  }
  void wrapFunc1D(double *out, const double *xValues,
                  const size_t nData) const {
    function1D(out, xValues, nData);
  }
};

class KerenTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static KerenTest *createSuite() { return new KerenTest(); }
  static void destroySuite(KerenTest *suite) { delete suite; }

  void test_name() {
    Keren function;
    TS_ASSERT_EQUALS("Keren", function.name());
  }

  void test_category() {
    Keren function;
    TS_ASSERT_EQUALS("Muon", function.category());
  }

  void test_relaxation() {
    TestFunction function;
    function.initialize();

    const double field = 100;
    const double larmor =
        Mantid::PhysicalConstants::MuonGyromagneticRatio * field * TWOPI;
    const double delta = larmor * 0.2;
    const double fluct = delta;

    TS_ASSERT_DELTA(function.wrapRelaxation(delta, larmor, fluct, 1.0), 0.2057,
                    0.0001);
    TS_ASSERT_DELTA(function.wrapRelaxation(delta, larmor, fluct, 5.0), 0.7261,
                    0.001);
    TS_ASSERT_DELTA(function.wrapRelaxation(delta, larmor, fluct, 10.0), 1.3811,
                    0.001);
  }

  void test_polarization() {
    TestFunction function;
    function.initialize();

    const double field = 100;
    const double larmor =
        Mantid::PhysicalConstants::MuonGyromagneticRatio * field * TWOPI;
    const double delta = larmor * 0.2;
    const double fluct = delta;

    TS_ASSERT_DELTA(function.wrapPolarization(delta, larmor, fluct, 1.0),
                    0.8141, 0.001);
    TS_ASSERT_DELTA(function.wrapPolarization(delta, larmor, fluct, 5.0),
                    0.4838, 0.001);
    TS_ASSERT_DELTA(function.wrapPolarization(delta, larmor, fluct, 10.0),
                    0.2513, 0.001);
  }

  void test_evaluateFunction() {
    TestFunction function;
    function.initialize();
    const double field = 100;
    const double delta =
        Mantid::PhysicalConstants::MuonGyromagneticRatio * field * TWOPI * 0.2;
    const double fluct = delta;
    function.setParameter("A", 1.0);
    function.setParameter("Delta", delta);
    function.setParameter("Field", field);
    function.setParameter("Fluct", fluct);

    double x = 1.0;
    double y;
    TS_ASSERT_THROWS_NOTHING(function.wrapFunc1D(&y, &x, 1));
    TS_ASSERT_DELTA(y, 0.8141, 0.001);
  }
};

#endif /* MANTID_CURVEFITTING_KERENTEST_H_ */
