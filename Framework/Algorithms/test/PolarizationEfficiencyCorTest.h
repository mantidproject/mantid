// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"
#include "MantidAlgorithms/PolarizationEfficiencyCor.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/Counts.h"
#include "MantidHistogramData/LinearGenerator.h"

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

#include <Eigen/Dense>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::Algorithms::PolarizationCorrectionsHelpers;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using namespace WorkspaceCreationHelper;

class PolarizationEfficiencyCorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PolarizationEfficiencyCorTest *createSuite() { return new PolarizationEfficiencyCorTest(); }
  static void destroySuite(PolarizationEfficiencyCorTest *suite) { delete suite; }

  PolarizationEfficiencyCorTest() {
    // To make sure API is initialized properly
    Mantid::API::FrameworkManager::Instance();
  }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_input_ws_no_inputs() {
    auto alg = createAlgorithm(WILDES_METHOD);
    // Error: Input workspaces are missing. Either a workspace group or a list
    // of workspace names must be given
    TS_ASSERT_THROWS(alg->execute(), const std::invalid_argument &);
  }

  void test_input_ws_default_group() {
    auto alg = createAlgorithm(WILDES_METHOD, WILDES_METHOD);
    alg->setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg->execute();
    checkOutputWorkspaceGroupSize(4);
  }

  void test_input_ws_wildes_group() {
    auto alg = createAlgorithm(WILDES_METHOD, WILDES_METHOD);
    alg->setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg->execute();
    checkOutputWorkspaceGroupSize(4);
  }

  void test_input_ws_fredrikze_group() {
    auto alg = createAlgorithm(FREDRIKZE_METHOD, FREDRIKZE_METHOD);
    alg->setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg->execute();
    checkOutputWorkspaceGroupSize(4);
  }

  void test_input_ws_wildes_wrong_input_size() {
    auto alg = createAlgorithm(WILDES_METHOD, WILDES_METHOD);
    alg->setProperty("InputWorkspaceGroup", createWorkspaceGroup(2));
    // Error: Some invalid Properties found
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }
  void test_input_ws_fredrikze_wrong_input_size() {
    auto alg = createAlgorithm(FREDRIKZE_METHOD, FREDRIKZE_METHOD);
    alg->setProperty("InputWorkspaceGroup", createWorkspaceGroup(2));
    // Error: For PA analysis, input group must have 4 periods
    TS_ASSERT_THROWS(alg->execute(), const std::invalid_argument &);
  }
  void test_input_ws_wildes_list() {
    auto alg = createAlgorithm(WILDES_METHOD, WILDES_METHOD);
    alg->setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg->execute();
    checkOutputWorkspaceGroupSize(4);
  }
  void test_input_ws_frederikze_needs_group() {
    auto alg = createAlgorithm(FREDRIKZE_METHOD, FREDRIKZE_METHOD);
    alg->setProperty("InputWorkspaces", createWorkspacesInADS(4));
    // Error: Input workspaces are required to be in a workspace group
    TS_ASSERT_THROWS(alg->execute(), const std::invalid_argument &);
  }
  void test_input_ws_cannot_be_both() {
    auto alg = createAlgorithm(WILDES_METHOD);
    alg->setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg->setProperty("InputWorkspaces", createWorkspacesInADS(4));
    // Error: Input workspaces must be given either as a workspace group or a
    // list of names
    TS_ASSERT_THROWS(alg->execute(), const std::invalid_argument &);
  }
  void test_input_ws_wildes_wrong_size() {
    auto alg = createAlgorithm(WILDES_METHOD, WILDES_METHOD);
    alg->setProperty("InputWorkspaces", createWorkspacesInADS(2));
    // Error: Some invalid Properties found
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void test_efficiencies_fredrikze_wrong_efficiencies() {
    auto alg = createAlgorithm(WILDES_METHOD, FREDRIKZE_METHOD);
    alg->setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    // Error: Efficiencey property not found: Rho;
    TS_ASSERT_THROWS(alg->execute(), const std::invalid_argument &);
  }
  void test_efficiencies_wildes_wrong_efficiencies() {
    auto alg = createAlgorithm(FREDRIKZE_METHOD, WILDES_METHOD);
    alg->setProperty("InputWorkspaces", createWorkspacesInADS(4));
    // Error: Some invalid Properties found
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void test_flippers_full() {
    auto alg = createAlgorithm(WILDES_METHOD, WILDES_METHOD);
    alg->setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg->setProperty("Flippers", "00, 01, 10, 11");
    alg->execute();
    checkOutputWorkspaceGroupSize(4);
  }

  void test_flippers_missing_01() {
    auto alg = createAlgorithm(WILDES_METHOD, WILDES_METHOD);
    alg->setProperty("InputWorkspaces", createWorkspacesInADS(3));
    alg->setProperty("Flippers", "00, 10, 11");
    alg->execute();
    checkOutputWorkspaceGroupSize(4);
  }
  void test_flippers_missing_10() {
    auto alg = createAlgorithm(WILDES_METHOD, WILDES_METHOD);
    alg->setProperty("InputWorkspaces", createWorkspacesInADS(3));
    alg->setProperty("Flippers", "00, 01, 11");
    alg->execute();
    checkOutputWorkspaceGroupSize(4);
  }
  void test_flippers_missing_0110() {
    auto alg = createAlgorithm(WILDES_METHOD, WILDES_METHOD);
    alg->setProperty("InputWorkspaces", createWorkspacesInADS(2));
    alg->setProperty("Flippers", "00, 11");
    alg->execute();
    checkOutputWorkspaceGroupSize(4);
  }
  void test_flippers_no_analyser() {
    auto alg = createAlgorithm(WILDES_METHOD, WILDES_METHOD);
    alg->setProperty("InputWorkspaces", createWorkspacesInADS(2));
    alg->setProperty("Flippers", "0, 1");
    alg->execute();
    checkOutputWorkspaceGroupSize(2);
  }

  void test_flippers_direct_beam() {
    auto alg = createAlgorithm(WILDES_METHOD, WILDES_METHOD);
    alg->setProperty("InputWorkspaces", createWorkspacesInADS(1));
    alg->setProperty("Flippers", "0");
    alg->execute();
    checkOutputWorkspaceGroupSize(1);
  }

  void test_that_wildes_can_work_with_spin_states_with_two_workspaces() {
    auto alg = createAlgorithm(WILDES_METHOD, WILDES_METHOD);
    alg->setProperty("InputWorkspaces", createWorkspacesInADS(2));
    alg->setProperty("Flippers", "0, 1");
    alg->setPropertyValue("SpinStatesOutWildes", "++, --");
    alg->execute();
    checkOutputWorkspaceGroupSize(2);
  }

  void test_that_wildes_can_work_with_spin_states_with_four_workspaces() {
    auto alg = createAlgorithm(WILDES_METHOD, WILDES_METHOD);
    alg->setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg->setPropertyValue("SpinStatesOutWildes", "++, --, -+, +-");
    alg->execute();
    checkOutputWorkspaceGroupSize(4);
  }

  void test_flippers_wrong_flippers() {
    auto alg = createAlgorithm(WILDES_METHOD, WILDES_METHOD);
    alg->setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg->setProperty("Flippers", "00, 10, 11");
    // Error: Some invalid Properties found
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }
  void test_flippers_wildes_no_pnr() {
    auto alg = createAlgorithm(WILDES_METHOD, WILDES_METHOD, "PNR");
    alg->setProperty("InputWorkspaces", createWorkspacesInADS(4));
    // Error: Property PolarizationAnalysis canot be used with the Wildes
    // method
    TS_ASSERT_THROWS(alg->execute(), const std::invalid_argument &);
  }
  void test_flippers_wildes_no_pa() {
    auto alg = createAlgorithm(WILDES_METHOD, WILDES_METHOD, "PA");
    alg->setProperty("InputWorkspaces", createWorkspacesInADS(4));
    // Error: Property PolarizationAnalysis canot be used with the Wildes
    TS_ASSERT_THROWS(alg->execute(), const std::invalid_argument &);
  }
  void test_polarization_analysis_pnr() {
    auto alg = createAlgorithm(FREDRIKZE_METHOD, FREDRIKZE_METHOD, "PNR");
    alg->setProperty("InputWorkspaceGroup", createWorkspaceGroup(2));
    alg->execute();
    checkOutputWorkspaceGroupSize(2);
  }
  void test_polarization_analysis_pa() {
    auto alg = createAlgorithm(FREDRIKZE_METHOD, FREDRIKZE_METHOD, "PA");
    alg->setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg->execute();
    checkOutputWorkspaceGroupSize(4);
  }
  void test_polarization_analysis_pa_with_spinstates() {
    auto alg = createAlgorithm(FREDRIKZE_METHOD, FREDRIKZE_METHOD, "PA");
    alg->setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg->setProperty("SpinStatesInFredrikze", "pp,pa,ap,aa");
    alg->setProperty("SpinStatesOutFredrikze", "pa,pp,ap,aa");
    alg->execute();
    checkOutputWorkspaceGroupSize(4);
  }
  void test_polarization_analysis_pnr_with_spinstates() {
    auto alg = createAlgorithm(FREDRIKZE_METHOD, FREDRIKZE_METHOD, "PNR");
    alg->setProperty("InputWorkspaceGroup", createWorkspaceGroup(2));
    alg->setProperty("SpinStatesInFredrikze", "p, a");
    alg->setProperty("SpinStatesOutFredrikze", "a, p");
    alg->execute();
    checkOutputWorkspaceGroupSize(2);
  }

  void test_that_fredrikze_input_spinstates_cannot_be_used_with_wildes() {
    auto alg = createAlgorithm(WILDES_METHOD, WILDES_METHOD);
    alg->setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg->setProperty("SpinStatesInFredrikze", "p, a");

    TS_ASSERT_THROWS_EQUALS(alg->execute(), const std::invalid_argument &e, std::string(e.what()),
                            "Property SpinStatesInFredrikze cannot be used with the Wildes method.");
  }

  void test_that_fredrikze_output_spinstates_cannot_be_used_with_wildes() {
    auto alg = createAlgorithm(WILDES_METHOD, WILDES_METHOD);
    alg->setProperty("InputWorkspaces", createWorkspacesInADS(1));
    alg->setProperty("SpinStatesOutFredrikze", "p, a");

    TS_ASSERT_THROWS_EQUALS(alg->execute(), const std::invalid_argument &e, std::string(e.what()),
                            "Property SpinStatesOutFredrikze cannot be used with the Wildes method.");
  }

  void test_polarization_analysis_wrong_group_size() {
    auto alg = createAlgorithm(FREDRIKZE_METHOD, FREDRIKZE_METHOD, "PNR");
    alg->setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    // Error: For PNR analysis, input group must have 2 periods
    TS_ASSERT_THROWS(alg->execute(), const std::invalid_argument &);
  }

  void test_polarization_analysis_no_flippers() {
    auto alg = createAlgorithm(FREDRIKZE_METHOD, FREDRIKZE_METHOD);
    alg->setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg->setProperty("Flippers", "00, 01, 10, 11");
    // Error: Property Flippers canot be used with the Fredrikze method
    TS_ASSERT_THROWS_EQUALS(alg->execute(), const std::invalid_argument &e, std::string(e.what()),
                            "Property Flippers cannot be used with the Fredrikze method.");
  }

  void test_that_wildes_output_spinstates_cannot_be_used_with_fredrikze() {
    auto alg = createAlgorithm(FREDRIKZE_METHOD, FREDRIKZE_METHOD);
    alg->setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg->setPropertyValue("SpinStatesOutWildes", "+, -");

    TS_ASSERT_THROWS_EQUALS(alg->execute(), const std::invalid_argument &e, std::string(e.what()),
                            "Property SpinStatesOutWildes cannot be used with the Fredrikze method.");
  }

  void test_histo() {
    auto alg = createAlgorithm("histo", WILDES_METHOD);
    alg->setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg->execute();
    checkOutputWorkspaceGroupSize(4);
  }

  void test_points() { pointTestHelper("points"); }

  void test_points_short() { pointTestHelper("points-short"); }

  void test_spin_state_log_not_added_by_default_Wildes() { runSpinStateLogTest(WILDES_METHOD, false); }

  void test_spin_state_log_added_when_requested_Wildes() { runSpinStateLogTest(WILDES_METHOD, true); }

  void test_spin_state_log_not_added_by_default_Fredrikze() { runSpinStateLogTest(FREDRIKZE_METHOD, false); }

private:
  const std::string WILDES_METHOD{"Wildes"};
  const std::string FREDRIKZE_METHOD{"Fredrikze"};
  const std::string OUTPUT_GRP_NAME{"out"};

  std::vector<MatrixWorkspace_sptr> createWorkspaces(int n) {
    std::vector<MatrixWorkspace_sptr> workspaces;
    for (int i = 0; i < n; ++i) {
      auto ws = create1DWorkspaceConstant(5, 2.0, 1.0, true);
      workspaces.emplace_back(ws);
    }
    return workspaces;
  }

  WorkspaceGroup_sptr createWorkspaceGroup(int n) {
    auto group = std::make_shared<WorkspaceGroup>();
    auto workspaces = createWorkspaces(n);
    for (auto &ws : workspaces) {
      ws->getAxis(0)->setUnit("Wavelength");
      group->addWorkspace(ws);
    }
    AnalysisDataService::Instance().addOrReplace("WS_GROUP_1", group);
    return group;
  }

  std::vector<std::string> createWorkspacesInADS(int n) {
    std::vector<std::string> names;
    auto workspaces = createWorkspaces(n);
    size_t i = 0;
    for (auto &ws : workspaces) {
      names.emplace_back("ws_" + std::to_string(i));
      AnalysisDataService::Instance().addOrReplace(names.back(), ws);
      ++i;
    }
    return names;
  }

  std::shared_ptr<PolarizationEfficiencyCor> createAlgorithm(const std::string &efficiencyMethod,
                                                             const std::string &method = "",
                                                             const std::string &analysisMethod = "") {
    auto alg = std::make_shared<PolarizationEfficiencyCor>();
    alg->setRethrows(true);
    alg->initialize();
    alg->setProperty("OutputWorkspace", OUTPUT_GRP_NAME);
    alg->setProperty("Efficiencies", createEfficiencies(efficiencyMethod));
    if (!method.empty()) {
      alg->setProperty("CorrectionMethod", method);
    }
    if (!analysisMethod.empty()) {
      alg->setProperty("PolarizationAnalysis", analysisMethod);
    }
    return alg;
  }

  void checkOutputWorkspaceGroupSize(size_t expectedSize) {
    WorkspaceGroup_sptr out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(OUTPUT_GRP_NAME);
    TS_ASSERT_EQUALS(out->size(), expectedSize);
  }

  MatrixWorkspace_sptr createEfficiencyOutputWorkspace(std::string const &kind) {
    const auto numWorkspaces = 4;
    const double endX = 10;

    std::vector<MatrixWorkspace_sptr> workspaces;
    if (kind == "histo") {
      for (int i = 0; i < numWorkspaces; ++i) {
        workspaces.push_back(createHistoWS(10, 0, endX));
      }
    } else if (kind == "points") {
      for (int i = 0; i < numWorkspaces; ++i) {
        workspaces.push_back(createPointWS(10, 0, endX));
      }
    } else if (kind == "points-short") {
      for (int i = 0; i < numWorkspaces; ++i) {
        workspaces.push_back(createPointWS(4, 0, endX));
      }
    } else {
      throw std::logic_error("Unknown efficiency test kind");
    }

    auto alg = AlgorithmFactory::Instance().create("JoinISISPolarizationEfficiencies", -1);
    alg->initialize();
    alg->setChild(true);
    alg->setRethrows(true);
    alg->setProperty("P1", workspaces[0]);
    alg->setProperty("P2", workspaces[1]);
    alg->setProperty("F1", workspaces[2]);
    alg->setProperty("F2", workspaces[3]);
    alg->setPropertyValue("OutputWorkspace", OUTPUT_GRP_NAME);
    alg->execute();

    return alg->getProperty("OutputWorkspace");
  }

  MatrixWorkspace_sptr createEfficiencies(std::string const &kind) {
    static std::map<std::string, std::vector<std::string>> const labels = {
        {WILDES_METHOD, {"P1", "P2", "F1", "F2"}}, {FREDRIKZE_METHOD, {"Pp", "Ap", "Rho", "Alpha"}}};
    if (kind == WILDES_METHOD || kind == FREDRIKZE_METHOD) {
      auto inWS = createWorkspaces(1)[0];
      MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create(inWS, 4);
      ws->getAxis(0)->setUnit("Wavelength");
      auto axis1 = std::make_unique<TextAxis>(4);
      auto axis1Raw = axis1.get();
      ws->replaceAxis(1, std::move(axis1));
      auto const &current_labels = labels.at(kind);
      for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
        axis1Raw->setLabel(i, current_labels[i]);
      }
      return ws;
    }

    return createEfficiencyOutputWorkspace(kind);
  }

  MatrixWorkspace_sptr createHistoWS(size_t size, double startX, double endX) const {
    double const dX = (endX - startX) / double(size);
    BinEdges xVals(size + 1, LinearGenerator(startX, dX));
    Counts yVals(size, 1.0);
    auto retVal = std::make_shared<Workspace2D>();
    retVal->initialize(1, Histogram(xVals, yVals));
    return retVal;
  }

  MatrixWorkspace_sptr createPointWS(size_t size, double startX, double endX) const {
    double const dX = (endX - startX) / double(size - 1);
    Points xVals(size, LinearGenerator(startX, dX));
    Counts yVals(size, 1.0);
    auto retVal = std::make_shared<Workspace2D>();
    retVal->initialize(1, Histogram(xVals, yVals));
    return retVal;
  }

  void runSpinStateLogTest(const std::string &correctionMethod, const bool expectLog) {
    auto alg = createAlgorithm(correctionMethod, correctionMethod);
    if (correctionMethod == FREDRIKZE_METHOD) {
      alg->setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    } else {
      alg->setProperty("InputWorkspaces", createWorkspacesInADS(4));
    }
    if (expectLog) {
      alg->setProperty("AddSpinStateToLog", true);
    }
    alg->execute();
    WorkspaceGroup_sptr out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(OUTPUT_GRP_NAME);
    TS_ASSERT_EQUALS(out->size(), 4);
    for (size_t i = 0; i < 4; ++i) {
      MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(i));
      TS_ASSERT(ws)
      TS_ASSERT_EQUALS(ws->run().hasProperty(SpinStatesORSO::LOG_NAME), expectLog)
    }
  }

  void pointTestHelper(const std::string &kind) {
    auto alg = createAlgorithm(kind, WILDES_METHOD);
    auto const inputs = createWorkspacesInADS(4);
    alg->setProperty("InputWorkspaces", inputs);
    alg->execute();
    WorkspaceGroup_sptr out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(OUTPUT_GRP_NAME);
    checkOutputWorkspaceGroupSize(4);

    for (size_t i = 0; i < out->size(); ++i) {
      auto ws = AnalysisDataService::Instance().retrieve(inputs[i]);
      auto checkAlg = AlgorithmManager::Instance().createUnmanaged("CompareWorkspaces");
      checkAlg->initialize();
      checkAlg->setChild(true);
      checkAlg->setProperty("Workspace1", ws);
      checkAlg->setProperty("Workspace2", out->getItem(i));
      checkAlg->setProperty("Tolerance", 3e-16);
      checkAlg->execute();
      TS_ASSERT(checkAlg->getProperty("Result"));
    }
  }
};
