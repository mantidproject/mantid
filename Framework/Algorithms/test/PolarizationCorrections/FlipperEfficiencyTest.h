// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <filesystem>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidAlgorithms/PolarizationCorrections/FlipperEfficiency.h"
#include "MantidKernel/ConfigService.h"

namespace {} // namespace

using Mantid::Algorithms::CreateSampleWorkspace;
using Mantid::Algorithms::FlipperEfficiency;
using Mantid::Algorithms::GroupWorkspaces;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::Kernel::ConfigService;

class FlipperEfficiencyTest : public CxxTest::TestSuite {
public:
  void setUp() override { m_defaultSaveDirectory = ConfigService::Instance().getString("defaultsave.directory"); }

  void tearDown() override { ConfigService::Instance().setString("defaultsave.directory", m_defaultSaveDirectory); }

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
    FlipperEfficiency alg;
    alg.initialize();
    auto const temp_filename = std::filesystem::temp_directory_path() /= "something.nxs";
    auto const &group = createTestingWorkspace("testWs");
    alg.setProperty("InputWorkspace", group);
    alg.setPropertyValue("OutputFilePath", temp_filename.string());
    alg.execute();
    TS_ASSERT(std::filesystem::exists(temp_filename))
    std::filesystem::remove(temp_filename);
  }

  void test_saving_relative() {
    auto tempDir = std::filesystem::temp_directory_path();
    ConfigService::Instance().setString("defaultsave.directory", tempDir);
    FlipperEfficiency alg;
    alg.initialize();
    std::string const &filename = "something.nxs";
    auto const &group = createTestingWorkspace("testWs");
    alg.setProperty("InputWorkspace", group);
    alg.setPropertyValue("OutputFilePath", filename);
    alg.execute();
    auto savedPath = tempDir /= filename;
    TS_ASSERT(std::filesystem::exists(savedPath))
    std::filesystem::remove(savedPath);
  }

  void test_saving_no_ext() {
    FlipperEfficiency alg;
    alg.initialize();
    auto const temp_filename = std::filesystem::temp_directory_path() /= "something";
    auto const &group = createTestingWorkspace("testWs");
    alg.setProperty("InputWorkspace", group);
    alg.setPropertyValue("OutputFilePath", temp_filename.string());
    alg.execute();
    auto savedPath = temp_filename.string() + ".nxs";
    TS_ASSERT(std::filesystem::exists(savedPath))
    std::filesystem::remove(savedPath);
  }

  /// Validation Tests

  void test_no_workspaces_or_file_output_fails() {
    FlipperEfficiency alg;
    alg.initialize();
    auto const &group = createTestingWorkspace("testWs");
    alg.setProperty("InputWorkspace", group);
    TS_ASSERT_THROWS_EQUALS(
        alg.execute(), std::runtime_error const &e, std::string(e.what()),
        "Some invalid Properties found: \n OutputFilePath: Either an output workspace or output file must be "
        "provided.\n OutputWorkspace: Either an output workspace or output file must be provided.")
  }

private:
  std::string m_defaultSaveDirectory;

  Mantid::API::WorkspaceGroup_sptr createTestingWorkspace(std::string const &outName, int const numSpectra = 1,
                                                          bool const isMonitor = true, double const binWidth = 0.1) {

    CreateSampleWorkspace makeWsAlg;
    makeWsAlg.initialize();
    makeWsAlg.setPropertyValue("Function", "User Defined");
    makeWsAlg.setPropertyValue("XUnit", "wavelength");
    if (isMonitor) {
      makeWsAlg.setProperty("NumBanks", 0);
      makeWsAlg.setProperty("NumMonitors", numSpectra);
    } else {
      makeWsAlg.setProperty("NumBanks", numSpectra);
    }
    makeWsAlg.setProperty("BankPixelWidth", 1);
    makeWsAlg.setProperty("XMin", 1.45);
    makeWsAlg.setProperty("XMax", 9.50);
    makeWsAlg.setProperty("BinWidth", binWidth);

    makeWsAlg.setPropertyValue("UserDefinedFunction",
                               "name=Lorentzian, Amplitude=48797.2, PeakCentre=2.774, FWHM=1.733");
    makeWsAlg.setPropertyValue("OutputWorkspace", outName + "_00");
    makeWsAlg.execute();

    makeWsAlg.setPropertyValue("UserDefinedFunction",
                               "name=Lorentzian, Amplitude=48797.2, PeakCentre=2.734, FWHM=1.733");
    makeWsAlg.setPropertyValue("OutputWorkspace", outName + "_11");
    makeWsAlg.execute();

    makeWsAlg.setPropertyValue("UserDefinedFunction",
                               "name=Lorentzian, Amplitude=21130.1, PeakCentre=2.574, FWHM=0.933");
    makeWsAlg.setPropertyValue("OutputWorkspace", outName + "_10");
    makeWsAlg.execute();

    makeWsAlg.setPropertyValue("UserDefinedFunction",
                               "name=Lorentzian, Amplitude=48797.2, PeakCentre=2.566, FWHM=0.933");
    makeWsAlg.setPropertyValue("OutputWorkspace", outName + "_01");
    makeWsAlg.execute();

    GroupWorkspaces groupAlg;
    groupAlg.initialize();
    groupAlg.setChild(true);
    std::vector<std::string> const &input{outName + "_10", outName + "_11", outName + "_01", outName + "_00"};
    groupAlg.setProperty("InputWorkspaces", input);
    groupAlg.setPropertyValue("OutputWorkspace", outName);
    groupAlg.execute();
    TS_ASSERT(groupAlg.isExecuted());

    return groupAlg.getProperty("OutputWorkspace");
  }
};