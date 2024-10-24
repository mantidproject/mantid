// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include <utility>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/LoadNXcanSAS.h"
#include "MantidDataHandling/NXcanSASDefinitions.h"
#include "MantidKernel/Unit.h"
#include "NXcanSASTestHelper.h"

using Mantid::DataHandling::LoadNXcanSAS;
using namespace NXcanSASTestHelper;
using namespace Mantid::API;
using namespace Mantid::DataHandling::NXcanSAS;
namespace {
const double eps = 1e-6;
}

class LoadNXcanSASTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created
  // statically
  // This means the constructor isn't called when running other tests
  static LoadNXcanSASTest *createSuite() { return new LoadNXcanSASTest(); }
  static void destroySuite(LoadNXcanSASTest *suite) { delete suite; }

  void test_that_1D_workspace_with_Q_resolution_can_be_loaded() {
    // Arrange
    NXcanSASTestParameters parameters;
    removeFile(parameters.filename);
    parameters.detectors.emplace_back("front-detector");
    parameters.detectors.emplace_back("rear-detector");
    parameters.invalidDetectors = false;
    parameters.hasDx = true;

    auto ws = provide1DWorkspace(parameters);
    setXValuesOn1DWorkspace(ws, parameters.xmin, parameters.xmax);
    parameters.idf = getIDFfromWorkspace(ws);
    save_file_no_issues(ws, parameters);

    const std::string outWsName = "loadNXcanSASTestOutputWorkspace";

    // Act
    auto wsOut = load_file_no_issues(parameters, false /*load transmission*/, outWsName);

    // Assert
    do_assert_load(ws, wsOut, parameters);

    // Clean up
    removeFile(parameters.filename);
    removeWorkspaceFromADS(outWsName);
  }

  void test_that_1D_workspace_without_Q_resolution_can_be_loaded() {
    // Arrange
    NXcanSASTestParameters parameters;
    removeFile(parameters.filename);
    parameters.detectors.emplace_back("front-detector");
    parameters.detectors.emplace_back("rear-detector");
    parameters.invalidDetectors = false;
    parameters.hasDx = false;

    auto ws = provide1DWorkspace(parameters);
    setXValuesOn1DWorkspace(ws, parameters.xmin, parameters.xmax);
    parameters.idf = getIDFfromWorkspace(ws);
    save_file_no_issues(ws, parameters);

    const std::string outWsName = "loadNXcanSASTestOutputWorkspace";

    // Act
    auto wsOut = load_file_no_issues(parameters, false /*load transmission*/, outWsName);

    // Assert
    do_assert_load(ws, wsOut, parameters);

    // Clean up
    removeFile(parameters.filename);
    removeWorkspaceFromADS(outWsName);
  }

  void test_that_1D_workspace_with_transmissions_can_be_loaded() {
    // Arrange
    NXcanSASTestParameters parameters;
    removeFile(parameters.filename);
    parameters.detectors.emplace_back("front-detector");
    parameters.detectors.emplace_back("rear-detector");
    parameters.invalidDetectors = false;
    parameters.hasDx = false;

    auto ws = provide1DWorkspace(parameters);
    setXValuesOn1DWorkspace(ws, parameters.xmin, parameters.xmax);
    parameters.idf = getIDFfromWorkspace(ws);

    const std::string outWsName = "loadNXcanSASTestOutputWorkspace";

    // Create transmission
    NXcanSASTestTransmissionParameters transmissionParameters;
    transmissionParameters.name = sasTransmissionSpectrumNameSampleAttrValue;
    transmissionParameters.usesTransmission = true;

    NXcanSASTestTransmissionParameters transmissionCanParameters;
    transmissionCanParameters.name = sasTransmissionSpectrumNameCanAttrValue;
    transmissionCanParameters.usesTransmission = true;

    auto transmission = getTransmissionWorkspace(transmissionParameters);
    setXValuesOn1DWorkspace(transmission, transmissionParameters.xmin, transmissionParameters.xmax);

    auto transmissionCan = getTransmissionWorkspace(transmissionCanParameters);
    setXValuesOn1DWorkspace(transmissionCan, transmissionCanParameters.xmin, transmissionCanParameters.xmax);

    save_file_no_issues(ws, parameters, transmission, transmissionCan);

    // Act
    auto wsOut = load_file_no_issues(parameters, true /*load transmission*/, outWsName);

    // Assert
    do_assert_load(ws, wsOut, parameters, transmission, transmissionCan, transmissionParameters,
                   transmissionCanParameters);

    // Clean up
    auto transName = ws->getTitle();
    const std::string transExtension = "_trans_" + transmissionParameters.name;
    transName += transExtension;

    auto transNameCan = ws->getTitle();
    const std::string transExtensionCan = "_trans_" + transmissionCanParameters.name;
    transNameCan += transExtensionCan;

    removeFile(parameters.filename);
    removeWorkspaceFromADS(outWsName);
    removeWorkspaceFromADS(transName);
    removeWorkspaceFromADS(transNameCan);
  }

  void test_that_legacy_transmissions_saved_as_histograms_are_loaded() {
    NXcanSASTestParameters parameters;
    parameters.filename = "NXcanSAS-histo-lambda.h5";
    const std::string outWsName{"loaded_histo_trans"};

    Mantid::API::MatrixWorkspace_sptr wsOut;
    TS_ASSERT_THROWS_NOTHING(wsOut = load_file_no_issues(parameters, true /*load transmission*/, outWsName));
    TS_ASSERT(!wsOut->isHistogramData());

    removeWorkspaceFromADS(outWsName);
  }

  void test_that_2D_workspace_can_be_loaded() {
    // Arrange
    NXcanSASTestParameters parameters;
    removeFile(parameters.filename);

    parameters.detectors.emplace_back("front-detector");
    parameters.detectors.emplace_back("rear-detector");
    parameters.invalidDetectors = false;

    parameters.is2dData = true;

    auto ws = provide2DWorkspace(parameters);
    set2DValues(ws);
    const std::string outWsName = "loadNXcanSASTestOutputWorkspace";
    parameters.idf = getIDFfromWorkspace(ws);

    save_file_no_issues(ws, parameters);

    // Act
    auto wsOut = load_file_no_issues(parameters, false /*load transmission*/, outWsName);

    // Assert
    do_assert_load(ws, wsOut, parameters);

    // Clean up
    removeFile(parameters.filename);
    removeWorkspaceFromADS(outWsName);
  }

  void test_that_2D_workspace_histogram_can_be_loaded() {
    // Arrange
    NXcanSASTestParameters parameters;
    removeFile(parameters.filename);

    parameters.detectors.emplace_back("front-detector");
    parameters.detectors.emplace_back("rear-detector");
    parameters.invalidDetectors = false;

    parameters.is2dData = true;
    parameters.isHistogram = true;

    auto ws = provide2DWorkspace(parameters);
    set2DValues(ws);
    const std::string outWsName = "loadNXcanSASTestOutputWorkspace";
    parameters.idf = getIDFfromWorkspace(ws);

    save_file_no_issues(ws, parameters);

    // Act
    auto wsOut = load_file_no_issues(parameters, false /*load transmission*/, outWsName);

    // Assert

    // We need to convert the ws file back into point data since the would have
    // loaded point data
    auto toPointAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("ConvertToPointData");
    std::string toPointOutputName("toPointOutput");
    toPointAlg->initialize();
    toPointAlg->setChild(true);
    toPointAlg->setProperty("InputWorkspace", ws);
    toPointAlg->setProperty("OutputWorkspace", toPointOutputName);
    toPointAlg->execute();
    MatrixWorkspace_sptr wsPoint = toPointAlg->getProperty("OutputWorkspace");

    do_assert_load(wsPoint, wsOut, parameters);

    // Clean up
    removeFile(parameters.filename);
    removeWorkspaceFromADS(outWsName);
  }

  void test_that_1D_workspace_histogram_can_be_loaded() {
    // Arrange
    NXcanSASTestParameters parameters;
    removeFile(parameters.filename);
    parameters.detectors.emplace_back("front-detector");
    parameters.detectors.emplace_back("rear-detector");
    parameters.invalidDetectors = false;
    parameters.hasDx = true;

    parameters.isHistogram = true;

    auto ws = provide1DWorkspace(parameters);
    setXValuesOn1DWorkspace(ws, parameters.xmin, parameters.xmax);
    parameters.idf = getIDFfromWorkspace(ws);
    save_file_no_issues(ws, parameters);

    const std::string outWsName = "loadNXcanSASTestOutputWorkspace";

    // Act
    auto wsOut = load_file_no_issues(parameters, false /*load transmission*/, outWsName);

    // Assert
    // We need to convert the ws file back into point data since the would have
    // loaded point data
    auto toPointAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("ConvertToPointData");
    std::string toPointOutputName("toPointOutput");
    toPointAlg->initialize();
    toPointAlg->setChild(true);
    toPointAlg->setProperty("InputWorkspace", ws);
    toPointAlg->setProperty("OutputWorkspace", toPointOutputName);
    toPointAlg->execute();
    MatrixWorkspace_sptr wsPoint = toPointAlg->getProperty("OutputWorkspace");

    do_assert_load(wsPoint, wsOut, parameters);

    // Clean up
    removeFile(parameters.filename);
    removeWorkspaceFromADS(outWsName);
  }

  void test_that_1D_workspace_with_sample_set_is_loaded_correctly() {
    // Arrange
    NXcanSASTestParameters parameters;
    removeFile(parameters.filename);
    parameters.detectors.emplace_back("front-detector");
    parameters.detectors.emplace_back("rear-detector");
    parameters.invalidDetectors = false;
    parameters.hasDx = true;
    parameters.geometry = "FlatPlate";
    parameters.beamHeight = 23;
    parameters.beamWidth = 12;
    parameters.sampleThickness = 6;

    parameters.isHistogram = true;

    auto ws = provide1DWorkspace(parameters);
    setXValuesOn1DWorkspace(ws, parameters.xmin, parameters.xmax);
    parameters.idf = getIDFfromWorkspace(ws);
    save_file_no_issues(ws, parameters);

    const std::string outWsName = "loadNXcanSASTestOutputWorkspace";

    // Act
    auto wsOut = load_file_no_issues(parameters, false /*load transmission*/, outWsName);

    // Assert
    // We need to convert the ws file back into point data since the would have
    // loaded point data
    auto toPointAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("ConvertToPointData");
    std::string toPointOutputName("toPointOutput");
    toPointAlg->initialize();
    toPointAlg->setChild(true);
    toPointAlg->setProperty("InputWorkspace", ws);
    toPointAlg->setProperty("OutputWorkspace", toPointOutputName);
    toPointAlg->execute();
    MatrixWorkspace_sptr wsPoint = toPointAlg->getProperty("OutputWorkspace");

    do_assert_load(wsPoint, wsOut, parameters);

    // Clean up
    removeFile(parameters.filename);
    removeWorkspaceFromADS(outWsName);
  }

