#ifndef MANTID_WORKFLOWALGORITHMS_MUONCALCULATEASYMMETRYTEST_H_
#define MANTID_WORKFLOWALGORITHMS_MUONCALCULATEASYMMETRYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidWorkflowAlgorithms/MuonCalculateAsymmetry.h"

using Mantid::WorkflowAlgorithms::MuonCalculateAsymmetry;

using namespace Mantid::Kernel;
using namespace Mantid::API;

class MuonCalculateAsymmetryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonCalculateAsymmetryTest *createSuite() {
    return new MuonCalculateAsymmetryTest();
  }
  static void destroySuite(MuonCalculateAsymmetryTest *suite) { delete suite; }

  MuonCalculateAsymmetryTest() {
    // To make sure everything is loaded
    FrameworkManager::Instance();
  }

  void test_Init() {
    MuonCalculateAsymmetry alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_groupCounts_singlePeriod() {
    // Name of the output workspace.
    const std::string outWSName =
        outputWorkspaceName("GroupCounts_SinglePeriod");

    auto inputWSGroup = boost::shared_ptr<WorkspaceGroup>(new WorkspaceGroup());
    inputWSGroup->addWorkspace(createWorkspace());

    MuonCalculateAsymmetry alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWSGroup));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "GroupCounts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupIndex", 1));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    auto ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
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

      TS_ASSERT_DELTA(ws->readE(0)[0], 0.4, 0.01);
      TS_ASSERT_DELTA(ws->readE(0)[1], 0.5, 0.01);
      TS_ASSERT_DELTA(ws->readE(0)[2], 0.6, 0.01);
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_groupCounts_twoPeriods_plus() {
    // Name of the output workspace.
    const std::string outWSName =
        outputWorkspaceName("GroupCounts_TwoPeriods_Plus");

    MatrixWorkspace_sptr inWSFirst = createWorkspace();
    MatrixWorkspace_sptr inWSSecond = createWorkspace();
    auto inputWSGroup = boost::shared_ptr<WorkspaceGroup>(new WorkspaceGroup());
    inputWSGroup->addWorkspace(inWSFirst);
    inputWSGroup->addWorkspace(inWSSecond);

    MuonCalculateAsymmetry alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWSGroup));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SummedPeriodSet", "1,2"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "GroupCounts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupIndex", 1));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    auto ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    TS_ASSERT(ws);

    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 3);

      TS_ASSERT_EQUALS(ws->readY(0)[0], 8);
      TS_ASSERT_EQUALS(ws->readY(0)[1], 10);
      TS_ASSERT_EQUALS(ws->readY(0)[2], 12);

      TS_ASSERT_EQUALS(ws->readX(0)[0], 1);
      TS_ASSERT_EQUALS(ws->readX(0)[1], 2);
      TS_ASSERT_EQUALS(ws->readX(0)[2], 3);

      TS_ASSERT_DELTA(ws->readE(0)[0], 0.566, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[1], 0.707, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[2], 0.849, 0.001);
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_groupCounts_twoPeriod_minus() {
    // Name of the output workspace.
    const std::string outWSName =
        outputWorkspaceName("GroupCounts_TwoPeriods_Minus");

    MatrixWorkspace_sptr inWSFirst = createWorkspace(3);
    MatrixWorkspace_sptr inWSSecond = createWorkspace();
    auto inputWSGroup = boost::shared_ptr<WorkspaceGroup>(new WorkspaceGroup());
    inputWSGroup->addWorkspace(inWSFirst);
    inputWSGroup->addWorkspace(inWSSecond);

    MuonCalculateAsymmetry alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWSGroup));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SummedPeriodSet", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SubtractedPeriodSet", "2"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "GroupCounts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupIndex", 1));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    auto ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    TS_ASSERT(ws);

    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 3);

      TS_ASSERT_EQUALS(ws->readY(0)[0], 3);
      TS_ASSERT_EQUALS(ws->readY(0)[1], 3);
      TS_ASSERT_EQUALS(ws->readY(0)[2], 3);

      TS_ASSERT_EQUALS(ws->readX(0)[0], 1);
      TS_ASSERT_EQUALS(ws->readX(0)[1], 2);
      TS_ASSERT_EQUALS(ws->readX(0)[2], 3);

      TS_ASSERT_DELTA(ws->readE(0)[0], 0.806, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[1], 0.943, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[2], 1.082, 0.001);
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  /**
   * Test period 1+2+3 for group counts
   */
  void test_groupCounts_threePeriods_plus() {
    // Name of the output workspace.
    const std::string outWSName =
        outputWorkspaceName("GroupCounts_ThreePeriods_Plus");

    MatrixWorkspace_sptr inWSFirst = createWorkspace();
    MatrixWorkspace_sptr inWSSecond = createWorkspace();
    MatrixWorkspace_sptr inWSThird = createWorkspace();
    auto inputWSGroup = boost::shared_ptr<WorkspaceGroup>(new WorkspaceGroup());
    inputWSGroup->addWorkspace(inWSFirst);
    inputWSGroup->addWorkspace(inWSSecond);
    inputWSGroup->addWorkspace(inWSThird);

    MuonCalculateAsymmetry alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWSGroup));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SummedPeriodSet", "1,2,3"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "GroupCounts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupIndex", 1));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    auto ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
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

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  /**
  * Test period 1+2-3 for group counts
  */
  void test_groupCounts_threePeriods_minus() {
    // Name of the output workspace.
    const std::string outWSName =
        outputWorkspaceName("GroupCounts_ThreePeriods_Minus");

    MatrixWorkspace_sptr inWSFirst = createWorkspace();
    MatrixWorkspace_sptr inWSSecond = createWorkspace();
    MatrixWorkspace_sptr inWSThird = createWorkspace();
    auto inputWSGroup = boost::shared_ptr<WorkspaceGroup>(new WorkspaceGroup());
    inputWSGroup->addWorkspace(inWSFirst);
    inputWSGroup->addWorkspace(inWSSecond);
    inputWSGroup->addWorkspace(inWSThird);

    MuonCalculateAsymmetry alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWSGroup));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SummedPeriodSet", "1,2"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SubtractedPeriodSet", "3"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "GroupCounts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupIndex", 1));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    auto ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
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

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_groupAsymmetry_singlePeriod() {
    // Name of the output workspace.
    const std::string outWSName = outputWorkspaceName("GroupAsymmetry");

    MatrixWorkspace_sptr inWS = createWorkspace();
    auto inputWSGroup = boost::shared_ptr<WorkspaceGroup>(new WorkspaceGroup());
    inputWSGroup->addWorkspace(inWS);

    MuonCalculateAsymmetry alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWSGroup));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "GroupAsymmetry"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupIndex", 2));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    auto ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    TS_ASSERT(ws);

    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 3);

      TS_ASSERT_DELTA(ws->readY(0)[0], -0.247, 0.001);
      TS_ASSERT_DELTA(ws->readY(0)[1], 0.356, 0.001);
      TS_ASSERT_DELTA(ws->readY(0)[2], 1.405, 0.001);

      TS_ASSERT_EQUALS(ws->readX(0)[0], 1);
      TS_ASSERT_EQUALS(ws->readX(0)[1], 2);
      TS_ASSERT_EQUALS(ws->readX(0)[2], 3);

      TS_ASSERT_DELTA(ws->readE(0)[0], 0.075, 0.01);
      TS_ASSERT_DELTA(ws->readE(0)[1], 0.136, 0.01);
      TS_ASSERT_DELTA(ws->readE(0)[2], 0.240, 0.01);
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_groupAsymmetry_twoPeriods_minus() {
    // Name of the output workspace.
    const std::string outWSName = outputWorkspaceName("GroupAsymmetry");

    MatrixWorkspace_sptr inWS = createWorkspace(3);
    MatrixWorkspace_sptr inWSSecond = createWorkspace();
    auto inputWSGroup = boost::shared_ptr<WorkspaceGroup>(new WorkspaceGroup());
    inputWSGroup->addWorkspace(inWS);
    inputWSGroup->addWorkspace(inWSSecond);

    MuonCalculateAsymmetry alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inputWSGroup);
    alg.setProperty("SummedPeriodSet", "1");
    alg.setProperty("SubtractedPeriodSet", "2");
    alg.setProperty("OutputType", "GroupAsymmetry");
    alg.setProperty("GroupIndex", 2);
    alg.setPropertyValue("OutputWorkspace", outWSName);
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    auto ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    TS_ASSERT(ws);

    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 3);

      TS_ASSERT_EQUALS(ws->readX(0)[0], 1);
      TS_ASSERT_EQUALS(ws->readX(0)[1], 2);
      TS_ASSERT_EQUALS(ws->readX(0)[2], 3);

      TS_ASSERT_DELTA(ws->readY(0)[0], 0.0030, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[1], -0.0455, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[2], -0.1511, 0.0001);

      TS_ASSERT_DELTA(ws->readE(0)[0], 0.1066, 0.0001);
      TS_ASSERT_DELTA(ws->readE(0)[1], 0.1885, 0.0001);
      TS_ASSERT_DELTA(ws->readE(0)[2], 0.3295, 0.0001);
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_groupAsymmetry_twoPeriods_plus() {

    // Name of the output workspace.
    const std::string outWSName = outputWorkspaceName("GroupAsymmetry");

    MatrixWorkspace_sptr inWS = createWorkspace(3);
    MatrixWorkspace_sptr inWSSecond = createWorkspace();
    auto inputWSGroup = boost::shared_ptr<WorkspaceGroup>(new WorkspaceGroup());
    inputWSGroup->addWorkspace(inWS);
    inputWSGroup->addWorkspace(inWSSecond);

    MuonCalculateAsymmetry alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inputWSGroup);
    alg.setProperty("SummedPeriodSet", "1,2");
    alg.setProperty("OutputType", "GroupAsymmetry");
    alg.setProperty("GroupIndex", 1);
    alg.setPropertyValue("OutputWorkspace", outWSName);
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    auto ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    TS_ASSERT(ws);

    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 3);

      TS_ASSERT_EQUALS(ws->readX(0)[0], 1);
      TS_ASSERT_EQUALS(ws->readX(0)[1], 2);
      TS_ASSERT_EQUALS(ws->readX(0)[2], 3);

      TS_ASSERT_DELTA(ws->readY(0)[0], -0.2529, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[1], 0.3918, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[2], 1.5316, 0.0001);

      TS_ASSERT_DELTA(ws->readE(0)[0], 0.0547, 0.0001);
      TS_ASSERT_DELTA(ws->readE(0)[1], 0.1010, 0.0001);
      TS_ASSERT_DELTA(ws->readE(0)[2], 0.1825, 0.0001);
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  /**
   * Test group asymmetry calculation for 3 periods 1+2+3
   */
  void test_groupAsymmetry_threePeriods_plus() {

    // Name of the output workspace.
    const std::string outWSName =
        outputWorkspaceName("GroupAsymmetry_Multi_Plus");

    MatrixWorkspace_sptr periodOne = createWorkspace();
    MatrixWorkspace_sptr periodTwo = createWorkspace(3);
    MatrixWorkspace_sptr periodThree = createWorkspace(1);
    auto inputWSGroup = boost::shared_ptr<WorkspaceGroup>(new WorkspaceGroup());
    inputWSGroup->addWorkspace(periodOne);
    inputWSGroup->addWorkspace(periodTwo);
    inputWSGroup->addWorkspace(periodThree);

    MuonCalculateAsymmetry alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inputWSGroup);
    alg.setProperty("SummedPeriodSet", "1,2,3");
    alg.setProperty("OutputType", "GroupAsymmetry");
    alg.setProperty("GroupIndex", 1);
    alg.setPropertyValue("OutputWorkspace", outWSName);
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    auto ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    TS_ASSERT(ws);

    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 3);

      TS_ASSERT_EQUALS(ws->readX(0)[0], 1);
      TS_ASSERT_EQUALS(ws->readX(0)[1], 2);
      TS_ASSERT_EQUALS(ws->readX(0)[2], 3);

      TS_ASSERT_DELTA(ws->readY(0)[0], -0.2523, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[1], 0.3996, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[2], 1.5549, 0.0001);

      TS_ASSERT_DELTA(ws->readE(0)[0], 0.0443, 0.0001);
      TS_ASSERT_DELTA(ws->readE(0)[1], 0.0823, 0.0001);
      TS_ASSERT_DELTA(ws->readE(0)[2], 0.1496, 0.0001);
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  /**
  * Test group asymmetry calculation for 3 periods 1+2-3
  */
  void test_groupAsymmetry_threePeriods_minus() {

    // Name of the output workspace.
    const std::string outWSName =
        outputWorkspaceName("GroupAsymmetry_Multi_Minus");

    MatrixWorkspace_sptr periodOne = createWorkspace();
    MatrixWorkspace_sptr periodTwo = createWorkspace(3);
    MatrixWorkspace_sptr periodThree = createWorkspace(1);
    auto inputWSGroup = boost::shared_ptr<WorkspaceGroup>(new WorkspaceGroup());
    inputWSGroup->addWorkspace(periodOne);
    inputWSGroup->addWorkspace(periodTwo);
    inputWSGroup->addWorkspace(periodThree);

    MuonCalculateAsymmetry alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inputWSGroup);
    alg.setProperty("SummedPeriodSet", "1,2");
    alg.setProperty("SubtractedPeriodSet", "3");
    alg.setProperty("OutputType", "GroupAsymmetry");
    alg.setProperty("GroupIndex", 1);
    alg.setPropertyValue("OutputWorkspace", outWSName);
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    auto ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    TS_ASSERT(ws);

    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 3);

      TS_ASSERT_EQUALS(ws->readX(0)[0], 1);
      TS_ASSERT_EQUALS(ws->readX(0)[1], 2);
      TS_ASSERT_EQUALS(ws->readX(0)[2], 3);

      TS_ASSERT_DELTA(ws->readY(0)[0], -0.0029, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[1], -0.0269, 0.0001);
      TS_ASSERT_DELTA(ws->readY(0)[2], -0.0777, 0.0001);

      TS_ASSERT_DELTA(ws->readE(0)[0], 0.0928, 0.0001);
      TS_ASSERT_DELTA(ws->readE(0)[1], 0.1741, 0.0001);
      TS_ASSERT_DELTA(ws->readE(0)[2], 0.3184, 0.0001);
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_pairAsymmetry_singlePeriod() {
    // Name of the output workspace.
    const std::string outWSName = outputWorkspaceName("GroupAsymmetry");

    MatrixWorkspace_sptr inWS = createWorkspace();
    auto inputWSGroup = boost::shared_ptr<WorkspaceGroup>(new WorkspaceGroup());
    inputWSGroup->addWorkspace(inWS);

    MuonCalculateAsymmetry alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWSGroup));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "PairAsymmetry"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PairFirstIndex", 2));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PairSecondIndex", 0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Alpha", 0.5));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    auto ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
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

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_pairAsymmetry_twoPeriods_minus() {
    // Name of the output workspace.
    const std::string outWSName = outputWorkspaceName("GroupAsymmetry");

    MatrixWorkspace_sptr inWS = createWorkspace(3);
    MatrixWorkspace_sptr inWSSecond = createWorkspace();
    auto inputWSGroup = boost::shared_ptr<WorkspaceGroup>(new WorkspaceGroup());
    inputWSGroup->addWorkspace(inWS);
    inputWSGroup->addWorkspace(inWSSecond);

    MuonCalculateAsymmetry alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inputWSGroup);
    alg.setProperty("SummedPeriodSet", "1");
    alg.setProperty("SubtractedPeriodSet", "2");
    alg.setProperty("OutputType", "PairAsymmetry");
    alg.setProperty("PairFirstIndex", 2);
    alg.setProperty("PairSecondIndex", 0);
    alg.setPropertyValue("OutputWorkspace", outWSName);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    auto ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
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

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_pairAsymmetry_twoPeriods_plus() {

    // Name of the output workspace.
    const std::string outWSName = outputWorkspaceName("GroupAsymmetry");

    MatrixWorkspace_sptr inWS = createWorkspace(3);
    MatrixWorkspace_sptr inWSSecond = createWorkspace();
    auto inputWSGroup = boost::shared_ptr<WorkspaceGroup>(new WorkspaceGroup());
    inputWSGroup->addWorkspace(inWS);
    inputWSGroup->addWorkspace(inWSSecond);

    MuonCalculateAsymmetry alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inputWSGroup);
    alg.setProperty("SummedPeriodSet", "1,2");
    alg.setProperty("OutputType", "PairAsymmetry");
    alg.setProperty("PairFirstIndex", 0);
    alg.setProperty("PairSecondIndex", 2);
    alg.setPropertyValue("OutputWorkspace", outWSName);
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    auto ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
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

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_pairAsymmetry_threePeriods_minus() {
    // Name of the output workspace.
    const std::string outWSName =
        outputWorkspaceName("PairAsymmetry_three_minus");

    MatrixWorkspace_sptr inWS = createWorkspace(3);
    MatrixWorkspace_sptr inWSSecond = createWorkspace();
    MatrixWorkspace_sptr inWSThird = createWorkspace(2);
    auto inputWSGroup = boost::shared_ptr<WorkspaceGroup>(new WorkspaceGroup());
    inputWSGroup->addWorkspace(inWS);
    inputWSGroup->addWorkspace(inWSSecond);
    inputWSGroup->addWorkspace(inWSThird);

    MuonCalculateAsymmetry alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inputWSGroup);
    alg.setProperty("SummedPeriodSet", "1,2");
    alg.setProperty("SubtractedPeriodSet", "3");
    alg.setProperty("OutputType", "PairAsymmetry");
    alg.setProperty("PairFirstIndex", 2);
    alg.setProperty("PairSecondIndex", 0);
    alg.setPropertyValue("OutputWorkspace", outWSName);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    auto ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
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

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_pairAsymmetry_threePeriods_plus() {
    // Name of the output workspace.
    const std::string outWSName =
        outputWorkspaceName("PairAsymmetry_three_plus");

    MatrixWorkspace_sptr inWS = createWorkspace(3);
    MatrixWorkspace_sptr inWSSecond = createWorkspace();
    MatrixWorkspace_sptr inWSThird = createWorkspace(2);
    auto inputWSGroup = boost::shared_ptr<WorkspaceGroup>(new WorkspaceGroup());
    inputWSGroup->addWorkspace(inWS);
    inputWSGroup->addWorkspace(inWSSecond);
    inputWSGroup->addWorkspace(inWSThird);

    MuonCalculateAsymmetry alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inputWSGroup);
    alg.setProperty("SummedPeriodSet", "1,2,3");
    alg.setProperty("OutputType", "PairAsymmetry");
    alg.setProperty("PairFirstIndex", 2);
    alg.setProperty("PairSecondIndex", 0);
    alg.setPropertyValue("OutputWorkspace", outWSName);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    auto ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
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

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  /// Test that algorithm throws an exception when passed an empty
  /// WorkspaceGroup as input.
  void test_throws_emptyGroup() {

    // Name of the output workspace.
    const std::string outWSName = outputWorkspaceName("GroupAsymmetry");

    auto inputWSGroup = boost::shared_ptr<WorkspaceGroup>(new WorkspaceGroup());

    MuonCalculateAsymmetry alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inputWSGroup);
    alg.setProperty("SummedPeriodSet", "1,2");
    alg.setProperty("OutputType", "PairAsymmetry");
    alg.setProperty("PairFirstIndex", 0);
    alg.setProperty("PairSecondIndex", 2);
    alg.setPropertyValue("OutputWorkspace", outWSName);
    // Should throw an exception
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
    // check that the algorithm didn't execute
    TS_ASSERT(!alg.isExecuted());
  }

private:
  std::string outputWorkspaceName(std::string testName) {
    return "MuonCalculateAsymmetryTest_" + testName + "_OutputWS";
  }

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
  MatrixWorkspace_sptr createWorkspace(double delta = 0.0) {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::Create2DWorkspace(3, 3);

    for (size_t i = 0; i < ws->getNumberHistograms(); i++) {
      for (size_t j = 0; j < ws->blocksize(); j++) {
        double v = static_cast<double>(i * ws->blocksize() + j) + 1.0 + delta;

        ws->dataY(i)[j] = v;
        ws->dataX(i)[j] = static_cast<double>(j + 1);
        ws->dataE(i)[j] = v * 0.1;
      }
    }

    return ws;
  }
};

#endif /* MANTID_WORKFLOWALGORITHMS_MUONCALCULATEASYMMETRYTEST_H_ */
