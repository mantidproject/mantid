#ifndef REFLECTIVITYMULFTEST_H_
#define REFLECTIVITYMULFTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidCurveFitting/Functions/ReflectivityMulf.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;

class ReflectivityMulfTest : public CxxTest::TestSuite {
public:
  void testValues() {
    double xs[] = {0.005, 0.015, 0.025, 0.035, 0.045, 0.055, 0.065, 0.075,
                   0.085, 0.095, 0.105, 0.115, 0.125, 0.135, 0.145};
    // these values were calculated at a point when ReflectivityMulf was
    // considered to return correct results
    double ys[] = {0.737817797785,     0.737817797785,     0.0232244527086,
                   0.004187444808788,  0.001350372079943,  0.00056960122268,
                   0.0002825464250802, 0.000156646192819,  9.435007427375e-05,
                   6.068179039974e-05, 4.121292994357e-05, 2.933589512335e-05,
                   2.176838693157e-05, 1.676996775521e-05, 1.336689996267e-05};
    TS_ASSERT(sizeof(xs) == sizeof(ys));
    const size_t n = sizeof(xs) / sizeof(double);

    FunctionDomain1DView x(xs, n);
    FunctionValues y(x);

    ReflectivityMulf fun;
    fun.initialize();
    fun.setParameter("Theta", 2.3);
    fun.setParameter("ScaleFactor", 0.74);
    fun.setParameter("AirSLD", 0);
    fun.setParameter("BulkSLD", 6.35e-6);
    fun.setParameter("Roughness", 2.5);
    fun.setParameter("BackGround", 5.2e-6);
    fun.setParameter("Resolution", 5);
    fun.function(x, y);

    for (size_t i = 0; i < n; i++) {
      TS_ASSERT_DELTA(ys[i] / y.getCalculated(i), 1.0, 1e-10);
    }
  }

  void testAttribute() {
    ReflectivityMulf fun;
    fun.initialize();

    TS_ASSERT_EQUALS(fun.nAttributes(), 1);
    auto keys = fun.getAttributeNames();
    TS_ASSERT_EQUALS(keys.size(), 1);
    TS_ASSERT_EQUALS(keys[0], "nlayer");
    TS_ASSERT_EQUALS(fun.getAttribute("nlayer").asInt(), 0);
    TS_ASSERT_EQUALS(fun.nParams(), 7);

    TS_ASSERT_THROWS_NOTHING(fun.setAttributeValue("nlayer", 1));
    TS_ASSERT_EQUALS(fun.nParams(), 10);
    TS_ASSERT_EQUALS(fun.getParameter("SLD_Layer0"), 0);
    TS_ASSERT_EQUALS(fun.getParameter("d_Layer0"), 0);
    TS_ASSERT_EQUALS(fun.getParameter("Rough_Layer0"), 0);
    TS_ASSERT_EQUALS(fun.getAttribute("nlayer").asInt(), 1);

    TS_ASSERT_THROWS_NOTHING(fun.setAttributeValue("nlayer", 2));
    TS_ASSERT_EQUALS(fun.nParams(), 13);
    TS_ASSERT_EQUALS(fun.getParameter("SLD_Layer0"), 0);
    TS_ASSERT_EQUALS(fun.getParameter("d_Layer0"), 0);
    TS_ASSERT_EQUALS(fun.getParameter("Rough_Layer0"), 0);
    TS_ASSERT_EQUALS(fun.getParameter("SLD_Layer1"), 0);
    TS_ASSERT_EQUALS(fun.getParameter("d_Layer1"), 0);
    TS_ASSERT_EQUALS(fun.getParameter("Rough_Layer1"), 0);
    TS_ASSERT_EQUALS(fun.getAttribute("nlayer").asInt(), 2);
  }
};

#endif /*REFLECTIVITYMULFTEST_H_*/