private:
  void removeWorkspaceFromADS(const std::string &toRemove) {
    if (AnalysisDataService::Instance().doesExist(toRemove)) {
      AnalysisDataService::Instance().remove(toRemove);
    }
  }

  MatrixWorkspace_sptr load_file_no_issues(NXcanSASTestParameters &parameters, bool loadTransmission,
                                           const std::string &outWsName) {
    LoadNXcanSAS alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", parameters.filename));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LoadTransmission", loadTransmission));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWsName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    Mantid::API::MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(outWsName));
    TS_ASSERT(ws);
    return ws;
  }

  void save_file_no_issues(const MatrixWorkspace_sptr &workspace, NXcanSASTestParameters &parameters,
                           const MatrixWorkspace_sptr &transmission = nullptr,
                           const MatrixWorkspace_sptr &transmissionCan = nullptr) {
    auto saveAlg = AlgorithmManager::Instance().createUnmanaged("SaveNXcanSAS");
    saveAlg->initialize();
    saveAlg->setProperty("Filename", parameters.filename);
    saveAlg->setProperty("InputWorkspace", workspace);
    saveAlg->setProperty("RadiationSource", parameters.radiationSource);
    if (!parameters.detectors.empty()) {
      std::string detectorsAsString = concatenateStringVector(parameters.detectors);
      saveAlg->setProperty("DetectorNames", detectorsAsString);
    }
    saveAlg->setProperty("Geometry", parameters.geometry);
    saveAlg->setProperty("SampleHeight", parameters.beamHeight);
    saveAlg->setProperty("SampleWidth", parameters.beamWidth);
    saveAlg->setProperty("SampleThickness", parameters.sampleThickness);

    if (transmission) {
      saveAlg->setProperty("Transmission", transmission);
    }
    if (transmissionCan) {
      saveAlg->setProperty("TransmissionCan", transmissionCan);
    }

    TSM_ASSERT_THROWS_NOTHING("Should not throw anything", saveAlg->execute());
    TSM_ASSERT("Should have executed", saveAlg->isExecuted());
  }

  template <typename Functor> void do_assert_data(MatrixWorkspace_sptr wsIn, MatrixWorkspace_sptr wsOut, Functor func) {
    TSM_ASSERT("Should have the same number of histograms",
               wsIn->getNumberHistograms() == wsOut->getNumberHistograms());
    auto numberOfHistograms = wsIn->getNumberHistograms();

    for (size_t index = 0; index < numberOfHistograms; ++index) {
      const auto &dataIn = func(wsIn, index);
      const auto &dataOut = func(wsOut, index);
      TSM_ASSERT("Should have the same number of bins", dataIn.size() == dataOut.size());
      auto size = dataIn.size();
      for (size_t binIndex = 0; binIndex < size; ++binIndex) {
        TSM_ASSERT_DELTA("Should be have the same values", dataIn[binIndex], dataOut[binIndex], eps);
      }
    }
  }

  void do_assert_units(const MatrixWorkspace_sptr &wsIn, const MatrixWorkspace_sptr &wsOut) {
    // Ensure that units of axis 0 are matching
    auto unit0In = wsIn->getAxis(0)->unit()->label().ascii();
    auto unit0Out = wsOut->getAxis(0)->unit()->label().ascii();
    TSM_ASSERT_EQUALS("Should have the same axis 0 unit", unit0In, unit0Out);

    if (!wsOut->getAxis(1)->isSpectra()) {
      // Ensure that units of axis 1 are matching
      auto unit1In = wsIn->getAxis(1)->unit()->label().ascii();
      auto unit1Out = wsOut->getAxis(1)->unit()->label().ascii();
      TSM_ASSERT_EQUALS("Should have the same axis 1 unit", unit1In, unit1Out);
    }

    // Ensure tat units of DataY are the same
    auto unitYIn = wsIn->YUnit();
    auto unitYOut = wsOut->YUnit();
    TSM_ASSERT_EQUALS("Should have the same y unit", unitYIn, unitYOut);
  }

  void do_assert_axis1_values_are_the_same(const MatrixWorkspace_sptr &wsIn, const MatrixWorkspace_sptr &wsOut) {
    if (!wsOut->getAxis(1)->isNumeric()) {
      return;
    }

    auto axis1In = wsIn->getAxis(1);
    auto axis1Out = wsOut->getAxis(1);

    auto length = axis1In->length();

    // The numeric axis of wsIn is histo or point data, while axisIn is point
    // data
    auto is_axis1_point_data = length == wsIn->getNumberHistograms();
    if (is_axis1_point_data) {
      for (size_t index = 0; index < length; ++index) {
        TSM_ASSERT_DELTA("Axis 1 should have the same value", axis1In->getValue(index), axis1Out->getValue(index), eps);
      }
    } else {
      for (size_t index = 0; index < length; ++index) {
        TSM_ASSERT_DELTA("Axis 1 should have the same value",
                         (axis1In->getValue(index + 1) + axis1In->getValue(index)) / 2.0, axis1Out->getValue(index),
                         eps);
      }
    }
  }

  void do_assert_sample(const MatrixWorkspace_sptr &wsIn, const MatrixWorkspace_sptr &wsOut) {
    auto &&sampleIn = wsIn->mutableSample();
    auto &&sampleOut = wsOut->mutableSample();

    TSM_ASSERT_EQUALS("Should load the geometry flag from the sample.", sampleIn.getGeometryFlag(),
                      sampleOut.getGeometryFlag());
    TSM_ASSERT_EQUALS("Should load the height of the aperture.", sampleIn.getHeight(), sampleOut.getHeight());
    TSM_ASSERT_EQUALS("Should load the width of the aperture.", sampleIn.getWidth(), sampleOut.getWidth());
    TSM_ASSERT_EQUALS("Should load the thickness of the sample.", sampleIn.getThickness(), sampleOut.getThickness());
  }

  void do_assert_sample_logs(const MatrixWorkspace_sptr &wsIn, const MatrixWorkspace_sptr &wsOut) {
    auto &runIn = wsIn->mutableRun();
    auto &runOut = wsOut->mutableRun();

    // Check for User file
    if (runIn.hasProperty(sasProcessUserFileInLogs)) {
      auto userFileIn = runIn.getProperty(sasProcessUserFileInLogs);
      auto userFileOut = runOut.getProperty(sasProcessUserFileInLogs);
      TSM_ASSERT_EQUALS("Should have loaded the name of the user file.", userFileIn->value(), userFileOut->value());
    }

    // Check for the run number
    if (runIn.hasProperty(sasEntryRunInLogs)) {
      auto runNumberIn = runIn.getProperty(sasEntryRunInLogs);
      auto runNumberOut = runOut.getProperty(sasEntryRunInLogs);
      TSM_ASSERT_EQUALS("Should have loaded the run number.", runNumberIn->value(), runNumberOut->value());
    }
  }

  void do_assert_instrument(const MatrixWorkspace_sptr &wsIn, const MatrixWorkspace_sptr &wsOut) {
    auto idfIn = getIDFfromWorkspace(std::move(wsIn));
    auto idfOut = getIDFfromWorkspace(std::move(wsOut));
    TSM_ASSERT_EQUALS("Should have the same instrument", idfIn, idfOut);
  }

  void do_assert_transmission(const MatrixWorkspace_sptr &mainWorkspace, const MatrixWorkspace_sptr &transIn,
                              const NXcanSASTestTransmissionParameters &parameters) {
    if (!parameters.usesTransmission || !transIn) {
      return;
    }

    auto transName = mainWorkspace->getTitle();
    const std::string transExtension = "_trans_" + parameters.name;
    transName += transExtension;

    auto transOut = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(transName);

    // Ensure that both have the same Y data
    auto readDataY = [](const MatrixWorkspace_sptr &ws, size_t index) { return ws->y(index); };
    do_assert_data(transIn, transOut, readDataY);

    // Ensure that both have the same E data
    auto readDataE = [](const MatrixWorkspace_sptr &ws, size_t index) { return ws->e(index); };
    do_assert_data(transIn, transOut, readDataE);

    // Ensure that both have the same X data
    auto readDataX = [](const MatrixWorkspace_sptr &ws, size_t index) { return ws->x(index); };
    do_assert_data(transIn, transOut, readDataX);
  }

  void do_assert_load(const MatrixWorkspace_sptr &wsIn, const MatrixWorkspace_sptr &wsOut,
                      NXcanSASTestParameters &parameters, const MatrixWorkspace_sptr &transmission = nullptr,
                      const MatrixWorkspace_sptr &transmissionCan = nullptr,
                      const NXcanSASTestTransmissionParameters &sampleParameters = NXcanSASTestTransmissionParameters(),
                      const NXcanSASTestTransmissionParameters &canParameters = NXcanSASTestTransmissionParameters()) {
    // Ensure that both have the same units
    do_assert_units(wsIn, wsOut);

    // Ensure that output workspace is not histogram
    TSM_ASSERT("Should be a point workspace", !wsOut->isHistogramData());

    // Ensure that both have the same Y data
    auto readDataY = [](const MatrixWorkspace_sptr &ws, size_t index) { return ws->y(index); };
    do_assert_data(wsIn, wsOut, readDataY);

    // Ensure that both have the same E data
    auto readDataE = [](const MatrixWorkspace_sptr &ws, size_t index) { return ws->e(index); };
    do_assert_data(wsIn, wsOut, readDataE);

    // Ensure that both have the same X data
    auto readDataX = [](const MatrixWorkspace_sptr &ws, size_t index) { return ws->x(index); };
    do_assert_data(wsIn, wsOut, readDataX);

    // If applicable, ensure that both have the same Xdev data
    if (parameters.hasDx) {
      auto readDataDX = [](const MatrixWorkspace_sptr &ws, size_t index) { return ws->dataDx(index); };
      do_assert_data(wsIn, wsOut, readDataDX);
    }

    // If applicable, ensure that axis1 are the same
    do_assert_axis1_values_are_the_same(wsIn, wsOut);

    // Ensure that the sample information is the same.
    do_assert_sample(wsIn, wsOut);

    // Ensure that both have the same basic logs
    do_assert_sample_logs(wsIn, wsOut);

    // Ensure that both have the same IDF loaded
    do_assert_instrument(wsIn, wsOut);

    // Test transmission workspaces
    do_assert_transmission(wsOut, std::move(transmission), std::move(sampleParameters));
    do_assert_transmission(wsOut, std::move(transmissionCan), std::move(canParameters));
  }
};

