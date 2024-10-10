// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

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
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    // Error: Input workspaces are missing. Either a workspace group or a list
    // of workspace names must be given
    TS_ASSERT_THROWS(alg.execute(), const std::invalid_argument &);
  }
  void test_input_ws_default_group() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    alg.execute();
    WorkspaceGroup_sptr out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("out");
    TS_ASSERT_EQUALS(out->size(), 4);
  }
  void test_input_ws_wildes_group() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    alg.execute();
    WorkspaceGroup_sptr out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("out");
    TS_ASSERT_EQUALS(out->size(), 4);
  }
  void test_input_ws_fredrikze_group() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg.setProperty("CorrectionMethod", "Fredrikze");
    alg.setProperty("Efficiencies", createEfficiencies("Fredrikze"));
    alg.execute();
    WorkspaceGroup_sptr out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("out");
    TS_ASSERT_EQUALS(out->size(), 4);
  }
  void test_input_ws_wildes_wrong_input_size() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaceGroup", createWorkspaceGroup(2));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    // Error: Some invalid Properties found
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }
  void test_input_ws_fredrikze_wrong_input_size() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaceGroup", createWorkspaceGroup(2));
    alg.setProperty("CorrectionMethod", "Fredrikze");
    alg.setProperty("Efficiencies", createEfficiencies("Fredrikze"));
    // Error: For PA analysis, input group must have 4 periods
    TS_ASSERT_THROWS(alg.execute(), const std::invalid_argument &);
  }
  void test_input_ws_wildes_list() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    alg.execute();
    WorkspaceGroup_sptr out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("out");
    TS_ASSERT_EQUALS(out->size(), 4);
  }
  void test_input_ws_frederikze_needs_group() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg.setProperty("CorrectionMethod", "Fredrikze");
    alg.setProperty("Efficiencies", createEfficiencies("Fredrikze"));
    // Error: Input workspaces are required to be in a workspace group
    TS_ASSERT_THROWS(alg.execute(), const std::invalid_argument &);
  }
  void test_input_ws_cannot_be_both() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    // Error: Input workspaces must be given either as a workspace group or a
    // list of names
    TS_ASSERT_THROWS(alg.execute(), const std::invalid_argument &);
  }
  void test_input_ws_wildes_wrong_size() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(2));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    // Error: Some invalid Properties found
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }

  void test_efficiencies_fredrikze_wrong_efficiencies() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg.setProperty("CorrectionMethod", "Fredrikze");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    // Error: Efficiencey property not found: Rho;
    TS_ASSERT_THROWS(alg.execute(), const std::invalid_argument &);
  }
  void test_efficiencies_wildes_wrong_efficiencies() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Fredrikze"));
    // Error: Some invalid Properties found
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }
  void test_flippers_full() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    alg.setProperty("Flippers", "00, 01, 10, 11");
    alg.execute();
    WorkspaceGroup_sptr out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("out");
    TS_ASSERT_EQUALS(out->size(), 4);
  }
  void test_flippers_missing_01() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(3));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    alg.setProperty("Flippers", "00, 10, 11");
    alg.execute();
    WorkspaceGroup_sptr out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("out");
    TS_ASSERT_EQUALS(out->size(), 4);
  }
  void test_flippers_missing_10() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(3));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    alg.setProperty("Flippers", "00, 01, 11");
    alg.execute();
    WorkspaceGroup_sptr out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("out");
    TS_ASSERT_EQUALS(out->size(), 4);
  }
  void test_flippers_missing_0110() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(2));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    alg.setProperty("Flippers", "00, 11");
    alg.execute();
    WorkspaceGroup_sptr out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("out");
    TS_ASSERT_EQUALS(out->size(), 4);
  }
  void test_flippers_no_analyser() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(2));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    alg.setProperty("Flippers", "0, 1");
    alg.execute();
    WorkspaceGroup_sptr out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("out");
    TS_ASSERT_EQUALS(out->size(), 2);
  }
  void test_flippers_direct_beam() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(1));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    alg.setProperty("Flippers", "0");
    alg.execute();
    WorkspaceGroup_sptr out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("out");
    TS_ASSERT_EQUALS(out->size(), 1);
  }
  void test_flippers_wrong_flippers() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    alg.setProperty("Flippers", "00, 10, 11");
    // Error: Some invalid Properties found
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }
  void test_flippers_wildes_no_pnr() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    alg.setProperty("PolarizationAnalysis", "PNR");
    // Error: Property PolarizationAnalysis canot be used with the Wildes
    // method
    TS_ASSERT_THROWS(alg.execute(), const std::invalid_argument &);
  }
  void test_flippers_wildes_no_pa() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    alg.setProperty("PolarizationAnalysis", "PA");
    // Error: Property PolarizationAnalysis canot be used with the Wildes
    // method
    TS_ASSERT_THROWS(alg.execute(), const std::invalid_argument &);
  }
  void test_polarization_analysis_pnr() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaceGroup", createWorkspaceGroup(2));
    alg.setProperty("CorrectionMethod", "Fredrikze");
    alg.setProperty("Efficiencies", createEfficiencies("Fredrikze"));
    alg.setProperty("PolarizationAnalysis", "PNR");
    alg.execute();
    WorkspaceGroup_sptr out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("out");
    TS_ASSERT_EQUALS(out->size(), 2);
  }
  void test_polarization_analysis_pa() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg.setProperty("CorrectionMethod", "Fredrikze");
    alg.setProperty("Efficiencies", createEfficiencies("Fredrikze"));
    alg.setProperty("PolarizationAnalysis", "PA");
    alg.execute();
    WorkspaceGroup_sptr out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("out");
    TS_ASSERT_EQUALS(out->size(), 4);
  }
  void test_polarization_analysis_pa_with_spinstates() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg.setProperty("CorrectionMethod", "Fredrikze");
    alg.setProperty("Efficiencies", createEfficiencies("Fredrikze"));
    alg.setProperty("PolarizationAnalysis", "PA");
    alg.setProperty("SpinStatesInFredrikze", "pp,pa,ap,aa");
    alg.setProperty("SpinStatesOutFredrikze", "pa,pp,ap,aa");
    alg.execute();
    WorkspaceGroup_sptr out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("out");
    TS_ASSERT_EQUALS(out->size(), 4);
  }
  void test_polarization_analysis_pnr_with_spinstates() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaceGroup", createWorkspaceGroup(2));
    alg.setProperty("CorrectionMethod", "Fredrikze");
    alg.setProperty("Efficiencies", createEfficiencies("Fredrikze"));
    alg.setProperty("PolarizationAnalysis", "PNR");
    alg.setProperty("SpinStatesInFredrikze", "p, a");
    alg.setProperty("SpinStatesOutFredrikze", "a, p");
    alg.execute();
    WorkspaceGroup_sptr out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("out");
    TS_ASSERT_EQUALS(out->size(), 2);
  }
  void test_polarization_analysis_wrong_group_size() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg.setProperty("CorrectionMethod", "Fredrikze");
    alg.setProperty("Efficiencies", createEfficiencies("Fredrikze"));
    alg.setProperty("PolarizationAnalysis", "PNR");
    // Error: For PNR analysis, input group must have 2 periods
    TS_ASSERT_THROWS(alg.execute(), const std::invalid_argument &);
  }
  void test_polarization_analysis_no_flippers() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg.setProperty("CorrectionMethod", "Fredrikze");
    alg.setProperty("Efficiencies", createEfficiencies("Fredrikze"));
    alg.setProperty("Flippers", "00, 01, 10, 11");
    // Error: Property Flippers canot be used with the Fredrikze method
    TS_ASSERT_THROWS(alg.execute(), const std::invalid_argument &);
  }

  void test_histo() {
    PolarizationEfficiencyCor alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("histo"));
    alg.execute();
    WorkspaceGroup_sptr out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("out");
    TS_ASSERT_EQUALS(out->size(), 4);
  }

  void test_points() {
    PolarizationEfficiencyCor alg;
    auto const inputs = createWorkspacesInADS(4);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", inputs);
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("points"));
    alg.execute();
    WorkspaceGroup_sptr out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("out");
    TS_ASSERT_EQUALS(out->size(), 4);

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

  void test_points_short() {
    PolarizationEfficiencyCor alg;
    auto const inputs = createWorkspacesInADS(4);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", inputs);
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("points-short"));
    alg.execute();
    WorkspaceGroup_sptr out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("out");
    TS_ASSERT_EQUALS(out->size(), 4);

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

