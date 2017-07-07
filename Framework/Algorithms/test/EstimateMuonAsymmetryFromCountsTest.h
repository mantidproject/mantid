#ifndef ESTIMATEMUONASYMMETRYFROMCOUNTSTEST_H_
#define ESTIMATEMUONASYMMETRYFROMCOUNTSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/EstimateMuonAsymmetryFromCounts.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using Mantid::MantidVec;
using Mantid::Algorithms::EstimateMuonAsymmetryFromCounts;

const std::string outputName = "EstimateMuonAsymmetryFromCounts_Output";

namespace {
struct yData {
  double operator()(const double x, size_t) {
    // Create a fake muon dataset
    double a = 0.1; // Amplitude of the oscillations
    double w = 25.; // Frequency of the oscillations
    double tau = Mantid::PhysicalConstants::MuonLifetime *
                 1e6; // Muon life time in microseconds
    double phi = 0.05;
    double e = exp(-x / tau);
    return (20. * (1.0 + a * cos(w * x + phi)) * e);
  }
};

struct eData {
  double operator()(const double, size_t) { return 0.005; }
};

MatrixWorkspace_sptr createWorkspace(size_t nspec, size_t maxt) {
  MatrixWorkspace_sptr ws =
      WorkspaceCreationHelper::create2DWorkspaceFromFunction(
          yData(), static_cast<int>(nspec), 0.0, 1.0,
          (1.0 / static_cast<double>(maxt)), true, eData());
  // Add  number of good frames
  ws->mutableRun().addProperty("goodfrm", 10);
  return ws;
}

IAlgorithm_sptr setUpAlg() {
  IAlgorithm_sptr asymmAlg =
      AlgorithmManager::Instance().create("EstimateMuonAsymmetryFromCounts");
  asymmAlg->initialize();
  asymmAlg->setChild(true);
  asymmAlg->setProperty("StartX", 0.1);
  asymmAlg->setProperty("EndX", 0.9);
  return asymmAlg;
}
}

class EstimateMuonAsymmetryFromCountsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EstimateMuonAsymmetryFromCountsTest *createSuite() {
    return new EstimateMuonAsymmetryFromCountsTest();
  }
  static void destroySuite(EstimateMuonAsymmetryFromCountsTest *suite) {
    delete suite;
  }

  EstimateMuonAsymmetryFromCountsTest() { FrameworkManager::Instance(); }

  void testInit() {
    IAlgorithm_sptr alg = setUpAlg();
    TS_ASSERT(alg->isInitialized())
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

    double Delta = 0.0001;
    for (int j = 0; j < 2; j++) {
      // Test some X values
      TS_ASSERT_DELTA(outWS->x(j)[10], 0.2000, Delta);
      TS_ASSERT_DELTA(outWS->x(j)[19], 0.3800, Delta);
      TS_ASSERT_DELTA(outWS->x(j)[49], 0.9800, Delta);
      // Test some Y values
      TS_ASSERT_DELTA(outWS->y(j)[10], 0.0366, Delta);
      TS_ASSERT_DELTA(outWS->y(j)[19], -0.0961, Delta);
      TS_ASSERT_DELTA(outWS->y(j)[49], 0.0871, Delta);
      // Test some E values
      TS_ASSERT_DELTA(outWS->e(j)[10], 0.0002, Delta);
      TS_ASSERT_DELTA(outWS->e(j)[19], 0.0003, Delta);
      TS_ASSERT_DELTA(outWS->e(j)[49], 0.0004, Delta);
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
      if (j != 0) { // check we have 2 spectra
        TS_ASSERT_EQUALS(workspaces[j]->getNumberHistograms(),
                         workspaces[0]->getNumberHistograms());
      }
      if (j != 2) { // check results match
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
    alg->setProperty("OutputWorkspace", outputName);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted())
    MatrixWorkspace_sptr result = alg->getProperty("OutputWorkspace");
    TS_ASSERT(result);
    TS_ASSERT_EQUALS(result->YUnitLabel(), "Asymmetry");
  }
  void test_NoRange() {
    auto ws = createWorkspace(1, 50);
    IAlgorithm_sptr alg = setUpAlg();
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("StartX", 0.1);
    alg->setProperty("EndX", 0.1);
    alg->setProperty("OutputWorkspace", outputName);
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
  }
  void test_BackwardsRange() {
    auto ws = createWorkspace(1, 50);
    IAlgorithm_sptr alg = setUpAlg();
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("StartX", 0.9);
    alg->setProperty("EndX", 0.1);
    alg->setProperty("OutputWorkspace", outputName);
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
  }
  void test_NumberOfDataPoints() {

    double dx = (1.0 / 300.0);
    auto fineWS = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        yData(), 1, 0.0, 1.0, dx, true, eData());
    fineWS->mutableRun().addProperty("goodfrm", 10);
    auto coarseWS = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        yData(), 1, dx, 1.0 + dx, 3.0 * dx, true, eData());

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

    double Delta = 0.05; // only expect numbers to be similar
    for (int j = 0; j < 28; j++) {
      // Test some X values
      TS_ASSERT_DELTA(fineOutWS->x(0)[1 + j * 3], coarseOutWS->x(0)[j], Delta);
      // Test some Y values
      TS_ASSERT_DELTA(fineOutWS->y(0)[1 + j * 3], coarseOutWS->y(0)[j], Delta);
      // Test some E values
      TS_ASSERT_DELTA(fineOutWS->e(0)[1 + j * 3], coarseOutWS->e(0)[j], Delta);
    }
  }
  void test_UserDefinedNorm() {

    auto ws = createWorkspace(1, 50);
    double userNorm = 10.2;
    IAlgorithm_sptr alg = setUpAlg();
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    alg->setProperty("NormalizationIn", userNorm);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
    auto normFromAlg =
        Mantid::Kernel::VectorHelper::splitStringIntoVector<double>(
            alg->getPropertyValue("NormalizationConstant"));

    double Delta = 0.0001;
    TS_ASSERT_DELTA(normFromAlg[0], userNorm, Delta);
    // Test some X values
    TS_ASSERT_DELTA(outWS->x(0)[10], 0.2000, Delta);
    TS_ASSERT_DELTA(outWS->x(0)[19], 0.3800, Delta);
    TS_ASSERT_DELTA(outWS->x(0)[49], 0.9800, Delta);
    // Test some Y values
    TS_ASSERT_DELTA(outWS->y(0)[10], -0.7974, Delta);
    TS_ASSERT_DELTA(outWS->y(0)[19], -0.8233, Delta);
    TS_ASSERT_DELTA(outWS->y(0)[49], -0.7875, Delta);
  }
};
// turn clang off, otherwise this does not compile
// clang-format off
class EstimateMuonAsymmetryFromCountsTestPerformance : public CxxTest::TestSuite {
  // clang-format on
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EstimateMuonAsymmetryFromCountsTestPerformance *createSuite() {
    return new EstimateMuonAsymmetryFromCountsTestPerformance();
  }
  // clang-format off
  static void  destroySuite(EstimateMuonAsymmetryFromCountsTestPerformance *suite) {
    // clang-format on
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  EstimateMuonAsymmetryFromCountsTestPerformance() {
    FrameworkManager::Instance();
  }

  void setUp() override { input = createWorkspace(1000, 100); }

  void testExec2D() {
    EstimateMuonAsymmetryFromCounts alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", input);
    alg.setPropertyValue("OutputWorkspace", "output");
    alg.setProperty("StartX", 0.1);
    alg.setProperty("EndX", 0.9);

    alg.execute();
  }

private:
  MatrixWorkspace_sptr input;
};
#endif /*ESTIMATEMUONASYMMETRYFROMCOUNTSTEST_H_*/
