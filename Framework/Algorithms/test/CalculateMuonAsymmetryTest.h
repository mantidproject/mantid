#ifndef CALCULATEMUONASYMMETRYTEST_H_
#define CALCULATEMUONASYMMETRYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/CalculateMuonAsymmetry.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using Mantid::MantidVec;
using Mantid::Algorithms::CalculateMuonAsymmetry;

const std::string outputName = "CalculateMuonAsymmetry_Output";

namespace {

struct yData {
  double operator()(const double x, size_t) {

    // Create a fake muon dataset
    double a = 10.0; // Amplitude of the oscillations
    double w = 5.0;  // Frequency of the oscillations
    double tau = Mantid::PhysicalConstants::MuonLifetime *
                 1e6; // Muon life time in microseconds
    double phi = 0.1;
    double e = exp(-x / tau);
    return (20. * (1.0 + a * cos(w * x + phi)) * e);
  }
};

struct eData {
  double operator()(const double, size_t) { return 0.005; }
};

struct yAsymmData {
  double operator()(const double x, size_t spec) {

    // Create a fake muon dataset
    double a = 1.20; // Amplitude of the oscillations
    double w = 5.0;  // Frequency of the oscillations
    double tau = Mantid::PhysicalConstants::MuonLifetime *
                 1e6; // Muon life time in microseconds
    double phi = 0.1;
    double e = exp(-x / tau);
    double factor = (static_cast<double>(spec) + 1.0) * 0.5;
    return (20. * factor * (1.0 + a * cos(w * x * factor + phi)) * e);
  }
};

MatrixWorkspace_sptr createWorkspace(size_t nspec, size_t maxt) {
  MatrixWorkspace_sptr ws =
      WorkspaceCreationHelper::create2DWorkspaceFromFunction(
          yData(), static_cast<int>(nspec), 0.0, 10.0,
          10.0 * (1.0 / static_cast<double>(maxt)), true, eData());
  // Add  number of good frames
  ws->mutableRun().addProperty("goodfrm", 10);
  return ws;
}

IAlgorithm_sptr setUpAlg() {
  IAlgorithm_sptr asymmAlg =
      AlgorithmManager::Instance().create("CalculateMuonAsymmetry");
  asymmAlg->initialize();
  asymmAlg->setChild(true);
  asymmAlg->setProperty("StartX", 0.1);
  asymmAlg->setProperty("EndX", 10.);
  asymmAlg->setProperty(
      "FittingFunction",
      "name=UserFunction,Formula=A*cos(omega*x+phi),A=10,omega=3.0,phi=0.0");
  return asymmAlg;
}

class CalculateMuonAsymmetryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateMuonAsymmetryTest *createSuite() {
    return new CalculateMuonAsymmetryTest();
  }
  static void destroySuite(CalculateMuonAsymmetryTest *suite) { delete suite; }

  CalculateMuonAsymmetryTest() { FrameworkManager::Instance(); }

  void testInit() {
    IAlgorithm_sptr alg = setUpAlg();

    TS_ASSERT(alg->isInitialized());
  }

  void test_Execute() {

    auto ws = createWorkspace(1, 50);

    IAlgorithm_sptr alg = setUpAlg();
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
  }
  void test_EmptySpectrumList() {

    auto ws = createWorkspace(2, 50);
    IAlgorithm_sptr alg = setUpAlg();
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");

    // Test some X values
    const double delta = 0.0001;
    for (int spec = 0; spec <= 1; spec++) {
      TS_ASSERT_EQUALS(outWS->x(spec)[10], 2.000);
      TS_ASSERT_DELTA(outWS->x(spec)[19], 3.800, delta);
      TS_ASSERT_DELTA(outWS->x(spec)[49], 9.800, delta);
      // Test some Y values
      TS_ASSERT_DELTA(outWS->y(spec)[10], -7.8056, delta);
      TS_ASSERT_DELTA(outWS->y(spec)[19], 9.6880, delta);
      TS_ASSERT_DELTA(outWS->y(spec)[49], 3.9431, delta);
      // Test some E values
      TS_ASSERT_DELTA(outWS->e(spec)[10], 0.0006, delta);
      TS_ASSERT_DELTA(outWS->e(spec)[19], 0.0014, delta);
      TS_ASSERT_DELTA(outWS->e(spec)[49], 0.0216, delta);
    }
  }
  void test_SpectrumList() {

    std::vector<MatrixWorkspace_sptr> workspaces;
    workspaces.push_back(createWorkspace(2, 50));
    // First, run the algorithm without specifying any spectrum
    IAlgorithm_sptr alg1 = setUpAlg();
    alg1->setProperty("InputWorkspace", workspaces[0]);
    alg1->setPropertyValue("OutputWorkspace", outputName);
    TS_ASSERT_THROWS_NOTHING(alg1->execute());
    TS_ASSERT(alg1->isExecuted());
    workspaces.push_back(alg1->getProperty("OutputWorkspace"));

    // Then run the algorithm on the second spectrum only
    IAlgorithm_sptr alg2 = setUpAlg();
    alg2->setProperty("InputWorkspace", workspaces[0]);
    alg2->setPropertyValue("OutputWorkspace", outputName);
    alg2->setPropertyValue("Spectra", "1");
    TS_ASSERT_THROWS_NOTHING(alg2->execute());
    TS_ASSERT(alg2->isExecuted());
    workspaces.push_back(alg2->getProperty("OutputWorkspace"));

    for (int j = 0; j < 3; j++) {
      if (j != 0) { // check ws have 2 spectra
        TS_ASSERT_EQUALS(workspaces[j]->getNumberHistograms(),
                         workspaces[0]->getNumberHistograms());
      }
      if (j != 2) { // check check results match
        TS_ASSERT_EQUALS(workspaces[j]->x(j).rawData(),
                         workspaces[2]->x(j).rawData());
        TS_ASSERT_EQUALS(workspaces[j]->y(j).rawData(),
                         workspaces[2]->y(j).rawData());
        TS_ASSERT_EQUALS(workspaces[j]->e(j).rawData(),
                         workspaces[2]->e(j).rawData());
      }
    }
  }
  void test_yUnitLabel() {

    auto ws = createWorkspace(1, 50);

    IAlgorithm_sptr alg = setUpAlg();
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted())

