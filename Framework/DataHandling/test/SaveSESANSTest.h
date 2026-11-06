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
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/LoadSESANS.h"
#include "MantidDataHandling/SaveSESANS.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/TimeROI.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidTypes/Core/DateAndTime.h"

#include <Poco/TemporaryFile.h>
#include <boost/algorithm/string/predicate.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>

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

  void test_metadata_headers_are_taken_from_workspace() {
    auto ws = createTestWorkspace();
    ws->mutableSample().setThickness(5);
    ws->setInstrument(std::make_shared<Geometry::Instrument>("LARMOR"));
    ws->mutableRun().addProperty("run_number", std::string("94015"));

    const auto headers = saveAndReadHeaders(ws);

    TS_ASSERT_EQUALS(headers.at("Facility"), "ISIS");
    TS_ASSERT_EQUALS(headers.at("Instrument"), "LARMOR");
    TS_ASSERT_EQUALS(headers.at("Run_number"), "94015");
    checkSoftwareHeaders(headers);
  }

  void test_metadata_properties_override_workspace_values() {
    auto ws = createTestWorkspace();
    ws->mutableSample().setThickness(5);
    ws->setInstrument(std::make_shared<Geometry::Instrument>("LARMOR"));
    ws->mutableRun().addProperty("run_number", std::string("94015"));

    const auto headers = saveAndReadHeaders(
        ws, {{"Facility", "TestFacility"}, {"Instrument", "TestInstrument"}, {"RunNumber", "12345"}});

    TS_ASSERT_EQUALS(headers.at("Facility"), "TestFacility");
    TS_ASSERT_EQUALS(headers.at("Instrument"), "TestInstrument");
    TS_ASSERT_EQUALS(headers.at("Run_number"), "12345");
    checkSoftwareHeaders(headers);
  }

  void test_facility_is_determined_from_instrument_property() {
    auto ws = createTestWorkspace();
    ws->mutableSample().setThickness(5);

    const auto headers = saveAndReadHeaders(ws, {{"Instrument", "LARMOR"}});

    TS_ASSERT_EQUALS(headers.at("Facility"), "ISIS");
    TS_ASSERT_EQUALS(headers.at("Instrument"), "LARMOR");
  }

  void test_facility_is_omitted_for_unknown_instrument() {
    auto ws = createTestWorkspace();
    ws->mutableSample().setThickness(5);
    ws->setInstrument(std::make_shared<Geometry::Instrument>("NotARealInstrument"));

    const auto headers = saveAndReadHeaders(ws);

    TS_ASSERT_EQUALS(headers.count("Facility"), 0u);
    TS_ASSERT_EQUALS(headers.at("Instrument"), "NotARealInstrument");
  }

  void test_optional_metadata_headers_are_omitted_when_unavailable() {
    auto ws = createTestWorkspace();
    ws->mutableSample().setThickness(5);

    const auto headers = saveAndReadHeaders(ws);

    TS_ASSERT_EQUALS(headers.count("Facility"), 0u);
    TS_ASSERT_EQUALS(headers.count("Instrument"), 0u);
    TS_ASSERT_EQUALS(headers.count("Run_number"), 0u);
    TS_ASSERT_EQUALS(headers.count("Sample_position"), 0u);
    TS_ASSERT_EQUALS(headers.count("Blank_run_number"), 0u);
    TS_ASSERT_EQUALS(headers.count("Sample_transmission_run_number"), 0u);
    TS_ASSERT_EQUALS(headers.count("Blank_transmission_run_number"), 0u);
    checkSoftwareHeaders(headers);
  }

  void test_orientation_defaults_to_z() {
    auto ws = createTestWorkspace();
    ws->mutableSample().setThickness(5);

    const auto headers = saveAndReadHeaders(ws);

    TS_ASSERT_EQUALS(headers.at("Orientation"), "Z");
  }

  void test_orientation_property_is_written_to_file() {
    auto ws = createTestWorkspace();
    ws->mutableSample().setThickness(5);

    const auto headers = saveAndReadHeaders(ws, {{"Orientation", "Y"}});

    TS_ASSERT_EQUALS(headers.at("Orientation"), "Y");
  }

  void test_sample_position_lists_distinct_positions_in_order() {
    auto ws = createTestWorkspace();
    ws->mutableSample().setThickness(5);
    addSamplePosLog<std::string>(ws, {"1CT", "1CT", "AT", "AT"});

    const auto headers = saveAndReadHeaders(ws);

    TS_ASSERT_EQUALS(headers.at("Sample_position"), "1CT, AT");
  }

  void test_sample_position_is_omitted_for_empty_sample_pos_log() {
    auto ws = createTestWorkspace();
    ws->mutableSample().setThickness(5);
    addSamplePosLog<std::string>(ws, {"", ""});

    const auto headers = saveAndReadHeaders(ws);

    TS_ASSERT_EQUALS(headers.count("Sample_position"), 0u);
  }

  void test_sample_position_is_omitted_for_sample_pos_log_padded_with_spaces() {
    // A blank position loaded from NeXus can be padded with spaces
    auto ws = createTestWorkspace();
    ws->mutableSample().setThickness(5);
    addSamplePosLog<std::string>(ws, {"    ", "    "});

    const auto headers = saveAndReadHeaders(ws);

    TS_ASSERT_EQUALS(headers.count("Sample_position"), 0u);
  }

  void test_sample_position_from_number_log() {
    auto ws = createTestWorkspace();
    ws->mutableSample().setThickness(5);
    addSamplePosLog<double>(ws, {-308.7, -308.7, 12.5});

    const auto headers = saveAndReadHeaders(ws);

    TS_ASSERT_EQUALS(headers.at("Sample_position"), "-308.7, 12.5");
  }

  void test_sample_position_respects_time_roi() {
    // FilterByTime sets the time region of interest on the run, without removing log values
    auto ws = createTestWorkspace();
    ws->mutableSample().setThickness(5);
    addSamplePosLog<std::string>(ws, {"1CT", "AT"});
    ws->mutableRun().setTimeROI(Kernel::TimeROI(logStartTime, logStartTime + 300.0));

    const auto headers = saveAndReadHeaders(ws);

    TS_ASSERT_EQUALS(headers.at("Sample_position"), "1CT");
  }

  void test_sample_position_ignores_positions_logged_before_the_run() {
    // IBEX logs can start hours before the run, as in LARMOR00101015
    auto ws = createTestWorkspace();
    ws->mutableSample().setThickness(5);
    addSamplePosLog<std::string>(ws, {"OT", "AT", "  "});
    setRunStartAndEnd(ws, 1800.0, 1900.0);

    const auto headers = saveAndReadHeaders(ws);

    TS_ASSERT_EQUALS(headers.count("Sample_position"), 0u);
  }

  void test_sample_position_includes_position_at_run_start() {
    auto ws = createTestWorkspace();
    ws->mutableSample().setThickness(5);
    addSamplePosLog<std::string>(ws, {"OT", "AT", "1CT"});
    setRunStartAndEnd(ws, 900.0, 1500.0);

    const auto headers = saveAndReadHeaders(ws);

    TS_ASSERT_EQUALS(headers.at("Sample_position"), "AT, 1CT");
  }

  void test_optional_headers_with_only_whitespace_are_omitted() {
    auto ws = createTestWorkspace();
    ws->mutableSample().setThickness(5);

    const auto headers = saveAndReadHeaders(ws, {{"SamplePosition", "   "}, {"BlankRunNumber", "  "}});

    TS_ASSERT_EQUALS(headers.count("Sample_position"), 0u);
    TS_ASSERT_EQUALS(headers.count("Blank_run_number"), 0u);
  }

  void test_sample_position_property_overrides_sample_pos_log() {
    auto ws = createTestWorkspace();
    ws->mutableSample().setThickness(5);
    addSamplePosLog<std::string>(ws, {"AT"});

    const auto headers = saveAndReadHeaders(ws, {{"SamplePosition", "1R"}});

    TS_ASSERT_EQUALS(headers.at("Sample_position"), "1R");
  }

  void test_blank_and_transmission_run_numbers_are_written() {
    auto ws = createTestWorkspace();
    ws->mutableSample().setThickness(5);

    const auto headers = saveAndReadHeaders(ws, {{"BlankRunNumber", "94001"},
                                                 {"SampleTransmissionRunNumber", "94002"},
                                                 {"BlankTransmissionRunNumber", "94003"}});

    // The long header names must still be separated from their values
    TS_ASSERT_EQUALS(headers.at("Blank_run_number"), "94001");
    TS_ASSERT_EQUALS(headers.at("Sample_transmission_run_number"), "94002");
    TS_ASSERT_EQUALS(headers.at("Blank_transmission_run_number"), "94003");
  }

