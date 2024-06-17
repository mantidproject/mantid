// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <filesystem>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidAlgorithms/PolarizationCorrections/FlipperEfficiency.h"
#include "MantidKernel/ConfigService.h"

namespace {} // namespace

using Mantid::Algorithms::ConvertUnits;
using Mantid::Algorithms::CreateSampleWorkspace;
using Mantid::Algorithms::FlipperEfficiency;
using Mantid::Algorithms::GroupWorkspaces;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::Kernel::ConfigService;

class FlipperEfficiencyTest : public CxxTest::TestSuite {
public:
  void setUp() override { m_defaultSaveDirectory = ConfigService::Instance().getString("defaultsave.directory"); }

  void tearDown() override {
    ConfigService::Instance().setString("defaultsave.directory", m_defaultSaveDirectory);
    Mantid::API::AnalysisDataService::Instance().clear();
  }

  void test_name() {
    FlipperEfficiency const alg;
    TS_ASSERT_EQUALS(alg.name(), "FlipperEfficiency")
  }

  void test_version() {
    FlipperEfficiency const alg;
    TS_ASSERT_EQUALS(alg.version(), 1)
  }

  void test_category() {
    FlipperEfficiency const alg;
    TS_ASSERT_EQUALS(alg.category(), "SANS\\PolarizationCorrections")
  }

  /// Saving Tests

  void test_saving_absolute() {
    auto const temp_filename = std::filesystem::temp_directory_path() /= "something.nxs";
    auto const &group = createTestingWorkspace("testWs");
    auto alg = initialize_alg(group, false);
    alg->setPropertyValue("OutputFilePath", temp_filename.string());
    alg->execute();
    TS_ASSERT(std::filesystem::exists(temp_filename))
    std::filesystem::remove(temp_filename);
  }

  void test_saving_relative() {
    auto tempDir = std::filesystem::temp_directory_path();
    ConfigService::Instance().setString("defaultsave.directory", tempDir.string());
    std::string const &filename = "something.nxs";
    auto const &group = createTestingWorkspace("testWs");
    auto alg = initialize_alg(group, false);
    alg->setPropertyValue("OutputFilePath", filename);
    alg->execute();
    auto savedPath = tempDir /= filename;
    TS_ASSERT(std::filesystem::exists(savedPath))
    std::filesystem::remove(savedPath);
  }

  void test_saving_no_ext() {
    auto const temp_filename = std::filesystem::temp_directory_path() /= "something";
    auto const &group = createTestingWorkspace("testWs");
    auto alg = initialize_alg(group, false);
    alg->setPropertyValue("OutputFilePath", temp_filename.string());
    alg->execute();
    auto savedPath = temp_filename.string() + ".nxs";
    TS_ASSERT(std::filesystem::exists(savedPath))
    std::filesystem::remove(savedPath);
  }

  /// Validation Tests

  void test_no_workspaces_or_file_output_fails() {
    auto const &group = createTestingWorkspace("testWs");
    auto alg = initialize_alg(group, false);
    TS_ASSERT_THROWS_EQUALS(
        alg->execute(), std::runtime_error const &e, std::string(e.what()),
        "Some invalid Properties found: \n OutputFilePath: Either an output workspace or output file must be "
        "provided.\n OutputWorkspace: Either an output workspace or output file must be provided.")
  }

  void test_invalid_group_size_is_captured() {
    auto const &group = createTestingWorkspace("testWs");
    group->removeItem(0);
    auto alg = initialize_alg(group);
    TS_ASSERT_THROWS_EQUALS(alg->execute(), std::runtime_error const &e, std::string(e.what()),
                            "Some invalid Properties found: \n InputWorkspace: The input group must contain a "
                            "workspace for all four spin states.")
  }

  void test_non_wavelength_workspace_is_captured() {
    auto const &group = createTestingWorkspace("testWs", 1.0, true);
    auto alg = initialize_alg(group);
    TS_ASSERT_THROWS_EQUALS(
        alg->execute(), std::runtime_error const &e, std::string(e.what()),
        "Some invalid Properties found: \n InputWorkspace: All input workspaces must be in units of Wavelength.")
  }

  void test_non_group_workspace_is_captured() {
    auto const &group = createTestingWorkspace("testWs", 1.0);
    auto alg = initialize_alg(group);
    TS_ASSERT_THROWS_EQUALS(alg->setProperty("InputWorkspace", group->getItem(0)), std::invalid_argument const &e,
                            std::string(e.what()), "Enter a name for the Input/InOut workspace")
  }

  void test_invalid_workspace_length_is_captured() {
    auto const &group = createTestingWorkspace("testWs", 1.0, false, 2);
    auto alg = initialize_alg(group);
    TS_ASSERT_THROWS_EQUALS(
        alg->execute(), std::runtime_error const &e, std::string(e.what()),
        "Some invalid Properties found: \n InputWorkspace: All input workspaces must contain only a single spectrum.")
  }

