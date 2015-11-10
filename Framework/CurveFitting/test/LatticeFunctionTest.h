#ifndef MANTID_CURVEFITTING_LATTICEFUNCTIONTEST_H_
#define MANTID_CURVEFITTING_LATTICEFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/LatticeFunction.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/LatticeDomain.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidGeometry/Crystal/UnitCell.h"

using Mantid::CurveFitting::LatticeFunction;
using Mantid::Kernel::V3D;
using Mantid::Geometry::UnitCell;
using Mantid::Geometry::unitCellToStr;

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
                     std::invalid_argument);

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

  void testFitExampleTable() {
    // Fit Silicon lattice with three peaks.
    ITableWorkspace_sptr table = WorkspaceFactory::Instance().createTable();
    table->addColumn("V3D", "HKL");
    table->addColumn("double", "d");

    TableRow newRow = table->appendRow();
    newRow << V3D(1, 1, 1) << 3.135702;
    newRow = table->appendRow();
    newRow << V3D(2, 2, 0) << 1.920217;
    newRow = table->appendRow();
    newRow << V3D(3, 1, 1) << 1.637567;

    IFunction_sptr fn =
        FunctionFactory::Instance().createFunction("LatticeFunction");
    fn->setAttributeValue("LatticeSystem", "Cubic");
    fn->addTies("ZeroShift=0.0");
    fn->setParameter("a", 5);

    IAlgorithm_sptr fit = AlgorithmManager::Instance().create("Fit");
    fit->setProperty("Function", fn);
    fit->setProperty("InputWorkspace", table);
    fit->setProperty("CostFunction", "Unweighted least squares");
    fit->setProperty("CreateOutput", true);
    fit->execute();

    TS_ASSERT_DELTA(fn->getParameter("a"), 5.4311946, 1e-6);
    TS_ASSERT_LESS_THAN(fn->getError(0), 1e-6);
  }
};

#endif /* MANTID_CURVEFITTING_LATTICEFUNCTIONTEST_H_ */
