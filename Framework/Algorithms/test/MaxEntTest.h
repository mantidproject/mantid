// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MAXENTTEST_H_
#define MANTID_ALGORITHMS_MAXENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/MaxEnt.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::Points;
using Mantid::MantidVec;

/**
 * This is a test class that exists to test the method validateInputs()
 */
class TestMaxEnt : public Mantid::Algorithms::MaxEnt {
public:
  std::map<std::string, std::string> wrapValidateInputs() {
    return this->validateInputs();
  }
};

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

  void test_sizes_for_real_data() {
    // Run one iteration, we just want to test the output workspaces' dimensions
    int nHist = 5;
    int nBins = 10;
    auto ws = WorkspaceCreationHelper::create2DWorkspace(nHist, nBins);

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

  void test_sizes_for_complex_data() {
    // Run one iteration, we just want to test the output workspaces' dimensions
    int nHist = 6;
    int nBins = 10;
    auto ws = WorkspaceCreationHelper::create2DWorkspace(nHist, nBins);

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

  void test_sizes_for_complex_data_adjustments() {
    // Run one iteration, we just want to test the output workspaces' dimensions
    int nHist = 6;
    int nBins = 10;
    auto ws = WorkspaceCreationHelper::create2DWorkspace(nHist, nBins);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("ComplexData", true);
    alg->setPropertyValue("MaxIterations", "1");
    alg->setProperty("DataLinearAdj", ws);
    alg->setProperty("DataConstAdj", ws);
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

  void test_sizes_for_complex_data_adjustments_together() {
    int nHist = 6;
    int nBins = 10;
    auto ws = WorkspaceCreationHelper::create2DWorkspace(nHist, nBins);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("ComplexData", true);
    alg->setPropertyValue("MaxIterations", "1");
    alg->setProperty("DataLinearAdj", ws);
    alg->setProperty("DataConstAdj", ws);
    alg->setProperty("PerSpectrumReconstruction", false);
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
    TS_ASSERT_EQUALS(image->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(chi->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(angle->getNumberHistograms(), 1);

    TS_ASSERT_EQUALS(data->blocksize(), nBins);
    TS_ASSERT_EQUALS(image->blocksize(), nBins);
    TS_ASSERT_EQUALS(chi->blocksize(), 1);
    TS_ASSERT_EQUALS(angle->blocksize(), 1);
  }

  void test_bad_complex_data() {

    auto ws = WorkspaceCreationHelper::create2DWorkspace(5, 10);

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

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void test_bad_linear_adjustment() {

    auto ws = WorkspaceCreationHelper::create2DWorkspace(5, 10);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("ComplexData", false);
    alg->setPropertyValue("MaxIterations", "1");
    alg->setProperty("DataLinearAdj", ws);
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void test_bad_const_adjustment() {

    auto ws = WorkspaceCreationHelper::create2DWorkspace(5, 10);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("ComplexData", false);
    alg->setPropertyValue("MaxIterations", "1");
    alg->setProperty("DataConstAdj", ws);
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void test_linear_adjustment_with_too_few_spectra() {

    auto ws = WorkspaceCreationHelper::create2DWorkspace(6, 10);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("ComplexData", false);
    alg->setPropertyValue("MaxIterations", "1");
    alg->setProperty("DataLinearAdj", ws); // We need twice as many histograms
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void test_const_adjustment_with_too_few_spectra() {

    auto ws = WorkspaceCreationHelper::create2DWorkspace(6, 10);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("ComplexData", false);
    alg->setPropertyValue("MaxIterations", "1");
    alg->setProperty("DataConstAdj", ws); // We need twice as many histograms
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void test_adjustments_together_too_few_spectra() {

    auto ws = WorkspaceCreationHelper::create2DWorkspace(6, 10);
    auto ws1 = WorkspaceCreationHelper::create2DWorkspace(2, 10);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("ComplexData", true);
    alg->setPropertyValue("MaxIterations", "1");
    // We need as many spectra in the adjustments as
    // in the input workspace even though images are summed.
    alg->setProperty("DataLinearAdj", ws1);
    alg->setProperty("DataConstAdj", ws1);
    alg->setProperty("perSpectrumReconstruction", false);
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void test_adjustments_together_real_data_not_supported() {

    auto ws = WorkspaceCreationHelper::create2DWorkspace(3, 10);
    auto ws1 = WorkspaceCreationHelper::create2DWorkspace(6, 10);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("MaxIterations", "1");
    alg->setProperty("DataLinearAdj", ws1);
    alg->setProperty("DataConstAdj", ws1);
    // Complex data needed for this
    alg->setProperty("perSpectrumReconstruction", false);
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");

    TS_ASSERT_THROWS(alg->execute(), const std::invalid_argument &);
  }

  void test_adjustment_arithmetic() {
    // Workspace has two spectra of three values all 3+3i
    std::vector<double> wsVal(12, 3.0);
    auto ws = createWorkspaceWithYValues(4, 3, wsVal);

    // First spectrum has no adjustments
    // Second spectrum has mixed adjustments

    // Linear adjustments 2nd spectrum
    // 1, 2i, 2i
    std::vector<double> linAdjVal(12, 0.0);
    linAdjVal[0] = 1.0;
    linAdjVal[1] = 1.0;
    linAdjVal[2] = 1.0;
    linAdjVal[3] = 1.0;
    linAdjVal[10] = 2.0;
    linAdjVal[11] = 2.0;
    auto linAdj = createWorkspaceWithYValues(4, 3, linAdjVal);

    // Const adjustments 2nd spectrum
    // 1-i, 0, 1-i
    std::vector<double> constAdjVal(12, 0.0);
    constAdjVal[3] = 1.0;
    constAdjVal[9] = -1.0;
    constAdjVal[5] = 1.0;
    constAdjVal[11] = -1.0;
    auto constAdj = createWorkspaceWithYValues(4, 3, constAdjVal);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("ComplexData", true);
    alg->setPropertyValue("MaxIterations", "1");
    alg->setProperty("DataLinearAdj", linAdj);
    alg->setProperty("DataConstAdj", constAdj);
    alg->setProperty("perSpectrumReconstruction", false);
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");

    TS_ASSERT_THROWS_NOTHING(alg->execute());

    MatrixWorkspace_sptr data = alg->getProperty("ReconstructedData");
    TS_ASSERT(data);

    // Compare adjusted second spectrum with non-adjust first spectrum
    // linear 1, const 1-i
    TS_ASSERT_DELTA(data->y(1)[0], data->y(0)[0] + 1.0, 0.001);
    TS_ASSERT_DELTA(data->y(3)[0], data->y(2)[0] - 1.0, 0.001);
    // linear 2i, const 0
    TS_ASSERT_DELTA(data->y(1)[1], -2.0 * data->y(2)[1], 0.001);
    TS_ASSERT_DELTA(data->y(3)[1], 2.0 * data->y(0)[1], 0.001);
    // linear 2i, const 1-i
    TS_ASSERT_DELTA(data->y(1)[2], -2.0 * data->y(2)[2] + 1.0, 0.001);
    TS_ASSERT_DELTA(data->y(3)[2], 2.0 * data->y(0)[2] - 1.0, 0.001);
  }

  void test_cosine() {

    auto ws = createWorkspaceReal(50, 0.0, 1);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("A", 0.01);
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
    TS_ASSERT_EQUALS(data->y(0).size(), 50);
    TS_ASSERT_DELTA(data->y(0)[25], 0.277, 0.001);
    TS_ASSERT_DELTA(data->y(0)[26], 0.454, 0.001);
    TS_ASSERT_DELTA(data->y(0)[27], 0.612, 0.001);

    // Test that the algorithm converged
    TS_ASSERT_DELTA(chi->y(0).back(), 1.000, 0.001);
    TS_ASSERT_DELTA(angle->y(0).back(), 0.001, 0.001);
  }

  void test_sine() {

    auto ws = createWorkspaceReal(50, M_PI / 2.0, 1);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("A", 0.01);
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
    TS_ASSERT_DELTA(data->y(0)[25], 0.893, 0.001);
    TS_ASSERT_DELTA(data->y(0)[26], 0.824, 0.001);
    TS_ASSERT_DELTA(data->y(0)[27], 0.721, 0.001);
    // Test that the algorithm converged
    TS_ASSERT_DELTA(chi->y(0).back(), 1.000, 0.001);
    TS_ASSERT_DELTA(angle->y(0).back(), 0.001, 0.001);
    // seg faults after these tests.....
  }

  void test_cosine_three_spectra() {

    auto ws = createWorkspaceReal(10, 0.0, 3);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("A", 0.01);
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
    TS_ASSERT_EQUALS(data->y(0).size(), 10);
    TS_ASSERT_EQUALS(data->y(1).size(), 10);
    TS_ASSERT_EQUALS(data->y(2).size(), 10);
    TS_ASSERT_EQUALS(data->y(5).size(), 10);
    TS_ASSERT_DELTA(data->y(0)[5], 0.261, 0.001);
    TS_ASSERT_DELTA(data->y(1)[5], 0.665, 0.001);
    TS_ASSERT_DELTA(data->y(2)[5], 0.898, 0.001);
    TS_ASSERT_DELTA(data->y(5)[5], 0.000, 0.001);

    // Test that the algorithm converged
    TS_ASSERT_DELTA(chi->y(0).back(), 1.000, 0.001);
    TS_ASSERT_DELTA(angle->y(0).back(), 0.001, 0.001);
    TS_ASSERT_DELTA(chi->y(1).back(), 1.000, 0.001);
    TS_ASSERT_DELTA(angle->y(1).back(), 0.001, 0.001);
    TS_ASSERT_DELTA(chi->y(2).back(), 1.000, 0.001);
    TS_ASSERT_DELTA(angle->y(2).back(), 0.001, 0.001);
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
    alg->setProperty("A", 0.01);
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");

    TS_ASSERT_THROWS_NOTHING(alg->execute());

    MatrixWorkspace_sptr data = alg->getProperty("ReconstructedData");
    TS_ASSERT(data);

    // Test some values
    TS_ASSERT_DELTA(data->y(0)[35], 0.8284631894, 0.0001);
    TS_ASSERT_DELTA(data->y(0)[36], 0.6667963448, 0.0001);
    TS_ASSERT_DELTA(data->y(0)[37], 0.3918500444, 0.0001);
    TS_ASSERT_DELTA(data->y(1)[35], 0.3302854368, 0.0001);
    TS_ASSERT_DELTA(data->y(1)[36], 0.6146197942, 0.0001);
    TS_ASSERT_DELTA(data->y(1)[37], 0.8119430900, 0.0001);
  }

  void test_sine_cosine_pos() {
    // Complex signal: cos(w * x) + i sin(w * x)
    // Positive images

    auto ws = createWorkspaceComplex();
    //    TS_ASSERT_EQUALS(ws->getNumberHistograms(),0)
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("ComplexData", true);
    alg->setProperty("PositiveImage", true);
    alg->setProperty("A", 0.01);
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");

    TS_ASSERT_THROWS_NOTHING(alg->execute());

    MatrixWorkspace_sptr data = alg->getProperty("ReconstructedData");
    TS_ASSERT(data);

    // Test some values
    TS_ASSERT_DELTA(data->y(0)[35], 0.8267522421, 0.0001);
    TS_ASSERT_DELTA(data->y(0)[36], 0.6722233773, 0.0001);
    TS_ASSERT_DELTA(data->y(0)[37], 0.3935, 0.0001);
    TS_ASSERT_DELTA(data->y(1)[35], 0.3248449519, 0.0001);
    TS_ASSERT_DELTA(data->y(1)[36], 0.6079783710, 0.0001);
    TS_ASSERT_DELTA(data->y(1)[37], 0.8078495801, 0.0001);
  }

  void test_sine_cosine_real_image() {
    // Complex signal: cos(w * x) + i sin(w * x)
    // Test real image (property ComplexImage set to False)

    auto ws = createWorkspaceComplex();

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("ComplexData", true);
    alg->setProperty("ComplexImage", false);
    alg->setProperty("A", 0.01);
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");

    TS_ASSERT_THROWS_NOTHING(alg->execute());

    MatrixWorkspace_sptr data = alg->getProperty("ReconstructedData");
    TS_ASSERT(data);

    // Test some values (should be close to those obtained in the previous two
    // tests)
    TS_ASSERT_DELTA(data->y(0)[35], 0.8469664801, 0.0001);
    TS_ASSERT_DELTA(data->y(0)[36], 0.6727449347, 0.0001);
    TS_ASSERT_DELTA(data->y(0)[37], 0.4058313316, 0.0001);
    TS_ASSERT_DELTA(data->y(1)[35], 0.3284565988, 0.0001);
    TS_ASSERT_DELTA(data->y(1)[36], 0.6122221939, 0.0001);
    TS_ASSERT_DELTA(data->y(1)[37], 0.8136355126, 0.0001);
  }

  void test_resolution_factor() {
    // Real signal: cos(w * x)

    size_t npoints = 50;

    auto ws = createWorkspaceReal(npoints, 0.0, 1);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("A", 0.01);
    alg->setProperty("ResolutionFactor", "3");
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");

    TS_ASSERT_THROWS_NOTHING(alg->execute());

    MatrixWorkspace_sptr data = alg->getProperty("ReconstructedData");
    MatrixWorkspace_sptr image = alg->getProperty("ReconstructedImage");

    TS_ASSERT(data);
    TS_ASSERT(image);

    // Test number of histograms and bins
    TS_ASSERT_EQUALS(data->blocksize(), npoints * 3);
    TS_ASSERT_EQUALS(image->blocksize(), npoints * 3);
    TS_ASSERT_EQUALS(data->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(image->getNumberHistograms(), 2);
    // Check that all X bins have been populated
    TS_ASSERT_EQUALS(data->readX(0).size(), data->readY(0).size());

    // Test some values
    TS_ASSERT_DELTA(image->y(0)[70], 6.829, 0.001);
    TS_ASSERT_DELTA(image->y(0)[71], 1.314, 0.001);
    TS_ASSERT_DELTA(image->y(1)[78], 0.102, 0.001);
    TS_ASSERT_DELTA(image->y(1)[79], 0.448, 0.001);
  }

  void test_adjustments() {

    auto ws = createWorkspaceReal(20, 0.0, 1);
    auto linAdj = createWorkspaceAdjustments(20, 1.05, 0.00, 0.0, 1);
    auto constAdj = createWorkspaceAdjustments(20, 0.0, 0.1, 0.2, 1);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("A", 0.01);
    alg->setProperty("DataLinearAdj", linAdj);
    alg->setProperty("DataConstAdj", constAdj);
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
    TS_ASSERT_EQUALS(data->y(0).size(), 20);
    TS_ASSERT_DELTA(data->y(0)[15], 0.245, 0.001);
    TS_ASSERT_DELTA(data->y(0)[16], -0.146, 0.001);
    TS_ASSERT_DELTA(data->y(0)[17], -0.602, 0.001);

    // Test that the algorithm converged
    TS_ASSERT_DELTA(chi->y(0).back(), 1.000, 0.001);
    TS_ASSERT_DELTA(angle->y(0).back(), 0.001, 0.001);
  }

  void test_adjustments_three_spectra() {

    auto ws = createWorkspaceReal(10, 0.0, 3);
    auto linAdj = createWorkspaceAdjustments(10, 1.05, 0.00, 0.0, 3);
    auto constAdj = createWorkspaceAdjustments(10, 0.0, 0.1, 0.2, 3);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("A", 0.01);
    alg->setProperty("DataLinearAdj", linAdj);
    alg->setProperty("DataConstAdj", constAdj);
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
    TS_ASSERT_EQUALS(data->y(0).size(), 10);
    TS_ASSERT_DELTA(data->y(0)[5], 0.237, 0.001);
    TS_ASSERT_DELTA(data->y(1)[5], 0.664, 0.001);
    TS_ASSERT_DELTA(data->y(2)[5], 0.895, 0.001);
    TS_ASSERT_EQUALS(data->y(5).size(), 10);
    TS_ASSERT_DELTA(data->y(5)[5], 0.0, 0.001);

    // Test that the algorithm converged
    TS_ASSERT_DELTA(chi->y(0).back(), 1.000, 0.001);
    TS_ASSERT_DELTA(angle->y(0).back(), 0.001, 0.001);
    TS_ASSERT_DELTA(chi->y(1).back(), 1.000, 0.001);
    TS_ASSERT_DELTA(angle->y(1).back(), 0.001, 0.001);
    TS_ASSERT_DELTA(chi->y(2).back(), 1.000, 0.001);
    TS_ASSERT_DELTA(angle->y(2).back(), 0.001, 0.001);
  }

  void test_adjustments_three_spectra_complex() {

    auto ws = createWorkspaceComplex(10, 0.0, 3, 0.0);
    auto linAdj = createWorkspaceAdjustments(10, 1.05, 0.00, 0.0, 3);
    auto constAdj = createWorkspaceAdjustments(10, 0.0, 0.1, 0.2, 3);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("ComplexData", true);
    alg->setProperty("A", 0.01);
    alg->setProperty("DataLinearAdj", linAdj);
    alg->setProperty("DataConstAdj", constAdj);
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
    TS_ASSERT_EQUALS(data->y(0).size(), 10);
    TS_ASSERT_DELTA(data->y(0)[5], -0.720, 0.001);
    TS_ASSERT_DELTA(data->y(1)[5], -0.742, 0.001);
    TS_ASSERT_DELTA(data->y(2)[5], -0.766, 0.001);
    TS_ASSERT_EQUALS(data->y(5).size(), 10);
    TS_ASSERT_DELTA(data->y(5)[5], 0.060, 0.001);

    // Test that the algorithm converged
    TS_ASSERT_DELTA(chi->y(0).back(), 1.000, 0.001);
    TS_ASSERT_DELTA(angle->y(0).back(), 0.001, 0.001);
    TS_ASSERT_DELTA(chi->y(1).back(), 1.000, 0.001);
    TS_ASSERT_DELTA(angle->y(1).back(), 0.001, 0.001);
    TS_ASSERT_DELTA(chi->y(2).back(), 1.000, 0.001);
    TS_ASSERT_DELTA(angle->y(2).back(), 0.001, 0.001);
  }

  void test_three_spectra_apart() {

    auto ws = createWorkspaceComplex(20, 0.0, 3, 0.0);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("ComplexData", true);
    alg->setProperty("A", 0.01);
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
    TS_ASSERT_EQUALS(data->y(0).size(), 20);
    TS_ASSERT_DELTA(data->y(0)[9], -0.422, 0.001);
    TS_ASSERT_DELTA(data->y(1)[9], -0.422, 0.001);
    TS_ASSERT_DELTA(data->y(2)[9], -0.422, 0.001);
    TS_ASSERT_EQUALS(data->y(5).size(), 20);
    TS_ASSERT_DELTA(data->y(5)[9], 0.580, 0.001);

    // Test that the algorithm converged
    TS_ASSERT_DELTA(chi->y(0).back(), 1.000, 0.001);
    TS_ASSERT_DELTA(angle->y(0).back(), 0.001, 0.001);
    TS_ASSERT_DELTA(chi->y(1).back(), 1.000, 0.001);
    TS_ASSERT_DELTA(angle->y(1).back(), 0.001, 0.001);
    TS_ASSERT_DELTA(chi->y(2).back(), 1.000, 0.001);
    TS_ASSERT_DELTA(angle->y(2).back(), 0.001, 0.001);
  }

  void test_three_spectra_together() {

    auto ws = createWorkspaceComplex(20, 0.0, 3, 0.0);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("ComplexData", true);
    alg->setProperty("A", 0.01);
    alg->setProperty("perSpectrumReconstruction", false);
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
    TS_ASSERT_EQUALS(data->y(0).size(), 20);
    TS_ASSERT_DELTA(data->y(0)[9], -0.421, 0.001);
    TS_ASSERT_DELTA(data->y(1)[9], -0.421, 0.001);
    TS_ASSERT_DELTA(data->y(2)[9], -0.421, 0.001);
    TS_ASSERT_EQUALS(data->y(5).size(), 20);
    TS_ASSERT_DELTA(data->y(5)[9], 0.580, 0.001);

    // Test that the algorithm converged
    TS_ASSERT_DELTA(chi->y(0).back(), 1.000, 0.001);
    TS_ASSERT_DELTA(angle->y(0).back(), 0.001, 0.001);
  }

  void test_adjustments_three_spectra_together() {

    auto ws = createWorkspaceComplex(20, 0.0, 3, 0.0);
    auto linAdj = createWorkspaceAdjustments(20, 1.00, 0.05, 0.0, 3);
    auto constAdj = createWorkspaceAdjustments(20, 0.0, 0.10, 0.0, 3);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("ComplexData", true);
    alg->setProperty("A", 0.01);
    alg->setProperty("DataLinearAdj", linAdj);
    alg->setProperty("DataConstAdj", constAdj);
    alg->setProperty("perSpectrumReconstruction", false);
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
    TS_ASSERT_EQUALS(data->y(0).size(), 20);
    TS_ASSERT_DELTA(data->y(0)[9], -0.370, 0.001);
    TS_ASSERT_DELTA(data->y(1)[9], -0.407, 0.001);
    TS_ASSERT_DELTA(data->y(2)[9], -0.449, 0.001);
    TS_ASSERT_EQUALS(data->y(5).size(), 20);
    TS_ASSERT_DELTA(data->y(5)[9], 0.665, 0.001);

    // Test that the algorithm converged
    TS_ASSERT_DELTA(chi->y(0).back(), 1.000, 0.001);
    TS_ASSERT_DELTA(angle->y(0).back(), 0.001, 0.001);
  }

  void test_output_label() {
    // Test the output label

    size_t npoints = 2;

    auto ws = createWorkspaceReal(npoints, 0.0, 1);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("A", 0.1);
    alg->setProperty("MaxIterations", "1");
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");

    auto label = boost::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(
        Mantid::Kernel::UnitFactory::Instance().create("Label"));

    // 1. From (Time, s) to (Frequency, Hz)
    label->setLabel("Time", "s");
    ws->getAxis(0)->unit() = label;
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    MatrixWorkspace_sptr image = alg->getProperty("ReconstructedImage");
    TS_ASSERT_EQUALS(image->getAxis(0)->unit()->caption(), "Frequency");
    TS_ASSERT_EQUALS(image->getAxis(0)->unit()->label().ascii(), "Hz");

    // 2. From (Time, ms) to (Frequency, MHz)
    label->setLabel("Time", "microsecond");
    ws->getAxis(0)->unit() = label;
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    image = alg->getProperty("ReconstructedImage");
    TS_ASSERT_EQUALS(image->getAxis(0)->unit()->caption(), "Frequency");
    TS_ASSERT_EQUALS(image->getAxis(0)->unit()->label().ascii(), "MHz");

    // 3. From (Frequency, Hz) to (Time, s)
    label->setLabel("Frequency", "Hz");
    ws->getAxis(0)->unit() = label;
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    image = alg->getProperty("ReconstructedImage");
    TS_ASSERT_EQUALS(image->getAxis(0)->unit()->caption(), "Time");
    TS_ASSERT_EQUALS(image->getAxis(0)->unit()->label().ascii(), "s");

    // 4. From (Frequency, MHz) to (Time, ms)
    label->setLabel("Frequency", "MHz");
    ws->getAxis(0)->unit() = label;
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    image = alg->getProperty("ReconstructedImage");
    TS_ASSERT_EQUALS(image->getAxis(0)->unit()->caption(), "Time");
    TS_ASSERT_EQUALS(image->getAxis(0)->unit()->label().ascii(), "microsecond");

    // 5. From (d-Spacing, Angstrom) to (q, Angstrom^-1)
    label->setLabel("d-Spacing", "Angstrom");
    ws->getAxis(0)->unit() = label;
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    image = alg->getProperty("ReconstructedImage");
    TS_ASSERT_EQUALS(image->getAxis(0)->unit()->caption(), "q");
    TS_ASSERT_EQUALS(image->getAxis(0)->unit()->label().ascii(), "Angstrom^-1");

    // 6. From (q, Angstrom^-1) to (d-Spacing, Angstrom)
    label->setLabel("q", "Angstrom^-1");
    ws->getAxis(0)->unit() = label;
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    image = alg->getProperty("ReconstructedImage");
    TS_ASSERT_EQUALS(image->getAxis(0)->unit()->caption(), "d-Spacing");
    TS_ASSERT_EQUALS(image->getAxis(0)->unit()->label().ascii(), "Angstrom");
  }

  /**
   * Test that the algorithm can handle a WorkspaceGroup as input without
   * crashing
   * We have to use the ADS to test WorkspaceGroups
   */
  void testValidateInputsWithWSGroup() {
    auto ws1 = boost::static_pointer_cast<Workspace>(
        WorkspaceCreationHelper::create2DWorkspace(5, 10));
    auto ws2 = boost::static_pointer_cast<Workspace>(
        WorkspaceCreationHelper::create2DWorkspace(5, 10));
    AnalysisDataService::Instance().add("workspace1", ws1);
    AnalysisDataService::Instance().add("workspace2", ws2);
    auto group = boost::make_shared<WorkspaceGroup>();
    AnalysisDataService::Instance().add("group", group);
    group->add("workspace1");
    group->add("workspace2");
    TestMaxEnt alg;
    alg.initialize();
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "group"));
    alg.setPropertyValue("MaxIterations", "1");
    alg.setPropertyValue("ReconstructedImage", "image");
    alg.setPropertyValue("ReconstructedData", "data");
    alg.setPropertyValue("EvolChi", "evolChi");
    alg.setPropertyValue("EvolAngle", "evolAngle");
    TS_ASSERT_THROWS_NOTHING(alg.wrapValidateInputs());
    AnalysisDataService::Instance().clear();
  }

  void testPhaseShift() {

    auto ws = createWorkspaceComplex();

    // Run MaxEnt
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("ComplexData", true);
    alg->setProperty("AutoShift", true);
    alg->setProperty("A", 0.01);
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");
    alg->execute();
    MatrixWorkspace_sptr outWS = alg->getProperty("ReconstructedImage");

    // Offset
    IAlgorithm_sptr scaleX = AlgorithmManager::Instance().create("ScaleX");
    scaleX->initialize();
    scaleX->setChild(true);
    scaleX->setProperty("InputWorkspace", ws);
    scaleX->setProperty("Factor", "1");
    scaleX->setProperty("Operation", "Add");
    scaleX->setPropertyValue("OutputWorkspace", "__NotUsed");
    scaleX->execute();
    MatrixWorkspace_sptr offsetted = scaleX->getProperty("OutputWorkspace");

    // Run MaxEnt on the offsetted ws
    alg->setProperty("InputWorkspace", offsetted);
    alg->execute();
    MatrixWorkspace_sptr outWSOffsetted =
        alg->getProperty("ReconstructedImage");

    // outWS and outWSOffsetted are shifted by ~pi -> there should be a factor
    // ~(-1) between them
    TS_ASSERT_DELTA(outWS->y(0)[28], -outWSOffsetted->y(0)[28], 0.1)
  }

  void test_unevenlySpacedInputData() {
    auto ws = createWorkspaceReal(3, 0.0, 1);
    Points xData{0, 1, 5};
    ws->setPoints(0, xData);
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    TS_ASSERT_THROWS(alg->setProperty("InputWorkspace", ws),
                     const std::invalid_argument &);
  }

  void test_histogram_workspace() {
    const size_t size = 10;
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceFactory::Instance().create("Workspace2D", 1, size + 1, size));
    // We don't care about values, except to check they are transferred
    // to data after one iteration.
    // Otherwise, we just want to test the number of
    // X points in the image.
    // For histogram input workspaces we should get the original number
    // of points minus one.
    for (size_t i = 0; i < size; i++) {
      double value = static_cast<double>(i);
      ws->dataX(0)[i] = value;
      ws->dataY(0)[i] = value;
      ws->dataE(0)[i] = value + 1.0;
    }
    ws->dataX(0)[size] = static_cast<double>(size);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("ComplexData", false);
    alg->setProperty("AutoShift", false);
    alg->setProperty("A", 1.0);
    alg->setPropertyValue("MaxIterations", "1");
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");
    alg->execute();
    MatrixWorkspace_sptr image = alg->getProperty("ReconstructedImage");
    MatrixWorkspace_sptr data = alg->getProperty("ReconstructedData");

    TS_ASSERT_EQUALS(image->readX(0).size(), ws->readX(0).size() - 1);
    TS_ASSERT_EQUALS(data->readX(0).size(), ws->readX(0).size());
    TS_ASSERT_EQUALS(data->readX(0), ws->readX(0));
  }

  void test_pointdata_workspace() {
    const size_t size = 10;
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceFactory::Instance().create("Workspace2D", 1, size, size));
    // We don't care about values, except to check they are transferred
    // to data after one iteration.
    // Otherwise, we just want to test the number of
    // X points in the image.
    // For pointdata input workspaces we should get the original number
    // of points.
    for (size_t i = 0; i < size; i++) {
      double value = static_cast<double>(i);
      ws->dataX(0)[i] = value;
      ws->dataY(0)[i] = value;
      ws->dataE(0)[i] = value + 1.0;
    }

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MaxEnt");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("ComplexData", false);
    alg->setProperty("AutoShift", false);
    alg->setProperty("A", 1.0);
    alg->setPropertyValue("MaxIterations", "1");
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");
    alg->execute();
    MatrixWorkspace_sptr image = alg->getProperty("ReconstructedImage");
    MatrixWorkspace_sptr data = alg->getProperty("ReconstructedData");

    TS_ASSERT_EQUALS(image->readX(0).size(), ws->readX(0).size());
    TS_ASSERT_EQUALS(data->readX(0).size(), ws->readX(0).size());
    TS_ASSERT_EQUALS(data->readX(0), ws->readX(0));
  }

  MatrixWorkspace_sptr
  createWorkspaceWithYValues(size_t nHist, size_t length,
                             std::vector<double> const &YVal) {

    size_t nPts = length * nHist;
    TS_ASSERT_EQUALS(nPts, YVal.size());
    MantidVec X(nPts);
    MantidVec Y(nPts);
    MantidVec E(nPts);
    for (size_t t = 0; t < length; t++) {
      double x = static_cast<double>(t);
      for (size_t s = 0; s < nHist; s++) {
        X[t + s * length] = x;
        Y[t + s * length] = YVal[t + s * length];
        E[t + s * length] = 0.1;
      }
    }
    auto createWS = AlgorithmManager::Instance().create("CreateWorkspace");
    createWS->initialize();
    createWS->setChild(true);
    createWS->setProperty("DataX", X);
    createWS->setProperty("DataY", Y);
    createWS->setProperty("DataE", E);
    createWS->setProperty("NSpec", static_cast<int>(nHist));
    createWS->setPropertyValue("OutputWorkspace", "ws");
    createWS->execute();
    MatrixWorkspace_sptr ws = createWS->getProperty("OutputWorkspace");

    return ws;
  }

  MatrixWorkspace_sptr createWorkspaceReal(size_t maxt, double phase,
                                           size_t nSpec) {

    // Create cosine with phase 'phase'

    // Frequency of the oscillations
    double w = 1.6;
    // phase shift between spectra
    double shift = 0.5;

    size_t nPts = maxt * nSpec;
    MantidVec X(nPts);
    MantidVec Y(nPts);
    MantidVec E(nPts);
    for (size_t t = 0; t < maxt; t++) {
      double x = 2. * M_PI * static_cast<double>(t) / static_cast<double>(maxt);
      for (size_t s = 0; s < nSpec; s++) {
        X[t + s * maxt] = x;
        Y[t + s * maxt] = cos(w * x + phase + static_cast<double>(s) * shift);
        E[t + s * maxt] = 0.1;
      }
    }
    auto createWS = AlgorithmManager::Instance().create("CreateWorkspace");
    createWS->initialize();
    createWS->setChild(true);
    createWS->setProperty("DataX", X);
    createWS->setProperty("DataY", Y);
    createWS->setProperty("DataE", E);
    createWS->setProperty("NSpec", static_cast<int>(nSpec));
    createWS->setPropertyValue("OutputWorkspace", "ws");
    createWS->execute();
    MatrixWorkspace_sptr ws = createWS->getProperty("OutputWorkspace");

    return ws;
  }

  MatrixWorkspace_sptr createWorkspaceComplex(size_t maxt, double phase,
                                              size_t nSpec, double shift) {

    // Create cosine with phase 'phase'

    // Frequency of the oscillations
    double w = 3.0;

    size_t nPts = maxt * nSpec;
    MantidVec X(2 * nPts);
    MantidVec Y(2 * nPts);
    MantidVec E(2 * nPts);
    for (size_t t = 0; t < maxt; t++) {
      double x = 2. * M_PI * static_cast<double>(t) / static_cast<double>(maxt);
      for (size_t s = 0; s < nSpec; s++) {
        X[t + s * maxt] = x;
        Y[t + s * maxt] = cos(w * x + phase + static_cast<double>(s) * shift);
        E[t + s * maxt] = 0.2;
        X[t + s * maxt + nPts] = x;
        Y[t + s * maxt + nPts] =
            sin(w * x + phase + static_cast<double>(s) * shift);
        E[t + s * maxt + nPts] = 0.2;
      }
    }
    auto createWS = AlgorithmManager::Instance().create("CreateWorkspace");
    createWS->initialize();
    createWS->setChild(true);
    createWS->setProperty("DataX", X);
    createWS->setProperty("DataY", Y);
    createWS->setProperty("DataE", E);
    createWS->setProperty("NSpec", static_cast<int>(2 * nSpec));
    createWS->setPropertyValue("OutputWorkspace", "ws");
    createWS->execute();
    MatrixWorkspace_sptr ws = createWS->getProperty("OutputWorkspace");

    return ws;
  }

  MatrixWorkspace_sptr createWorkspaceComplex() {

    const size_t size = 51;

    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceFactory::Instance().create("Workspace2D", 2, size, size));

    // x = 2 * pi * i / N
    // Real
    ws->setHistogram(
        0,
        Points{0.0000, 0.1232, 0.2464, 0.3696, 0.4928, 0.6160, 0.7392, 0.8624,
               0.9856, 1.1088, 1.2320, 1.3552, 1.4784, 1.6016, 1.7248, 1.8480,
               1.9712, 2.0944, 2.2176, 2.3408, 2.4640, 2.5872, 2.7104, 2.8336,
               2.9568, 3.0800, 3.2032, 3.3264, 3.4496, 3.5728, 3.6960, 3.8192,
               3.9424, 4.0656, 4.1888, 4.3120, 4.4352, 4.5584, 4.6816, 4.8048,
               4.9280, 5.0512, 5.1744, 5.2976, 5.4208, 5.5440, 5.6672, 5.7904,
               5.9136, 6.0368, 6.1600},
        Counts{1.07,  0.95,  0.84,  0.51,  -0.04, -0.42, -0.47, -0.98, -0.96,
               -1.03, -0.71, -0.70, -0.13, -0.04, 0.59,  0.84,  0.91,  0.93,
               1.03,  0.75,  0.40,  0.18,  -0.24, -0.48, -0.78, -0.95, -0.94,
               -0.87, -0.46, -0.19, 0.13,  0.35,  0.88,  1.01,  0.92,  0.79,
               0.80,  0.44,  0.15,  -0.26, -0.49, -0.79, -0.84, -1.04, -0.80,
               -0.73, -0.26, 0.09,  0.45,  0.67,  0.92},
        CountStandardDeviations{
            0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1,
            0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1,
            0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1,
            0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1});

    // Imaginary
    ws->setHistogram(
        1, ws->points(0),
        Counts{0.07,  0.25,  0.82,  0.75,  1.08,  0.84,  0.82,  0.62,  0.33,
               -0.20, -0.58, -0.88, -0.85, -1.10, -0.77, -0.59, -0.36, 0.13,
               0.39,  0.62,  0.87,  1.03,  0.82,  0.94,  0.47,  0.30,  -0.22,
               -0.39, -0.86, -0.91, -0.88, -0.84, -0.59, -0.27, 0.14,  0.36,
               0.69,  0.98,  0.98,  0.95,  0.71,  0.41,  0.32,  -0.13, -0.53,
               -0.74, -0.82, -0.91, -0.82, -0.60, -0.32},
        ws->countStandardDeviations(0));

    return ws;
  }

  MatrixWorkspace_sptr createWorkspaceAdjustments(size_t maxt, double base,
                                                  double magnitude,
                                                  double phase, size_t nSpec) {

    // Frequency of the oscillations
    double w = 2.4;
    // phase shift between spectra
    double shift = 0.5;

    size_t nPts = maxt * nSpec;
    MantidVec X(2 * nPts);
    MantidVec Y(2 * nPts);
    MantidVec E(2 * nPts);
    for (size_t t = 0; t < maxt; t++) {
      double x = 2. * M_PI * static_cast<double>(t) / static_cast<double>(maxt);
      for (size_t s = 0; s < nSpec; s++) {
        // Real
        X[t + s * maxt] = 0.0;
        Y[t + s * maxt] =
            base +
            magnitude * cos(w * x + phase + static_cast<double>(s) * shift);
        E[t + s * maxt] = 0.0;
        // Imaginary
        X[t + s * maxt + nPts] = 0.0;
        Y[t + s * maxt + nPts] =
            magnitude * sin(w * x + phase + static_cast<double>(s) * shift);
        E[t + s * maxt + nPts] = 0.0;
      }
    }
    auto createWS = AlgorithmManager::Instance().create("CreateWorkspace");
    createWS->initialize();
    createWS->setChild(true);
    createWS->setProperty("DataX", X);
    createWS->setProperty("DataY", Y);
    createWS->setProperty("DataE", E);
    createWS->setProperty("NSpec", 2 * static_cast<int>(nSpec));
    createWS->setPropertyValue("OutputWorkspace", "ws");
    createWS->execute();
    MatrixWorkspace_sptr ws = createWS->getProperty("OutputWorkspace");

    return ws;
  }
};

class MaxEntTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaxEntTestPerformance *createSuite() {
    return new MaxEntTestPerformance();
  }
  static void destroySuite(MaxEntTestPerformance *suite) { delete suite; }

  MaxEntTestPerformance() {
    input = WorkspaceCreationHelper::create2DWorkspaceBinned(10000, 100);
    alg = AlgorithmManager::Instance().create("MaxEnt");
  }

  void testExecReal() {
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", input);
    alg->setPropertyValue("MaxIterations", "1");
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");
    alg->execute();
  }

  void testExecComplex() {
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", input);
    alg->setPropertyValue("MaxIterations", "1");
    alg->setProperty("ComplexData", true);
    alg->setPropertyValue("ReconstructedImage", "image");
    alg->setPropertyValue("ReconstructedData", "data");
    alg->setPropertyValue("EvolChi", "evolChi");
    alg->setPropertyValue("EvolAngle", "evolAngle");
    alg->execute();
  }

private:
  MatrixWorkspace_sptr input;
  IAlgorithm_sptr alg;
};

#endif /* MANTID_ALGORITHMS_MAXENTTEST_H_ */
