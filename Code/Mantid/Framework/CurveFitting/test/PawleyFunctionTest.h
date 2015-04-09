#ifndef MANTID_CURVEFITTING_PAWLEYFUNCTIONTEST_H_
#define MANTID_CURVEFITTING_PAWLEYFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/PawleyFunction.h"
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

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

  void testSetParametersFromUnitCell() {
    PawleyParameterFunction fn;
    fn.initialize();

    fn.setAttributeValue("CrystalSystem", "Triclinic");

    UnitCell cell(3., 4., 5., 101., 111., 103.);

    TS_ASSERT_THROWS_NOTHING(fn.setParametersFromUnitCell(cell));

    TS_ASSERT_EQUALS(fn.getParameter("a"), 3.0);
    TS_ASSERT_EQUALS(fn.getParameter("b"), 4.0);
    TS_ASSERT_EQUALS(fn.getParameter("c"), 5.0);
    TS_ASSERT_EQUALS(fn.getParameter("Alpha"), 101.0);
    TS_ASSERT_EQUALS(fn.getParameter("Beta"), 111.0);
    TS_ASSERT_EQUALS(fn.getParameter("Gamma"), 103.0);

    fn.setAttributeValue("CrystalSystem", "Cubic");

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
                     std::invalid_argument);

    TS_ASSERT_THROWS(fn.setAttributeValue("ProfileFunction", "DoesNotExist"),
                     Exception::NotFoundError);
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

  void testFunctionFitSi() {
    /* This example generates a spectrum with the first two reflections
     * of Silicon with lattice parameter a = 5.4311946 Angstr.
     *    hkl      d       height      fwhm
     *   1 1 1  3.13570    40.0       0.006
     *   2 2 0  1.92022    110.0      0.004
     */
    auto ws = getWorkspace(
        "name=Gaussian,PeakCentre=3.13570166,Height=40.0,Sigma=0.003;name="
        "Gaussian,PeakCentre=1.92021727,Height=110.0,Sigma=0.002",
        1.85, 3.2, 400);

    PawleyFunction_sptr pawleyFn = boost::make_shared<PawleyFunction>();
    pawleyFn->initialize();
    pawleyFn->setCrystalSystem("Cubic");
    pawleyFn->addPeak(V3D(1, 1, 1), 0.0065, 35.0);
    pawleyFn->addPeak(V3D(2, 2, 0), 0.0045, 110.0);
    pawleyFn->setUnitCell("5.4295 5.4295 5.4295");

    // fix ZeroShift
    pawleyFn->fix(pawleyFn->parameterIndex("f0.ZeroShift"));

    IAlgorithm_sptr fit = AlgorithmManager::Instance().create("Fit");
    fit->setProperty("Function",
                     boost::dynamic_pointer_cast<IFunction>(pawleyFn));
    fit->setProperty("InputWorkspace", ws);
    fit->execute();

    PawleyParameterFunction_sptr parameters =
        pawleyFn->getPawleyParameterFunction();

    TS_ASSERT_DELTA(parameters->getParameter("a"), 5.4311946, 1e-6);
  }

  void testFunctionFitSiZeroShift() {
    /* This example generates a spectrum with the first three reflections
     * of Silicon with lattice parameter a = 5.4311946 Angstr.
     *    hkl      d       height     ca. fwhm
     *   1 1 1  3.13570    40.0       0.006
     *   2 2 0  1.92022    110.0      0.004
     *   3 1 1  1.63757    101.0      0.003
     */
    auto ws = getWorkspace(
        "name=Gaussian,PeakCentre=3.13870166,Height=40.0,Sigma=0.003;name="
        "Gaussian,PeakCentre=1.92321727,Height=110.0,Sigma=0.002;name=Gaussian,"
        "PeakCentre=1.6405667,Height=105.0,Sigma=0.0016",
        1.6, 3.2, 800);

    PawleyFunction_sptr pawleyFn = boost::make_shared<PawleyFunction>();
    pawleyFn->initialize();
    pawleyFn->setCrystalSystem("Cubic");
    pawleyFn->addPeak(V3D(1, 1, 1), 0.0065, 35.0);
    pawleyFn->addPeak(V3D(2, 2, 0), 0.0045, 115.0);
    pawleyFn->addPeak(V3D(3, 1, 1), 0.0035, 115.0);
    pawleyFn->setUnitCell("5.433 5.433 5.433");
    pawleyFn->setParameter("f0.ZeroShift", 0.001);

    IAlgorithm_sptr fit = AlgorithmManager::Instance().create("Fit");
    fit->setProperty("Function",
                     boost::dynamic_pointer_cast<IFunction>(pawleyFn));
    fit->setProperty("InputWorkspace", ws);
    fit->execute();

    PawleyParameterFunction_sptr parameters =
        pawleyFn->getPawleyParameterFunction();

    TS_ASSERT_DELTA(parameters->getParameter("a"), 5.4311946, 1e-5);
    TS_ASSERT_DELTA(parameters->getParameter("ZeroShift"), 0.003, 1e-4);
  }

private:
  MatrixWorkspace_sptr getWorkspace(const std::string &functionString,
                                    double xMin, double xMax, size_t n) {
    IFunction_sptr siFn =
        FunctionFactory::Instance().createInitialized(functionString);

    auto ws = WorkspaceFactory::Instance().create("Workspace2D", 1, n, n);

    FunctionDomain1DVector xValues(xMin, xMax, n);
    FunctionValues yValues(xValues);
    std::vector<double> eValues(n, 1.0);

    siFn->function(xValues, yValues);

    std::vector<double> &xData = ws->dataX(0);
    std::vector<double> &yData = ws->dataY(0);
    std::vector<double> &eData = ws->dataE(0);

    for (size_t i = 0; i < n; ++i) {
      xData[i] = xValues[i];
      yData[i] = yValues[i];
      eData[i] = eValues[i];
    }

    WorkspaceCreationHelper::addNoise(ws, 0, -0.1, 0.1);

    return ws;
  }

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
