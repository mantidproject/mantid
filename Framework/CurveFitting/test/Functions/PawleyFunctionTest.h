// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_PAWLEYFUNCTIONTEST_H_
#define MANTID_CURVEFITTING_PAWLEYFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/PawleyFunction.h"
#include "MantidGeometry/Crystal/PointGroup.h"

using namespace Mantid::CurveFitting;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

using Mantid::CurveFitting::Functions::PawleyFunction;
using Mantid::CurveFitting::Functions::PawleyParameterFunction;
using Mantid::CurveFitting::Functions::PawleyParameterFunction_sptr;

class PawleyFunctionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PawleyFunctionTest *createSuite() { return new PawleyFunctionTest(); }
  static void destroySuite(PawleyFunctionTest *suite) { delete suite; }

  void testLatticeSystem() {
    PawleyParameterFunction fn;
    fn.initialize();

    TS_ASSERT(fn.hasAttribute("LatticeSystem"));

    // Cubic, check case insensitivity
    TS_ASSERT_THROWS_NOTHING(fn.setAttributeValue("LatticeSystem", "cubic"));
    TS_ASSERT_EQUALS(fn.getLatticeSystem(), PointGroup::LatticeSystem::Cubic);
    TS_ASSERT_THROWS_NOTHING(fn.setAttributeValue("LatticeSystem", "Cubic"));
    TS_ASSERT_EQUALS(fn.getLatticeSystem(), PointGroup::LatticeSystem::Cubic);
    TS_ASSERT_THROWS_NOTHING(fn.setAttributeValue("LatticeSystem", "CUBIC"));
    TS_ASSERT_EQUALS(fn.getLatticeSystem(), PointGroup::LatticeSystem::Cubic);

    // Tetragonal
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("LatticeSystem", "tetragonal"));
    TS_ASSERT_EQUALS(fn.getLatticeSystem(),
                     PointGroup::LatticeSystem::Tetragonal);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("LatticeSystem", "Tetragonal"));
    TS_ASSERT_EQUALS(fn.getLatticeSystem(),
                     PointGroup::LatticeSystem::Tetragonal);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("LatticeSystem", "TETRAGONAL"));
    TS_ASSERT_EQUALS(fn.getLatticeSystem(),
                     PointGroup::LatticeSystem::Tetragonal);

    // Hexagonal
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("LatticeSystem", "hexagonal"));
    TS_ASSERT_EQUALS(fn.getLatticeSystem(),
                     PointGroup::LatticeSystem::Hexagonal);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("LatticeSystem", "Hexagonal"));
    TS_ASSERT_EQUALS(fn.getLatticeSystem(),
                     PointGroup::LatticeSystem::Hexagonal);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("LatticeSystem", "HEXAGONAL"));
    TS_ASSERT_EQUALS(fn.getLatticeSystem(),
                     PointGroup::LatticeSystem::Hexagonal);

    // Orthorhombic
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("LatticeSystem", "orthorhombic"));
    TS_ASSERT_EQUALS(fn.getLatticeSystem(),
                     PointGroup::LatticeSystem::Orthorhombic);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("LatticeSystem", "Orthorhombic"));
    TS_ASSERT_EQUALS(fn.getLatticeSystem(),
                     PointGroup::LatticeSystem::Orthorhombic);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("LatticeSystem", "ORTHORHOMBIC"));
    TS_ASSERT_EQUALS(fn.getLatticeSystem(),
                     PointGroup::LatticeSystem::Orthorhombic);

    // Monoclinic
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("LatticeSystem", "monoclinic"));
    TS_ASSERT_EQUALS(fn.getLatticeSystem(),
                     PointGroup::LatticeSystem::Monoclinic);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("LatticeSystem", "Monoclinic"));
    TS_ASSERT_EQUALS(fn.getLatticeSystem(),
                     PointGroup::LatticeSystem::Monoclinic);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("LatticeSystem", "MONOCLINIC"));
    TS_ASSERT_EQUALS(fn.getLatticeSystem(),
                     PointGroup::LatticeSystem::Monoclinic);

    // Triclinic
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("LatticeSystem", "triclinic"));
    TS_ASSERT_EQUALS(fn.getLatticeSystem(),
                     PointGroup::LatticeSystem::Triclinic);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("LatticeSystem", "Triclinic"));
    TS_ASSERT_EQUALS(fn.getLatticeSystem(),
                     PointGroup::LatticeSystem::Triclinic);
    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("LatticeSystem", "TRICLINIC"));
    TS_ASSERT_EQUALS(fn.getLatticeSystem(),
                     PointGroup::LatticeSystem::Triclinic);

    // invalid string
    TS_ASSERT_THROWS(fn.setAttributeValue("LatticeSystem", "invalid"),
                     const std::invalid_argument &);
  }

  void testLatticeSystemConstraintsCubic() {
    PawleyParameterFunction fn;
    fn.initialize();

    fn.setAttributeValue("LatticeSystem", "Cubic");

    TS_ASSERT_EQUALS(fn.nParams(), 2);

    fn.setParameter("a", 3.0);
    TS_ASSERT_EQUALS(fn.getParameter("a"), 3.0);

    TS_ASSERT_THROWS(fn.getParameter("b"), const std::invalid_argument &);
    TS_ASSERT_THROWS(fn.getParameter("c"), const std::invalid_argument &);
    TS_ASSERT_THROWS(fn.getParameter("Alpha"), const std::invalid_argument &);
    TS_ASSERT_THROWS(fn.getParameter("Beta"), const std::invalid_argument &);
    TS_ASSERT_THROWS(fn.getParameter("Gamma"), const std::invalid_argument &);

    UnitCell cell = fn.getUnitCellFromParameters();
    cellParametersAre(cell, 3.0, 3.0, 3.0, 90.0, 90.0, 90.0);
  }

  void testLatticeSystemConstraintsTetragonal() {
    PawleyParameterFunction fn;
    fn.initialize();

    fn.setAttributeValue("LatticeSystem", "Tetragonal");

    TS_ASSERT_EQUALS(fn.nParams(), 3);

    fn.setParameter("a", 3.0);
    TS_ASSERT_EQUALS(fn.getParameter("a"), 3.0);
    fn.setParameter("c", 5.0);
    TS_ASSERT_EQUALS(fn.getParameter("c"), 5.0);

    TS_ASSERT_THROWS(fn.getParameter("b"), const std::invalid_argument &);
    TS_ASSERT_THROWS(fn.getParameter("Alpha"), const std::invalid_argument &);
    TS_ASSERT_THROWS(fn.getParameter("Beta"), const std::invalid_argument &);
    TS_ASSERT_THROWS(fn.getParameter("Gamma"), const std::invalid_argument &);

    UnitCell cell = fn.getUnitCellFromParameters();
    cellParametersAre(cell, 3.0, 3.0, 5.0, 90.0, 90.0, 90.0);
  }

  void testLatticeSystemConstraintsHexagonal() {
    PawleyParameterFunction fn;
    fn.initialize();

    fn.setAttributeValue("LatticeSystem", "Hexagonal");

    TS_ASSERT_EQUALS(fn.nParams(), 3);

    fn.setParameter("a", 3.0);
    TS_ASSERT_EQUALS(fn.getParameter("a"), 3.0);
    fn.setParameter("c", 5.0);
    TS_ASSERT_EQUALS(fn.getParameter("c"), 5.0);

    TS_ASSERT_THROWS(fn.getParameter("b"), const std::invalid_argument &);
    TS_ASSERT_THROWS(fn.getParameter("Alpha"), const std::invalid_argument &);
    TS_ASSERT_THROWS(fn.getParameter("Beta"), const std::invalid_argument &);
    TS_ASSERT_THROWS(fn.getParameter("Gamma"), const std::invalid_argument &);

    UnitCell cell = fn.getUnitCellFromParameters();
    cellParametersAre(cell, 3.0, 3.0, 5.0, 90.0, 90.0, 120.0);
  }

  void testLatticeSystemConstraintsRhombohedral() {
    PawleyParameterFunction fn;
    fn.initialize();

    fn.setAttributeValue("LatticeSystem", "Rhombohedral");

    TS_ASSERT_EQUALS(fn.nParams(), 3);

    fn.setParameter("a", 3.0);
    TS_ASSERT_EQUALS(fn.getParameter("a"), 3.0);
    fn.setParameter("Alpha", 101.0);
    TS_ASSERT_EQUALS(fn.getParameter("Alpha"), 101.0);

    TS_ASSERT_THROWS(fn.getParameter("b"), const std::invalid_argument &);
    TS_ASSERT_THROWS(fn.getParameter("c"), const std::invalid_argument &);
    TS_ASSERT_THROWS(fn.getParameter("Beta"), const std::invalid_argument &);
    TS_ASSERT_THROWS(fn.getParameter("Gamma"), const std::invalid_argument &);

    UnitCell cell = fn.getUnitCellFromParameters();
    cellParametersAre(cell, 3.0, 3.0, 3.0, 101.0, 101.0, 101.0);
  }

  void testLatticeSystemConstraintsOrthorhombic() {
    PawleyParameterFunction fn;
    fn.initialize();

    fn.setAttributeValue("LatticeSystem", "Orthorhombic");

    TS_ASSERT_EQUALS(fn.nParams(), 4);

    fn.setParameter("a", 3.0);
    TS_ASSERT_EQUALS(fn.getParameter("a"), 3.0);
    fn.setParameter("b", 4.0);
    TS_ASSERT_EQUALS(fn.getParameter("b"), 4.0);
    fn.setParameter("c", 5.0);
    TS_ASSERT_EQUALS(fn.getParameter("c"), 5.0);

    TS_ASSERT_THROWS(fn.getParameter("Alpha"), const std::invalid_argument &);
    TS_ASSERT_THROWS(fn.getParameter("Beta"), const std::invalid_argument &);
    TS_ASSERT_THROWS(fn.getParameter("Gamma"), const std::invalid_argument &);

    UnitCell cell = fn.getUnitCellFromParameters();
    cellParametersAre(cell, 3.0, 4.0, 5.0, 90.0, 90.0, 90.0);
  }

  void testLatticeSystemConstraintsMonoclinic() {
    PawleyParameterFunction fn;
    fn.initialize();

    fn.setAttributeValue("LatticeSystem", "Monoclinic");

    TS_ASSERT_EQUALS(fn.nParams(), 5);

    fn.setParameter("a", 3.0);
    TS_ASSERT_EQUALS(fn.getParameter("a"), 3.0);
    fn.setParameter("b", 4.0);
    TS_ASSERT_EQUALS(fn.getParameter("b"), 4.0);
    fn.setParameter("c", 5.0);
    TS_ASSERT_EQUALS(fn.getParameter("c"), 5.0);
    fn.setParameter("Beta", 101.0);
    TS_ASSERT_EQUALS(fn.getParameter("Beta"), 101.0);

    TS_ASSERT_THROWS(fn.getParameter("Alpha"), const std::invalid_argument &);
    TS_ASSERT_THROWS(fn.getParameter("Gamma"), const std::invalid_argument &);

    UnitCell cell = fn.getUnitCellFromParameters();
    cellParametersAre(cell, 3.0, 4.0, 5.0, 90.0, 101.0, 90.0);
  }

  void testLatticeSystemConstraintsTriclinic() {
    PawleyParameterFunction fn;
    fn.initialize();

    fn.setAttributeValue("LatticeSystem", "Triclinic");

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

  void testSetParametersFromUnitCell() {
    PawleyParameterFunction fn;
    fn.initialize();

    fn.setAttributeValue("LatticeSystem", "Triclinic");

    UnitCell cell(3., 4., 5., 101., 111., 103.);

    TS_ASSERT_THROWS_NOTHING(fn.setParametersFromUnitCell(cell));

    TS_ASSERT_EQUALS(fn.getParameter("a"), 3.0);
    TS_ASSERT_EQUALS(fn.getParameter("b"), 4.0);
    TS_ASSERT_EQUALS(fn.getParameter("c"), 5.0);
    TS_ASSERT_EQUALS(fn.getParameter("Alpha"), 101.0);
    TS_ASSERT_EQUALS(fn.getParameter("Beta"), 111.0);
    TS_ASSERT_EQUALS(fn.getParameter("Gamma"), 103.0);

    fn.setAttributeValue("LatticeSystem", "Cubic");

    cell.seta(5.43);
    TS_ASSERT_THROWS_NOTHING(fn.setParametersFromUnitCell(cell));

    TS_ASSERT_EQUALS(fn.getParameter("a"), 5.43);
  }

  void testProfileFunctionName() {
    PawleyParameterFunction fn;
    fn.initialize();

    TS_ASSERT_THROWS_NOTHING(
        fn.setAttributeValue("ProfileFunction", "Gaussian"));
    TS_ASSERT_EQUALS(fn.getProfileFunctionName(), "Gaussian");

    // works only with IPeakFunctions
    TS_ASSERT_THROWS(fn.setAttributeValue("ProfileFunction", "Chebyshev"),
                     const std::invalid_argument &);

    TS_ASSERT_THROWS(fn.setAttributeValue("ProfileFunction", "DoesNotExist"),
                     const Exception::NotFoundError &);
  }

  void testPawleyFunctionInitialization() {
    PawleyFunction fn;
    fn.initialize();

    TS_ASSERT(boost::dynamic_pointer_cast<CompositeFunction>(
        fn.getDecoratedFunction()));

    // The base parameters of PawleyParameterFunction
    TS_ASSERT_EQUALS(fn.nParams(), 7);
  }

  void testPawleyFunctionSetLatticeSystem() {
    PawleyFunction fn;
    fn.initialize();

    TS_ASSERT_EQUALS(fn.nParams(), 7);

    fn.setLatticeSystem("Cubic");

    TS_ASSERT_EQUALS(fn.nParams(), 2);
  }

  void testPawleyFunctionAddPeak() {
    PawleyFunction fn;
    fn.initialize();
    TS_ASSERT_EQUALS(fn.getPeakCount(), 0);

    TS_ASSERT_EQUALS(fn.nParams(), 7);

    fn.addPeak(V3D(), 3.0, 4.0);

    TS_ASSERT_EQUALS(fn.nParams(), 10);
    TS_ASSERT_EQUALS(fn.getPeakCount(), 1);
  }

  void testPawleyFunctionClearPeaks() {
    PawleyFunction fn;
    fn.initialize();

    fn.addPeak(V3D(), 3.0, 4.0);
    TS_ASSERT_EQUALS(fn.getPeakCount(), 1);
    TS_ASSERT_THROWS_NOTHING(fn.clearPeaks());
    TS_ASSERT_EQUALS(fn.getPeakCount(), 0);
  }

  void testPawleyFunctionGetPeakHKL() {
    PawleyFunction fn;
    fn.initialize();

    fn.addPeak(V3D(1, 1, 1), 3.0, 4.0);
    TS_ASSERT_EQUALS(fn.getPeakCount(), 1);
    TS_ASSERT_EQUALS(fn.getPeakHKL(0), V3D(1, 1, 1));
  }

  void testPawleyFunctionGetPeakFunction() {
    PawleyFunction fn;
    fn.initialize();

    fn.addPeak(V3D(1, 1, 1), 3.0, 4.0);
    TS_ASSERT_EQUALS(fn.getPeakCount(), 1);

    IPeakFunction_sptr peak = fn.getPeakFunction(0);
    TS_ASSERT(peak);
    TS_ASSERT_EQUALS(peak->fwhm(), 3.0);
    TS_ASSERT_EQUALS(peak->height(), 4.0);
  }

  void testPawleyFunctionSetProfileFunction() {
    PawleyFunction fn;
    fn.initialize();

    TS_ASSERT_EQUALS(fn.nParams(), 7);

    fn.addPeak(V3D(), 3.0, 4.0);

    TS_ASSERT_EQUALS(fn.nParams(), 10);

    fn.setProfileFunction("PseudoVoigt");

    TS_ASSERT_EQUALS(fn.nParams(), 11);
  }

  void testPawleyFunctionGetParameterFunction() {
    PawleyFunction fn;
    fn.initialize();

    TS_ASSERT(fn.getPawleyParameterFunction());
  }

  void testPawleyFunctionSetUnitCell() {
    PawleyFunction fn;
    fn.initialize();

    TS_ASSERT_THROWS_NOTHING(fn.setUnitCell("1.0 2.0 3.0 90 91 92"));

    PawleyParameterFunction_sptr parameters = fn.getPawleyParameterFunction();
    TS_ASSERT_EQUALS(parameters->getParameter("a"), 1.0);
    TS_ASSERT_EQUALS(parameters->getParameter("b"), 2.0);
    TS_ASSERT_EQUALS(parameters->getParameter("c"), 3.0);
    TS_ASSERT_EQUALS(parameters->getParameter("Alpha"), 90.0);
    TS_ASSERT_EQUALS(parameters->getParameter("Beta"), 91.0);
    TS_ASSERT_EQUALS(parameters->getParameter("Gamma"), 92.0);

    TS_ASSERT_THROWS_NOTHING(fn.setUnitCell("2.0 3.0 4.0"));

    TS_ASSERT_EQUALS(parameters->getParameter("a"), 2.0);
    TS_ASSERT_EQUALS(parameters->getParameter("b"), 3.0);
    TS_ASSERT_EQUALS(parameters->getParameter("c"), 4.0);
    TS_ASSERT_EQUALS(parameters->getParameter("Alpha"), 90.0);
    TS_ASSERT_EQUALS(parameters->getParameter("Beta"), 90.0);
    TS_ASSERT_EQUALS(parameters->getParameter("Gamma"), 90.0);
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