private:
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

  MatrixWorkspace_sptr createEfficiencies(std::string const &kind) {
    static std::map<std::string, std::vector<std::string>> const labels = {{"Wildes", {"P1", "P2", "F1", "F2"}},
                                                                           {"Fredrikze", {"Pp", "Ap", "Rho", "Alpha"}}};
    if (kind == "Wildes" || kind == "Fredrikze") {
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
    } else if (kind == "histo") {
      auto ws1 = createHistoWS(10, 0, 10);
      auto ws2 = createHistoWS(10, 0, 10);
      auto ws3 = createHistoWS(10, 0, 10);
      auto ws4 = createHistoWS(10, 0, 10);

      auto alg = AlgorithmFactory::Instance().create("JoinISISPolarizationEfficiencies", -1);
      alg->initialize();
      alg->setChild(true);
      alg->setRethrows(true);
      alg->setProperty("P1", ws1);
      alg->setProperty("P2", ws2);
      alg->setProperty("F1", ws3);
      alg->setProperty("F2", ws4);
      alg->setPropertyValue("OutputWorkspace", "dummy");
      alg->execute();
      MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
      return outWS;
    } else if (kind == "points") {
      auto ws1 = createPointWS(10, 0, 10);
      auto ws2 = createPointWS(10, 0, 10);
      auto ws3 = createPointWS(10, 0, 10);
      auto ws4 = createPointWS(10, 0, 10);

      auto alg = AlgorithmFactory::Instance().create("JoinISISPolarizationEfficiencies", -1);
      alg->initialize();
      alg->setChild(true);
      alg->setRethrows(true);
      alg->setProperty("P1", ws1);
      alg->setProperty("P2", ws2);
      alg->setProperty("F1", ws3);
      alg->setProperty("F2", ws4);
      alg->setPropertyValue("OutputWorkspace", "dummy");
      alg->execute();
      MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
      return outWS;
    } else if (kind == "points-short") {
      auto ws1 = createPointWS(4, 0, 10);
      auto ws2 = createPointWS(4, 0, 10);
      auto ws3 = createPointWS(4, 0, 10);
      auto ws4 = createPointWS(4, 0, 10);

      auto alg = AlgorithmFactory::Instance().create("JoinISISPolarizationEfficiencies", -1);
      alg->initialize();
      alg->setChild(true);
      alg->setRethrows(true);
      alg->setProperty("P1", ws1);
      alg->setProperty("P2", ws2);
      alg->setProperty("F1", ws3);
      alg->setProperty("F2", ws4);
      alg->setPropertyValue("OutputWorkspace", "dummy");
      alg->execute();
      MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
      return outWS;
    }
    throw std::logic_error("Unknown efficeincy test kind");
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
};
