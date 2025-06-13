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
#include "MantidKernel/Strings.h"

#include <boost/algorithm/string/join.hpp>
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

  void testEmptySpinState() { runTestWorkspaceForSpinState({"01", "11"}, "", true); }

  void testDuplicateSpinStates() { runTestWorkspaceForSpinState({"01", "01", "11"}, "01", false, "01"); }

  void testWorkspaceForSpinStateUsingFredrikzeSpinStates() {
    runTestWorkspaceForSpinState(FREDRIKZE_SPIN_STATES, SpinStateConfigurationsFredrikze::PARA_ANTI, false,
                                 SpinStateConfigurationsFredrikze::PARA_ANTI);
  }

  void testWorkspaceForSpinStateUsingWildesSpinStates() {
    runTestWorkspaceForSpinState(WILDES_SPIN_STATES, SpinStateConfigurationsWildes::PLUS_MINUS, false,
                                 SpinStateConfigurationsWildes::PLUS_MINUS);
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

  void testErrorPropagation_linear() {
    constexpr size_t vars = 1;
    using Types = Arithmetic::ErrorTypeHelper<vars>;

    const double m = 5.0;
    const double c = 3.0;
    const auto errorProp = Arithmetic::makeErrorPropagation<vars>([m, c](const auto &x) { return m * x[0] + c; });
    const auto result = errorProp.evaluate(Types::InputArray{{20.0}}, Types::InputArray{{0.5}});

    TS_ASSERT_EQUALS(result.error, 2.5);
    TS_ASSERT_EQUALS(result.value, 103);
  }

  void testErrorPropagation_quad() {
    const size_t vars = 1;
    using Types = Arithmetic::ErrorTypeHelper<vars>;
    const double a = 5.0;
    const double b = 3.0;
    const double c = 10.0;
    const auto errorProp =
        Arithmetic::makeErrorPropagation<vars>([a, b, c](const auto &x) { return (a * x[0] * x[0]) + (b * x[0]) + c; });
    const auto result = errorProp.evaluate(Types::InputArray{{20.0}}, Types::InputArray{{0.5}});

    TS_ASSERT_DELTA(result.error, 101.5, 0.001);
    TS_ASSERT_EQUALS(result.value, 2070);
  }

  void testErrorPropagation_mult_vars() {
    const size_t vars = 2;
    using Types = Arithmetic::ErrorTypeHelper<vars>;

    const auto errorProp =
        Arithmetic::makeErrorPropagation<vars>([](const auto &x) { return x[0] * sin(x[0]) + exp(x[1]); });
    const auto result = errorProp.evaluate(Types::InputArray{{3.141, 1.0}}, Types::InputArray{{0.01, 0.05}});

    TS_ASSERT_DELTA(result.error, 0.139495, 0.001);
    TS_ASSERT_DELTA(result.value, 2.72014, 0.00001);
  }

  void testErrorPropagation_workspaces_quad() {
    const size_t vars = 1;
    const double a = 5.0;
    const double b = 3.0;
    const double c = 10.0;
    const auto errorProp =
        Arithmetic::makeErrorPropagation<vars>([a, b, c](const auto &x) { return (a * x[0] * x[0]) + (b * x[0]) + c; });

    const auto ws = createWorkspace("ws", {0.0, 1.0, 2.0, 3.0, 4.0}, {20, 22, 24, 26, 28}, {0.5, 0.6, 0.7, 0.8, 0.9});
    const auto result = errorProp.evaluateWorkspaces(ws);

    const std::vector<double> expectedY{2070, 2496, 2962, 3468, 4014};
    const std::vector<double> expectedE{101.5, 133.8, 170.1, 210.4, 254.7};
    for (size_t i = 0; i < result->size(); i++) {
      TS_ASSERT_DELTA(expectedY.at(i), result->y(0)[i], 0.001);
      TS_ASSERT_DELTA(expectedE.at(i), result->e(0)[i], 0.001);
    }
  }

  void testErrorPropagation_workspaces_mult_vars() {
    const size_t vars = 2;
    const auto errorProp =
        Arithmetic::makeErrorPropagation<vars>([](const auto &x) { return x[0] * sin(x[0]) + exp(x[1]); });

    auto wsA = createWorkspace("wsA", {0.0, 1.0, 2.0}, {3.141, 3.141, 3.141}, {0.01, 0.01, 0.01});
    auto wsB = createWorkspace("wsB", {0.0, 1.0, 2.0}, {1.0, 1.0, 1.0}, {0.05, 0.05, 0.05});
    const auto result = errorProp.evaluateWorkspaces(wsA, wsB);
    for (size_t i = 0; i < result->size(); i++) {
      TS_ASSERT_DELTA(2.72014, result->y(0)[i], 0.001);
      TS_ASSERT_DELTA(0.139495, result->e(0)[i], 0.001);
    }
  }

  void testErrorPropagation_linear_workspace_dist() {
    constexpr size_t vars = 1;
    const double m = 5.0;
    const double c = 3.0;
    auto ws = createWorkspace("wsA", {0.0, 1.0, 2.0}, {20.0, 20.0, 20.0}, {0.5, 0.5, 0.5});
    const auto errorProp = Arithmetic::makeErrorPropagation<vars>([m, c](const auto &x) { return m * x[0] + c; });
    const auto result = errorProp.evaluateWorkspaces(true, ws);
    for (size_t i = 0; i < result->size(); i++) {
      TS_ASSERT_DELTA(103, result->y(0)[i], 0.001);
      TS_ASSERT_DELTA(2.5, result->e(0)[i], 0.001);
    }
    TS_ASSERT(result->isDistribution());
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
      auto trimmedSpinState = Kernel::Strings::strip(spinStateOrder[i]);
      grp->addWorkspace(createWorkspace(trimmedSpinState));
    }
    return grp;
  }

  MatrixWorkspace_sptr createWorkspace(const std::string &name, const std::vector<double> &dataX = {0, 1},
                                       const std::vector<double> &dataY = {0, 1},
                                       const std::vector<double> &dataE = {}) {
    auto createWorkspace = AlgorithmManager::Instance().create("CreateWorkspace");
    createWorkspace->initialize();
    createWorkspace->setProperty("DataX", dataX);
    createWorkspace->setProperty("DataY", dataY);
    createWorkspace->setProperty("DataE", dataE);
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
      auto trimmedSpinState = Kernel::Strings::strip(spinState);
      TS_ASSERT_EQUALS(trimmedSpinState, ws->getName());
    }
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

  void runTestWorkspaceForSpinState(const std::vector<std::string> &spinStateOrder, const std::string &targetSpinState,
                                    bool expectedIsNull, const std::string &expectedWorkspaceName = "") {
    auto grp = createGroupWorkspaceToMatchSpinStates(spinStateOrder);
    auto ws =
        PolarizationCorrectionsHelpers::workspaceForSpinState(grp, boost::join(spinStateOrder, ","), targetSpinState);

    TS_ASSERT_EQUALS(ws == nullptr, expectedIsNull);
    if (!expectedIsNull) {
      TS_ASSERT_EQUALS(ws->getName(), expectedWorkspaceName);
    }
  }
};
