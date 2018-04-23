#ifndef MANTID_ALGORITHMS_POLARIZATIONEFFICIENCYCORTEST_H_
#define MANTID_ALGORITHMS_POLARIZATIONEFFICIENCYCORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/PolarizationEfficiencyCor.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <Eigen/Dense>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace WorkspaceCreationHelper;

class PolarizationEfficiencyCorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PolarizationEfficiencyCorTest *createSuite() {
    return new PolarizationEfficiencyCorTest();
  }
  static void destroySuite(PolarizationEfficiencyCorTest *suite) {
    delete suite;
  }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_no_input_ws_wildes() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    // Error: Wildes method expects a list of input workspace names.
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
  }
  void test_no_input_ws_fredrikze() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("CorrectionMethod", "Fredrikze");
    alg.setProperty("Efficiencies", createEfficiencies("Fredrikze"));
    // Error: Fredrikze method expects a WorkspaceGroup as input.
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
  }
  void test_input_ws_Wildes_expects_list() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    // Error: Wildes method expects a list of input workspace names.
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
  }
  void test_input_ws_fredrikze() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg.setProperty("CorrectionMethod", "Fredrikze");
    alg.setProperty("Efficiencies", createEfficiencies("Fredrikze"));
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    WorkspaceGroup_sptr out = alg.getProperty("OutputWorkspace");
    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->size(), 4);
  }
  void test_input_ws_wildes() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    WorkspaceGroup_sptr out = alg.getProperty("OutputWorkspace");
    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->size(), 4);
  }
  void test_input_ws_fredrikze_needs_group() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg.setProperty("CorrectionMethod", "Fredrikze");
    alg.setProperty("Efficiencies", createEfficiencies("Fredrikze"));
    // Error: Fredrikze method doesn't allow to use a list of names for input.
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
  }
  void test_input_ws_wildes_needs_list() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    // Error: Wildes method doesn't allow to use a WorkspaceGroup for input.
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
  }
  void test_input_ws_fredrikze_cannot_take_both() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg.setProperty("CorrectionMethod", "Fredrikze");
    alg.setProperty("Efficiencies", createEfficiencies("Fredrikze"));
    // Error: Fredrikze method doesn't allow to use a list of names for input.
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
  }
  void test_input_ws_wildes_incompatible_with_efficiencies() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(2));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    // Error: Some invalid Properties found;
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
  }
  void test_efficiencies_fredrikze_wrong_efficiencies() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg.setProperty("CorrectionMethod", "Fredrikze");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    // Error: Efficiencey property not found: CRho;
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
  }
  void test_efficiencies_wildes_wrong_efficiencies() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Fredrikze"));
    // Error: Some invalid Properties found;
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
  }
  void test_flippers_full() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    alg.setProperty("Flippers", "00, 01, 10, 11");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    WorkspaceGroup_sptr out = alg.getProperty("OutputWorkspace");
    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->size(), 4);
  }
  void test_flippers_missing_01() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(3));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    alg.setProperty("Flippers", "00, 10, 11");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    WorkspaceGroup_sptr out = alg.getProperty("OutputWorkspace");
    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->size(), 4);
  }
  void test_flippers_missing_10() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(3));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    alg.setProperty("Flippers", "00, 01, 11");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    WorkspaceGroup_sptr out = alg.getProperty("OutputWorkspace");
    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->size(), 4);
  }
  void test_flippers_missing_01_10() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(2));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    alg.setProperty("Flippers", "00, 11");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    WorkspaceGroup_sptr out = alg.getProperty("OutputWorkspace");
    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->size(), 4);
  }
  void test_flippers_no_analyzer() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(2));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    alg.setProperty("Flippers", "0, 1");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    WorkspaceGroup_sptr out = alg.getProperty("OutputWorkspace");
    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->size(), 2);
  }
  void test_flippers_direct_beam() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(1));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    alg.setProperty("Flippers", "0");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    WorkspaceGroup_sptr out = alg.getProperty("OutputWorkspace");
    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->size(), 1);
  }
  void test_flippers_inconsistent() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    alg.setProperty("Flippers", "00, 10, 11");
    // Error: Some invalid Properties found;
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
  }
  void test_flippers_no_pnr() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    alg.setProperty("PolarizationAnalysis", "PNR");
    // Error: Property PolarizationAnalysis canot be used with the Wildes
    // method.
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
  }
  void test_flippers_no_pa() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaces", createWorkspacesInADS(4));
    alg.setProperty("CorrectionMethod", "Wildes");
    alg.setProperty("Efficiencies", createEfficiencies("Wildes"));
    alg.setProperty("PolarizationAnalysis", "PA");
    // Error: Property PolarizationAnalysis canot be used with the Wildes
    // method.
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
  }
  void test_polarization_analysis_pnr() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaceGroup", createWorkspaceGroup(2));
    alg.setProperty("CorrectionMethod", "Fredrikze");
    alg.setProperty("Efficiencies", createEfficiencies("Fredrikze"));
    alg.setProperty("PolarizationAnalysis", "PNR");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    WorkspaceGroup_sptr out = alg.getProperty("OutputWorkspace");
    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->size(), 2);
  }
  void test_polarization_analysis_pa() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg.setProperty("CorrectionMethod", "Fredrikze");
    alg.setProperty("Efficiencies", createEfficiencies("Fredrikze"));
    alg.setProperty("PolarizationAnalysis", "PA");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    WorkspaceGroup_sptr out = alg.getProperty("OutputWorkspace");
    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->size(), 4);
  }
  void test_polarization_analysis_wrong_pnr_input() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg.setProperty("CorrectionMethod", "Fredrikze");
    alg.setProperty("Efficiencies", createEfficiencies("Fredrikze"));
    alg.setProperty("PolarizationAnalysis", "PNR");
    // Error: For PNR analysis, input group must have 2 periods.
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
  }
  void test_polarization_analysis_no_flippers() {
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("OutputWorkspace", "out");
    alg.setProperty("InputWorkspaceGroup", createWorkspaceGroup(4));
    alg.setProperty("CorrectionMethod", "Fredrikze");
    alg.setProperty("Efficiencies", createEfficiencies("Fredrikze"));
    alg.setProperty("Flippers", "00, 01, 10, 11");
    // Error: Property Flippers canot be used with the Fredrikze method.
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
  }

