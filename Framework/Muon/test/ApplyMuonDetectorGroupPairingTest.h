// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/CompareWorkspaces.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/PhysicalConstants.h"

#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/MuonWorkspaceCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidMuon/ApplyMuonDetectorGroupPairing.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::Muon;

namespace {

// Set algorithm properties to sensible defaults (assuming data with 10 groups)
// Use when specifying groups manually
void setPairAlgorithmProperties(ApplyMuonDetectorGroupPairing &alg, const std::string &inputWSName,
                                const std::string &wsGroupName) {
  alg.setProperty("SpecifyGroupsManually", true);
  alg.setProperty("PairName", "test");
  alg.setProperty("Alpha", 1.0);
  alg.setProperty("InputWorkspace", inputWSName);
  alg.setProperty("InputWorkspaceGroup", wsGroupName);
  alg.setProperty("Group1", "1-5");
  alg.setProperty("Group2", "5-10");
  alg.setProperty("TimeMin", 0.0);
  alg.setProperty("TimeMax", 30.0);
  alg.setProperty("RebinArgs", "");
  alg.setProperty("TimeOffset", 0.0);
  alg.setProperty("SummedPeriods", std::to_string(1));
  alg.setProperty("SubtractedPeriods", "");
  alg.setProperty("ApplyDeadTimeCorrection", false);
  alg.setLogging(false);
}

// Set algorithm properties to sensible defaults (assuming data with 10 groups)
// Use when entering workspaces to pair
void setPairAlgorithmPropertiesForInputWorkspace(ApplyMuonDetectorGroupPairing &alg, const std::string &inputWSName,
                                                 const std::string &wsGroupName) {
  alg.setProperty("SpecifyGroupsManually", false);
  alg.setProperty("PairName", "test");
  alg.setProperty("Alpha", 1.0);
  alg.setProperty("InputWorkspace", inputWSName);
  alg.setProperty("InputWorkspaceGroup", wsGroupName);
  alg.setLogging(false);
}

// Simple class to set up the ADS with the configuration required by the
// algorithm (a MatrixWorkspace and an empty group).
class setUpADSWithWorkspace {
public:
  setUpADSWithWorkspace(const Workspace_sptr &ws) {
    AnalysisDataService::Instance().addOrReplace(inputWSName, ws);
    wsGroup = std::make_shared<WorkspaceGroup>();
    AnalysisDataService::Instance().addOrReplace(groupWSName, wsGroup);
  };

  ~setUpADSWithWorkspace() { AnalysisDataService::Instance().clear(); };
  WorkspaceGroup_sptr wsGroup;

  const std::string inputWSName = "inputData";
  const std::string groupWSName = "inputGroup";
};

} // namespace

class ApplyMuonDetectorGroupPairingTest : public CxxTest::TestSuite {
public:
  // WorkflowAlgorithms do not appear in the FrameworkManager without this line
  ApplyMuonDetectorGroupPairingTest() { Mantid::API::FrameworkManager::Instance(); }
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ApplyMuonDetectorGroupPairingTest *createSuite() { return new ApplyMuonDetectorGroupPairingTest(); }
  static void destroySuite(ApplyMuonDetectorGroupPairingTest *suite) { delete suite; }

