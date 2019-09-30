// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_LATTICEFUNCTIONTEST_H_
#define MANTID_CURVEFITTING_LATTICEFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/LatticeFunction.h"
#include "MantidGeometry/Crystal/UnitCell.h"

using Mantid::CurveFitting::LatticeFunction;
using Mantid::Geometry::UnitCell;
using Mantid::Geometry::unitCellToStr;
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

  void testSetLatticeSystem() {
    LatticeFunction fn;
    fn.initialize();

    TS_ASSERT_THROWS_NOTHING(fn.setLatticeSystem("Cubic"));
    TS_ASSERT_THROWS_NOTHING(fn.setLatticeSystem("Tetragonal"));
    TS_ASSERT_THROWS_NOTHING(fn.setLatticeSystem("triclinic"));

    TS_ASSERT_THROWS(fn.setLatticeSystem("DoesNotExist"),
                     const std::invalid_argument &);

    fn.setLatticeSystem("Cubic");
    // a and ZeroShift
    TS_ASSERT_EQUALS(fn.nParams(), 2);

    fn.setLatticeSystem("Hexagonal");
    // a, c and ZeroShift
    TS_ASSERT_EQUALS(fn.nParams(), 3);

    /* The basic functionality is covered by the tests for
     * PawleyParameterFunction.
     */
  }

  void testSetUnitCellString() {
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

  void testSetUnitCellUnitCell() {
    LatticeFunction fn;
    fn.initialize();

    TS_ASSERT_THROWS_NOTHING(fn.setUnitCell(UnitCell(1, 2, 3, 90, 100, 110)));
    TS_ASSERT_EQUALS(fn.getParameter("a"), 1.0);
    TS_ASSERT_EQUALS(fn.getParameter("b"), 2.0);
    TS_ASSERT_EQUALS(fn.getParameter("c"), 3.0);
    TS_ASSERT_EQUALS(fn.getParameter("Alpha"), 90.0);
    TS_ASSERT_EQUALS(fn.getParameter("Beta"), 100.0);
    TS_ASSERT_EQUALS(fn.getParameter("Gamma"), 110.0);
  }

  void testGetUnitCell() {
    LatticeFunction fn;
    fn.initialize();

    UnitCell cell(1, 2, 3, 90, 100, 110);
    TS_ASSERT_THROWS_NOTHING(fn.setUnitCell(cell));

    TS_ASSERT_EQUALS(unitCellToStr(fn.getUnitCell()), unitCellToStr(cell));
  }

  void testFunctionValues() {
    LatticeFunction fn;
    fn.initialize();

    // Al2O3, from PoldiCreatePeaksFromCell system test.
    fn.setLatticeSystem("Hexagonal");
    fn.setParameter("a", 4.7605);
    fn.setParameter("c", 12.9956);

    std::vector<V3D> hkls{{1, 0, -2}, {1, 0, 4}, {0, 0, 6}, {5, -2, -5}};

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