private:
  std::vector<MatrixWorkspace_sptr> createWorkspaces(int n) {
    std::vector<MatrixWorkspace_sptr> workspaces;
    for (int i = 0; i < n; ++i) {
      auto ws = create1DWorkspaceConstant(1, 2.0, 1.0, true);
      workspaces.push_back(ws);
    }
    return workspaces;
  }

  WorkspaceGroup_sptr createWorkspaceGroup(int n) {
    auto group = boost::make_shared<WorkspaceGroup>();
    auto workspaces = createWorkspaces(n);
    for (auto &ws : workspaces) {
      ws->getAxis(0)->setUnit("Wavelength");
      group->addWorkspace(ws);
    }
    return group;
  }

  std::vector<std::string> createWorkspacesInADS(int n) {
    std::vector<std::string> names;
    auto workspaces = createWorkspaces(n);
    size_t i = 0;
    for (auto &ws : workspaces) {
      names.push_back("ws_" + std::to_string(i));
      AnalysisDataService::Instance().addOrReplace(names.back(), ws);
      ++i;
    }
    return names;
  }

  MatrixWorkspace_sptr createEfficiencies(std::string const &kind) {
    static std::map<std::string, std::vector<std::string>> const labels = {
        {"Wildes", {"P1", "P2", "F1", "F2"}},
        {"Fredrikze", {"CPp", "CAp", "CRho", "CAlpha"}}};
    auto inWS = createWorkspaces(1)[0];
    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create(inWS, 4);
    auto axis1 = new TextAxis(4);
    ws->replaceAxis(1, axis1);
    auto const &current_labels = labels.at(kind);
    for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
      axis1->setLabel(i, current_labels[i]);
    }
    return ws;
  }
};

#endif /* MANTID_ALGORITHMS_POLARIZATIONEFFICIENCYCORTEST_H_ */
