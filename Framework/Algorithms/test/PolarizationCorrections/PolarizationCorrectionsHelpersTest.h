// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class PolarizationCorrectionsHelpersTest : public CxxTest::TestSuite {
public:
  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void testWorkspaceForFourSpinStates() { runTest({"00", "11", "10", "01"}); }

  void testWorkspaceForThreeSpinStates() { runTest({"01", "10", "00"}); }

  void testWorkspaceForTwoSpinStates() { runTest({"11", "10"}); }

  void testWorkspaceForOneSpinState() { runTest({"01"}); }

  void testSurplusWhitespace() { runTest({"01 ", "11", " 10", " 00 "}); }

  void testNoWorkspaceForRequestedSpinStateReturnsNull() {
    // Create a group with more child workspaces than are in the spin state order that we'll use for the test.
    // This is to test that we're getting the correct result even when the spin state order and workspace group sizes
    // are not the same.
    const auto grp = createGroupWorkspaceToMatchSpinStates({"01", "11", "10"});

    const auto spinStateOrder = "01,11";
    const auto missingSpinState = "00";

    auto ws = PolarizationCorrectionsHelpers::workspaceForSpinState(grp, spinStateOrder, missingSpinState);
    TS_ASSERT(ws == nullptr);
  }

  void testGetORSONotationForSpinStateForWildes() { runTestGetORSONotationForSpinStates(WILDES_SPIN_STATES); }

  void testGetORSONotationForSpinStateForFredrikze() { runTestGetORSONotationForSpinStates(FREDRIKZE_SPIN_STATES); }

  void testGetORSONotationForSpinStateForInvalidSpinStateThrowsError() {
    TS_ASSERT_THROWS(SpinStatesORSO::getORSONotationForSpinState("invalidSpinState"), std::invalid_argument &);
  }

  void testAddORSOLogForSpinStateForWildes() {
    auto ws = createWorkspace("testWs");
    runTestAddORSOLogForSpinStates(ws, WILDES_SPIN_STATES);
  }

  void testAddORSOLogForSpinStateForFredrikze() {
    auto ws = createWorkspace("testWs");
    runTestAddORSOLogForSpinStates(ws, FREDRIKZE_SPIN_STATES);
  }

  void testAddORSOLogForSpinStateForInvalidSpinStateThrowsError() {
    auto ws = createWorkspace("testWs");
    TS_ASSERT_THROWS(SpinStatesORSO::addORSOLogForSpinState(ws, "invalidSpinState"), std::invalid_argument &);
  }

  void testSplitSpinStateStringSuccessful() { runTestSplitSpinStateString("01,11,10,00", {"01", "11", "10", "00"}); }

  void testSplitSpinStateStringWithSpaces() {
    runTestSplitSpinStateString(" 01 ,  11 , 10 ,  00 ", {"01", "11", "10", "00"});
  }

  void testSplitSpinStateStringEmptyString() { runTestSplitSpinStateString("", {}); }

  void testSplitSpinStateStringSingleItem() { runTestSplitSpinStateString("01", {"01"}); }

  void testIndexOfWorkspaceForSpinState_TargetStateExists() {
    runTestIndexOfWorkspaceForSpinState({"00", "11", "10", "01"}, "10", true, 2);
  }

  void testIndexOfWorkspaceForSpinState_TargetStateDoesNotExist() {
    runTestIndexOfWorkspaceForSpinState({"00", "11", "10", "01"}, "invalid_state", false);
  }

  void testIndexOfWorkspaceForSpinState_EmptySpinStateOrder() { runTestIndexOfWorkspaceForSpinState({}, "10", false); }

  void testIndexOfWorkspaceForSpinState_DuplicateEntries() {
    runTestIndexOfWorkspaceForSpinState({"10", "10", "11"}, "10", true, 0);
  }

  void testIndexOfWorkspaceForSpinState_TrimWhitespace() {
    runTestIndexOfWorkspaceForSpinState({"00", "11", "10", "01"}, " 10 ", true, 2);
  }

  void testEmptySpinStateOrder() { runTestWorkspaceForSpinState({"01", "11"}, "", true); }

  void testDuplicateSpinStates() { runTestWorkspaceForSpinState({"01", "01", "11"}, "01", false, "01"); }

  void testWorkspaceForSpinStateUsingFredrikzeSpinStates() {
    runTestWorkspaceForSpinState(FREDRIKZE_SPIN_STATES, SpinStateConfigurationsFredrikze::PARA_ANTI, false,
                                 SpinStateConfigurationsFredrikze::PARA_ANTI);
  }

  void testWorkspaceForSpinStateUsingWildesSpinStates() {
    runTestWorkspaceForSpinState(WILDES_SPIN_STATES, SpinStateConfigurationsWildes::PLUS_MINUS, false,
                                 SpinStateConfigurationsWildes::PLUS_MINUS);
  }

