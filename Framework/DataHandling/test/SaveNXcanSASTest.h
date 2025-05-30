// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/NXcanSASDefinitions.h"
#include "MantidDataHandling/SaveNXcanSAS.h"
#include "MantidDataHandling/SaveNXcanSASHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

#include "NXcanSASFileTest.h"
#include "NXcanSASTestHelper.h"

#include <filesystem>

using Mantid::DataHandling::SaveNXcanSAS;
using namespace Mantid::DataHandling::NXcanSAS;
using namespace NXcanSASTestHelper;

class SaveNXcanSASTest : public CxxTest::TestSuite, public NXcanSASFileTest {
public:
  // This pair of boilerplate methods prevent the suite being created
  // statically
  // This means the constructor isn't called when running other tests
  static SaveNXcanSASTest *createSuite() { return new SaveNXcanSASTest(); }
  static void destroySuite(SaveNXcanSASTest *suite) { delete suite; }

  void test_that_workspace_without_momentum_transfer_units_is_invalid() {
    // Arrange
    auto ws = WorkspaceCreationHelper::create1DWorkspaceConstantWithXerror(10 /*size*/, 1.23 /*value&*/, 2.3 /*error*/,
                                                                           23.4 /*xerror*/);
    const std::string filename = "SaveNXcanSASTestFile.h5";

    // Act + Assert
    auto saveAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("SaveNXcanSAS");
    saveAlg->setChild(true);
    saveAlg->initialize();
    saveAlg->setProperty("Filename", filename);
    TSM_ASSERT_THROWS_ANYTHING("Should not save file without momentum transfer units.",
                               saveAlg->setProperty("InputWorkspace", ws));
  }

  void test_that_can_set_run_numbers_as_string_properties() {
    auto saveAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("SaveNXcanSAS");
    saveAlg->setChild(true);
    saveAlg->initialize();

    TSM_ASSERT_THROWS_NOTHING("Should be able to set SampleTransmissionRunNumber property",
                              saveAlg->setProperty("SampleTransmissionRunNumber", "5"));
    TSM_ASSERT_THROWS_NOTHING("Should be able to set SampleDirectRunNumber property",
                              saveAlg->setProperty("SampleDirectRunNumber", "6"));
    TSM_ASSERT_THROWS_NOTHING("Should be able to set CanScatterRunNumber property",
                              saveAlg->setProperty("CanScatterRunNumber", "7"));
    TSM_ASSERT_THROWS_NOTHING("Should be able to set CanDirectRunNumber property",
                              saveAlg->setProperty("CanDirectRunNumber", "8"));
  }

  void test_that_sample_run_numbers_included_if_sample_transmission_property_is_set() {
    NXcanSASTestParameters parameters;

    parameters.detectors.emplace_back("front-detector");
    parameters.detectors.emplace_back("rear-detector");
    parameters.invalidDetectors = false;
    parameters.sampleDirectRun = "5";
    parameters.canDirectRun = "6";
    parameters.hasSampleRuns = true;

    auto ws = provide1DWorkspace(parameters);
    setXValuesOn1DWorkspace(ws, parameters.xmin, parameters.xmax);

    parameters.idf = getIDFfromWorkspace(ws);

    // Create transmission can
    parameters.transmissionParameters = TransmissionTestParameters(sasTransmissionSpectrumNameSampleAttrValue);
    auto transmission = getTransmissionWorkspace(parameters.transmissionParameters);

    // Act
    save_file_no_issues(ws, parameters, transmission, nullptr);

    // Assert
    do_assert(parameters);

    // Clean up
    removeFile(parameters.filePath);
  }

  void test_that_can_run_numbers_included_if_can_transmission_property_is_set() {
    NXcanSASTestParameters parameters;

    parameters.detectors.emplace_back("front-detector");
    parameters.detectors.emplace_back("rear-detector");
    parameters.invalidDetectors = false;
    parameters.sampleDirectRun = "5";
    parameters.canDirectRun = "6";
    parameters.hasCanRuns = true;

    auto ws = provide1DWorkspace(parameters);
    setXValuesOn1DWorkspace(ws, parameters.xmin, parameters.xmax);

    parameters.idf = getIDFfromWorkspace(ws);

    // Create transmission can
    parameters.transmissionCanParameters = TransmissionTestParameters(sasTransmissionSpectrumNameCanAttrValue);
    auto transmissionCan = getTransmissionWorkspace(parameters.transmissionCanParameters);

    // Act
    save_file_no_issues(ws, parameters, nullptr, transmissionCan);

    // Assert
    do_assert(parameters);

    // Clean up
    removeFile(parameters.filePath);
  }

