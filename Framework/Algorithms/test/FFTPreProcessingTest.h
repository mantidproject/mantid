#ifndef FFTPREPROCESSINGTEST_H_
#define FFTPREPROCESSINGTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/FFTPreProcessing.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using Mantid::MantidVec;
using Mantid::Algorithms::FFTPreProcessing;

const std::string outputName = "FFTPReProcessing_Output";

namespace {
struct yData {
  double operator()(const double x, size_t) { return x; }
};

struct eData {
  double operator()(const double, size_t) { return 0.005; }
};
MatrixWorkspace_sptr createWorkspace(size_t nspec, size_t maxt) {
  MatrixWorkspace_sptr ws =
      WorkspaceCreationHelper::create2DWorkspaceFromFunction(
          yData(), static_cast<int>(nspec), 0.0, 10.0,
          10.0 * (1.0 / static_cast<double>(maxt)), true, eData());
  return ws;
}

IAlgorithm_sptr setUpAlg() {
  IAlgorithm_sptr FFTPreProcess =
      AlgorithmManager::Instance().create("FFTPreProcessing");
  FFTPreProcess->initialize();
  FFTPreProcess->setChild(true);
  FFTPreProcess->setProperty("DecayConstant", 2.0);
  return FFTPreProcess;
}
}

class FFTPreProcessingTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FFTPreProcessingTest *createSuite() {
    return new FFTPreProcessingTest();
  }
  static void destroySuite(FFTPreProcessingTest *suite) { delete suite; }

  FFTPreProcessingTest() { FrameworkManager::Instance(); }

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
      TS_ASSERT_DELTA(outWS->x(j)[10], 2.000, Delta);
      TS_ASSERT_DELTA(outWS->x(j)[19], 3.800, Delta);
      TS_ASSERT_DELTA(outWS->x(j)[49], 9.800, Delta);
      // Test some Y values
      TS_ASSERT_DELTA(outWS->y(j)[10], 2.000, Delta);
      TS_ASSERT_DELTA(outWS->y(j)[19], 3.800, Delta);
      TS_ASSERT_DELTA(outWS->y(j)[49], 9.800, Delta);
      // Test some E values
      TS_ASSERT_DELTA(outWS->e(j)[10], 0.005, Delta);
      TS_ASSERT_DELTA(outWS->e(j)[19], 0.005, Delta);
      TS_ASSERT_DELTA(outWS->e(j)[49], 0.005, Delta);
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
    TS_ASSERT_THROWS_NOTHING(alg2->execute());
    TS_ASSERT(alg2->isExecuted());
    workspaces.push_back(alg2->getProperty("OutputWorkspace"));

    for (int j = 0; j < 3; j++) {
      if (j != 0) { // check we have 2 spectra
        TS_ASSERT_EQUALS(workspaces[j]->getNumberHistograms(),
                         workspaces[0]->getNumberHistograms());
      }
      if (j == 1) { // check results match
        TS_ASSERT_EQUALS(workspaces[j]->x(j).rawData(),
                         workspaces[2]->x(j).rawData());
        TS_ASSERT_EQUALS(workspaces[j]->y(j).rawData(),
                         workspaces[2]->y(j).rawData());
        TS_ASSERT_EQUALS(workspaces[j]->e(j).rawData(),
                         workspaces[2]->e(j).rawData());
      }
    }
  }
  void test_Lorentz() {

    auto ws = createWorkspace(1, 50);
    IAlgorithm_sptr alg = setUpAlg();
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    alg->setProperty("ApodizationFunction", "Lorentz");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");

    double Delta = 0.0001;
    // Test some X values
    TS_ASSERT_DELTA(outWS->x(0)[10], 2.000, Delta);
    TS_ASSERT_DELTA(outWS->x(0)[19], 3.800, Delta);
    TS_ASSERT_DELTA(outWS->x(0)[49], 9.800, Delta);
    // Test some Y values
    TS_ASSERT_DELTA(outWS->y(0)[10], exp(-1.0) * 2.0, Delta);
    TS_ASSERT_DELTA(outWS->y(0)[19], exp(-3.8 / 2.) * 3.8, Delta);
    TS_ASSERT_DELTA(outWS->y(0)[49], exp(-9.8 / 2.) * 9.8, Delta);
  }
  void test_Gaussian() {

    auto ws = createWorkspace(1, 50);
    IAlgorithm_sptr alg = setUpAlg();
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    alg->setProperty("ApodizationFunction", "Gaussian");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");

    double Delta = 0.0001;
    // Test some X values
    TS_ASSERT_DELTA(outWS->x(0)[10], 2.000, Delta);
    TS_ASSERT_DELTA(outWS->x(0)[19], 3.800, Delta);
    TS_ASSERT_DELTA(outWS->x(0)[49], 9.800, Delta);
    // Test some Y values
    TS_ASSERT_DELTA(outWS->y(0)[10], 0.606531 * 2.0, Delta);
    TS_ASSERT_DELTA(outWS->y(0)[19], 0.164474 * 3.8, Delta);
    TS_ASSERT_DELTA(outWS->y(0)[49], 6.11e-6 * 9.8, Delta);
  }
  void test_PaddingOne() {

    auto ws = createWorkspace(1, 50);
    IAlgorithm_sptr alg = setUpAlg();
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    alg->setProperty("Padding", 1);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");

    double Delta = 0.0001;
    // Test padding is applied
    TS_ASSERT_EQUALS(outWS->x(0).size(), 100);
    TS_ASSERT_DELTA(outWS->y(0)[ws->x(0).size()], 0.0, Delta);
  }
  void test_PaddingTwelve() {

    auto ws = createWorkspace(1, 50);
    IAlgorithm_sptr alg = setUpAlg();
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    alg->setProperty("Padding", 12);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");

    double Delta = 0.0001;
    // Test padding is applied
    TS_ASSERT_EQUALS(outWS->x(0).size(), 650);
    TS_ASSERT_DELTA(outWS->y(0)[ws->x(0).size()], 0.0, Delta);
    TS_ASSERT_DELTA(outWS->y(0)[ws->x(0).size() * 4], 0.0, Delta);
  }

  void test_PaddingOneBothSides() {

    auto ws = createWorkspace(1, 50);
    IAlgorithm_sptr alg = setUpAlg();
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    alg->setProperty("Padding", 1);
    alg->setProperty("NegativePAdding", true);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");

    double Delta = 0.0001;
    // Test padding is applied
    TS_ASSERT_EQUALS(outWS->x(0).size(), 100);
    TS_ASSERT_DELTA(outWS->y(0)[1], 0.0, Delta);
    TS_ASSERT_DELTA(outWS->y(0)[75], 0.0, Delta);
  }
  void test_PaddingTwelveBoth() {

    auto ws = createWorkspace(1, 50);
    IAlgorithm_sptr alg = setUpAlg();
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    alg->setProperty("Padding", 12);
    alg->setProperty("NegativePAdding", true);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");

    double Delta = 0.0001;
    // Test padding is applied
    TS_ASSERT_EQUALS(outWS->x(0).size(), 650);
    TS_ASSERT_DELTA(outWS->y(0)[0], 0.0, Delta);
    TS_ASSERT_DELTA(outWS->y(0)[300], ws->y(0)[0], Delta);
    TS_ASSERT_DELTA(outWS->y(0)[350], 0.0, Delta);
  }
};

#endif /*FFTPREPROCESSINGTSTEST_H_*/
