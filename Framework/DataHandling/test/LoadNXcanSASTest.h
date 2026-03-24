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
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/Load.h"
#include "MantidDataHandling/LoadNXcanSAS.h"
#include "MantidDataHandling/NXcanSASDefinitions.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/VectorHelper.h"
#include "NXcanSASTestHelper.h"

using Mantid::DataHandling::NXcanSAS::LoadNXcanSAS;
using namespace NXcanSASTestHelper;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling::NXcanSAS;
namespace {
constexpr double eps = 1e-6;

MatrixWorkspace_sptr convertToPointData(const MatrixWorkspace_sptr &ws) {
  const auto toPointAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("ConvertToPointData");
  toPointAlg->initialize();
  toPointAlg->setChild(true);
  toPointAlg->setProperty("InputWorkspace", ws);
  toPointAlg->setProperty("OutputWorkspace", "toPointOutput");
  toPointAlg->execute();
  return toPointAlg->getProperty("OutputWorkspace");
}

void compareLogTo(const MatrixWorkspace_sptr &ws, const std::string &logName, const std::string &logValue) {
  const auto &run = ws->mutableRun();
  // Check for User file
  const bool hasProperty = run.hasProperty(logName);
  TS_ASSERT(hasProperty);
  if (hasProperty) {
    const auto prop = run.getProperty(logName);
    TS_ASSERT_EQUALS(logValue, prop->value());
  }
}
} // namespace

class LoadNXcanSASTest : public CxxTest::TestSuite {
public:
  LoadNXcanSASTest() : m_ads(Mantid::API::AnalysisDataService::Instance()), m_parameters() {}
  // This pair of boilerplate methods prevent the suite being created
  // statically
  // This means the constructor isn't called when running other tests
  static LoadNXcanSASTest *createSuite() { return new LoadNXcanSASTest(); }
  static void destroySuite(LoadNXcanSASTest *suite) { delete suite; }
  void setUp() override { m_parameters = NXcanSASTestParameters(); }
  void tearDown() override {
    m_ads.clear();
    removeFile(m_parameters.filePath());
  }

  void test_that_1D_workspace_with_Q_resolution_can_be_loaded() {
    m_parameters.detectors.emplace_back("front-detector");
    m_parameters.detectors.emplace_back("rear-detector");
    m_parameters.invalidDetectors = false;
    m_parameters.hasDx = true;

    const auto ws = provide1DWorkspace(m_parameters);
    setXValuesOn1DWorkspace(ws, m_parameters.xmin, m_parameters.xmax);
    m_parameters.idf = getIDFfromWorkspace(ws);
    save_file_no_issues(ws);

    // Act
    const auto wsOut = std::dynamic_pointer_cast<MatrixWorkspace>(load_file_no_issues());

    // Assert
    do_assert_load(ws, wsOut);
  }

  void test_that_1D_workspace_without_Q_resolution_can_be_loaded() {
    // Arrange
    m_parameters.detectors.emplace_back("front-detector");
    m_parameters.detectors.emplace_back("rear-detector");
    m_parameters.invalidDetectors = false;
    m_parameters.hasDx = false;

    const auto ws = provide1DWorkspace(m_parameters);
    setXValuesOn1DWorkspace(ws, m_parameters.xmin, m_parameters.xmax);
    m_parameters.idf = getIDFfromWorkspace(ws);
    save_file_no_issues(ws);

    // Act
    const auto wsOut = std::dynamic_pointer_cast<MatrixWorkspace>(load_file_no_issues());

    // Assert
    do_assert_load(ws, wsOut);
  }

  void test_that_1D_workspace_with_transmissions_can_be_loaded() {
    // Arrange
    m_parameters.detectors.emplace_back("front-detector");
    m_parameters.detectors.emplace_back("rear-detector");
    m_parameters.invalidDetectors = false;
    m_parameters.hasDx = false;
    m_parameters.loadTransmission = true;

    const auto ws = provide1DWorkspace(m_parameters);
    setXValuesOn1DWorkspace(ws, m_parameters.xmin, m_parameters.xmax);
    m_parameters.idf = getIDFfromWorkspace(ws);

    // Create transmission
    m_parameters.transmissionParameters = TransmissionTestParameters(sasTransmissionSpectrumNameSampleAttrValue);
    m_parameters.transmissionCanParameters = TransmissionTestParameters(sasTransmissionSpectrumNameCanAttrValue);

    const auto transmission = getTransmissionWorkspace(m_parameters.transmissionParameters);
    const auto transmissionCan = getTransmissionWorkspace(m_parameters.transmissionCanParameters);
    save_file_no_issues(ws, transmission, transmissionCan);

    // Act
    const auto wsOut = std::dynamic_pointer_cast<MatrixWorkspace>(load_file_no_issues());

    // Assert
    do_assert_load(ws, wsOut, transmission, transmissionCan);
  }

  void test_that_legacy_transmissions_saved_as_histograms_are_loaded() {
    m_parameters.overwriteFilePath("NXcanSAS-histo-lambda.h5");
    const auto wsOut = std::dynamic_pointer_cast<MatrixWorkspace>(load_file_no_issues());
    TS_ASSERT(!wsOut->isHistogramData());
  }