  void test_that_can_and_sample_runs_included_if_both_transmission_properties_are_set() {
    NXcanSASTestParameters parameters;

    parameters.detectors.emplace_back("front-detector");
    parameters.detectors.emplace_back("rear-detector");
    parameters.invalidDetectors = false;
    parameters.sampleDirectRun = "5";
    parameters.canDirectRun = "6";
    parameters.hasCanRuns = true;
    parameters.hasSampleRuns = true;

    auto ws = provide1DWorkspace(parameters);
    setXValuesOn1DWorkspace(ws, parameters.xmin, parameters.xmax);

    parameters.idf = getIDFfromWorkspace(ws);

    // Create transmission
    parameters.transmissionParameters = TransmissionTestParameters(sasTransmissionSpectrumNameSampleAttrValue);
    parameters.transmissionCanParameters = TransmissionTestParameters(sasTransmissionSpectrumNameCanAttrValue);
    auto transmission = getTransmissionWorkspace(parameters.transmissionParameters);
    auto transmissionCan = getTransmissionWorkspace(parameters.transmissionCanParameters);

    // Act
    save_file_no_issues(ws, parameters, transmission, transmissionCan);

    // Assert
    do_assert(parameters);

    // Clean up
    removeFile(parameters.filePath);
  }

  void test_that_1D_workspace_without_transmissions_is_saved_correctly() {
    // Arrange
    NXcanSASTestParameters parameters;

    parameters.detectors.emplace_back("front-detector");
    parameters.detectors.emplace_back("rear-detector");
    parameters.invalidDetectors = false;

    auto ws = provide1DWorkspace(parameters);
    setXValuesOn1DWorkspace(ws, parameters.xmin, parameters.xmax);

    parameters.idf = getIDFfromWorkspace(ws);

    // Act
    save_file_no_issues(ws, parameters);

    // Assert
    do_assert(parameters);

    // Clean up
    removeFile(parameters.filePath);
  }

  void test_that_sample_bgsub_values_included_if_properties_are_set() {
    NXcanSASTestParameters parameters;

    parameters.detectors.emplace_back("front-detector");
    parameters.detectors.emplace_back("rear-detector");
    parameters.invalidDetectors = false;
    parameters.scaledBgSubWorkspace = "a_workspace";
    parameters.scaledBgSubScaleFactor = 1.5;
    parameters.hasBgSub = true;

    auto ws = provide1DWorkspace(parameters);
    setXValuesOn1DWorkspace(ws, parameters.xmin, parameters.xmax);

    parameters.idf = getIDFfromWorkspace(ws);

    // Create transmission can
    parameters.transmissionParameters = TransmissionTestParameters(sasTransmissionSpectrumNameSampleAttrValue);
    auto transmission = getTransmissionWorkspace(parameters.transmissionParameters);

    // Act
    save_file_no_issues(ws, parameters, transmission, nullptr);

    // Assert
    do_assert(parameters);

    // Clean up
    removeFile(parameters.filePath);
  }

  void test_that_unknown_detector_names_are_not_saved() {
    // Arrange
    NXcanSASTestParameters parameters;

    parameters.detectors.emplace_back("wrong-detector1");
    parameters.detectors.emplace_back("wrong-detector2");
    parameters.invalidDetectors = true;

    auto ws = provide1DWorkspace(parameters);

    setXValuesOn1DWorkspace(ws, parameters.xmin, parameters.xmax);

    parameters.idf = getIDFfromWorkspace(ws);

    // Act
    save_file_no_issues(ws, parameters);

    // Assert
    do_assert(parameters);

    // Clean up
    removeFile(parameters.filePath);
  }

  void test_that_1D_workspace_without_transmissions_and_without_xerror_is_saved_correctly() {
    // Arrange
    NXcanSASTestParameters parameters;

    parameters.detectors.emplace_back("front-detector");
    parameters.detectors.emplace_back("rear-detector");
    parameters.invalidDetectors = false;

    parameters.hasDx = false;

    auto ws = provide1DWorkspace(parameters);
    setXValuesOn1DWorkspace(ws, parameters.xmin, parameters.xmax);

    parameters.idf = getIDFfromWorkspace(ws);

    // Act
    save_file_no_issues(ws, parameters);

    // Assert
    do_assert(parameters);

    // Clean up
    removeFile(parameters.filePath);
  }

  void test_that_1D_workspace_with_point_transmissions_is_saved_correctly() {
    const bool isHistogram{false};
    run_test_1D_workspace_with_transmissions_is_saved_correctly(isHistogram);
  }

  void test_that_1D_workspace_with_histogram_transmissions_is_saved_as_points() {
    const bool isHistogram{true};
    run_test_1D_workspace_with_transmissions_is_saved_correctly(isHistogram);
  }

