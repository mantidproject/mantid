// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/PolSANSWorkspaceValidator.h"
#include "MantidAlgorithms/PolarizationCorrections/PolarizerEfficiency.h"
#include "MantidKernel/ConfigService.h"

#include "PolarizationCorrectionsTestUtils.h"

#include <cxxtest/TestSuite.h>
#include <filesystem>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace PolCorrTestUtils;

class PolarizerEfficiencyTest : public CxxTest::TestSuite {
public:
  void setUp() override {
    m_parameters = TestWorkspaceParameters();
    // Use an analyser efficiency of 1 to make test calculations simpler
    (void)generateFunctionDefinedWorkspace(TestWorkspaceParameters(ANALYSER_EFFICIENCY_WS_NAME, fillFuncStr({1.0})));
    m_defaultSaveDirectory = Kernel::ConfigService::Instance().getString("defaultsave.directory");
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    Kernel::ConfigService::Instance().setString("defaultsave.directory", m_defaultSaveDirectory);
  }

  void test_name() {
    PolarizerEfficiency alg;
    TS_ASSERT_EQUALS(alg.name(), "PolarizerEfficiency");
  }

  void test_init() {
    PolarizerEfficiency alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
    auto prop = dynamic_cast<Mantid::API::WorkspaceProperty<Mantid::API::WorkspaceGroup> *>(
        alg.getPointerToProperty("InputWorkspace"));
    TS_ASSERT(prop);
    auto validator = std::dynamic_pointer_cast<Mantid::API::PolSANSWorkspaceValidator>(prop->getValidator());
    TS_ASSERT(validator);
  }

  void test_output() {
    auto polariserEfficiency = createPolarizerEfficiencyAlgorithm();
    polariserEfficiency->execute();

    const auto &workspaces = AnalysisDataService::Instance().getObjectNames();
    TS_ASSERT(std::find(workspaces.cbegin(), workspaces.cend(), "psm") != workspaces.cend());
  }

  void test_spin_configurations() {
    auto polariserEfficiency = AlgorithmManager::Instance().create("PolarizerEfficiency");
    TS_ASSERT_THROWS(polariserEfficiency->setProperty("SpinStates", "bad"), std::invalid_argument &);
    TS_ASSERT_THROWS(polariserEfficiency->setProperty("SpinStates", "00,00,11,11"), std::invalid_argument &);
    TS_ASSERT_THROWS(polariserEfficiency->setProperty("SpinStates", "02,20,22,00"), std::invalid_argument &);
  }

  void test_missing_required_spin_config() {
    auto polariserEfficiency = createPolarizerEfficiencyAlgorithm();
    polariserEfficiency->setProperty("SpinStates", "11, 10");
    TS_ASSERT_THROWS(polariserEfficiency->execute(), const std::runtime_error &);
  }

  void test_non_matching_bins_fails() {
    (void)generateFunctionDefinedWorkspace(
        TestWorkspaceParameters(ANALYSER_EFFICIENCY_WS_NAME, fillFuncStr({1.0}), X_UNIT, 1, 1, 8, 0.1));
    auto polariserEfficiency = createPolarizerEfficiencyAlgorithm();

    TS_ASSERT_THROWS(polariserEfficiency->execute(), const std::runtime_error &);
  }

  void test_invalid_analyzer_ws_fails() {
    (void)generateFunctionDefinedWorkspace(
        TestWorkspaceParameters(ANALYSER_EFFICIENCY_WS_NAME, fillFuncStr({1.0}), X_UNIT, 2));
    auto polariserEfficiency = createPolarizerEfficiencyAlgorithm();
    TS_ASSERT_THROWS(polariserEfficiency->execute(), const std::runtime_error &);
  }

  void test_fails_with_non_matching_group_size_and_number_of_spin_states() {
    auto grpWs = createPolarizedTestGroup(GROUP_NAME, m_parameters, std::vector(4, 4.0));
    auto polariserEfficiency = createPolarizerEfficiencyAlgorithm(grpWs);
    // The 00 spin state is deliberately placed at the end of the input string so that it does not match a workspace in
    // the group. The algorithm validation normally tries to look up the 00 workspace, so this checks that we don't do
    // that when the spin states and workspace group length don't match.
    polariserEfficiency->setProperty("SpinStates", "10, 01, 00");
    TS_ASSERT_THROWS(polariserEfficiency->execute(), const std::runtime_error &);
  }