class LoadNXcanSASTestPerformance : public CxxTest::TestSuite {
public:
  static LoadNXcanSASTestPerformance *createSuite() { return new LoadNXcanSASTestPerformance(); }

  static void destroySuite(LoadNXcanSASTestPerformance *suite) { delete suite; }

  LoadNXcanSASTestPerformance() {
    setup_1D();
    setup_2D();
  }

  ~LoadNXcanSASTestPerformance() {
    AnalysisDataService::Instance().clear();
    removeFile(parameters1D.filename);
    removeFile(parameters2D.filename);
  }

  void test_execute_1D() { alg1D.execute(); }

  void test_execute_2D() { alg2D.execute(); }

private:
  LoadNXcanSAS alg1D;
  LoadNXcanSAS alg2D;
  NXcanSASTestParameters parameters1D;
  NXcanSASTestParameters parameters2D;

  void save_no_assert(const MatrixWorkspace_sptr &ws, NXcanSASTestParameters &parameters) {
    auto saveAlg = AlgorithmManager::Instance().createUnmanaged("SaveNXcanSAS");
    saveAlg->initialize();
    saveAlg->setProperty("Filename", parameters.filename);
    saveAlg->setProperty("InputWorkspace", ws);
    saveAlg->setProperty("RadiationSource", parameters.radiationSource);
    if (!parameters.detectors.empty()) {
      std::string detectorsAsString = concatenateStringVector(parameters.detectors);
      saveAlg->setProperty("DetectorNames", detectorsAsString);
    }

    saveAlg->execute();
  }

