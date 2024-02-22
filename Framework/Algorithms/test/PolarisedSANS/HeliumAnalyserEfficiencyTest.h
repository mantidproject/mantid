// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAlgorithms/PolarisedSANS/HeliumAnalyserEfficiency.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class HeliumAnalyserEfficiencyTest : public CxxTest::TestSuite {
public:
  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void testName() {
    HeliumAnalyserEfficiency alg;
    TS_ASSERT_EQUALS(alg.name(), "HeliumAnalyserEfficiency");
  }

  void testInit() {
    HeliumAnalyserEfficiency alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
  }

  void testInputWorkspaceFormat() {
    std::vector<double> x{1, 2, 3};
    std::vector<double> y{1, 4, 9};

    MatrixWorkspace_sptr ws1 = generateWorkspace("ws1", x, y);

    auto heliumAnalyserEfficiency = AlgorithmManager::Instance().create("HeliumAnalyserEfficiency");
    heliumAnalyserEfficiency->initialize();
    heliumAnalyserEfficiency->setProperty("InputWorkspace", ws1->getName());
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->execute(), const std::runtime_error &);

    MatrixWorkspace_sptr ws2 = generateWorkspace("ws2", x, y);
    WorkspaceGroup_sptr groupWs = groupWorkspaces("grp", std::vector<MatrixWorkspace_sptr>{ws1, ws2});
    heliumAnalyserEfficiency->initialize();
    heliumAnalyserEfficiency->setProperty("InputWorkspace", groupWs->getName());
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->execute(), const std::runtime_error &);

    MatrixWorkspace_sptr ws3 = generateWorkspace("ws3", x, y);
    groupWs = groupWorkspaces("grp", std::vector<MatrixWorkspace_sptr>{ws1, ws2, ws3});
    heliumAnalyserEfficiency->initialize();
    heliumAnalyserEfficiency->setProperty("InputWorkspace", groupWs->getName());
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->execute(), const std::runtime_error &);

    MatrixWorkspace_sptr ws4 = generateWorkspace("ws4", x, y);
    groupWs = groupWorkspaces("grp", std::vector<MatrixWorkspace_sptr>{ws1, ws2, ws3, ws4});
    heliumAnalyserEfficiency->initialize();
    heliumAnalyserEfficiency->setProperty("InputWorkspace", groupWs->getName());
    TS_ASSERT_THROWS_NOTHING(heliumAnalyserEfficiency->execute());
  }

  void testOutputs() {
    auto wsGrp = createExampleGroupWorkspace("wsGrp");

    auto heliumAnalyserEfficiency = AlgorithmManager::Instance().create("HeliumAnalyserEfficiency");
    heliumAnalyserEfficiency->initialize();
    heliumAnalyserEfficiency->setProperty("InputWorkspace", wsGrp->getName());
    heliumAnalyserEfficiency->execute();

    ASSERT_TRUE(AnalysisDataService::Instance().doesExist("T"));
    AnalysisDataService::Instance().remove("T");
    ASSERT_TRUE(AnalysisDataService::Instance().doesExist("p_He"));
    AnalysisDataService::Instance().remove("p_He");
    ASSERT_TRUE(AnalysisDataService::Instance().doesExist("T_para"));
    AnalysisDataService::Instance().remove("T_para");
    ASSERT_TRUE(AnalysisDataService::Instance().doesExist("T_anti"));
    AnalysisDataService::Instance().remove("T_anti");

    auto members = wsGrp->getNames();
    AnalysisDataService::Instance().remove(wsGrp->getName());
    for (size_t i = 0; i < members.size(); ++i) {
      AnalysisDataService::Instance().remove(members[i]);
    }

    TS_ASSERT_EQUALS(0, AnalysisDataService::Instance().size());
  }

  void testSpinConfigurations() {}

  void testNonWavelengthInput() {}

  void testSampleFit() {}

  WorkspaceGroup_sptr createExampleGroupWorkspace(const std::string &name) {
    const std::vector<double> x{1, 2, 3};
    const std::vector<double> y{1, 4, 9};
    std::vector<MatrixWorkspace_sptr> wsVec(4);
    for (size_t i = 0; i < 4; ++i) {
      wsVec[i] = generateWorkspace("ws" + std::to_string(i), x, y);
    }
    return groupWorkspaces(name, wsVec);
  }

  MatrixWorkspace_sptr generateWorkspace(const std::string &name, const std::vector<double> &x,
                                         const std::vector<double> &y, const std::string &xUnit = "Wavelength") {
    auto createWorkspace = AlgorithmManager::Instance().create("CreateWorkspace");
    createWorkspace->initialize();
    createWorkspace->setProperty("DataX", x);
    createWorkspace->setProperty("DataY", y);
    createWorkspace->setProperty("UnitX", xUnit);
    createWorkspace->setProperty("OutputWorkspace", name);
    createWorkspace->execute();

    auto convertToHistogram = AlgorithmManager::Instance().create("ConvertToHistogram");
    convertToHistogram->initialize();
    convertToHistogram->setProperty("InputWorkspace", name);
    convertToHistogram->setProperty("OutputWorkspace", name);
    convertToHistogram->execute();

    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(name);
    return ws;
  }

  WorkspaceGroup_sptr groupWorkspaces(const std::string &name, const std::vector<MatrixWorkspace_sptr> &wsToGroup) {
    auto groupWorkspace = AlgorithmManager::Instance().create("GroupWorkspaces");
    groupWorkspace->initialize();
    std::vector<std::string> wsToGroupNames(wsToGroup.size());
    std::transform(wsToGroup.cbegin(), wsToGroup.cend(), wsToGroupNames.begin(),
                   [](MatrixWorkspace_sptr w) { return w->getName(); });
    groupWorkspace->setProperty("InputWorkspaces", wsToGroupNames);
    groupWorkspace->setProperty("OutputWorkspace", name);
    groupWorkspace->execute();
    WorkspaceGroup_sptr group = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(name);
    return group;
  }
};