  /// Calculation Tests

  void test_normal_perfect_calculation_occurs() {
    auto const &group = createTestingWorkspace("testWs");
    auto alg = initialize_alg(group);
    alg->execute();

    MatrixWorkspace_sptr const outWs = alg->getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(outWs->getNumberHistograms(),
                     std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(group->getItem(0))->getNumberHistograms())
    auto const &outY = outWs->dataY(0);
    auto const &outE = outWs->dataE(0);
    auto const numBins = outY.size();
    for (size_t i = 0; i < numBins; ++i) {
      TS_ASSERT_EQUALS(1.0, outY[i])
      TS_ASSERT_DELTA(0.4216370213557839, outE[i], 1e-8);
    }
  }

  void test_normal_typical_calculation_occurs() {
    auto const &group = createTestingWorkspace("testWs", 0.9);
    auto alg = initialize_alg(group);
    alg->execute();

    MatrixWorkspace_sptr const outWs = alg->getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(outWs->getNumberHistograms(),
                     std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(group->getItem(0))->getNumberHistograms())
    auto const &outY = outWs->dataY(0);
    auto const &outE = outWs->dataE(0);
    auto const numBins = outY.size();
    for (size_t i = 0; i < numBins; ++i) {
      TS_ASSERT_DELTA(0.9710144925, outY[i], 1e-8);
      TS_ASSERT_DELTA(0.42616729754693666, outE[i], 1e-8);
    }
  }

private:
  std::string m_defaultSaveDirectory;

  Mantid::API::WorkspaceGroup_sptr createTestingWorkspace(std::string const &outName,
                                                          double const flipAmplitudeMultiplier = 1.0,
                                                          bool const TOFWs = false, int const numBanks = 1) {

    CreateSampleWorkspace makeWsAlg;
    makeWsAlg.initialize();
    makeWsAlg.setPropertyValue("Function", "User Defined");
    if (TOFWs) {
      makeWsAlg.setPropertyValue("XUnit", "TOF");
    } else {
      makeWsAlg.setPropertyValue("XUnit", "wavelength");
    }
    makeWsAlg.setProperty("NumBanks", numBanks);
    makeWsAlg.setProperty("BankPixelWidth", 1);
    makeWsAlg.setProperty("XMin", 1.45);
    makeWsAlg.setProperty("XMax", 9.50);
    makeWsAlg.setProperty("BinWidth", 0.1);

    double const FLIPPER_OFF_PARA_AMP = 4;
    double const ANTI_AMP = 1;
    double const flipperOnParaAmp = FLIPPER_OFF_PARA_AMP * flipAmplitudeMultiplier;

    makeWsAlg.setPropertyValue("UserDefinedFunction",
                               "name=UserFunction, Formula=x*0+" + std::to_string(FLIPPER_OFF_PARA_AMP));
    makeWsAlg.setPropertyValue("OutputWorkspace", outName + "_00");
    makeWsAlg.execute();

    makeWsAlg.setPropertyValue("UserDefinedFunction",
                               "name=UserFunction, Formula=x*0+" + std::to_string(flipperOnParaAmp));
    makeWsAlg.setPropertyValue("OutputWorkspace", outName + "_11");
    makeWsAlg.execute();

    makeWsAlg.setPropertyValue("UserDefinedFunction", "name=UserFunction, Formula=x*0+" + std::to_string(ANTI_AMP));
    makeWsAlg.setPropertyValue("OutputWorkspace", outName + "_10");
    makeWsAlg.execute();

    makeWsAlg.setPropertyValue("UserDefinedFunction", "name=UserFunction, Formula=x*0+" + std::to_string(ANTI_AMP));
    makeWsAlg.setPropertyValue("OutputWorkspace", outName + "_01");
    makeWsAlg.execute();

    GroupWorkspaces groupAlg;
    groupAlg.initialize();
    groupAlg.setChild(true);
    std::vector<std::string> const &input{outName + "_01", outName + "_11", outName + "_00", outName + "_10"};
    groupAlg.setProperty("InputWorkspaces", input);
    groupAlg.setPropertyValue("OutputWorkspace", outName);
    groupAlg.execute();
    TS_ASSERT(groupAlg.isExecuted());

    return groupAlg.getProperty("OutputWorkspace");
  }

  std::unique_ptr<FlipperEfficiency> initialize_alg(Mantid::API::WorkspaceGroup_sptr const &inputWorkspace,
                                                    bool const setOutput = true) {
    auto alg = std::make_unique<FlipperEfficiency>();
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", inputWorkspace);
    if (setOutput) {
      alg->setPropertyValue("OutputWorkspace", "out");
    }
    return alg;
  }
};
