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

  void testExecute() {

    auto ws = createWorkspace(4, 50);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("RemoveExpDecay");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    alg->setPropertyValue("Spectra", "0");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
  }

  void testExecuteWhereSepctraNotSet() {

    auto ws = createWorkspace(4, 50);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("RemoveExpDecay");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted())

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
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
