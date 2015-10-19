#ifndef MUONREMOVEEXPDECAYTEST_H_
#define MUONREMOVEEXPDECAYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using Mantid::MantidVec;

const std::string outputName = "MuonRemoveExpDecay_Output";

class RemoveExpDecayTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RemoveExpDecayTest *createSuite() { return new RemoveExpDecayTest(); }
  static void destroySuite(RemoveExpDecayTest *suite) { delete suite; }

  RemoveExpDecayTest() { FrameworkManager::Instance(); }

  void testInit() {
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("RemoveExpDecay");
    alg->initialize();
    TS_ASSERT(alg->isInitialized())
  }

  void test_Execute() {

    auto ws = createWorkspace(1, 50);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("RemoveExpDecay");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
  }

  void test_EmptySpectrumList() {

    auto ws = createWorkspace(2, 50);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("RemoveExpDecay");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");

    // First spectrum
    // Test some X values
    TS_ASSERT_DELTA(outWS->readX(0)[10], 0.2000, 0.0001);
    TS_ASSERT_DELTA(outWS->readX(0)[19], 0.3800, 0.0001);
    TS_ASSERT_DELTA(outWS->readX(0)[49], 0.9800, 0.0001);
    // Test some Y values
    TS_ASSERT_DELTA(outWS->readY(0)[10], -0.0992, 0.0001);
    TS_ASSERT_DELTA(outWS->readY(0)[19], -0.0111, 0.0001);
    TS_ASSERT_DELTA(outWS->readY(0)[49], -0.0622, 0.0001);
    // Test some E values
    TS_ASSERT_DELTA(outWS->readE(0)[10], 0.0054, 0.0001);
    TS_ASSERT_DELTA(outWS->readE(0)[19], 0.0059, 0.0001);
    TS_ASSERT_DELTA(outWS->readE(0)[49], 0.0077, 0.0001);

    // Second spectrum
    // Test some X values
    TS_ASSERT_DELTA(outWS->readX(1)[10], 0.2000, 0.0001);
    TS_ASSERT_DELTA(outWS->readX(1)[19], 0.3800, 0.0001);
    TS_ASSERT_DELTA(outWS->readX(1)[49], 0.9800, 0.0001);
    // Test some Y values
    TS_ASSERT_DELTA(outWS->readY(1)[10], 0.0274, 0.0001);
    TS_ASSERT_DELTA(outWS->readY(1)[19], -0.1003, 0.0001);
    TS_ASSERT_DELTA(outWS->readY(1)[49], 0.0802, 0.0001);
    // Test some E values
    TS_ASSERT_DELTA(outWS->readE(1)[10], 0.0054, 0.0001);
    TS_ASSERT_DELTA(outWS->readE(1)[19], 0.0059, 0.0001);
    TS_ASSERT_DELTA(outWS->readE(1)[49], 0.0078, 0.0001);
  }

  void test_SpectrumList() {

    auto ws = createWorkspace(2, 50);

    // First, run the algorithm without specifying any spectrum
    IAlgorithm_sptr alg1 =
        AlgorithmManager::Instance().create("RemoveExpDecay");
    alg1->initialize();
    alg1->setChild(true);
    alg1->setProperty("InputWorkspace", ws);
    alg1->setPropertyValue("OutputWorkspace", outputName);
    TS_ASSERT_THROWS_NOTHING(alg1->execute());
    TS_ASSERT(alg1->isExecuted());
    MatrixWorkspace_sptr out1 = alg1->getProperty("OutputWorkspace");

    // Then run the algorithm on the second spectrum only
    IAlgorithm_sptr alg2 =
        AlgorithmManager::Instance().create("RemoveExpDecay");
    alg2->initialize();
    alg2->setChild(true);
    alg2->setProperty("InputWorkspace", ws);
    alg2->setPropertyValue("OutputWorkspace", outputName);
    alg2->setPropertyValue("Spectra", "1");
    TS_ASSERT_THROWS_NOTHING(alg2->execute());
    TS_ASSERT(alg2->isExecuted());
    MatrixWorkspace_sptr out2 = alg2->getProperty("OutputWorkspace");

    // Both output workspaces should have 2 spectra
    TS_ASSERT_EQUALS(out1->getNumberHistograms(), ws->getNumberHistograms());
    TS_ASSERT_EQUALS(out2->getNumberHistograms(), ws->getNumberHistograms());

    // Compare results, they should match for the selected spectrum
    TS_ASSERT_EQUALS(out1->readX(1), out2->readX(1));
    TS_ASSERT_EQUALS(out1->readY(1), out2->readY(1));
    TS_ASSERT_EQUALS(out1->readE(1), out2->readE(1));

    // Compare non-selected spectra, the should match the input ones
    TS_ASSERT_EQUALS(ws->readX(0), out2->readX(0));
    TS_ASSERT_EQUALS(ws->readY(0), out2->readY(0));
    TS_ASSERT_EQUALS(ws->readE(0), out2->readE(0));
  }

  void test_yUnitLabel() {

    auto ws = createWorkspace(4, 50);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("RemoveExpDecay");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted())

    MatrixWorkspace_sptr result = alg->getProperty("OutputWorkspace");
    TS_ASSERT(result);
    TS_ASSERT_EQUALS(result->YUnitLabel(), "Asymmetry");
  }

  MatrixWorkspace_sptr createWorkspace(size_t nspec, size_t maxt) {

    // Create a fake muon dataset
    double a = 0.1;   // Amplitude of the oscillations
    double w = 25.;   // Frequency of the oscillations
    double tau = 2.2; // Muon life time

    MantidVec X;
    MantidVec Y;
    MantidVec E;
    for (size_t s = 0; s < nspec; s++) {
      for (size_t t = 0; t < maxt; t++) {
        double x = static_cast<double>(t) / static_cast<double>(maxt);
        double e = exp(-x / tau);
        X.push_back(x);
        Y.push_back(a * sin(w * x +
                            static_cast<double>(s) * M_PI /
                                static_cast<double>(nspec)) *
                        e +
                    e);
        E.push_back(0.005);
      }
    }

    auto createWS = AlgorithmManager::Instance().create("CreateWorkspace");
    createWS->initialize();
    createWS->setChild(true);
    createWS->setProperty("DataX", X);
    createWS->setProperty("DataY", Y);
    createWS->setProperty("DataE", E);
    createWS->setProperty("NSpec", static_cast<int>(nspec));
    createWS->setPropertyValue("OutputWorkspace", "ws");
    createWS->execute();
    MatrixWorkspace_sptr ws = createWS->getProperty("OutputWorkspace");

    return ws;
  }
};

#endif /*MUONREMOVEEXPDECAYTEST_H_*/
