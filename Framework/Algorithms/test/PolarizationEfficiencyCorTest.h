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

  void test_input_ws_Wildes() {
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
    AnalysisDataService::Instance().clear();
  }

  void test_input_ws_Fredrikze() {
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
    AnalysisDataService::Instance().clear();
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