  void test_that_2D_workspace_can_be_loaded() {
    m_parameters.detectors.emplace_back("front-detector");
    m_parameters.detectors.emplace_back("rear-detector");
    m_parameters.invalidDetectors = false;
    m_parameters.is2dData = true;
    m_parameters.hasDx = false;

    const auto ws = provide2DWorkspace(m_parameters);
    set2DValues(ws);
    m_parameters.idf = getIDFfromWorkspace(ws);

    save_file_no_issues(ws);

    // Act
    const auto wsOut = std::dynamic_pointer_cast<MatrixWorkspace>(load_file_no_issues());

    // Assert
    do_assert_load(ws, wsOut);
  }

  void test_that_2D_workspace_histogram_can_be_loaded() {
    m_parameters.detectors.emplace_back("front-detector");
    m_parameters.detectors.emplace_back("rear-detector");
    m_parameters.invalidDetectors = false;

    m_parameters.is2dData = true;
    m_parameters.hasDx = false;
    m_parameters.isHistogram = true;

    const auto ws = provide2DWorkspace(m_parameters);
    set2DValues(ws);
    const std::string outWsName = "loadNXcanSASTestOutputWorkspace";
    m_parameters.idf = getIDFfromWorkspace(ws);

    save_file_no_issues(ws);

    // Act
    const auto wsOut = std::dynamic_pointer_cast<MatrixWorkspace>(load_file_no_issues());

    // NXcanSAS loads as point data
    const auto wsPoint = convertToPointData(ws);

    // Assert
    do_assert_load(wsPoint, wsOut);
  }

  void test_that_1D_workspace_histogram_can_be_loaded() {
    m_parameters.detectors.emplace_back("front-detector");
    m_parameters.detectors.emplace_back("rear-detector");
    m_parameters.invalidDetectors = false;
    m_parameters.hasDx = true;
    m_parameters.isHistogram = true;

    const auto ws = provide1DWorkspace(m_parameters);
    setXValuesOn1DWorkspace(ws, m_parameters.xmin, m_parameters.xmax);
    m_parameters.idf = getIDFfromWorkspace(ws);
    save_file_no_issues(ws);

    // Act
    const auto wsOut = std::dynamic_pointer_cast<MatrixWorkspace>(load_file_no_issues());

    // NXcanSAS loads as point data
    const auto wsPoint = convertToPointData(ws);

    // Assert
    do_assert_load(wsPoint, wsOut);
  }

  void test_that_1D_workspace_with_sample_set_is_loaded_correctly() {
    m_parameters.detectors.emplace_back("front-detector");
    m_parameters.detectors.emplace_back("rear-detector");
    m_parameters.invalidDetectors = false;
    m_parameters.hasDx = true;
    m_parameters.geometry = "FlatPlate";
    m_parameters.beamHeight = 23;
    m_parameters.beamWidth = 12;
    m_parameters.sampleThickness = 6;
    m_parameters.isHistogram = true;

    const auto ws = provide1DWorkspace(m_parameters);
    setXValuesOn1DWorkspace(ws, m_parameters.xmin, m_parameters.xmax);
    m_parameters.idf = getIDFfromWorkspace(ws);
    save_file_no_issues(ws);

    // Act
    const auto wsOut = std::dynamic_pointer_cast<MatrixWorkspace>(load_file_no_issues());

    // NXcanSAS loads as point data
    const auto wsPoint = convertToPointData(ws);

    // Assert
    do_assert_load(wsPoint, wsOut);
  }

  void test_1D_half_polarized_data_is_loaded_correctly() {
    m_parameters.polWorkspaceNumber = 2;
    m_parameters.is2dData = false;
    m_parameters.isPolarized = true;
    m_parameters.hasDx = false;
    m_parameters.inputSpinStates = std::string("0-1,0+1");
    m_parameters.magneticFieldDirection = "1,2,3";

    const auto groupIn = providePolarizedGroup(m_ads, m_parameters);
    save_file_no_issues(std::dynamic_pointer_cast<Workspace>(groupIn));
    const auto groupOut = std::dynamic_pointer_cast<WorkspaceGroup>(load_file_no_issues());

    // Assert
    do_assert_polarized_groups(groupIn, groupOut);
  }

  void test_2d_full_polarized_data_is_loaded_correctly() {
    m_parameters.polWorkspaceNumber = 4;
    m_parameters.is2dData = true;
    m_parameters.isPolarized = true;
    m_parameters.hasDx = false;
    m_parameters.inputSpinStates = std::string("-1-1,-1+1,+1-1,+1+1");
    m_parameters.magneticFieldDirection = "1,2,3";

    const auto groupIn = providePolarizedGroup(m_ads, m_parameters);
    save_file_no_issues(std::dynamic_pointer_cast<Workspace>(groupIn));
    const auto groupOut = std::dynamic_pointer_cast<WorkspaceGroup>(load_file_no_issues());

    // Assert
    do_assert_polarized_groups(groupIn, groupOut);
  }