  void run_test_1D_workspace_with_transmissions_is_saved_correctly(const bool isHistogram) {
    // Arrange
    NXcanSASTestParameters parameters;

    parameters.detectors.emplace_back("front-detector");
    parameters.detectors.emplace_back("rear-detector");
    parameters.invalidDetectors = false;
    parameters.isHistogram = isHistogram;
    parameters.hasDx = true;
    auto ws = provide1DWorkspace(parameters);
    parameters.idf = getIDFfromWorkspace(ws);

    // Create transmission
    parameters.transmissionParameters = TransmissionTestParameters(sasTransmissionSpectrumNameSampleAttrValue);
    parameters.transmissionParameters.isHistogram = isHistogram;
    parameters.transmissionCanParameters = TransmissionTestParameters(sasTransmissionSpectrumNameSampleAttrValue);
    parameters.transmissionCanParameters.isHistogram = isHistogram;

    auto transmission = getTransmissionWorkspace(parameters.transmissionParameters);
    auto transmissionCan = getTransmissionWorkspace(parameters.transmissionCanParameters);

    // Act
    save_file_no_issues(ws, parameters, transmission, transmissionCan);

    // Assert
    do_assert(parameters);

    // Clean up
    removeFile(parameters.filePath);
  }

  void test_that_2D_workspace_is_saved_correctly() {
    // Arrange
    NXcanSASTestParameters parameters;

    parameters.detectors.emplace_back("front-detector");
    parameters.detectors.emplace_back("rear-detector");
    parameters.invalidDetectors = false;

    parameters.is2dData = true;

    auto ws = provide2DWorkspace(parameters);
    set2DValues(ws);

    parameters.idf = getIDFfromWorkspace(ws);

    // Act
    save_file_no_issues(ws, parameters);

    // Assert
    do_assert(parameters);

    // Clean up
    removeFile(parameters.filePath);
  }

  void test_that_group_workspaces_are_saved_correctly_in_individual_files() {
    // Arrange
    NXcanSASTestParameters parameters;

    auto &ads = Mantid::API::AnalysisDataService::Instance();
    auto const ws_group = provideGroupWorkspace(ads, parameters);

    // Act
    save_file_no_issues(std::dynamic_pointer_cast<Mantid::API::Workspace>(ws_group), parameters);

    for (auto const &suffix : parameters.expectedGroupSuffices) {
      // Assert
      auto tmpFilePath = parameters.filePath;
      parameters.filePath.insert(tmpFilePath.size() - 3, suffix);
      TS_ASSERT(!std::filesystem::is_empty(parameters.filePath));
      do_assert(parameters);

      // clean files
      removeFile(parameters.filePath);
      parameters.filePath = tmpFilePath;
    }
    // Clean ads
    ads.clear();
  }

  void test_that_2D_workspace_histogram_is_saved_correctly() {
    // Arrange
    NXcanSASTestParameters parameters;

    parameters.detectors.emplace_back("front-detector");
    parameters.detectors.emplace_back("rear-detector");
    parameters.invalidDetectors = false;

    parameters.is2dData = true;
    parameters.isHistogram = true;

    auto ws = provide2DWorkspace(parameters);
    set2DValues(ws);

    parameters.idf = getIDFfromWorkspace(ws);

    // Act
    save_file_no_issues(ws, parameters);

    // Assert
    do_assert(parameters);

    // Clean up
    removeFile(parameters.filePath);
  }

private:
  void save_file_no_issues(const Mantid::API::Workspace_sptr &workspace, NXcanSASTestParameters &parameters,
                           const Mantid::API::MatrixWorkspace_sptr &transmission = nullptr,
                           const Mantid::API::MatrixWorkspace_sptr &transmissionCan = nullptr) {
    auto saveAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("SaveNXcanSAS");
    saveAlg->initialize();
    parameters.filePath = generateRandomFilename();

    saveAlg->setProperty("Filename", parameters.filePath);
    saveAlg->setProperty("InputWorkspace", workspace);
    saveAlg->setProperty("RadiationSource", parameters.radiationSource);
    saveAlg->setProperty("Geometry", parameters.geometry);
    saveAlg->setProperty("SampleHeight", parameters.beamHeight);
    saveAlg->setProperty("SampleWidth", parameters.beamWidth);
    if (!parameters.detectors.empty()) {
      std::string detectorsAsString = concatenateStringVector(parameters.detectors);
      saveAlg->setProperty("DetectorNames", detectorsAsString);
    }
    saveAlg->setProperty("SampleThickness", parameters.sampleThickness);

    if (transmission) {
      saveAlg->setProperty("Transmission", transmission);
    }
    if (transmissionCan) {
      saveAlg->setProperty("TransmissionCan", transmissionCan);
    }

    saveAlg->setProperty("SampleTransmissionRunNumber", parameters.sampleTransmissionRun);
    saveAlg->setProperty("SampleDirectRunNumber", parameters.sampleDirectRun);
    saveAlg->setProperty("CanScatterRunNumber", parameters.canScatterRun);
    saveAlg->setProperty("CanDirectRunNumber", parameters.canDirectRun);
    saveAlg->setProperty("BackgroundSubtractionWorkspace", parameters.scaledBgSubWorkspace);
    saveAlg->setProperty("BackgroundSubtractionScaleFactor", parameters.scaledBgSubScaleFactor);

    TSM_ASSERT_THROWS_NOTHING("Should not throw anything", saveAlg->execute());
    TSM_ASSERT("Should have executed", saveAlg->isExecuted());
  }
};