private:
  const std::vector<std::string> WILDES_SPIN_STATES{
      SpinStateConfigurationsWildes::PLUS_PLUS,  SpinStateConfigurationsWildes::PLUS_MINUS,
      SpinStateConfigurationsWildes::MINUS_PLUS, SpinStateConfigurationsWildes::MINUS_MINUS,
      SpinStateConfigurationsWildes::PLUS,       SpinStateConfigurationsWildes::MINUS};

  const std::vector<std::string> FREDRIKZE_SPIN_STATES{
      SpinStateConfigurationsFredrikze::PARA_PARA, SpinStateConfigurationsFredrikze::PARA_ANTI,
      SpinStateConfigurationsFredrikze::ANTI_PARA, SpinStateConfigurationsFredrikze::ANTI_ANTI,
      SpinStateConfigurationsFredrikze::PARA,      SpinStateConfigurationsFredrikze::ANTI};

  const std::vector<std::string> ORSO_SPIN_STATES{SpinStatesORSO::PP, SpinStatesORSO::PM, SpinStatesORSO::MP,
                                                  SpinStatesORSO::MM, SpinStatesORSO::PO, SpinStatesORSO::MO};

  WorkspaceGroup_sptr createGroupWorkspaceToMatchSpinStates(const std::vector<std::string> &spinStateOrder) {
    WorkspaceGroup_sptr grp = std::make_shared<WorkspaceGroup>();
    for (size_t i = 0; i < spinStateOrder.size(); ++i) {
      auto trimmedSpinState = trimString(spinStateOrder[i]);
      grp->addWorkspace(createWorkspace(trimmedSpinState));
    }
    return grp;
  }

  MatrixWorkspace_sptr createWorkspace(const std::string &name) {
    auto createWorkspace = AlgorithmManager::Instance().create("CreateWorkspace");
    createWorkspace->initialize();
    createWorkspace->setProperty("DataX", std::vector<double>{0, 1});
    createWorkspace->setProperty("DataY", std::vector<double>{0, 1});
    createWorkspace->setProperty("OutputWorkspace", name);
    createWorkspace->execute();
    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(name);
    return ws;
  }

  void runTest(const std::vector<std::string> &spinStates) {
    auto spinStateOrder = boost::join(spinStates, ",");
    auto grp = createGroupWorkspaceToMatchSpinStates(spinStates);
    for (const auto &spinState : spinStates) {
      auto ws = PolarizationCorrectionsHelpers::workspaceForSpinState(grp, spinStateOrder, spinState);
      // The workspace name is not going to have any spaces in, regardless of the input. This is not
      // related to the actual workspaceForSpinState algorithm, it's just how the test is checking the
      // order
      auto trimmedSpinState = trimString(spinState);
      TS_ASSERT_EQUALS(trimmedSpinState, ws->getName());
    }
  }

  std::string trimString(const std::string &input) {
    auto trimmedInput = input;
    boost::trim(trimmedInput);
    return trimmedInput;
  }

  void runTestGetORSONotationForSpinStates(const std::vector<std::string> &spinStates) {
    for (size_t i = 0; i < spinStates.size(); i++) {
      const auto spinStateORSO = SpinStatesORSO::getORSONotationForSpinState(spinStates[i]);
      TS_ASSERT_EQUALS(spinStateORSO, ORSO_SPIN_STATES[i]);
    }
  }

  void runTestAddORSOLogForSpinStates(const MatrixWorkspace_sptr &ws, const std::vector<std::string> &spinStates) {
    // Looping through the spin states in this test also checks that we can overwrite any existing ORSO spin state log
    for (size_t i = 0; i < spinStates.size(); i++) {
      SpinStatesORSO::addORSOLogForSpinState(ws, spinStates[i]);
      const auto &run = ws->run();
      TS_ASSERT(run.hasProperty(SpinStatesORSO::LOG_NAME));
      TS_ASSERT_EQUALS(run.getPropertyValueAsType<std::string>(SpinStatesORSO::LOG_NAME), ORSO_SPIN_STATES[i]);
    }
  }

  void runTestSplitSpinStateString(const std::string &spinStates, const std::vector<std::string> &expectedResult) {
    std::vector<std::string> result = PolarizationCorrectionsHelpers::splitSpinStateString(spinStates);
    TS_ASSERT_EQUALS(result.size(), expectedResult.size());

    for (size_t i = 0; i < result.size(); ++i) {
      TS_ASSERT_EQUALS(result[i], expectedResult[i]);
    }
  }

  void runTestIndexOfWorkspaceForSpinState(const std::vector<std::string> &spinStateOrder,
                                           const std::string &targetSpinState, bool expectedHasValue,
                                           int expectedIndex = -1) {
    auto index = PolarizationCorrectionsHelpers::indexOfWorkspaceForSpinState(spinStateOrder, targetSpinState);

    if (expectedHasValue) {
      TS_ASSERT(index.has_value());
      TS_ASSERT_EQUALS(index.value(), expectedIndex);
    } else {
      ASSERT_FALSE(index.has_value());
    }
  }

  void runTestWorkspaceForSpinState(const std::vector<std::string> &spinStateOrder, const std::string &targetSpinState,
                                    bool expectedIsNull, const std::string &expectedWorkspaceName = "") {
    auto grp = createGroupWorkspaceToMatchSpinStates(spinStateOrder);
    auto ws =
        PolarizationCorrectionsHelpers::workspaceForSpinState(grp, boost::join(spinStateOrder, ","), targetSpinState);

    if (expectedIsNull) {
      TS_ASSERT(ws == nullptr);
    } else {
      TS_ASSERT(ws != nullptr);
      TS_ASSERT_EQUALS(ws->getName(), expectedWorkspaceName);
    }
  }
};