  void setup_1D() {
    removeFile(parameters1D.filename);
    parameters1D.detectors.emplace_back("front-detector");
    parameters1D.detectors.emplace_back("rear-detector");
    parameters1D.invalidDetectors = false;
    parameters1D.hasDx = true;

    auto ws = provide1DWorkspace(parameters1D);
    setXValuesOn1DWorkspace(ws, parameters1D.xmin, parameters1D.xmax);
    parameters1D.idf = getIDFfromWorkspace(ws);

    save_no_assert(ws, parameters1D);

    const std::string outWsName = "loadNXcanSASTestOutputWorkspace";

    alg1D.initialize();
    alg1D.setPropertyValue("Filename", parameters1D.filename);

    alg1D.setProperty("LoadTransmission", true);

    alg1D.setPropertyValue("OutputWorkspace", outWsName);
  }

  void setup_2D() {
    removeFile(parameters2D.filename);

    parameters2D.detectors.emplace_back("front-detector");
    parameters2D.detectors.emplace_back("rear-detector");
    parameters2D.invalidDetectors = false;

    parameters2D.is2dData = true;

    auto ws = provide2DWorkspace(parameters2D);
    set2DValues(ws);
    const std::string outWsName = "loadNXcanSASTestOutputWorkspace";
    parameters2D.idf = getIDFfromWorkspace(ws);

    save_no_assert(ws, parameters2D);

    alg2D.initialize();
    alg2D.setPropertyValue("Filename", parameters2D.filename);

    alg2D.setProperty("LoadTransmission", true);

    alg2D.setPropertyValue("OutputWorkspace", outWsName);
  }
};
