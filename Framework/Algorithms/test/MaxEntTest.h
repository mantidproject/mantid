#ifndef MANTID_ALGORITHMS_MAXENTTEST_H_
#define MANTID_ALGORITHMS_MAXENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

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

  void test_real_data() {
    // Run one iteration, we just want to test the output workspaces' dimensions
    int nHist = 5;
    int nBins = 10;
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(nHist, nBins);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("MaxIterations", "1");
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");

    TS_ASSERT_THROWS_NOTHING(alg->execute());

    MatrixWorkspace_sptr data = alg->getProperty("ReconstructedData");
    MatrixWorkspace_sptr image = alg->getProperty("ReconstructedImage");
    MatrixWorkspace_sptr chi = alg->getProperty("EvolChi");
    MatrixWorkspace_sptr angle = alg->getProperty("EvolAngle");

    TS_ASSERT_EQUALS(data->getNumberHistograms(), nHist * 2);
    TS_ASSERT_EQUALS(image->getNumberHistograms(), nHist * 2);
    TS_ASSERT_EQUALS(chi->getNumberHistograms(), nHist);
    TS_ASSERT_EQUALS(angle->getNumberHistograms(), nHist);

    TS_ASSERT_EQUALS(data->blocksize(), nBins);
    TS_ASSERT_EQUALS(image->blocksize(), nBins);
    TS_ASSERT_EQUALS(chi->blocksize(), 1);
    TS_ASSERT_EQUALS(angle->blocksize(), 1);
  }

  void test_complex_data() {
    // Run one iteration, we just want to test the output workspaces' dimensions
    int nHist = 6;
    int nBins = 10;
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(nHist, nBins);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("ComplexData", true);
    alg->setPropertyValue("MaxIterations", "1");
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");

    TS_ASSERT_THROWS_NOTHING(alg->execute());

    MatrixWorkspace_sptr data = alg->getProperty("ReconstructedData");
    MatrixWorkspace_sptr image = alg->getProperty("ReconstructedImage");
    MatrixWorkspace_sptr chi = alg->getProperty("EvolChi");
    MatrixWorkspace_sptr angle = alg->getProperty("EvolAngle");

    TS_ASSERT_EQUALS(data->getNumberHistograms(), nHist);
    TS_ASSERT_EQUALS(image->getNumberHistograms(), nHist);
    TS_ASSERT_EQUALS(chi->getNumberHistograms(), nHist / 2);
    TS_ASSERT_EQUALS(angle->getNumberHistograms(), nHist / 2);

    TS_ASSERT_EQUALS(data->blocksize(), nBins);
    TS_ASSERT_EQUALS(image->blocksize(), nBins);
    TS_ASSERT_EQUALS(chi->blocksize(), 1);
    TS_ASSERT_EQUALS(angle->blocksize(), 1);
  }

  void test_bad_complex_data() {

    auto ws = WorkspaceCreationHelper::Create2DWorkspace(5, 10);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("ComplexData", true);
    alg->setPropertyValue("MaxIterations", "1");
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");

    TS_ASSERT_THROWS_ANYTHING(alg->execute());
  }

  void test_cosine() {

    auto ws = createWorkspaceReal(50, 0.0);

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

    auto ws = createWorkspaceReal(50, M_PI / 2.);

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

  void test_sine_cosine_neg() {
    // Complex signal: cos(w * x) + i sin(w * x)
    // PosNeg images

    auto ws = createWorkspaceComplex();

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("ComplexData", true);
    alg->setProperty("A", 1.0);
    alg->setProperty("ChiTarget", 102.);
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");

    TS_ASSERT_THROWS_NOTHING(alg->execute());

    MatrixWorkspace_sptr data = alg->getProperty("ReconstructedData");
    TS_ASSERT(data);

    // Test some values
    TS_ASSERT_DELTA(data->readY(0)[35], 0.8489, 0.0001);
    TS_ASSERT_DELTA(data->readY(0)[36], 0.6727, 0.0001);
    TS_ASSERT_DELTA(data->readY(0)[37], 0.4058, 0.0001);
    TS_ASSERT_DELTA(data->readY(1)[35], 0.3288, 0.0001);
    TS_ASSERT_DELTA(data->readY(1)[36], 0.6133, 0.0001);
    TS_ASSERT_DELTA(data->readY(1)[37], 0.8149, 0.0001);
  }

  void test_sine_cosine_pos() {
    // Complex signal: cos(w * x) + i sin(w * x)
    // Positive images

    auto ws = createWorkspaceComplex();

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("ComplexData", true);
    alg->setProperty("PositiveImage", true);
    alg->setProperty("A", 0.01);
    alg->setProperty("ChiTarget", 102.);
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");

    TS_ASSERT_THROWS_NOTHING(alg->execute());

    MatrixWorkspace_sptr data = alg->getProperty("ReconstructedData");
    TS_ASSERT(data);

    // Test some values
    TS_ASSERT_DELTA(data->readY(0)[35], 0.8295, 0.0001);
    TS_ASSERT_DELTA(data->readY(0)[36], 0.6735, 0.0001);
    TS_ASSERT_DELTA(data->readY(0)[37], 0.3935, 0.0001);
    TS_ASSERT_DELTA(data->readY(1)[35], 0.3266, 0.0001);
    TS_ASSERT_DELTA(data->readY(1)[36], 0.6101, 0.0001);
    TS_ASSERT_DELTA(data->readY(1)[37], 0.8074, 0.0001);
  }

  MatrixWorkspace_sptr createWorkspaceReal(size_t maxt, double phase) {

    // Create cosine with phase 'phase'

    // Frequency of the oscillations
    double w = 1.6;

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

  MatrixWorkspace_sptr createWorkspaceComplex() {

    const size_t size = 51;

    // x = 2 * pi * i / N
    std::array<double, 51> vecx = {
        {0.0000, 0.1232, 0.2464, 0.3696, 0.4928, 0.6160, 0.7392, 0.8624, 0.9856,
         1.1088, 1.2320, 1.3552, 1.4784, 1.6016, 1.7248, 1.8480, 1.9712, 2.0944,
         2.2176, 2.3408, 2.4640, 2.5872, 2.7104, 2.8336, 2.9568, 3.0800, 3.2032,
         3.3264, 3.4496, 3.5728, 3.6960, 3.8192, 3.9424, 4.0656, 4.1888, 4.3120,
         4.4352, 4.5584, 4.6816, 4.8048, 4.9280, 5.0512, 5.1744, 5.2976, 5.4208,
         5.5440, 5.6672, 5.7904, 5.9136, 6.0368, 6.1600}};
    std::array<double, 51> vecyRe = {
        {1.07,  0.95,  0.84,  0.51,  -0.04, -0.42, -0.47, -0.98, -0.96,
         -1.03, -0.71, -0.70, -0.13, -0.04, 0.59,  0.84,  0.91,  0.93,
         1.03,  0.75,  0.40,  0.18,  -0.24, -0.48, -0.78, -0.95, -0.94,
         -0.87, -0.46, -0.19, 0.13,  0.35,  0.88,  1.01,  0.92,  0.79,
         0.80,  0.44,  0.15,  -0.26, -0.49, -0.79, -0.84, -1.04, -0.80,
         -0.73, -0.26, 0.09,  0.45,  0.67,  0.92}};
    std::array<double, 51> vecyIm = {
        {0.07,  0.25,  0.82,  0.75,  1.08,  0.84,  0.82,  0.62,  0.33,
         -0.20, -0.58, -0.88, -0.85, -1.10, -0.77, -0.59, -0.36, 0.13,
         0.39,  0.62,  0.87,  1.03,  0.82,  0.94,  0.47,  0.30,  -0.22,
         -0.39, -0.86, -0.91, -0.88, -0.84, -0.59, -0.27, 0.14,  0.36,
         0.69,  0.98,  0.98,  0.95,  0.71,  0.41,  0.32,  -0.13, -0.53,
         -0.74, -0.82, -0.91, -0.82, -0.60, -0.32}};
    std::array<double, 51> vece = {
        {0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1,
         0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1,
         0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1,
         0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1}};

    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceFactory::Instance().create("Workspace2D", 2, size, size));

    for (size_t i = 0; i < size; i++) {
      ws->dataX(0)[i] = vecx[i];
      ws->dataX(1)[i] = vecx[i];
      ws->dataY(0)[i] = vecyRe[i];
      ws->dataY(1)[i] = vecyIm[i];
      ws->dataE(0)[i] = vece[i];
      ws->dataE(1)[i] = vece[i];
    }

    return ws;
  }
};

#endif /* MANTID_ALGORITHMS_MAXENTTEST_H_ */