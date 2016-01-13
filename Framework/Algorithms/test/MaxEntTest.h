#ifndef MANTID_ALGORITHMS_MAXENTTEST_H_
#define MANTID_ALGORITHMS_MAXENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"

using namespace Mantid::API;
using Mantid::MantidVec;

class MaxEntTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaxEntTest *createSuite() { return new MaxEntTest(); }
  static void destroySuite(MaxEntTest *suite) { delete suite; }

  void test_init() {
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    TS_ASSERT(alg->isInitialized())
  }

  void test_cosine() {

    auto ws = createWorkspace(0.0);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("A", 0.01);
    alg->setProperty("ChiTarget", 50.);
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");

    TS_ASSERT_THROWS_NOTHING(alg->execute());

    MatrixWorkspace_sptr data = alg->getProperty("ReconstructedData");
    MatrixWorkspace_sptr image = alg->getProperty("ReconstructedImage");
    MatrixWorkspace_sptr chi = alg->getProperty("EvolChi");
    MatrixWorkspace_sptr angle = alg->getProperty("EvolAngle");

    TS_ASSERT(data);
    TS_ASSERT(image);
    TS_ASSERT(chi);
    TS_ASSERT(angle);

    // Test some values
    TS_ASSERT_DELTA(data->readY(0)[25], 0.3112, 0.0001);
    TS_ASSERT_DELTA(data->readY(0)[26], 0.4893, 0.0001);
    TS_ASSERT_DELTA(data->readY(0)[27], 0.6485, 0.0001);
  }

  void test_sine() {

    auto ws = createWorkspace(M_PI / 2.);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("A", 0.01);
    alg->setProperty("ChiTarget", 50.);
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");

    TS_ASSERT_THROWS_NOTHING(alg->execute());

    MatrixWorkspace_sptr data = alg->getProperty("ReconstructedData");
    MatrixWorkspace_sptr image = alg->getProperty("ReconstructedImage");
    MatrixWorkspace_sptr chi = alg->getProperty("EvolChi");
    MatrixWorkspace_sptr angle = alg->getProperty("EvolAngle");

    TS_ASSERT(data);
    TS_ASSERT(image);
    TS_ASSERT(chi);
    TS_ASSERT(angle);

    // Test some values
    TS_ASSERT_DELTA(data->readY(0)[25], 0.8961, 0.0001);
    TS_ASSERT_DELTA(data->readY(0)[26], 0.8257, 0.0001);
    TS_ASSERT_DELTA(data->readY(0)[27], 0.7218, 0.0001);
  }

  MatrixWorkspace_sptr createWorkspace(double phase) {

    // Create cosine with phase 'phase'

    // Frequency of the oscillations
    double w = 1.6;
    // Maximum time
    size_t maxt = 50;

    MantidVec X;
    MantidVec Y;
    MantidVec E;
    for (size_t t = 0; t < maxt; t++) {
      double x = 2. * M_PI * static_cast<double>(t) / static_cast<double>(maxt);
      X.push_back(x);
      Y.push_back(cos(w * x + phase));
      E.push_back(0.1);
    }

    auto createWS = AlgorithmManager::Instance().create("CreateWorkspace");
    createWS->initialize();
    createWS->setChild(true);
    createWS->setProperty("DataX", X);
    createWS->setProperty("DataY", Y);
    createWS->setProperty("DataE", E);
    createWS->setPropertyValue("OutputWorkspace", "ws");
    createWS->execute();
    MatrixWorkspace_sptr ws = createWS->getProperty("OutputWorkspace");

    return ws;
  }
};

#endif /* MANTID_ALGORITHMS_MAXENTTEST_H_ */