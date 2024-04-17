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

namespace {} // namespace

using Mantid::Algorithms::CreateSampleWorkspace;
using Mantid::Algorithms::FlipperEfficiency;
using Mantid::Algorithms::GroupWorkspaces;
using Mantid::API::MatrixWorkspace_sptr;

class FlipperEfficiencyTest : public CxxTest::TestSuite {
public:
  void test_name() {
    FlipperEfficiency const alg;
    TS_ASSERT_EQUALS(alg.name(), "FlipperEfficiency");
  }

  void test_version() {
    FlipperEfficiency const alg;
    TS_ASSERT_EQUALS(alg.version(), 1);
  }

  void test_category() {
    FlipperEfficiency const alg;
    TS_ASSERT_EQUALS(alg.category(), "SANS\\PolarizationCorrections");
  }

  void test_saving_absolute() {
    FlipperEfficiency alg;
    alg.initialize();
    auto temp_filename = std::filesystem::temp_directory_path() /= "something.nxs";
    auto group = createTestingWorkspace("testWs");
    alg.setProperty("InputWorkspace", group);
    alg.setPropertyValue("OutputFilePath", temp_filename.string());
    TS_ASSERT(std::filesystem::exists(temp_filename));
  }

private:
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
    TS_ASSERT_THROWS_NOTHING(groupAlg.setProperty("InputWorkspaces", input));
    TS_ASSERT_THROWS_NOTHING(groupAlg.setPropertyValue("OutputWorkspace", outName));
    TS_ASSERT_THROWS_NOTHING(groupAlg.execute());
    TS_ASSERT(groupAlg.isExecuted());

    return groupAlg.getProperty("OutputWorkspace");
  }
};