  void test_Init() {
    ApplyMuonDetectorGroupPairing alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_nonAlphanumericPairNamesNotAllowed() {
    MatrixWorkspace_sptr ws = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(10, 10);
    setUpADSWithWorkspace setup(ws);

    ApplyMuonDetectorGroupPairing alg;
    alg.initialize();
    setPairAlgorithmProperties(alg, setup.inputWSName, setup.groupWSName);

    std::vector<std::string> badPairNames = {"", "!", ";name;", ".", ",", ";", ":"};
    for (auto badName : badPairNames) {
      alg.setProperty("PairName", badName);
      TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
      TS_ASSERT(!alg.isExecuted());
    }
  }

  void test_zeroOrNegativeAlphaNotAllowed() {
    MatrixWorkspace_sptr ws = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(10, 10);
    setUpADSWithWorkspace setup(ws);

    ApplyMuonDetectorGroupPairing alg;
    alg.initialize();
    setPairAlgorithmProperties(alg, setup.inputWSName, setup.groupWSName);

    std::vector<double> badAlphas = {0.0, -1.0};
    for (auto badAlpha : badAlphas) {
      alg.setProperty("Alpha", badAlpha);
      TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
      TS_ASSERT(!alg.isExecuted());
    }
  }

  void test_throwsIfTwoGroupsAreIdentical() {
    MatrixWorkspace_sptr ws = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(10, 10);
    setUpADSWithWorkspace setup(ws);

    ApplyMuonDetectorGroupPairing alg;
    alg.initialize();
    setPairAlgorithmProperties(alg, setup.inputWSName, setup.groupWSName);

    std::vector<std::string> badGroup1 = {"1-5", "1-5", "1-5", "1-5"};
    std::vector<std::string> badGroup2 = {"1-5", "1,2,3,4,5", "5,4,3,2,1", "1,2,2,3,4,5,5,5"};
    for (size_t i = 0; i < badGroup1.size(); i++) {
      alg.setProperty("Group1", badGroup1[i]);
      alg.setProperty("Group2", badGroup2[i]);
      TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
      TS_ASSERT(!alg.isExecuted());
    }
  }

  void test_throwsIfTimeMinGreaterThanTimeMax() {
    MatrixWorkspace_sptr ws = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(10, 10);
    setUpADSWithWorkspace setup(ws);

    ApplyMuonDetectorGroupPairing alg;
    alg.initialize();
    setPairAlgorithmProperties(alg, setup.inputWSName, setup.groupWSName);

    alg.setProperty("TimeMin", 10.0);
    alg.setProperty("TimeMax", 5.0);
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_throwsIfPeriodOutOfRange() {
    // If inputWS is a matrixWorkspace then the summed/subtracted
    // periods are set to "1" and "" and so no checks are needed.
    int nPeriods = 2;
    WorkspaceGroup_sptr ws =
        MuonWorkspaceCreationHelper::createMultiPeriodWorkspaceGroup(nPeriods, 10, 10, "MuonAnalysis");
    ;
    setUpADSWithWorkspace setup(ws);

    ApplyMuonDetectorGroupPairing alg;
    alg.initialize();
    setPairAlgorithmProperties(alg, setup.inputWSName, setup.groupWSName);

    std::vector<std::string> badPeriods = {"3", "1,2,3,4", "-1"};

    for (auto &&badPeriod : badPeriods) {
      alg.setProperty("SummedPeriods", badPeriod);
      // This throw comes from MuonProcess
      TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
      TS_ASSERT(!alg.isExecuted());
    }

    for (auto &&badPeriod : badPeriods) {
      alg.setProperty("SubtractedPeriods", badPeriod);
      // This throw comes from MuonProcess
      TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
      TS_ASSERT(!alg.isExecuted());
    }
  }

  void test_producesOutputWorkspacesInWorkspaceGroup() {
    MatrixWorkspace_sptr ws = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(10, 5);
    setUpADSWithWorkspace setup(ws);

    ApplyMuonDetectorGroupPairing alg;
    alg.initialize();
    setPairAlgorithmProperties(alg, setup.inputWSName, setup.groupWSName);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    TS_ASSERT(setup.wsGroup);
    if (setup.wsGroup) {
      TS_ASSERT_EQUALS(setup.wsGroup->getNumberOfEntries(), 2);
    }
  }

  void test_outputWorkspacesHaveCorrectName() {
    MatrixWorkspace_sptr ws = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(10, 5);
    setUpADSWithWorkspace setup(ws);

    ApplyMuonDetectorGroupPairing alg;
    alg.initialize();
    setPairAlgorithmProperties(alg, setup.inputWSName, setup.groupWSName);
    alg.execute();

    TS_ASSERT(setup.wsGroup->getItem("inputGroup; Pair; test; Asym; #1"));
    TS_ASSERT(setup.wsGroup->getItem("inputGroup; Pair; test; Asym; #1_Raw"));
  }

  void test_workspacePairingHasCorrectAsymmetryValues() {
    MatrixWorkspace_sptr ws =
        MuonWorkspaceCreationHelper::createAsymmetryWorkspace(10, 10, MuonWorkspaceCreationHelper::yDataAsymmetry());
    setUpADSWithWorkspace setup(ws);

    ApplyMuonDetectorGroupPairing alg;
    alg.initialize();
    setPairAlgorithmProperties(alg, setup.inputWSName, setup.groupWSName);
    alg.execute();
    auto wsOut =
        std::dynamic_pointer_cast<MatrixWorkspace>(setup.wsGroup->getItem("inputGroup; Pair; test; Asym; #1_Raw"));

    // Current behaviour is to convert bin edge x-values to bin centre x-values
    // (point data) so there is on fewer x-value now.
    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.050, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.450, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.950, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], -0.4692, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 1.0000, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], -0.6119, 0.0001);

