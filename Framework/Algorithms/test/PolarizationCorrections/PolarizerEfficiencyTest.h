// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/PolarizationCorrections/PolarizerEfficiency.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class PolarizerEfficiencyTest : public CxxTest::TestSuite {
public:
  void setUp() override {
    // Use an analyser efficiency of 1 to make test calculations simpler
    generateFunctionDefinedWorkspace(ANALYSER_EFFICIENCY_WS_NAME, "1 + x*0");
  }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void testName() {
    PolarizerEfficiency alg;
    TS_ASSERT_EQUALS(alg.name(), "PolarizerEfficiency");
  }

  void testInit() {
    PolarizerEfficiency alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
  }

  void testNonGroupWorkspaceInput() {
    // Should accept a group workspace containing four workspaces, corresponding to the four spin configurations
    std::vector<double> x{1, 2, 3, 4, 5};
    std::vector<double> y{1, 4, 9, 16, 25};

    MatrixWorkspace_sptr ws1 = generateWorkspace("ws1", x, y);

    auto polariserEfficiency = AlgorithmManager::Instance().create("PolarizerEfficiency");
    polariserEfficiency->initialize();
    polariserEfficiency->setProperty("InputWorkspace", ws1->getName());
    TS_ASSERT_THROWS(polariserEfficiency->execute(), const std::runtime_error &);
  }

  void testGroupWorkspaceWithWrongSize() {
    // Should accept a group workspace containing four workspaces, corresponding to the four spin configurations
    std::vector<double> x{1, 2, 3, 4, 5};
    std::vector<double> y{1, 4, 9, 16, 25};

    MatrixWorkspace_sptr ws1 = generateWorkspace("ws1", x, y);
    MatrixWorkspace_sptr ws2 = generateWorkspace("ws2", x, y);
    WorkspaceGroup_sptr groupWs = groupWorkspaces("grp", std::vector<MatrixWorkspace_sptr>{ws1, ws2});
    auto polariserEfficiency = createPolarizerEfficiencyAlgorithm(groupWs);
    TS_ASSERT_THROWS(polariserEfficiency->execute(), const std::runtime_error &);
  }

  void testOutput() {
    auto polariserEfficiency = createPolarizerEfficiencyAlgorithm();
    polariserEfficiency->execute();

    const auto workspaces = AnalysisDataService::Instance().getObjectNames();
    ASSERT_TRUE(std::find(workspaces.cbegin(), workspaces.cend(), "psm") != workspaces.cend());
  }

  void testSpinConfigurations() {
    auto polariserEfficiency = AlgorithmManager::Instance().create("PolarizerEfficiency");
    TS_ASSERT_THROWS(polariserEfficiency->setProperty("SpinStates", "bad"), std::invalid_argument &);
    TS_ASSERT_THROWS(polariserEfficiency->setProperty("SpinStates", "10,01"), std::invalid_argument &);
    TS_ASSERT_THROWS(polariserEfficiency->setProperty("SpinStates", "00,00,11,11"), std::invalid_argument &);
    TS_ASSERT_THROWS(polariserEfficiency->setProperty("SpinStates", "02,20,22,00"), std::invalid_argument &);
  }

  void testNonWavelengthInput() {
    // The units of the input workspace should be wavelength
    auto wsGrp = createExampleGroupWorkspace("wsGrp", "TOF");
    auto polariserEfficiency = AlgorithmManager::Instance().create("PolarizerEfficiency");
    polariserEfficiency->initialize();
    TS_ASSERT_THROWS(polariserEfficiency->setProperty("InputWorkspace", wsGrp->getName()), std::invalid_argument &);
  }

  void testExampleCalculation() {
    auto tPara = generateFunctionDefinedWorkspace("T_para", "4 + x*0");
    auto tPara1 = generateFunctionDefinedWorkspace("T_para1", "4 + x*0");
    auto tAnti = generateFunctionDefinedWorkspace("T_anti", "2 + x*0");
    auto tAnti1 = generateFunctionDefinedWorkspace("T_anti1", "2 + x*0");

    auto grpWs = groupWorkspaces("grpWs", {tPara, tAnti, tAnti1, tPara1});

    auto polariserEfficiency = createPolarizerEfficiencyAlgorithm(grpWs);
    polariserEfficiency->execute();
    MatrixWorkspace_sptr calculatedPolariserEfficiency = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        polariserEfficiency->getProperty("OutputWorkspace"));

    // The T_para and T_anti curves are 4 and 2 (constant wrt wavelength) respectively, and the analyser
    // efficiency is 1 for all wavelengths, which should give us a polarizer efficiency of 2/3
    for (const double &y : calculatedPolariserEfficiency->dataY(0)) {
      TS_ASSERT_DELTA(2.0 / 3.0, y, 1e-8);
    }
  }

  void testErrors() {
    auto polarizerEfficiency = createPolarizerEfficiencyAlgorithm();
    polarizerEfficiency->execute();
    MatrixWorkspace_sptr eff = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        polarizerEfficiency->getProperty("OutputWorkspace"));
    const auto errors = eff->dataE(0);
    // Skip the first error because with this toy data it'll be NaN
    const std::vector<double> expectedErrors{293.15439618057928, 130.29700166149377, 73.301389823113183,
                                             46.925472826600263};
    for (size_t i = 0; i < expectedErrors.size(); ++i) {
      TS_ASSERT_DELTA(expectedErrors[i], errors[i + 1], 1e-7);
    }
  }

