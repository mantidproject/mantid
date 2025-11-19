// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/LoadSESANS.h"
#include "MantidDataHandling/SaveSESANS.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

#include <Poco/TemporaryFile.h>
#include <boost/algorithm/string/predicate.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>

using Mantid::DataHandling::SaveSESANS;
using namespace Mantid::DataObjects;
using namespace Mantid;

class SaveSESANSTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveSESANSTest *createSuite() { return new SaveSESANSTest(); }
  static void destroySuite(SaveSESANSTest *suite) { delete suite; }

  void test_init() {
    TS_ASSERT_THROWS_NOTHING(testAlg.initialize());
    TS_ASSERT(testAlg.isInitialized());
    testAlg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("Filename", "dummy.ses"));
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("ThetaZMax", 0.09));
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("ThetaYMax", 0.09));
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("EchoConstant", echoConstant));
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("Sample", "Sample set in algorithm"));
  }

  void test_rejectTooManySpectra() {
    auto ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("InputWorkspace", ws));

    // Should throw, as we can't save more than one histogram
    TS_ASSERT_THROWS(testAlg.execute(), const std::runtime_error &);
  }

  void test_exec() {
    auto ws = createTestWorkspace();
    ws->mutableSample().setThickness(5);

    setCommonAlgorithmProperties(ws);

    // Execute the algorithm
    TS_ASSERT_THROWS_NOTHING(testAlg.execute());

    checkOutput(ws->sample().getThickness());
  }

  void test_exec_with_no_sample_thickness() {
    auto ws = createTestWorkspace();

    setCommonAlgorithmProperties(ws);

    // Execute the algorithm
    TS_ASSERT_THROWS(testAlg.execute(), const std::runtime_error &);
  }

  void test_exec_with_invalid_sample_thickness() {
    auto ws = createTestWorkspace();
    ws->mutableSample().setThickness(withinTolerance);

    setCommonAlgorithmProperties(ws);

    // Execute the algorithm
    TS_ASSERT_THROWS(testAlg.execute(), const std::runtime_error &);
  }

  void test_exec_thickness_property() {
    auto ws = createTestWorkspace();

    setCommonAlgorithmProperties(ws);
    const double thickness = 20;
    testAlg.setProperty("OverrideSampleThickness", thickness);

    // Execute the algorithm
    TS_ASSERT_THROWS_NOTHING(testAlg.execute());

    checkOutput(thickness);
  }

  void test_exec_invalid_thickness_property() {
    auto ws = createTestWorkspace();

    testAlg.setProperty("InputWorkspace", ws);
    testAlg.setProperty("Sample", "Sample set in SaveSESANSTest");
    testAlg.setProperty("OverrideSampleThickness", "0");

    // Execute the algorithm
    TS_ASSERT_THROWS(testAlg.execute(), const std::runtime_error &);
  }

  void test_exec_thickness_property_within_tolerance() {
    auto ws = createTestWorkspace();

    testAlg.setProperty("InputWorkspace", ws);
    testAlg.setProperty("Sample", "Sample set in SaveSESANSTest");
    testAlg.setProperty("OverrideSampleThickness", withinTolerance);

    // Execute the algorithm
    TS_ASSERT_THROWS(testAlg.execute(), const std::runtime_error &);
  }

  void test_exec_thickness_property_plus_sample_thickness_uses_property_value() {
    auto ws = createTestWorkspace();
    ws->mutableSample().setThickness(5);

    setCommonAlgorithmProperties(ws);
    const double thickness = 20;
    testAlg.setProperty("OverrideSampleThickness", thickness);

    // Execute the algorithm
    TS_ASSERT_THROWS_NOTHING(testAlg.execute());

    checkOutput(thickness);
  }

private:
  SaveSESANS testAlg;
  const double root2 = std::sqrt(2.0);
  const double ln2 = log(2.0);
  const double echoConstant = 1.5;
  const std::string workspaceTitle = "Sample workspace";
  const std::string sampleName = "Sample set in SaveSESANSTest";
  const double withinTolerance = 1e-10;

  Workspace2D_sptr createTestWorkspace() {
    // Set up workspace
    // X = [1 to 11], Y = [2] * 10, E = [sqrt(2)] * 10
    auto ws = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 10, 1, 1);

    // Set workspace attributes
    ws->setTitle(workspaceTitle);

    return ws;
  }

  void setCommonAlgorithmProperties(const Workspace2D_sptr &ws) {
    testAlg.setProperty("InputWorkspace", ws);
    testAlg.setProperty("Sample", sampleName);

    // Make a temporary file
    Poco::TemporaryFile tempFile;
    const std::string &tempFileName = tempFile.path();
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("Filename", tempFileName));
  }

  void checkOutput(const double sampleThickness) {
    // Get absolute path to the output file
    std::string outputPath = testAlg.getPropertyValue("Filename");

    // Make sure we can load the output file with no problems
    DataHandling::LoadSESANS loader;
    loader.initialize();
    std::string outWSName = "outWS";

    TS_ASSERT_THROWS_NOTHING(loader.setProperty("Filename", outputPath));
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(std::filesystem::exists(outputPath));

    // Check the file against original data - load it into a workspace
    API::Workspace_sptr loadedWS;
    TS_ASSERT_THROWS_NOTHING(loadedWS = API::AnalysisDataService::Instance().retrieve(outWSName));
    API::MatrixWorkspace_sptr data = std::dynamic_pointer_cast<API::MatrixWorkspace>(loadedWS);
    // Check titles were set
    TS_ASSERT_EQUALS(data->getTitle(), workspaceTitle);
    TS_ASSERT_EQUALS(data->sample().getName(), sampleName);

    // Check (a small sample of) the values we wrote are correct
    TS_ASSERT_EQUALS(static_cast<int>(data->getNumberHistograms()), 1);
    const auto &xValues = data->x(0);
    const auto &yValues = data->y(0);
    const auto &eValues = data->e(0);

    TS_ASSERT_EQUALS(static_cast<int>(xValues.size()), 10);
    TS_ASSERT_EQUALS(static_cast<int>(yValues.size()), 10);
    TS_ASSERT_EQUALS(static_cast<int>(eValues.size()), 10);

    // Check the actual values match
    double tolerance(1e-05);
    double thickness = sampleThickness * 0.1;
    for (size_t i = 0; i < xValues.size(); i++) {
      // X values are 0.5 higher than they were when we set them, as we set the
      // bin edges but are now dealing with bin middles
      // X value is now spinEchoLength = wavelength ^ 2 * echoConstant
      // Wavelength is x in original workspace
      double wavelengthSquared = (static_cast<double>(i) + 1.5) * (static_cast<double>(i) + 1.5);
      TS_ASSERT_DELTA(xValues[i], wavelengthSquared * echoConstant, tolerance);

      // Y value is now depolarisation = log(Y) / wavelength ^ 2 / thickness in cm
      // Where Y is Y value from original workspace (constantly 2 in this case)
      TS_ASSERT_DELTA(yValues[i], ln2 / wavelengthSquared / thickness, tolerance);

      // Error is now E / (Y * wavelength ^ 2) / thickness in cm
      // Where E and Y are from the original workspace (sqrt(2) and 2
      // respectively)
      TS_ASSERT_DELTA(eValues[i], root2 / (2.0 * wavelengthSquared) / thickness, tolerance);
    }

    // Clean up the file
    TS_ASSERT_THROWS_NOTHING(std::filesystem::remove(outputPath));
    TS_ASSERT(!std::filesystem::exists(outputPath));
  }
};