private:
  SaveSESANS testAlg;
  const double root2 = std::sqrt(2.0);
  const double ln2 = log(2.0);
  const double echoConstant = 1.5;
  const std::string workspaceTitle = "Sample workspace";
  const std::string sampleName = "Sample set in SaveSESANSTest";
  const double withinTolerance = 1e-10;
  const Types::Core::DateAndTime logStartTime{"2026-01-01T00:00:00"};

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

  /// Save the workspace with a fresh algorithm instance and return the header key-value pairs from the file
  std::map<std::string, std::string>
  saveAndReadHeaders(const Workspace2D_sptr &ws, const std::map<std::string, std::string> &extraProperties = {}) {
    SaveSESANS alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", ws);
    alg.setProperty("ThetaZMax", 0.09);
    alg.setProperty("ThetaYMax", 0.09);
    alg.setProperty("EchoConstant", echoConstant);
    alg.setProperty("Sample", sampleName);
    for (const auto &[name, value] : extraProperties) {
      alg.setPropertyValue(name, value);
    }
    Poco::TemporaryFile tempFile;
    alg.setPropertyValue("Filename", tempFile.path() + ".ses");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    const std::string outputPath = alg.getPropertyValue("Filename");

    // The file must still be readable by LoadSESANS
    DataHandling::LoadSESANS loader;
    loader.initialize();
    loader.setRethrows(true);
    loader.setProperty("Filename", outputPath);
    loader.setProperty("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_NOTHING(loader.execute());

    std::map<std::string, std::string> headers;
    std::ifstream infile(outputPath);
    std::string line;
    while (std::getline(infile, line) && line != "BEGIN_DATA") {
      const auto keyEnd = line.find(' ');
      if (keyEnd == std::string::npos) {
        continue;
      }
      const auto valueStart = line.find_first_not_of(' ', keyEnd);
      headers[line.substr(0, keyEnd)] = valueStart == std::string::npos ? "" : line.substr(valueStart);
    }
    infile.close();

    std::filesystem::remove(outputPath);
    API::AnalysisDataService::Instance().remove("outWS");
    return headers;
  }

  /// Add a SamplePos time series log, as written by IBEX, with one entry every 10 minutes from logStartTime
  template <typename T> void addSamplePosLog(const Workspace2D_sptr &ws, const std::vector<T> &values) {
    auto log = std::make_unique<Kernel::TimeSeriesProperty<T>>("SamplePos");
    for (size_t i = 0; i < values.size(); ++i) {
      log->addValue(logStartTime + 600.0 * static_cast<double>(i), values[i]);
    }
    ws->mutableRun().addProperty(std::move(log));
  }

  /// Set the run start and end times, in seconds after logStartTime
  void setRunStartAndEnd(const Workspace2D_sptr &ws, const double start, const double end) {
    ws->mutableRun().addProperty("start_time", (logStartTime + start).toISO8601String());
    ws->mutableRun().addProperty("end_time", (logStartTime + end).toISO8601String());
  }

  void checkSoftwareHeaders(const std::map<std::string, std::string> &headers) {
    TS_ASSERT_EQUALS(headers.at("Reduction_software"), "Mantid");
    TS_ASSERT_EQUALS(headers.at("Software_version"), Kernel::MantidVersion::version());
    TS_ASSERT(!headers.at("Reduction_date").empty());
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
