#ifndef MANTID_CURVEFITTING_PAWLEYFUNCTIONTEST_H_
#define MANTID_CURVEFITTING_PAWLEYFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/PawleyFunction.h"
#include "MantidGeometry/Crystal/PointGroup.h"

using namespace Mantid::CurveFitting;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class PawleyFunctionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PawleyFunctionTest *createSuite() { return new PawleyFunctionTest(); }
  static void destroySuite(PawleyFunctionTest *suite) { delete suite; }

  void testCrystalSystem() {
    PawleyParameterFunction fn;
    fn.initialize();

    TS_ASSERT(fn.hasAttribute("CrystalSystem"));

    // Cubic, check case insensitivity
    TS_ASSERT_THROWS_NOTHING(fn.setAttributeValue("CrystalSystem", "cubic"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Cubic);
    TS_ASSERT_THROWS_NOTHING(fn.setAttributeValue("CrystalSystem", "Cubic"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Cubic);
    TS_ASSERT_THROWS_NOTHING(fn.setAttributeValue("CrystalSystem", "CUBIC"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Cubic);

    // Tetragonal
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "tetragonal"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Tetragonal);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "Tetragonal"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Tetragonal);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "TETRAGONAL"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Tetragonal);

    // Hexagonal
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "hexagonal"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Hexagonal);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "Hexagonal"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Hexagonal);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "HEXAGONAL"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Hexagonal);

    // Orthorhombic
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "orthorhombic"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Orthorhombic);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "Orthorhombic"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Orthorhombic);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "ORTHORHOMBIC"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Orthorhombic);

    // Monoclinic
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "monoclinic"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Monoclinic);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "Monoclinic"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Monoclinic);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "MONOCLINIC"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Monoclinic);

    // Triclinic
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "triclinic"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Triclinic);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "Triclinic"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Triclinic);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("CrystalSystem", "TRICLINIC"));
    TS_ASSERT_EQUALS(fn.getCrystalSystem(), PointGroup::Triclinic);

    // invalid string
    TS_ASSERT_THROWS(fn.setAttributeValue("CrystalSystem", "invalid"),
                     std::invalid_argument);
  }

  void testCrystalSystemConstraintsCubic() {
    PawleyParameterFunction fn;
    fn.initialize();

    fn.setAttributeValue("CrystalSystem", "Cubic");

    TS_ASSERT_EQUALS(fn.nParams(), 2);

    fn.setParameter("a", 3.0);
    TS_ASSERT_EQUALS(fn.getParameter("a"), 3.0);

    TS_ASSERT_THROWS(fn.getParameter("b"), std::invalid_argument);
    TS_ASSERT_THROWS(fn.getParameter("c"), std::invalid_argument);
    TS_ASSERT_THROWS(fn.getParameter("Alpha"), std::invalid_argument);
    TS_ASSERT_THROWS(fn.getParameter("Beta"), std::invalid_argument);
    TS_ASSERT_THROWS(fn.getParameter("Gamma"), std::invalid_argument);

    UnitCell cell = fn.getUnitCellFromParameters();
    cellParametersAre(cell, 3.0, 3.0, 3.0, 90.0, 90.0, 90.0);
  }

  void testCrystalSystemConstraintsTetragonal() {
    PawleyParameterFunction fn;
    fn.initialize();

    fn.setAttributeValue("CrystalSystem", "Tetragonal");

    TS_ASSERT_EQUALS(fn.nParams(), 3);

    fn.setParameter("a", 3.0);
    TS_ASSERT_EQUALS(fn.getParameter("a"), 3.0);
    fn.setParameter("c", 5.0);
    TS_ASSERT_EQUALS(fn.getParameter("c"), 5.0);

    TS_ASSERT_THROWS(fn.getParameter("b"), std::invalid_argument);
    TS_ASSERT_THROWS(fn.getParameter("Alpha"), std::invalid_argument);
    TS_ASSERT_THROWS(fn.getParameter("Beta"), std::invalid_argument);
    TS_ASSERT_THROWS(fn.getParameter("Gamma"), std::invalid_argument);

    UnitCell cell = fn.getUnitCellFromParameters();
    cellParametersAre(cell, 3.0, 3.0, 5.0, 90.0, 90.0, 90.0);
  }

  void testCrystalSystemConstraintsHexagonal() {
    PawleyParameterFunction fn;
    fn.initialize();

    fn.setAttributeValue("CrystalSystem", "Hexagonal");

    TS_ASSERT_EQUALS(fn.nParams(), 3);

    fn.setParameter("a", 3.0);
    TS_ASSERT_EQUALS(fn.getParameter("a"), 3.0);
    fn.setParameter("c", 5.0);
    TS_ASSERT_EQUALS(fn.getParameter("c"), 5.0);

    TS_ASSERT_THROWS(fn.getParameter("b"), std::invalid_argument);
    TS_ASSERT_THROWS(fn.getParameter("Alpha"), std::invalid_argument);
    TS_ASSERT_THROWS(fn.getParameter("Beta"), std::invalid_argument);
    TS_ASSERT_THROWS(fn.getParameter("Gamma"), std::invalid_argument);

    UnitCell cell = fn.getUnitCellFromParameters();
    cellParametersAre(cell, 3.0, 3.0, 5.0, 90.0, 90.0, 120.0);
  }

  void testCrystalSystemConstraintsTrigonal() {
    PawleyParameterFunction fn;
    fn.initialize();

    fn.setAttributeValue("CrystalSystem", "Trigonal");

    TS_ASSERT_EQUALS(fn.nParams(), 3);

    fn.setParameter("a", 3.0);
    TS_ASSERT_EQUALS(fn.getParameter("a"), 3.0);
    fn.setParameter("Alpha", 101.0);
    TS_ASSERT_EQUALS(fn.getParameter("Alpha"), 101.0);

    TS_ASSERT_THROWS(fn.getParameter("b"), std::invalid_argument);
    TS_ASSERT_THROWS(fn.getParameter("c"), std::invalid_argument);
    TS_ASSERT_THROWS(fn.getParameter("Beta"), std::invalid_argument);
    TS_ASSERT_THROWS(fn.getParameter("Gamma"), std::invalid_argument);

    UnitCell cell = fn.getUnitCellFromParameters();
    cellParametersAre(cell, 3.0, 3.0, 3.0, 101.0, 101.0, 101.0);
  }

  void testCrystalSystemConstraintsOrthorhombic() {
    PawleyParameterFunction fn;
    fn.initialize();

    fn.setAttributeValue("CrystalSystem", "Orthorhombic");

    TS_ASSERT_EQUALS(fn.nParams(), 4);

    fn.setParameter("a", 3.0);
    TS_ASSERT_EQUALS(fn.getParameter("a"), 3.0);
    fn.setParameter("b", 4.0);
    TS_ASSERT_EQUALS(fn.getParameter("b"), 4.0);
    fn.setParameter("c", 5.0);
    TS_ASSERT_EQUALS(fn.getParameter("c"), 5.0);

    TS_ASSERT_THROWS(fn.getParameter("Alpha"), std::invalid_argument);
    TS_ASSERT_THROWS(fn.getParameter("Beta"), std::invalid_argument);
    TS_ASSERT_THROWS(fn.getParameter("Gamma"), std::invalid_argument);

    UnitCell cell = fn.getUnitCellFromParameters();
    cellParametersAre(cell, 3.0, 4.0, 5.0, 90.0, 90.0, 90.0);
  }

  void testCrystalSystemConstraintsMonoclinic() {
    PawleyParameterFunction fn;
    fn.initialize();

    fn.setAttributeValue("CrystalSystem", "Monoclinic");

    TS_ASSERT_EQUALS(fn.nParams(), 5);

    fn.setParameter("a", 3.0);
    TS_ASSERT_EQUALS(fn.getParameter("a"), 3.0);
    fn.setParameter("b", 4.0);
    TS_ASSERT_EQUALS(fn.getParameter("b"), 4.0);
    fn.setParameter("c", 5.0);
    TS_ASSERT_EQUALS(fn.getParameter("c"), 5.0);
    fn.setParameter("Beta", 101.0);
    TS_ASSERT_EQUALS(fn.getParameter("Beta"), 101.0);

    TS_ASSERT_THROWS(fn.getParameter("Alpha"), std::invalid_argument);
    TS_ASSERT_THROWS(fn.getParameter("Gamma"), std::invalid_argument);

    UnitCell cell = fn.getUnitCellFromParameters();
    cellParametersAre(cell, 3.0, 4.0, 5.0, 90.0, 101.0, 90.0);
  }

  void testCrystalSystemConstraintsTriclinic() {
    PawleyParameterFunction fn;
    fn.initialize();

    fn.setAttributeValue("CrystalSystem", "Triclinic");

    TS_ASSERT_EQUALS(fn.nParams(), 7);

    fn.setParameter("a", 3.0);
    TS_ASSERT_EQUALS(fn.getParameter("a"), 3.0);
    fn.setParameter("b", 4.0);
    TS_ASSERT_EQUALS(fn.getParameter("b"), 4.0);
    fn.setParameter("c", 5.0);
    TS_ASSERT_EQUALS(fn.getParameter("c"), 5.0);
    fn.setParameter("Alpha", 101.0);
    TS_ASSERT_EQUALS(fn.getParameter("Alpha"), 101.0);
    fn.setParameter("Beta", 111.0);
    TS_ASSERT_EQUALS(fn.getParameter("Beta"), 111.0);
    fn.setParameter("Gamma", 103.0);
    TS_ASSERT_EQUALS(fn.getParameter("Gamma"), 103.0);

    UnitCell cell = fn.getUnitCellFromParameters();
    cellParametersAre(cell, 3.0, 4.0, 5.0, 101.0, 111.0, 103.0);
  }

  void testPawleyFunctionInitialization() {
    PawleyFunction fn;
    fn.initialize();

    TS_ASSERT(boost::dynamic_pointer_cast<CompositeFunction>(
        fn.getDecoratedFunction()));

    // The base parameters of PawleyParameterFunction
    TS_ASSERT_EQUALS(fn.nParams(), 7);
  }

  void testPawleyFunctionSetCrystalSystem() {
    PawleyFunction fn;
    fn.initialize();

    TS_ASSERT_EQUALS(fn.nParams(), 7);

    fn.setCrystalSystem("Cubic");

    TS_ASSERT_EQUALS(fn.nParams(), 2);
  }

  void testPawleyFunctionAddPeak() {
    PawleyFunction fn;
    fn.initialize();

    TS_ASSERT_EQUALS(fn.nParams(), 7);

    fn.addPeak(V3D(), 2.0, 3.0, 4.0);

    TS_ASSERT_EQUALS(fn.nParams(), 10);
  }

  void testPawleyFunctionSetProfileFunction() {
    PawleyFunction fn;
    fn.initialize();

    TS_ASSERT_EQUALS(fn.nParams(), 7);

    fn.addPeak(V3D(), 2.0, 3.0, 4.0);

    TS_ASSERT_EQUALS(fn.nParams(), 10);

    fn.setProfileFunction("PseudoVoigt");

    TS_ASSERT_EQUALS(fn.nParams(), 11);
  }

private:
  void cellParametersAre(const UnitCell &cell, double a, double b, double c,
                         double alpha, double beta, double gamma) {
    TS_ASSERT_DELTA(cell.a(), a, 1e-9);
    TS_ASSERT_DELTA(cell.b(), b, 1e-9);
    TS_ASSERT_DELTA(cell.c(), c, 1e-9);

    TS_ASSERT_DELTA(cell.alpha(), alpha, 1e-9);
    TS_ASSERT_DELTA(cell.beta(), beta, 1e-9);
    TS_ASSERT_DELTA(cell.gamma(), gamma, 1e-9);
  }
};

#endif /* MANTID_CURVEFITTING_PAWLEYFUNCTIONTEST_H_ */