private:
  const std::string ANALYSER_EFFICIENCY_WS_NAME = "effAnalyser";

  WorkspaceGroup_sptr createExampleGroupWorkspace(const std::string &name, const std::string &xUnit = "Wavelength",
                                                  const size_t numBins = 5) {
    std::vector<double> x(numBins);
    std::vector<double> y(numBins);
    std::vector<double> e(numBins);
    for (size_t i = 0; i < numBins; ++i) {
      x[i] = static_cast<double>(i) + 1.0;
      y[i] = x[i] * x[i];
      e[i] = 1000;
    }
    std::vector<MatrixWorkspace_sptr> wsVec(4);
    for (size_t i = 0; i < 4; ++i) {
      wsVec[i] = generateWorkspace("ws" + std::to_string(i), x, y, e, xUnit);
    }
    return groupWorkspaces(name, wsVec);
  }

  MatrixWorkspace_sptr generateWorkspace(const std::string &name, const std::vector<double> &x,
                                         const std::vector<double> &y, const std::string &xUnit = "Wavelength") {
    auto e = std::vector<double>(x.size());
    for (size_t i = 0; i < e.size(); ++i) {
      e[i] = 0;
    }
    return generateWorkspace(name, x, y, e, xUnit);
  }

  MatrixWorkspace_sptr generateWorkspace(const std::string &name, const std::vector<double> &x,
                                         const std::vector<double> &y, const std::vector<double> &e,
                                         const std::string &xUnit = "Wavelength") {
    auto createWorkspace = AlgorithmManager::Instance().create("CreateWorkspace");
    createWorkspace->initialize();
    createWorkspace->setProperty("DataX", x);
    createWorkspace->setProperty("DataY", y);
    createWorkspace->setProperty("DataE", e);
    createWorkspace->setProperty("UnitX", xUnit);
    createWorkspace->setProperty("OutputWorkspace", name);
    createWorkspace->setProperty("Distribution", true);
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

  MatrixWorkspace_sptr generateFunctionDefinedWorkspace(const std::string &name, const std::string &func) {
    auto createSampleWorkspace = AlgorithmManager::Instance().create("CreateSampleWorkspace");
    createSampleWorkspace->initialize();
    createSampleWorkspace->setProperty("WorkspaceType", "Histogram");
    createSampleWorkspace->setProperty("OutputWorkspace", name);
    createSampleWorkspace->setProperty("Function", "User Defined");
    createSampleWorkspace->setProperty("UserDefinedFunction", "name=UserFunction,Formula=" + func);
    createSampleWorkspace->setProperty("XUnit", "Wavelength");
    createSampleWorkspace->setProperty("XMin", "1");
    createSampleWorkspace->setProperty("XMax", "8");
    createSampleWorkspace->setProperty("BinWidth", "1");
    createSampleWorkspace->setProperty("NumBanks", 1);
    createSampleWorkspace->setProperty("BankPixelWidth", 1);
    createSampleWorkspace->execute();

    MatrixWorkspace_sptr result = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(name);
    result->setYUnit("");
    result->setDistribution(true);
    return result;
  }

  IAlgorithm_sptr createPolarizerEfficiencyAlgorithm(WorkspaceGroup_sptr inputGrp = nullptr) {
    if (inputGrp == nullptr) {
      inputGrp = createExampleGroupWorkspace("wsGrp");
    }
    auto polarizerEfficiency = AlgorithmManager::Instance().create("PolarizerEfficiency");
    polarizerEfficiency->initialize();
    polarizerEfficiency->setProperty("InputWorkspace", inputGrp->getName());
    polarizerEfficiency->setProperty("AnalyserEfficiency", ANALYSER_EFFICIENCY_WS_NAME);
    polarizerEfficiency->setProperty("OutputWorkspace", "psm");
    return polarizerEfficiency;
  }
};