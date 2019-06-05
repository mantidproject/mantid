// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_WORKFLOWALGORITHMS_IMUONASYMMETRYCALCULATORTEST_H_
#define MANTID_WORKFLOWALGORITHMS_IMUONASYMMETRYCALCULATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidWorkflowAlgorithms/MuonGroupAsymmetryCalculator.h"
#include "MantidWorkflowAlgorithms/MuonGroupCountsCalculator.h"
#include "MantidWorkflowAlgorithms/MuonPairAsymmetryCalculator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::WorkflowAlgorithms::IMuonAsymmetryCalculator;
using Mantid::WorkflowAlgorithms::MuonGroupAsymmetryCalculator;
using Mantid::WorkflowAlgorithms::MuonGroupCountsCalculator;
using Mantid::WorkflowAlgorithms::MuonPairAsymmetryCalculator;
using IMuonAsymCalc_uptr = std::unique_ptr<IMuonAsymmetryCalculator>;

/**
 * Tests for all classes deriving from IMuonAsymmetryCalculator:
 * MuonGroupCountsCalculator
 * MuonGroupAsymmetryCalculator
 * MuonPairAsymmetryCalculator
 */
class IMuonAsymmetryCalculatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IMuonAsymmetryCalculatorTest *createSuite() {
    return new IMuonAsymmetryCalculatorTest();
  }
  static void destroySuite(IMuonAsymmetryCalculatorTest *suite) {
    delete suite;
  }

  IMuonAsymmetryCalculatorTest() {
    // To make sure everything is loaded
    FrameworkManager::Instance();
  }

  //------ Group Counts test section --------------

  void test_groupCounts_singlePeriod() {
    auto inputWSGroup = boost::make_shared<WorkspaceGroup>();
    inputWSGroup->addWorkspace(createWorkspace());

    std::vector<int> summed{1};
    std::vector<int> subtracted{};
    int groupIndex(1);
    IMuonAsymCalc_uptr calc = std::make_unique<MuonGroupCountsCalculator>(
        inputWSGroup, summed, subtracted, groupIndex);
    MatrixWorkspace_sptr outWS;
    TS_ASSERT_THROWS_NOTHING(outWS = calc->calculate());
    TS_ASSERT(outWS);

    if (outWS) {
      TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(outWS->blocksize(), 3);

      TS_ASSERT_EQUALS(outWS->readY(0)[0], 4);
      TS_ASSERT_EQUALS(outWS->readY(0)[1], 5);
      TS_ASSERT_EQUALS(outWS->readY(0)[2], 6);

      TS_ASSERT_EQUALS(outWS->readX(0)[0], 1);
      TS_ASSERT_EQUALS(outWS->readX(0)[1], 2);
      TS_ASSERT_EQUALS(outWS->readX(0)[2], 3);

      TS_ASSERT_DELTA(outWS->readE(0)[0], 0.4, 0.01);
      TS_ASSERT_DELTA(outWS->readE(0)[1], 0.5, 0.01);
      TS_ASSERT_DELTA(outWS->readE(0)[2], 0.6, 0.01);
    }
  }

  void test_groupCounts_twoPeriods_plus() {
    MatrixWorkspace_sptr inWSFirst = createWorkspace();
    MatrixWorkspace_sptr inWSSecond = createWorkspace();
    auto inputWSGroup = boost::make_shared<WorkspaceGroup>();
    inputWSGroup->addWorkspace(inWSFirst);
    inputWSGroup->addWorkspace(inWSSecond);

    std::vector<int> summed{1, 2};
    std::vector<int> subtracted{};
    int groupIndex(1);
    IMuonAsymCalc_uptr calc = std::make_unique<MuonGroupCountsCalculator>(
        inputWSGroup, summed, subtracted, groupIndex);
    MatrixWorkspace_sptr outWS;
    TS_ASSERT_THROWS_NOTHING(outWS = calc->calculate());
    TS_ASSERT(outWS);

    if (outWS) {
      TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(outWS->blocksize(), 3);

      TS_ASSERT_EQUALS(outWS->readY(0)[0], 8);
      TS_ASSERT_EQUALS(outWS->readY(0)[1], 10);
      TS_ASSERT_EQUALS(outWS->readY(0)[2], 12);

      TS_ASSERT_EQUALS(outWS->readX(0)[0], 1);
      TS_ASSERT_EQUALS(outWS->readX(0)[1], 2);
      TS_ASSERT_EQUALS(outWS->readX(0)[2], 3);

      TS_ASSERT_DELTA(outWS->readE(0)[0], 0.566, 0.001);
      TS_ASSERT_DELTA(outWS->readE(0)[1], 0.707, 0.001);
      TS_ASSERT_DELTA(outWS->readE(0)[2], 0.849, 0.001);
    }
  }

  void test_groupCounts_twoPeriod_minus() {
    MatrixWorkspace_sptr inWSFirst = createWorkspace(3);
    MatrixWorkspace_sptr inWSSecond = createWorkspace();
    auto inputWSGroup = boost::make_shared<WorkspaceGroup>();
    inputWSGroup->addWorkspace(inWSFirst);
    inputWSGroup->addWorkspace(inWSSecond);

    std::vector<int> summed{1};
    std::vector<int> subtracted{2};
    int groupIndex(1);
    IMuonAsymCalc_uptr calc = std::make_unique<MuonGroupCountsCalculator>(
        inputWSGroup, summed, subtracted, groupIndex);
    MatrixWorkspace_sptr outWS;
    TS_ASSERT_THROWS_NOTHING(outWS = calc->calculate());
    TS_ASSERT(outWS);

    if (outWS) {
      TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(outWS->blocksize(), 3);

      TS_ASSERT_EQUALS(outWS->readY(0)[0], 3);
      TS_ASSERT_EQUALS(outWS->readY(0)[1], 3);
      TS_ASSERT_EQUALS(outWS->readY(0)[2], 3);

      TS_ASSERT_EQUALS(outWS->readX(0)[0], 1);
      TS_ASSERT_EQUALS(outWS->readX(0)[1], 2);
      TS_ASSERT_EQUALS(outWS->readX(0)[2], 3);

      TS_ASSERT_DELTA(outWS->readE(0)[0], 0.806, 0.001);
      TS_ASSERT_DELTA(outWS->readE(0)[1], 0.943, 0.001);
      TS_ASSERT_DELTA(outWS->readE(0)[2], 1.082, 0.001);
    }
  }

  /**
   * Test period 1+2+3 for group counts
   */
  void test_groupCounts_threePeriods_plus() {
    MatrixWorkspace_sptr inWSFirst = createWorkspace();
    MatrixWorkspace_sptr inWSSecond = createWorkspace();
    MatrixWorkspace_sptr inWSThird = createWorkspace();
    auto inputWSGroup = boost::make_shared<WorkspaceGroup>();
    inputWSGroup->addWorkspace(inWSFirst);
    inputWSGroup->addWorkspace(inWSSecond);
    inputWSGroup->addWorkspace(inWSThird);

    std::vector<int> summed{1, 2, 3};
    std::vector<int> subtracted{};
    int groupIndex(1);
    IMuonAsymCalc_uptr calc = std::make_unique<MuonGroupCountsCalculator>(
        inputWSGroup, summed, subtracted, groupIndex);
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = calc->calculate());
    TS_ASSERT(ws);

    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 3);

      TS_ASSERT_EQUALS(ws->readY(0)[0], 12);
      TS_ASSERT_EQUALS(ws->readY(0)[1], 15);
      TS_ASSERT_EQUALS(ws->readY(0)[2], 18);

      TS_ASSERT_EQUALS(ws->readX(0)[0], 1);
      TS_ASSERT_EQUALS(ws->readX(0)[1], 2);
      TS_ASSERT_EQUALS(ws->readX(0)[2], 3);

      TS_ASSERT_DELTA(ws->readE(0)[0], 0.693, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[1], 0.866, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[2], 1.039, 0.001);
    }
  }

  /**
   * Test period 1+2-3 for group counts
   */
  void test_groupCounts_threePeriods_minus() {
    MatrixWorkspace_sptr inWSFirst = createWorkspace();
    MatrixWorkspace_sptr inWSSecond = createWorkspace();
    MatrixWorkspace_sptr inWSThird = createWorkspace();
    auto inputWSGroup = boost::make_shared<WorkspaceGroup>();
    inputWSGroup->addWorkspace(inWSFirst);
    inputWSGroup->addWorkspace(inWSSecond);
    inputWSGroup->addWorkspace(inWSThird);

    std::vector<int> summed{1, 2};
    std::vector<int> subtracted{3};
    int groupIndex(1);
    IMuonAsymCalc_uptr calc = std::make_unique<MuonGroupCountsCalculator>(
        inputWSGroup, summed, subtracted, groupIndex);
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = calc->calculate());
    TS_ASSERT(ws);

    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 3);

      TS_ASSERT_EQUALS(ws->readY(0)[0], 4);
      TS_ASSERT_EQUALS(ws->readY(0)[1], 5);
      TS_ASSERT_EQUALS(ws->readY(0)[2], 6);

      TS_ASSERT_EQUALS(ws->readX(0)[0], 1);
      TS_ASSERT_EQUALS(ws->readX(0)[1], 2);
      TS_ASSERT_EQUALS(ws->readX(0)[2], 3);

      TS_ASSERT_DELTA(ws->readE(0)[0], 0.693, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[1], 0.866, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[2], 1.039, 0.001);
    }
  }

  //------ Group Asymmetry test section --------------

  void test_groupAsymmetry_singlePeriod() {
    MatrixWorkspace_sptr inWS = createWorkspace();
    inWS->mutableRun().addProperty("goodfrm", 10);
    auto inputWSGroup = boost::make_shared<WorkspaceGroup>();
    inputWSGroup->addWorkspace(inWS);

    std::vector<int> summed{1};
    std::vector<int> subtracted{};
    int groupIndex(2);
    IMuonAsymCalc_uptr calc = std::make_unique<MuonGroupAsymmetryCalculator>(
        inputWSGroup, summed, subtracted, groupIndex);
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = calc->calculate());
    TS_ASSERT(ws);

    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 3);

      TS_ASSERT_DELTA(ws->readY(0)[0], -0.610, 0.001);
      TS_ASSERT_DELTA(ws->readY(0)[1], -0.298, 0.001);
      TS_ASSERT_DELTA(ws->readY(0)[2], 0.2446, 0.001);

      TS_ASSERT_EQUALS(ws->readX(0)[0], 1);
      TS_ASSERT_EQUALS(ws->readX(0)[1], 2);
      TS_ASSERT_EQUALS(ws->readX(0)[2], 3);

      TS_ASSERT_DELTA(ws->readE(0)[0], 0.04, 0.01);
      TS_ASSERT_DELTA(ws->readE(0)[1], 0.07, 0.01);
      TS_ASSERT_DELTA(ws->readE(0)[2], 0.12, 0.01);
    }
  }

  void test_groupAsymmetry_twoPeriods_minus() {
    MatrixWorkspace_sptr inWS = createWorkspace(3);
    inWS->mutableRun().addProperty("goodfrm", 10);
    MatrixWorkspace_sptr inWSSecond = createWorkspace();
    inWSSecond->mutableRun().addProperty("goodfrm", 10);
    auto inputWSGroup = boost::make_shared<WorkspaceGroup>();
    inputWSGroup->addWorkspace(inWS);
    inputWSGroup->addWorkspace(inWSSecond);

    std::vector<int> summed{1};
    std::vector<int> subtracted{2};
    int groupIndex(2);
    IMuonAsymCalc_uptr calc = std::make_unique<MuonGroupAsymmetryCalculator>(
        inputWSGroup, summed, subtracted, groupIndex);
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = calc->calculate());
    TS_ASSERT(ws);

    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 3);

      TS_ASSERT_EQUALS(ws->readX(0)[0], 1);
      TS_ASSERT_EQUALS(ws->readX(0)[1], 2);
      TS_ASSERT_EQUALS(ws->readX(0)[2], 3);

      TS_ASSERT_DELTA(ws->readY(0)[0], 0.0152, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[1], 0.0000, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[2], -0.0378, 0.0001);

      TS_ASSERT_DELTA(ws->readE(0)[0], 0.0562, 0.0001);
      TS_ASSERT_DELTA(ws->readE(0)[1], 0.0992, 0.0001);
      TS_ASSERT_DELTA(ws->readE(0)[2], 0.1734, 0.0001);
    }
  }

  void test_groupAsymmetry_twoPeriods_plus() {
    MatrixWorkspace_sptr inWS = createWorkspace(3);
    inWS->mutableRun().addProperty("goodfrm", 10);
    MatrixWorkspace_sptr inWSSecond = createWorkspace();
    inWSSecond->mutableRun().addProperty("goodfrm", 10);
    auto inputWSGroup = boost::make_shared<WorkspaceGroup>();
    inputWSGroup->addWorkspace(inWS);
    inputWSGroup->addWorkspace(inWSSecond);

    std::vector<int> summed{1, 2};
    std::vector<int> subtracted{};
    int groupIndex(1);
    IMuonAsymCalc_uptr calc = std::make_unique<MuonGroupAsymmetryCalculator>(
        inputWSGroup, summed, subtracted, groupIndex);
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = calc->calculate());
    TS_ASSERT(ws);

    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 3);

      TS_ASSERT_EQUALS(ws->readX(0)[0], 1);
      TS_ASSERT_EQUALS(ws->readX(0)[1], 2);
      TS_ASSERT_EQUALS(ws->readX(0)[2], 3);

      TS_ASSERT_DELTA(ws->readY(0)[0], -0.6233, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[1], -0.2982, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[2], 0.2765, 0.0001);

      TS_ASSERT_DELTA(ws->readE(0)[0], 0.0276, 0.0001);
      TS_ASSERT_DELTA(ws->readE(0)[1], 0.0509, 0.0001);
      TS_ASSERT_DELTA(ws->readE(0)[2], 0.0921, 0.0001);
    }
  }

  /**
   * Test group asymmetry calculation for 3 periods 1+2+3
   */
  void test_groupAsymmetry_threePeriods_plus() {
    MatrixWorkspace_sptr periodOne = createWorkspace();
    periodOne->mutableRun().addProperty("goodfrm", 10);
    MatrixWorkspace_sptr periodTwo = createWorkspace(3);
    periodTwo->mutableRun().addProperty("goodfrm", 10);
    MatrixWorkspace_sptr periodThree = createWorkspace(1);
    periodThree->mutableRun().addProperty("goodfrm", 10);
    auto inputWSGroup = boost::make_shared<WorkspaceGroup>();
    inputWSGroup->addWorkspace(periodOne);
    inputWSGroup->addWorkspace(periodTwo);
    inputWSGroup->addWorkspace(periodThree);

    std::vector<int> summed{1, 2, 3};
    std::vector<int> subtracted{};
    int groupIndex(1);
    IMuonAsymCalc_uptr calc = std::make_unique<MuonGroupAsymmetryCalculator>(
        inputWSGroup, summed, subtracted, groupIndex);
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = calc->calculate());
    TS_ASSERT(ws);

    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 3);

      TS_ASSERT_EQUALS(ws->readX(0)[0], 1);
      TS_ASSERT_EQUALS(ws->readX(0)[1], 2);
      TS_ASSERT_EQUALS(ws->readX(0)[2], 3);

      TS_ASSERT_DELTA(ws->readY(0)[0], -0.6251, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[1], -0.2982, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[2], 0.2810, 0.0001);

      TS_ASSERT_DELTA(ws->readE(0)[0], 0.0222, 0.0001);
      TS_ASSERT_DELTA(ws->readE(0)[1], 0.0413, 0.0001);
      TS_ASSERT_DELTA(ws->readE(0)[2], 0.0750, 0.0001);
    }
  }

  /**
   * Test group asymmetry calculation for 3 periods 1+2-3
   */
  void test_groupAsymmetry_threePeriods_minus() {
    MatrixWorkspace_sptr periodOne = createWorkspace();
    periodOne->mutableRun().addProperty("goodfrm", 10);
    MatrixWorkspace_sptr periodTwo = createWorkspace(3);
    periodTwo->mutableRun().addProperty("goodfrm", 10);
    MatrixWorkspace_sptr periodThree = createWorkspace(1);
    periodThree->mutableRun().addProperty("goodfrm", 10);
    auto inputWSGroup = boost::make_shared<WorkspaceGroup>();
    inputWSGroup->addWorkspace(periodOne);
    inputWSGroup->addWorkspace(periodTwo);
    inputWSGroup->addWorkspace(periodThree);

    std::vector<int> summed{1, 2};
    std::vector<int> subtracted{3};
    int groupIndex(1);
    IMuonAsymCalc_uptr calc = std::make_unique<MuonGroupAsymmetryCalculator>(
        inputWSGroup, summed, subtracted, groupIndex);
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = calc->calculate());
    TS_ASSERT(ws);

    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 3);

      TS_ASSERT_EQUALS(ws->readX(0)[0], 1);
      TS_ASSERT_EQUALS(ws->readX(0)[1], 2);
      TS_ASSERT_EQUALS(ws->readX(0)[2], 3);

      TS_ASSERT_DELTA(ws->readY(0)[0], 0.0057, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[1], 0.0000, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[2], -0.0142, 0.0001);

      TS_ASSERT_DELTA(ws->readE(0)[0], 0.0462, 0.0001);
      TS_ASSERT_DELTA(ws->readE(0)[1], 0.0867, 0.0001);
      TS_ASSERT_DELTA(ws->readE(0)[2], 0.1585, 0.0001);
    }
  }

  //------ Pair Asymmetry test section --------------

  void test_pairAsymmetry_singlePeriod() {
    MatrixWorkspace_sptr inWS = createWorkspace();
    auto inputWSGroup = boost::make_shared<WorkspaceGroup>();
    inputWSGroup->addWorkspace(inWS);

    std::vector<int> summed{1};
    std::vector<int> subtracted{};
    int firstIndex(2), secondIndex(0);
    double alpha(0.5);
    IMuonAsymCalc_uptr calc = std::make_unique<MuonPairAsymmetryCalculator>(
        inputWSGroup, summed, subtracted, firstIndex, secondIndex, alpha);
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = calc->calculate());
    TS_ASSERT(ws);

    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 3);

      TS_ASSERT_DELTA(ws->readY(0)[0], 0.867, 0.001);
      TS_ASSERT_DELTA(ws->readY(0)[1], 0.778, 0.001);
      TS_ASSERT_DELTA(ws->readY(0)[2], 0.714, 0.001);

      TS_ASSERT_EQUALS(ws->readX(0)[0], 1.5);
      TS_ASSERT_EQUALS(ws->readX(0)[1], 2.5);
      TS_ASSERT_EQUALS(ws->readX(0)[2], 3);

      TS_ASSERT_DELTA(ws->readE(0)[0], 0.475, 0.01);
      TS_ASSERT_DELTA(ws->readE(0)[1], 0.410, 0.01);
      TS_ASSERT_DELTA(ws->readE(0)[2], 0.365, 0.01);
    }
  }

  void test_pairAsymmetry_twoPeriods_minus() {
    MatrixWorkspace_sptr inWS = createWorkspace(3);
    MatrixWorkspace_sptr inWSSecond = createWorkspace();
    auto inputWSGroup = boost::make_shared<WorkspaceGroup>();
    inputWSGroup->addWorkspace(inWS);
    inputWSGroup->addWorkspace(inWSSecond);

    std::vector<int> summed{1};
    std::vector<int> subtracted{2};
    int firstIndex(2), secondIndex(0);
    IMuonAsymCalc_uptr calc = std::make_unique<MuonPairAsymmetryCalculator>(
        inputWSGroup, summed, subtracted, firstIndex, secondIndex);
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = calc->calculate());
    TS_ASSERT(ws);

    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 3);

      TS_ASSERT_DELTA(ws->readY(0)[0], -0.3214, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[1], -0.2250, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[2], -0.1666, 0.0001);

      TS_ASSERT_EQUALS(ws->readX(0)[0], 1.5);
      TS_ASSERT_EQUALS(ws->readX(0)[1], 2.5);
      TS_ASSERT_EQUALS(ws->readX(0)[2], 3);

      TS_ASSERT_DELTA(ws->readE(0)[0], 0.5290, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[1], 0.4552, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[2], 0.4073, 0.001);
    }
  }

  void test_pairAsymmetry_twoPeriods_plus() {
    MatrixWorkspace_sptr inWS = createWorkspace(3);
    MatrixWorkspace_sptr inWSSecond = createWorkspace();
    auto inputWSGroup = boost::make_shared<WorkspaceGroup>();
    inputWSGroup->addWorkspace(inWS);
    inputWSGroup->addWorkspace(inWSSecond);

    std::vector<int> summed{1, 2};
    std::vector<int> subtracted{};
    int firstIndex(0), secondIndex(2);
    IMuonAsymCalc_uptr calc = std::make_unique<MuonPairAsymmetryCalculator>(
        inputWSGroup, summed, subtracted, firstIndex, secondIndex);
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = calc->calculate());
    TS_ASSERT(ws);

    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 3);

      TS_ASSERT_EQUALS(ws->readX(0)[0], 1.5);
      TS_ASSERT_EQUALS(ws->readX(0)[1], 2.5);
      TS_ASSERT_EQUALS(ws->readX(0)[2], 3);

      TS_ASSERT_DELTA(ws->readY(0)[0], -0.5454, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[1], -0.4615, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[2], -0.4000, 0.0001);

      TS_ASSERT_DELTA(ws->readE(0)[0], 0.2428, 0.0001);
      TS_ASSERT_DELTA(ws->readE(0)[1], 0.2159, 0.0001);
      TS_ASSERT_DELTA(ws->readE(0)[2], 0.1966, 0.0001);
    }
  }

  void test_pairAsymmetry_threePeriods_minus() {
    MatrixWorkspace_sptr inWS = createWorkspace(3);
    MatrixWorkspace_sptr inWSSecond = createWorkspace();
    MatrixWorkspace_sptr inWSThird = createWorkspace(2);
    auto inputWSGroup = boost::make_shared<WorkspaceGroup>();
    inputWSGroup->addWorkspace(inWS);
    inputWSGroup->addWorkspace(inWSSecond);
    inputWSGroup->addWorkspace(inWSThird);

    std::vector<int> summed{1, 2};
    std::vector<int> subtracted{3};
    int firstIndex(2), secondIndex(0);
    IMuonAsymCalc_uptr calc = std::make_unique<MuonPairAsymmetryCalculator>(
        inputWSGroup, summed, subtracted, firstIndex, secondIndex);
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = calc->calculate());
    TS_ASSERT(ws);

    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 3);

      TS_ASSERT_DELTA(ws->readY(0)[0], 0.0455, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[1], 0.0330, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[2], 0.0250, 0.0001);

      TS_ASSERT_EQUALS(ws->readX(0)[0], 1.5);
      TS_ASSERT_EQUALS(ws->readX(0)[1], 2.5);
      TS_ASSERT_EQUALS(ws->readX(0)[2], 3);

      TS_ASSERT_DELTA(ws->readE(0)[0], 0.4039, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[1], 0.3622, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[2], 0.3315, 0.001);
    }
  }

  void test_pairAsymmetry_threePeriods_plus() {
    MatrixWorkspace_sptr inWS = createWorkspace(3);
    MatrixWorkspace_sptr inWSSecond = createWorkspace();
    MatrixWorkspace_sptr inWSThird = createWorkspace(2);
    auto inputWSGroup = boost::make_shared<WorkspaceGroup>();
    inputWSGroup->addWorkspace(inWS);
    inputWSGroup->addWorkspace(inWSSecond);
    inputWSGroup->addWorkspace(inWSThird);

    std::vector<int> summed{1, 2, 3};
    std::vector<int> subtracted{};
    int firstIndex(2), secondIndex(0);
    IMuonAsymCalc_uptr calc = std::make_unique<MuonPairAsymmetryCalculator>(
        inputWSGroup, summed, subtracted, firstIndex, secondIndex);
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = calc->calculate());
    TS_ASSERT(ws);

    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 3);

      TS_ASSERT_DELTA(ws->readY(0)[0], 0.5294, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[1], 0.4500, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[2], 0.3913, 0.0001);

      TS_ASSERT_EQUALS(ws->readX(0)[0], 1.5);
      TS_ASSERT_EQUALS(ws->readX(0)[1], 2.5);
      TS_ASSERT_EQUALS(ws->readX(0)[2], 3);

      TS_ASSERT_DELTA(ws->readE(0)[0], 0.1940, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[1], 0.1733, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[2], 0.1583, 0.001);
    }
  }

  /// Test that exception is thrown when passed an empty
  /// WorkspaceGroup as input.
  void test_throws_emptyGroup() {

    auto inputWSGroup = boost::make_shared<WorkspaceGroup>();

    std::vector<int> summed{1, 2};
    std::vector<int> subtracted{};
    int firstIndex(0), secondIndex(2);
    IMuonAsymCalc_uptr calc = std::make_unique<MuonPairAsymmetryCalculator>(
        inputWSGroup, summed, subtracted, firstIndex, secondIndex);
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS(ws = calc->calculate(), const std::out_of_range &);
    TS_ASSERT(!ws);
  }

private:
  /**
   * Creates 3x3 workspace with values:
   *     1 2 3
   *     4 5 6
   *     7 8 9
   *
   * Delta is added to every value if specified.
   *
   * Errors are the same values but divided by 10.
   *
   * X values are 1 2 3 for all the histograms.
   */
  MatrixWorkspace_sptr createWorkspace(const double delta = 0.0) {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(3, 3);

    for (size_t i = 0; i < ws->getNumberHistograms(); i++) {
      auto &x = ws->dataX(i);
      auto &y = ws->dataY(i);
      auto &e = ws->dataE(i);

      const size_t numBins = y.size();
      for (size_t j = 0; j < numBins; j++) {
        const double v = static_cast<double>(i * numBins + j) + 1.0 + delta;

        x[j] = static_cast<double>(j + 1);
        y[j] = v;
        e[j] = v * 0.1;
      }
    }

    return ws;
  }
};

#endif /* MANTID_WORKFLOWALGORITHMS_IMUONASYMMETRYCALCULATORTEST_H_ */
