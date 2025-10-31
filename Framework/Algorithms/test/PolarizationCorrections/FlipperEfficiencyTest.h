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
#include "MantidAPI/PolSANSWorkspaceValidator.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidAlgorithms/PolarizationCorrections/FlipperEfficiency.h"
#include "MantidKernel/ConfigService.h"

#include "PolarizationCorrectionsTestUtils.h"

using Mantid::Algorithms::ConvertUnits;
using Mantid::Algorithms::CreateSampleWorkspace;
using Mantid::Algorithms::FlipperEfficiency;
using Mantid::Algorithms::GroupWorkspaces;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::Kernel::ConfigService;
using namespace PolCorrTestUtils;

class FlipperEfficiencyTest : public CxxTest::TestSuite {
public:
  void setUp() override {
    m_defaultSaveDirectory = ConfigService::Instance().getString("defaultsave.directory");
    m_parameters = TestWorkspaceParameters();
  }

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
    auto const &group = createPolarizedTestGroup("testWs", m_parameters, FLIPPER_AMPS);
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
    auto const &group = createPolarizedTestGroup("testWs", m_parameters, FLIPPER_AMPS);
    auto alg = initialize_alg(group, false);
    alg->setPropertyValue("OutputFilePath", filename);
    alg->execute();
    auto savedPath = tempDir /= filename;
    TS_ASSERT(std::filesystem::exists(savedPath))
    std::filesystem::remove(savedPath);
  }

  void test_saving_no_ext() {
    auto const temp_filename = std::filesystem::temp_directory_path() /= "something";
    auto const &group = createPolarizedTestGroup("testWs", m_parameters, FLIPPER_AMPS);
    auto alg = initialize_alg(group, false);
    alg->setPropertyValue("OutputFilePath", temp_filename.string());
    alg->execute();
    auto savedPath = temp_filename.string() + ".nxs";
    TS_ASSERT(std::filesystem::exists(savedPath))
    std::filesystem::remove(savedPath);
  }

  /// Validation Tests

  void test_input_workspace_has_correct_validator() {
    auto alg = std::make_unique<FlipperEfficiency>();
    alg->initialize();
    auto prop = dynamic_cast<Mantid::API::WorkspaceProperty<Mantid::API::WorkspaceGroup> *>(
        alg->getPointerToProperty("InputWorkspace"));
    TS_ASSERT(prop);
    auto validator = std::dynamic_pointer_cast<Mantid::API::PolSANSWorkspaceValidator>(prop->getValidator());
    TS_ASSERT(validator);
  }

  void test_no_workspaces_or_file_output_fails() {
    auto const &group = createPolarizedTestGroup("testWs", m_parameters, FLIPPER_AMPS);
    auto alg = initialize_alg(group, false);
    TS_ASSERT_THROWS_EQUALS(
        alg->execute(), std::runtime_error const &e, std::string(e.what()),
        "Some invalid Properties found: \n OutputFilePath: Either an output workspace or output file must be "
        "provided.\n OutputWorkspace: Either an output workspace or output file must be provided.")
  }

  /// Calculation Tests

  void test_normal_perfect_calculation_occurs() {
    auto const &group = createPolarizedTestGroup("testWs", m_parameters, FLIPPER_AMPS);
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

  void test_normal_perfect_calculation_flipper_analyser() {
    auto const &group = createPolarizedTestGroup("testWs", m_parameters, FLIPPER_AMPS);
    auto alg = initialize_alg(group);
    alg->setProperty("Flipper", "Analyzer");
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

  void test_flipper_polariser_and_analyser_eff_have_same_values_for_equal_amplitudes() {
    auto const &group = createPolarizedTestGroup("testWs", m_parameters, FLIPPER_AMPS);
    auto alg = initialize_alg(group);
    alg->setProperty("Flipper", "Analyzer");
    alg->execute();
    MatrixWorkspace_sptr const outWsA = alg->getProperty("OutputWorkspace");

    alg->setProperty("Flipper", "Polarizer");
    alg->execute();
    MatrixWorkspace_sptr const outWsP = alg->getProperty("OutputWorkspace");

    auto const &outYA = outWsA->dataY(0);
    auto const &outYP = outWsP->dataY(0);
    TS_ASSERT_DELTA(outYA, outYP, 1e-8);
  }

  void test_normal_typical_calculation_occurs() {
    auto flipAmps{FLIPPER_AMPS};
    flipAmps[0] *= 0.9;
    auto const &group = createPolarizedTestGroup("testWs", m_parameters, flipAmps);
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
  //  default flipper configuration on alg: 11, 10, 01, 00
  const std::vector<double> FLIPPER_AMPS = {4.0, 1.0, 1.0, 4.0};
  std::string m_defaultSaveDirectory;
  TestWorkspaceParameters m_parameters;

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
