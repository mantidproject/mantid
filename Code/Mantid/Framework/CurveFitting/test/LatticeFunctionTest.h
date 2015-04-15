#ifndef MANTID_CURVEFITTING_LATTICEFUNCTIONTEST_H_
#define MANTID_CURVEFITTING_LATTICEFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/LatticeFunction.h"
#include "MantidAPI/LatticeDomain.h"

using Mantid::CurveFitting::LatticeFunction;
using Mantid::Kernel::V3D;

using namespace Mantid::API;

class LatticeFunctionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LatticeFunctionTest *createSuite() {
    return new LatticeFunctionTest();
  }
  static void destroySuite(LatticeFunctionTest *suite) { delete suite; }

  void testSetCrystalSystem() {
    LatticeFunction fn;
    fn.initialize();

    TS_ASSERT_THROWS_NOTHING(fn.setCrystalSystem("Cubic"));
    TS_ASSERT_THROWS_NOTHING(fn.setCrystalSystem("Tetragonal"));
    TS_ASSERT_THROWS_NOTHING(fn.setCrystalSystem("triclinic"));

    TS_ASSERT_THROWS(fn.setCrystalSystem("DoesNotExist"),
                     std::invalid_argument);

    fn.setCrystalSystem("Cubic");
    // a and ZeroShift
    TS_ASSERT_EQUALS(fn.nParams(), 2);

    fn.setCrystalSystem("Hexagonal");
    // a, c and ZeroShift
    TS_ASSERT_EQUALS(fn.nParams(), 3);

    /* The basic functionality is covered by the tests for
     * PawleyParameterFunction.
     */
  }

  void testSetUnitCell() {
    LatticeFunction fn;
    fn.initialize();

    TS_ASSERT_THROWS_NOTHING(fn.setUnitCell("1.0 2.0 3.0 90 100 110"));
    TS_ASSERT_EQUALS(fn.getParameter("a"), 1.0);
    TS_ASSERT_EQUALS(fn.getParameter("b"), 2.0);
    TS_ASSERT_EQUALS(fn.getParameter("c"), 3.0);
    TS_ASSERT_EQUALS(fn.getParameter("Alpha"), 90.0);
    TS_ASSERT_EQUALS(fn.getParameter("Beta"), 100.0);
    TS_ASSERT_EQUALS(fn.getParameter("Gamma"), 110.0);

    TS_ASSERT_THROWS_NOTHING(fn.setUnitCell("1.0 2.0 3.0"));
    TS_ASSERT_EQUALS(fn.getParameter("a"), 1.0);
    TS_ASSERT_EQUALS(fn.getParameter("b"), 2.0);
    TS_ASSERT_EQUALS(fn.getParameter("c"), 3.0);
    TS_ASSERT_EQUALS(fn.getParameter("Alpha"), 90.0);
    TS_ASSERT_EQUALS(fn.getParameter("Beta"), 90.0);
    TS_ASSERT_EQUALS(fn.getParameter("Gamma"), 90.0);
  }

  void testFunctionValues() {
      LatticeFunction fn;
      fn.initialize();

      // Al2O3, from PoldiCreatePeaksFromCell system test.
      fn.setCrystalSystem("Hexagonal");
      fn.setParameter("a", 4.7605);
      fn.setParameter("c", 12.9956);

      std::vector<V3D> hkls;
      hkls.push_back(V3D(1, 0, -2));
      hkls.push_back(V3D(1, 0, 4));
      hkls.push_back(V3D(0, 0, 6));
      hkls.push_back(V3D(5, -2, -5));

      LatticeDomain domain(hkls);
      FunctionValues values(domain);

      // Calculate d-values
      TS_ASSERT_THROWS_NOTHING(fn.function(domain, values));

      // Check values.
      TS_ASSERT_DELTA(values[0], 3.481144, 1e-6);
      TS_ASSERT_DELTA(values[1], 2.551773, 1e-6);
      TS_ASSERT_DELTA(values[2], 2.165933, 1e-6);
      TS_ASSERT_DELTA(values[3], 0.88880, 1e-5);
  }
};

#endif /* MANTID_CURVEFITTING_LATTICEFUNCTIONTEST_H_ */