    MatrixWorkspace_sptr result = alg->getProperty("OutputWorkspace");
    TS_ASSERT(result);
    TS_ASSERT_EQUALS(result->YUnitLabel(), "Asymmetry");
  }
  void test_BackwardsRange() {
    auto ws = createWorkspace(1, 50);
    IAlgorithm_sptr alg = setUpAlg();
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("OutputWorkspace", outputName);
    alg->setProperty("StartX", 0.9);
    alg->setProperty("EndX", 0.1);
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
  }
  void test_NoFittingFunction() {

    auto ws = createWorkspace(1, 50);

    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().create("CalculateMuonAsymmetry");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    alg->setProperty("StartX", 0.1);
    alg->setProperty("EndX", 10.);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
    double Delta = 0.0001;
    // First spectrum
    // Test some X values
    TS_ASSERT_DELTA(outWS->x(0)[10], 2.000, Delta);
    TS_ASSERT_DELTA(outWS->x(0)[19], 3.800, Delta);
    TS_ASSERT_DELTA(outWS->x(0)[49], 9.800, Delta);
    // Test some Y values
    TS_ASSERT_DELTA(outWS->y(0)[10], -14.6114, Delta);
    TS_ASSERT_DELTA(outWS->y(0)[19], 20.3760, Delta);
    TS_ASSERT_DELTA(outWS->y(0)[49], 8.8861, Delta);
    // Test some E values
    TS_ASSERT_DELTA(outWS->e(0)[10], 0.0012, Delta);
    TS_ASSERT_DELTA(outWS->e(0)[19], 0.0028, Delta);
    TS_ASSERT_DELTA(outWS->e(0)[49], 0.0433, Delta);
  }

  void test_NumberOfDataPoints() {

    double dx = 10.0 * (1.0 / static_cast<double>(300.0));
    auto fineWS = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        yData(), 1, 0.0, 10.0, dx, true, eData());
    fineWS->mutableRun().addProperty("goodfrm", 10);
    auto coarseWS = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        yData(), 1, dx, 10.0 + dx, 3.0 * dx, true, eData());

    coarseWS->mutableRun().addProperty("goodfrm", 10);

    IAlgorithm_sptr fineAlg = setUpAlg();
    fineAlg->setProperty("InputWorkspace", fineWS);
    fineAlg->setPropertyValue("OutputWorkspace", "fineOutWS");
    TS_ASSERT_THROWS_NOTHING(fineAlg->execute());
    TS_ASSERT(fineAlg->isExecuted());
    MatrixWorkspace_sptr fineOutWS = fineAlg->getProperty("OutputWorkspace");

    IAlgorithm_sptr coarseAlg = setUpAlg();
    coarseAlg->setProperty("InputWorkspace", coarseWS);
    coarseAlg->setPropertyValue("OutputWorkspace", "coarseOutWS");
    TS_ASSERT_THROWS_NOTHING(coarseAlg->execute());
    TS_ASSERT(coarseAlg->isExecuted());
    MatrixWorkspace_sptr coarseOutWS =
        coarseAlg->getProperty("OutputWorkspace");

    double Delta = 0.0001;
    for (int j = 0; j < 28; j++) {
      // Test some X values
      TS_ASSERT_DELTA(fineOutWS->x(0)[1 + j * 3], coarseOutWS->x(0)[j], Delta);
      // Test some Y values
      TS_ASSERT_DELTA(fineOutWS->y(0)[1 + j * 3], coarseOutWS->y(0)[j], Delta);
      // Test some E values
      TS_ASSERT_DELTA(fineOutWS->e(0)[1 + j * 3], coarseOutWS->e(0)[j], Delta);
    }
  }
};

class CalculateMuonAsymmetryTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateMuonAsymmetryTestPerformance *createSuite() {
    return new CalculateMuonAsymmetryTestPerformance();
  }
  static void destroySuite(CalculateMuonAsymmetryTestPerformance *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  CalculateMuonAsymmetryTestPerformance() { FrameworkManager::Instance(); }

  void setUp() override { input = createWorkspace(1000, 100); }

  void testExec2D() {
    CalculateMuonAsymmetry alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", input);
    alg.setPropertyValue("OutputWorkspace", "output");
    alg.setProperty("StartX", 0.1);
    alg.setProperty("EndX", 10.);
    alg.setProperty(
        "FittingFunction",
        "name=UserFunction,Formula=A*cos(omega*x+phi),A=10,omega=3.0,phi=0.0");

    alg.execute();
  }

private:
  MatrixWorkspace_sptr input;
};
} // close namespace
#endif /*CALCULATEMUONASYMMETRYF_H_*/