    // The error calculation as per Issue #5035
    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.04212, 0.00001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.06946, 0.00001);
  }

  void test_timeOffsetShiftsTimeAxisCorrectly() {
    MatrixWorkspace_sptr ws = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(10, 10);
    setUpADSWithWorkspace setup(ws);

    ApplyMuonDetectorGroupPairing alg;
    alg.initialize();
    setPairAlgorithmProperties(alg, setup.inputWSName, setup.groupWSName);
    alg.setProperty("TimeOffset", 0.2);
    alg.execute();
    auto wsOut =
        std::dynamic_pointer_cast<MatrixWorkspace>(setup.wsGroup->getItem("inputGroup; Pair; test; Asym; #1_Raw"));

    // Account for the bin edges to point data conversion
    double shift = 0.2 + 0.05;
    TS_ASSERT_DELTA(wsOut->readX(0)[0], ws->readX(0)[0] + shift, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], ws->readX(0)[4] + shift, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], ws->readX(0)[9] + shift, 0.001);
  }

  void test_throwsIfRequestedDetectorIDsNotInWorkspace() {

    MatrixWorkspace_sptr ws = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(5, 10);
    setUpADSWithWorkspace setup(ws);

    ApplyMuonDetectorGroupPairing alg;
    alg.initialize();
    // Expects 10 IDs
    setPairAlgorithmProperties(alg, setup.inputWSName, setup.groupWSName);

    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }

  void test_summingPeriodsGivesCorrectAsymmetryValues() {
    WorkspaceGroup_sptr ws = MuonWorkspaceCreationHelper::createMultiPeriodWorkspaceGroup(4, 10, 10, "MuonAnalysis");
    setUpADSWithWorkspace setup(ws);

    ApplyMuonDetectorGroupPairing alg;
    alg.initialize();
    setPairAlgorithmProperties(alg, setup.inputWSName, setup.groupWSName);
    alg.setProperty("SummedPeriods", "1,2");
    alg.execute();
    auto wsOut =
        std::dynamic_pointer_cast<MatrixWorkspace>(setup.wsGroup->getItem("inputGroup; Pair; test; Asym; #1_Raw"));

    // Summation of periods occurs before asymmetry calculation
    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.050, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.450, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.950, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], -0.5755, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], -0.5368, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], -0.4963, 0.0001);

    // The error calculation as per Issue #5035
    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.03625, 0.00001);
    TS_ASSERT_DELTA(wsOut->readE(0)[4], 0.03420, 0.00001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.03208, 0.00001);
  }

  void test_subtractingPeriodsGivesCorrectAsymmetryValues() {
    WorkspaceGroup_sptr ws = MuonWorkspaceCreationHelper::createMultiPeriodWorkspaceGroup(4, 10, 10, "MuonAnalysis");
    setUpADSWithWorkspace setup(ws);

    ApplyMuonDetectorGroupPairing alg;
    alg.initialize();
    setPairAlgorithmProperties(alg, setup.inputWSName, setup.groupWSName);
    alg.setProperty("SummedPeriods", "1,2");
    alg.setProperty("SubtractedPeriods", "3");
    alg.execute();
    auto wsOut =
        std::dynamic_pointer_cast<MatrixWorkspace>(setup.wsGroup->getItem("inputGroup; Pair; test; Asym; #1_Raw"));

    // Summation of periods occurs before asymmetry calculation
    // Subtraction of periods occurs AFTER asymmetry calculation
    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.050, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.450, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.950, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], -0.0153, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], -0.0130, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], -0.0108, 0.0001);

    // The error calculation as per Issue #5035
    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.0619, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[4], 0.0585, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.0550, 0.0001);
  }

  void test_applyingDeadTimeCorrectionGivesCorrectAsymmetryValues() {

    MatrixWorkspace_sptr ws =
        MuonWorkspaceCreationHelper::createAsymmetryWorkspace(10, 10, MuonWorkspaceCreationHelper::yDataAsymmetry());
    setUpADSWithWorkspace setup(ws);

    ApplyMuonDetectorGroupPairing alg;
    alg.initialize();
    setPairAlgorithmProperties(alg, setup.inputWSName, setup.groupWSName);

    // Apply same dead time to every spectra
    std::vector<double> deadTimes(10, 0.0025);
    ITableWorkspace_sptr deadTimeTable = MuonWorkspaceCreationHelper::createDeadTimeTable(10, deadTimes);

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ApplyDeadTimeCorrection", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DeadTimeTable", deadTimeTable));

    alg.execute();

    auto wsOut =
        std::dynamic_pointer_cast<MatrixWorkspace>(setup.wsGroup->getItem("inputGroup; Pair; test; Asym; #1_Raw"));

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.050, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.450, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.950, 0.001);

    // Dead time applied before asymmetry
    TS_ASSERT_DELTA(wsOut->readY(0)[0], -0.5181, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 1.0000, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], -0.6350, 0.0001);

    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.0386, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.0668, 0.0001);
  }

  void test_asymmetryValuesCorrectWhenEnteringWorkspacesByHand() {

    MatrixWorkspace_sptr ws = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(10, 10);
    setUpADSWithWorkspace setup(ws);
    ApplyMuonDetectorGroupPairing alg;
    alg.initialize();
    setPairAlgorithmPropertiesForInputWorkspace(alg, setup.inputWSName, setup.groupWSName);
    MatrixWorkspace_sptr groupWS1 = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(
        1, 10, MuonWorkspaceCreationHelper::yDataAsymmetry(0.5, 0.1));
    MatrixWorkspace_sptr groupWS2 = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(
        1, 10, MuonWorkspaceCreationHelper::yDataAsymmetry(1.0, 0.2));
    const std::string groupWS1Name = "EMU000012345; Group; fwd; Counts; #1_Raw";
    const std::string groupWS2Name = "EMU000012345; Group; bwd; Counts; #1_Raw";
    AnalysisDataService::Instance().addOrReplace(groupWS1Name, groupWS1);
    AnalysisDataService::Instance().addOrReplace(groupWS2Name, groupWS2);
    setup.wsGroup->add(groupWS1Name);
    setup.wsGroup->add(groupWS2Name);
    alg.setProperty("InputWorkspace1", groupWS1Name);
    alg.setProperty("InputWorkspace2", groupWS2Name);
    alg.execute();

    auto wsOut =
        std::dynamic_pointer_cast<MatrixWorkspace>(setup.wsGroup->getItem("inputGroup; Pair; test; Asym; #1_Raw"));

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.050, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.450, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.950, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], -0.1388, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 0.2900, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], -0.02262, 0.001);

    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.2421, 0.001);
    TS_ASSERT_DELTA(wsOut->readE(0)[4], 0.4737, 0.001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.3950, 0.001);
  }

  void test_inputWorkspaceWithMultipleSpectraFails() {
    // We expect the input workspaces to have a single spectra.

    MatrixWorkspace_sptr ws = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(10, 10);
    setUpADSWithWorkspace setup(ws);
    ApplyMuonDetectorGroupPairing alg;
    alg.initialize();
    setPairAlgorithmPropertiesForInputWorkspace(alg, setup.inputWSName, setup.groupWSName);
    MatrixWorkspace_sptr groupWS1 = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(
        2, 10, MuonWorkspaceCreationHelper::yDataAsymmetry(0.5, 0.1));
    MatrixWorkspace_sptr groupWS2 = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(
        1, 10, MuonWorkspaceCreationHelper::yDataAsymmetry(1.0, 0.2));
    const std::string groupWS1Name = "EMU000012345; Group; fwd; Counts; #1_Raw";
    const std::string groupWS2Name = "EMU000012345; Group; bwd; Counts; #1_Raw";
    AnalysisDataService::Instance().addOrReplace(groupWS1Name, groupWS1);
    AnalysisDataService::Instance().addOrReplace(groupWS2Name, groupWS2);
    setup.wsGroup->add(groupWS1Name);
    setup.wsGroup->add(groupWS2Name);
    alg.setProperty("InputWorkspace1", groupWS1Name);
    alg.setProperty("InputWorkspace2", groupWS2Name);

    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }

  void test_inputWorkspaceWithDifferentTimeAxisFails() {
    // e.g. rebin with non-rebin should throw an error from this algorithm.

    MatrixWorkspace_sptr ws = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(10, 10);
    setUpADSWithWorkspace setup(ws);
    ApplyMuonDetectorGroupPairing alg;
    alg.initialize();
    setPairAlgorithmPropertiesForInputWorkspace(alg, setup.inputWSName, setup.groupWSName);
    MatrixWorkspace_sptr groupWS1 = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(
        1, 10, MuonWorkspaceCreationHelper::yDataAsymmetry(0.5, 0.1));
    MatrixWorkspace_sptr groupWS2 = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(
        1, 20, MuonWorkspaceCreationHelper::yDataAsymmetry(1.0, 0.2));
    const std::string groupWS1Name = "EMU000012345; Group; fwd; Counts; #1_Raw";
    const std::string groupWS2Name = "EMU000012345; Group; bwd; Counts; #1_Raw";
    AnalysisDataService::Instance().addOrReplace(groupWS1Name, groupWS1);
    AnalysisDataService::Instance().addOrReplace(groupWS2Name, groupWS2);
    setup.wsGroup->add(groupWS1Name);
    setup.wsGroup->add(groupWS2Name);
    alg.setProperty("InputWorkspace1", groupWS1Name);
    alg.setProperty("InputWorkspace2", groupWS2Name);

    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }
};