  void test_example_calculation() {
    auto grpWs = createPolarizedTestGroup(GROUP_NAME, m_parameters, std::vector({4.0, 2.0, 2.0, 4.0}));
    auto polariserEfficiency = createPolarizerEfficiencyAlgorithm(grpWs);
    polariserEfficiency->execute();
    MatrixWorkspace_sptr calculatedPolariserEfficiency = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        polariserEfficiency->getProperty("OutputWorkspace"));
    // The T_para(00,11) and T_anti(01,10)) curves are 4 and 2 (constant wrt wavelength) respectively, and the analyser
    // efficiency is 1 for all wavelengths, which should give us a polarizer efficiency of 2/3
    for (const double &y : calculatedPolariserEfficiency->dataY(0)) {
      TS_ASSERT_DELTA(2.0 / 3.0, y, 1e-8);
    }
  }

  void test_example_calculation_two_inputs() {
    auto grpWs = createPolarizedTestGroup(GROUP_NAME, m_parameters, std::vector({4.0, 2.0}), false);
    auto polariserEfficiency = createPolarizerEfficiencyAlgorithm(grpWs);
    polariserEfficiency->setProperty("SpinStates", "00,01");
    polariserEfficiency->execute();
    MatrixWorkspace_sptr calculatedPolariserEfficiency = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        polariserEfficiency->getProperty("OutputWorkspace"));

    // The T_para and T_anti curves are 4 and 2 (constant wrt wavelength) respectively, and the analyser
    // efficiency is 1 for all wavelengths, which should give us a polarizer efficiency of 2/3
    for (const double &y : calculatedPolariserEfficiency->dataY(0)) {
      TS_ASSERT_DELTA(2.0 / 3.0, y, 1e-8);
    }
  }

  void test_errors() {
    m_parameters.funcStr = "name=UserFunction,Formula=x^2";
    auto grpWs = createPolarizedTestGroup(GROUP_NAME, m_parameters);
    auto polarizerEfficiency = createPolarizerEfficiencyAlgorithm(grpWs);
    polarizerEfficiency->execute();
    MatrixWorkspace_sptr eff = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        polarizerEfficiency->getProperty("OutputWorkspace"));
    const auto &errors = eff->dataE(0);
    const std::vector<double> expectedErrors{0.23570, 0.14142, 0.10101, 0.07856, 0.06428, 0.05439, 0.047140};
    for (size_t i = 0; i < expectedErrors.size(); ++i) {
      TS_ASSERT_DELTA(expectedErrors[i], errors[i], 1e-5);
    }
  }

  /// Saving Tests

  void test_saving_absolute() {
    auto const temp_filename = std::filesystem::temp_directory_path() /= "something.nxs";
    auto polarizerEfficiency = createPolarizerEfficiencyAlgorithm();
    polarizerEfficiency->setPropertyValue("OutputWorkspace", "");
    polarizerEfficiency->setPropertyValue("OutputFilePath", temp_filename.string());
    polarizerEfficiency->execute();
    TS_ASSERT(std::filesystem::exists(temp_filename))
    std::filesystem::remove(temp_filename);
  }

  void test_saving_relative() {
    auto tempDir = std::filesystem::temp_directory_path();
    Kernel::ConfigService::Instance().setString("defaultsave.directory", tempDir.string());
    std::string const &filename = "something.nxs";
    auto polarizerEfficiency = createPolarizerEfficiencyAlgorithm();
    polarizerEfficiency->setPropertyValue("OutputFilePath", filename);
    polarizerEfficiency->execute();
    auto savedPath = tempDir /= filename;
    TS_ASSERT(std::filesystem::exists(savedPath))
    std::filesystem::remove(savedPath);
  }

  void test_saving_no_ext() {
    auto const temp_filename = std::filesystem::temp_directory_path() /= "something";
    auto polarizerEfficiency = createPolarizerEfficiencyAlgorithm();
    polarizerEfficiency->setPropertyValue("OutputFilePath", temp_filename.string());
    polarizerEfficiency->execute();
    auto savedPath = temp_filename.string() + ".nxs";
    TS_ASSERT(std::filesystem::exists(savedPath))
    std::filesystem::remove(savedPath);
  }

private:
  std::string m_defaultSaveDirectory;
  TestWorkspaceParameters m_parameters;

  IAlgorithm_sptr createPolarizerEfficiencyAlgorithm(WorkspaceGroup_sptr inputGrp = nullptr) {
    if (!inputGrp) {
      auto testParameters = TestWorkspaceParameters();
      inputGrp = createPolarizedTestGroup("wsGrp", testParameters, std::vector({4.0, 1.0, 1.0, 4.0}));
    }
    auto polarizerEfficiency = AlgorithmManager::Instance().create("PolarizerEfficiency");
    polarizerEfficiency->initialize();
    polarizerEfficiency->setProperty("InputWorkspace", inputGrp->getName());
    polarizerEfficiency->setProperty("AnalyserEfficiency", ANALYSER_EFFICIENCY_WS_NAME);
    polarizerEfficiency->setProperty("OutputWorkspace", "psm");

    return polarizerEfficiency;
  }
};