  void test_load_will_load() {
    // create a file
    m_parameters.detectors.emplace_back("front-detector");
    m_parameters.detectors.emplace_back("rear-detector");
    m_parameters.invalidDetectors = false;
    m_parameters.is2dData = true;
    m_parameters.hasDx = false;

    const auto ws = provide2DWorkspace(m_parameters);
    set2DValues(ws);
    m_parameters.idf = getIDFfromWorkspace(ws);

    save_file_no_issues(ws);

    // now try to load it with NxcanSAS
    Mantid::DataHandling::Load load;
    load.initialize();
    load.setPropertyValue("Filename", m_parameters.filePath());
    TS_ASSERT_EQUALS(load.getPropertyValue("LoaderName"), "LoadNXcanSAS");
  }

private:
  AnalysisDataServiceImpl &m_ads;
  NXcanSASTestParameters m_parameters;

  Workspace_sptr load_file_no_issues() {
    LoadNXcanSAS alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", m_parameters.filePath()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LoadTransmission", m_parameters.loadTransmission));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", m_parameters.loadedWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    Mantid::API::Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::Workspace>(
                                 m_parameters.loadedWSName));
    if (m_parameters.isPolarized) {
      TS_ASSERT(ws->isGroup());
    }
    TS_ASSERT(ws);
    return ws;
  }

  void save_file_no_issues(const Workspace_sptr &workspace, const MatrixWorkspace_sptr &transmission = nullptr,
                           const MatrixWorkspace_sptr &transmissionCan = nullptr) {

    const std::string saveAlgName = m_parameters.isPolarized ? "SavePolarizedNXcanSAS" : "SaveNXcanSAS";
    const auto saveAlg = AlgorithmManager::Instance().createUnmanaged(saveAlgName);
    saveAlg->initialize();
    saveAlg->setProperty("Filename", m_parameters.filePath());

    if (m_parameters.isPolarized) {
      saveAlg->setProperty("InputWorkspace", std::dynamic_pointer_cast<WorkspaceGroup>(workspace));
      saveAlg->setProperty("InputSpinStates", m_parameters.inputSpinStates);
      saveAlg->setProperty("MagneticFieldDirection", m_parameters.magneticFieldDirection);
    } else {
      saveAlg->setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(workspace));
    }

    saveAlg->setProperty("RadiationSource", m_parameters.radiationSource);
    if (!m_parameters.detectors.empty()) {
      saveAlg->setProperty("DetectorNames", concatenateStringVector(m_parameters.detectors));
    }
    saveAlg->setProperty("Geometry", m_parameters.geometry);
    saveAlg->setProperty("SampleHeight", m_parameters.beamHeight);
    saveAlg->setProperty("SampleWidth", m_parameters.beamWidth);
    saveAlg->setProperty("SampleThickness", m_parameters.sampleThickness);

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
    const auto numberOfHistograms = wsIn->getNumberHistograms();

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
                              const TransmissionTestParameters &transmissionParams) {
    if (!transmissionParams.usesTransmission || !transIn) {
      return;
    }

    auto transName = mainWorkspace->getTitle();
    const std::string transExtension = "_trans_" + transmissionParams.name;
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
                      const MatrixWorkspace_sptr &transmission = nullptr,
                      const MatrixWorkspace_sptr &transmissionCan = nullptr) {
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
    if (m_parameters.hasDx) {
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
    if (m_parameters.transmissionParameters.usesTransmission) {
      do_assert_transmission(wsOut, std::move(transmission), m_parameters.transmissionParameters);
    }
    if (m_parameters.transmissionCanParameters.usesTransmission) {
      do_assert_transmission(wsOut, std::move(transmissionCan), m_parameters.transmissionCanParameters);
    }
  }

  void do_assert_polarized_groups(const WorkspaceGroup_sptr &groupIn, const WorkspaceGroup_sptr &groupOut) {
    const auto spinVec = VectorHelper::splitStringIntoVector<std::string>(m_parameters.inputSpinStates, ",");
    TSM_ASSERT_EQUALS("Both input/output groups must have the same number of entries", groupIn->getNumberOfEntries(),
                      groupOut->getNumberOfEntries());
    for (auto n = 0; n < groupOut->getNumberOfEntries(); n++) {
      const auto wsIn = std::dynamic_pointer_cast<MatrixWorkspace>(groupIn->getItem(n));
      const auto wsPoint = convertToPointData(wsIn);
      const auto wsOut = std::dynamic_pointer_cast<MatrixWorkspace>(groupOut->getItem(n));
      compareLogTo(wsOut, "spin_state_NXcanSAS", spinVec.at(n));
      compareLogTo(wsOut, sasSampleEMFieldDirectionPolar, "1");
      compareLogTo(wsOut, sasSampleEMFieldDirectionAzimuthal, "2");
      compareLogTo(wsOut, sasSampleEMFieldDirectionRotation, "3");
      do_assert_load(wsPoint, wsOut);
    }
  }
};
