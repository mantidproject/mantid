// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/AppendSpectra.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidFrameworkTestHelpers/InstrumentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using Mantid::Types::Core::DateAndTime;

namespace {
constexpr std::string TOP_WS_NAME = "TOP_WS_NAME";
constexpr std::string BOTTOM_WS_NAME = "BOTTOM_WS_NAME";
} // namespace

class AppendSpectraTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AppendSpectraTest *createSuite() { return new AppendSpectraTest(); }
  static void destroySuite(AppendSpectraTest *suite) { delete suite; }
  void tearDown() override { AnalysisDataService::Instance().clear(); }

  AppendSpectraTest() {}

  void test_algorithm_execution() {
    loadTestWorkspaces(TOP_WS_NAME, BOTTOM_WS_NAME);

    // Get the two input workspaces for later
    MatrixWorkspace_sptr in1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(TOP_WS_NAME);
    MatrixWorkspace_sptr in2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(BOTTOM_WS_NAME);

    // Mask a spectrum and check it is carried over
    const size_t maskTOP_WS_NAME(5), maskBOTTOM_WS_NAME(10);
    in1->getSpectrum(maskTOP_WS_NAME).clearData();
    in2->getSpectrum(maskBOTTOM_WS_NAME).clearData();
    in1->mutableSpectrumInfo().setMasked(maskTOP_WS_NAME, true);
    in2->mutableSpectrumInfo().setMasked(maskBOTTOM_WS_NAME, true);

    // Now it should succeed
    auto appendSpectra = Mantid::API::AlgorithmManager::Instance().create("AppendSpectra");
    doAppendSpectraWithWorkspacesTest(appendSpectra, TOP_WS_NAME, BOTTOM_WS_NAME, TOP_WS_NAME);

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(TOP_WS_NAME));
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 25);
    // Check a few values
    TS_ASSERT_EQUALS(output->x(0)[0], in1->x(0)[0]);
    TS_ASSERT_EQUALS(output->x(15)[444], in2->x(5)[444]);
    TS_ASSERT_EQUALS(output->y(3)[99], in1->y(3)[99]);
    TS_ASSERT_EQUALS(output->e(7)[700], in1->e(7)[700]);
    TS_ASSERT_EQUALS(output->y(19)[55], in2->y(9)[55]);
    TS_ASSERT_EQUALS(output->e(10)[321], in2->e(0)[321]);
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(5), in1->getAxis(1)->spectraNo(5));
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(12), in2->getAxis(1)->spectraNo(2));

    // Check masking
    TS_ASSERT_EQUALS(output->spectrumInfo().isMasked(maskTOP_WS_NAME), true);
    TS_ASSERT_EQUALS(output->spectrumInfo().isMasked(10 + maskBOTTOM_WS_NAME), true);
  }

  void test_algorithm_execution_with_multiple_appends() {
    loadTestWorkspaces(TOP_WS_NAME, BOTTOM_WS_NAME);

    // Get the two input workspaces for later
    MatrixWorkspace_sptr in1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(TOP_WS_NAME);
    MatrixWorkspace_sptr in2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(BOTTOM_WS_NAME);

    // Now it should succeed
    auto appendSpectra = Mantid::API::AlgorithmManager::Instance().create("AppendSpectra");
    doAppendSpectraWithWorkspacesTest(appendSpectra, TOP_WS_NAME, BOTTOM_WS_NAME, TOP_WS_NAME, false, false, false, 2);

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(TOP_WS_NAME));
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 40);
    // Check a few values
    TS_ASSERT_EQUALS(output->x(0)[0], in1->x(0)[0]);
    TS_ASSERT_EQUALS(output->x(15)[444], in2->x(5)[444]);
    TS_ASSERT_EQUALS(output->x(30)[444], in2->x(5)[444]);
    TS_ASSERT_EQUALS(output->y(3)[99], in1->y(3)[99]);
    TS_ASSERT_EQUALS(output->e(7)[700], in1->e(7)[700]);
    TS_ASSERT_EQUALS(output->y(19)[55], in2->y(9)[55]);
    TS_ASSERT_EQUALS(output->e(10)[321], in2->e(0)[321]);
    TS_ASSERT_EQUALS(output->y(34)[55], in2->y(9)[55]);
    TS_ASSERT_EQUALS(output->e(25)[321], in2->e(0)[321]);
    // There will be a spectra number clash here so all spectra numbers
    // should be reset
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(5), 5);
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(12), 12);
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(27), 27);
  }

  void test_two_workspaces_with_different_types() {
    MatrixWorkspace_sptr ews = WorkspaceCreationHelper::createEventWorkspace(10, 10);

    // Check it fails if mixing event workspaces and workspace 2Ds
    AppendSpectra alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace1", ews));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace2", WorkspaceCreationHelper::create2DWorkspace(10, 10)));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "outevent"));
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_two_workspaces_with_non_constant_bins() {
    AppendSpectra alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace1", WorkspaceCreationHelper::create2DWorkspace(10, 10)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace2", WorkspaceCreationHelper::create2DWorkspace(10, 15)));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "outExecNonConstantBins"));
    alg.execute();
    TS_ASSERT(!alg.isExecuted());
  }

  void test_event_workspaces() { doTypeAndParamsTest(true); }

  void test_event_workspaces_with_merging_logs() { doTypeAndParamsTest(true, true); }

  void test_event_workspaces_with_multiple_appends() { doTypeAndParamsTest(true, false, 3); }

  void test_event_workspaces_with_merging_logs_and_multiple_appends() { doTypeAndParamsTest(true, true, 3); }

  void test_2D_workspaces() { doTypeAndParamsTest(false); }

  void test_2D_workspaces_with_merging_logs() { doTypeAndParamsTest(false, true); }

  void test_2D_workspaces_with_multiple_appends() { doTypeAndParamsTest(false, false, 3); }

  void test_2D_workspaces_with_merging_logs_and_multiple_appends() { doTypeAndParamsTest(false, true, 3); }

  void test_no_empty_text_axis() {
    const std::string inputWorkspace = "weRebinned";
    const std::string outputWorkspace = "appended";
    auto appendSpectra = Mantid::API::AlgorithmManager::Instance().create("AppendSpectra");

    createWorkspaceWithAxisAndLabel(inputWorkspace, "Text", "Text");
    doAppendSpectraWithWorkspacesTest(appendSpectra, inputWorkspace, inputWorkspace, outputWorkspace);
    MatrixWorkspace_const_sptr inputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputWorkspace);
    MatrixWorkspace_const_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWorkspace);

    // Y axis number is 1, no need to cast up to TextAxis as we're only reading
    // the values
    const auto inputAxis = inputWS->getAxis(1);
    const auto outputAxis = outputWS->getAxis(1);

    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      // check that all labels are the same
      TS_ASSERT_EQUALS(inputAxis->label(0), outputAxis->label(i));
    }
  }

  void test_empty_text_axis() {
    const std::string inputWorkspace = "weRebinned";
    const std::string outputWorkspace = "appended";
    auto appendSpectra = Mantid::API::AlgorithmManager::Instance().create("AppendSpectra");

    createWorkspaceWithAxisAndLabel(inputWorkspace, "Text", "");
    doAppendSpectraWithWorkspacesTest(appendSpectra, inputWorkspace, inputWorkspace, outputWorkspace);
    MatrixWorkspace_const_sptr inputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputWorkspace);
    MatrixWorkspace_const_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWorkspace);

    // Y axis number is 1, no need to cast up to TextAxis as we're only reading
    // the values
    const auto inputAxis = inputWS->getAxis(1);
    const auto outputAxis = outputWS->getAxis(1);

    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      // check that all labels are the same
      TS_ASSERT_EQUALS(inputAxis->label(0), outputAxis->label(i));
    }
  }

  void test_empty_and_not_empty_text_axis() {
    const std::string inputWorkspace1 = "weRebinned1";
    const std::string inputWorkspace2 = "weRebinned2";
    const std::string outputWorkspace = "appended";
    auto appendSpectra = Mantid::API::AlgorithmManager::Instance().create("AppendSpectra");

    createWorkspaceWithAxisAndLabel(inputWorkspace1, "Text", "Text");
    createWorkspaceWithAxisAndLabel(inputWorkspace2, "Text", "");
    doAppendSpectraWithWorkspacesTest(appendSpectra, inputWorkspace1, inputWorkspace2, outputWorkspace);

    MatrixWorkspace_const_sptr inputWS1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputWorkspace1);
    MatrixWorkspace_const_sptr inputWS2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputWorkspace2);
    MatrixWorkspace_const_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWorkspace);

    // Y axis number is 1, no need to cast up to TextAxis as we're only reading
    // the values
    const auto inputAxis1 = inputWS1->getAxis(1);
    const auto inputAxis2 = inputWS2->getAxis(1);
    const auto outputAxis = outputWS->getAxis(1);

    const auto ws1len = inputWS1->getNumberHistograms();

    for (size_t i = 0; i < outputWS->getNumberHistograms() / 2; ++i) {
      // check that all labels are the same
      // this axis label will have value "Text"
      TS_ASSERT_EQUALS(inputAxis1->label(0), outputAxis->label(i));

      // this axis label will have value "" <= empty string
      // this will check the labels for the second workspace that is
      // appended at position starting from the length of the first workspace
      TS_ASSERT_EQUALS(inputAxis2->label(0), outputAxis->label(i + ws1len));
    }
  }

  void test_numeric_axis() {
    const std::string inputWorkspace = "weRebinned";
    const std::string outputWorkspace = "appended";
    auto appendSpectra = Mantid::API::AlgorithmManager::Instance().create("AppendSpectra");

    createWorkspaceWithAxisAndLabel(inputWorkspace, "Time", "1.0");
    doAppendSpectraWithWorkspacesTest(appendSpectra, inputWorkspace, inputWorkspace, outputWorkspace);
    MatrixWorkspace_const_sptr inputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputWorkspace);
    MatrixWorkspace_const_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWorkspace);

    // Y axis number is 1, no need to cast up to TextAxis as we're only reading
    // the values
    const auto inputAxis = inputWS->getAxis(1);
    const auto outputAxis = outputWS->getAxis(1);

    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      // check that all labels are the same
      TS_ASSERT_EQUALS(inputAxis->getValue(0), outputAxis->getValue(i));
    }
  }

  void test_different_numeric_axis() {
    const std::string inputWorkspace1 = "weRebinned1";
    const std::string inputWorkspace2 = "weRebinned2";
    const std::string outputWorkspace = "appended";
    auto appendSpectra = Mantid::API::AlgorithmManager::Instance().create("AppendSpectra");

    createWorkspaceWithAxisAndLabel(inputWorkspace1, "Time", "1.0");
    createWorkspaceWithAxisAndLabel(inputWorkspace2, "Time", "2.0");
    doAppendSpectraWithWorkspacesTest(appendSpectra, inputWorkspace1, inputWorkspace2, outputWorkspace);

    MatrixWorkspace_const_sptr inputWS1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputWorkspace1);
    MatrixWorkspace_const_sptr inputWS2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputWorkspace2);
    MatrixWorkspace_const_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWorkspace);

    // Y axis number is 1, no need to cast up to TextAxis as we're only reading
    // the values
    const auto inputAxis1 = inputWS1->getAxis(1);
    const auto inputAxis2 = inputWS2->getAxis(1);
    const auto outputAxis = outputWS->getAxis(1);

    const auto ws1len = inputWS1->getNumberHistograms();

    for (size_t i = 0; i < outputWS->getNumberHistograms() / 2; ++i) {
      // check that all labels are the same
      TS_ASSERT_EQUALS(inputAxis1->getValue(0), outputAxis->getValue(i));
      // this will check the labels for the second workspace that is
      // appended at position starting from the length of the first workspace
      TS_ASSERT_EQUALS(inputAxis2->getValue(0), outputAxis->getValue(i + ws1len));
    }
  }

  void test_overlapped_append_y_axis_with_numeric_axis() { doAppendWorkspacesWithNumericAxisTest(true, false, true); }

  void test_overlapped_append_y_axis_with_numeric_axis_disabled() {
    doAppendWorkspacesWithNumericAxisTest(false, false, true);
  }

  void test_overlapped_append_y_axis_with_bin_edges_axis() { doAppendWorkspacesWithNumericAxisTest(true, true, true); }

  void test_overlapped_append_y_axis_with_bin_edges_axis_disabled() {
    doAppendWorkspacesWithNumericAxisTest(false, true, true);
  }

  void test_append_y_axis_with_numeric_axis() { doAppendWorkspacesWithNumericAxisTest(true, false, false); }

  void test_append_y_axis_with_numeric_axis_disabled() { doAppendWorkspacesWithNumericAxisTest(false, false, false); }

  void test_append_y_axis_with_bin_edges_axis() { doAppendWorkspacesWithNumericAxisTest(true, true, false); }

  void test_append_y_axis_with_bin_edges_axis_disabled() { doAppendWorkspacesWithNumericAxisTest(false, true, false); }

  void test_append_y_axis_with_different_y_axis_type() {
    doAppendWorkspacesWithNumericAxisTest(false, false, false, true);
  }

  void test_append_spectra_with_rewrite_on_non_overlapping_detectors() {
    const std::string outName = "outWs";
    loadTestWorkspaces(TOP_WS_NAME, BOTTOM_WS_NAME);

    const auto appendSpectra = Mantid::API::AlgorithmManager::Instance().create("AppendSpectra");
    doAppendSpectraWithWorkspacesTest(appendSpectra, TOP_WS_NAME, BOTTOM_WS_NAME, outName, false, false, false, 1,
                                      true);

    const auto wsBottom = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(BOTTOM_WS_NAME);
    const auto wsTop = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(TOP_WS_NAME);
    const auto output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outName);
    const auto &siBottom = wsBottom->spectrumInfo();
    const auto &siTop = wsTop->spectrumInfo();
    const auto &siOutput = output->spectrumInfo();

    TS_ASSERT_EQUALS(output->getNumberHistograms(), wsBottom->getNumberHistograms() + wsTop->getNumberHistograms());
    TS_ASSERT_DELTA(siBottom.position(0).norm(), siOutput.position(wsTop->getNumberHistograms()).norm(), 1e-5);
    TS_ASSERT_DELTA(siTop.position(0).norm(), siOutput.position(0).norm(), 1e-5);
  }

  void test_append_spectra_with_rewrite_on_overlapping_detectors() {
    const std::string outName = "outWs";
    createOverlappingGroupedWorkspaces();

    const auto appendSpectra = Mantid::API::AlgorithmManager::Instance().create("AppendSpectra");
    doAppendSpectraWithWorkspacesTest(appendSpectra, TOP_WS_NAME, BOTTOM_WS_NAME, outName, false, false, false, 1,
                                      true);

    const auto wsBottom = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(BOTTOM_WS_NAME);
    const auto wsTop = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(TOP_WS_NAME);
    const auto output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outName);
    const auto &siBottom = wsBottom->spectrumInfo();
    const auto &siTop = wsTop->spectrumInfo();
    const auto &siOutput = output->spectrumInfo();
    // output should be 6 spectrum, 3 from each input ws with non repeating detector position for spectra 0 and 3 (which
    // corresponds to same bank as per grouped)
    TS_ASSERT_EQUALS(output->getNumberHistograms(), wsBottom->getNumberHistograms() + wsTop->getNumberHistograms());
    TS_ASSERT_DELTA(siBottom.position(0).norm(), siOutput.position(3).norm(), 1e-5);
    TS_ASSERT_DELTA(siTop.position(0).norm(), siOutput.position(0).norm(), 1e-5);
  }

  void test_append_spectra_with_rewrite_does_not_validate_with_not_enough_detectors() {
    // Creating a sample workspace using basic_rect instrument fulfills this condition
    const auto createSampleWs = AlgorithmManager::Instance().create("CreateSampleWorkspace");
    createSampleWs->setProperty("OutputWorkspace", TOP_WS_NAME);
    createSampleWs->execute();
    createSampleWs->setProperty("OutputWorkspace", BOTTOM_WS_NAME);
    createSampleWs->execute();

    const auto appendSpectra = Mantid::API::AlgorithmManager::Instance().create("AppendSpectra");
    appendSpectra->setProperty("OutputWorkspace", "out");
    appendSpectra->setProperty("InputWorkspace1", TOP_WS_NAME);
    appendSpectra->setProperty("InputWorkspace2", BOTTOM_WS_NAME);
    appendSpectra->setProperty("RewriteSpectraMap", true);
    TSM_ASSERT_THROWS(
        "Basic Rect instrument has 200 detectors, appended spectra has 400 spectrum, not enough to rewrite to "
        "spectrum per detector",
        appendSpectra->execute(), const std::runtime_error &);
  }

  void test_append_spectra_with_rewrite_validates_non_monitor_spectra_only() {
    const auto ws1 = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(6, 20, true, false, true, "testInst");
    const auto ws2 = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 20, true, false, true, "testInst");
    AnalysisDataService::Instance().addOrReplace(TOP_WS_NAME, ws1);
    AnalysisDataService::Instance().addOrReplace(BOTTOM_WS_NAME, ws2);

    auto appendSpectra = Mantid::API::AlgorithmManager::Instance().create("AppendSpectra");
    doAppendSpectraWithWorkspacesTest(appendSpectra, TOP_WS_NAME, BOTTOM_WS_NAME, "out", false, false, false, 1, true);

    const auto output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out");
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 8);
  }

  void test_append_spectra_with_rewrite_does_not_validate_with_non_instrument_ws() {
    const auto ws1 = WorkspaceCreationHelper::create2DWorkspace(5, 20);
    const auto ws2 = WorkspaceCreationHelper::create2DWorkspace(5, 20);
    AnalysisDataService::Instance().addOrReplace(TOP_WS_NAME, ws1);
    AnalysisDataService::Instance().addOrReplace(BOTTOM_WS_NAME, ws2);

    const auto appendSpectra = Mantid::API::AlgorithmManager::Instance().create("AppendSpectra");
    appendSpectra->setRethrows(true);
    appendSpectra->setProperty("OutputWorkspace", "out");
    appendSpectra->setProperty("InputWorkspace1", TOP_WS_NAME);
    appendSpectra->setProperty("InputWorkspace2", BOTTOM_WS_NAME);
    appendSpectra->setProperty("RewriteSpectraMap", true);
    TSM_ASSERT_THROWS("Workspaces with no associated instrument can't rewrite spectra map to detector number",
                      appendSpectra->execute(), const std::runtime_error &);
  }

private:
  void doTypeAndParamsTest(bool event, bool combineLogs = false, int number = 1) {
    const std::string ws1Name = "AppendSpectraTest_grp1";
    const std::string ws2Name = "AppendSpectraTest_grp2";

    MatrixWorkspace_sptr ws1, ws2, out;
    int numBins = 20;

    if (event) {
      ws1 = WorkspaceCreationHelper::createEventWorkspace2(10, numBins); // 2 events per bin
      ws2 = WorkspaceCreationHelper::createEventWorkspace2(5, numBins);
    } else {
      ws1 = WorkspaceCreationHelper::create2DWorkspace(10, numBins);
      ws2 = WorkspaceCreationHelper::create2DWorkspace(5, numBins);
    }
    // Add instrument so detector IDs are valid and get copied.
    InstrumentCreationHelper::addFullInstrumentToWorkspace(*ws1, false, false, "");
    InstrumentCreationHelper::addFullInstrumentToWorkspace(*ws2, false, false, "");

    auto ws1Log = new TimeSeriesProperty<std::string>("aLog");
    ws1Log->addValue(DateAndTime("2014-06-19T16:40:00"), "Hello");
    ws1->mutableRun().addLogData(ws1Log);

    auto ws2Log = new TimeSeriesProperty<std::string>("aLog");
    ws2Log->addValue(DateAndTime("2014-06-19T16:40:10"), "World");
    ws2->mutableRun().addLogData(ws2Log);

    AnalysisDataService::Instance().addOrReplace(ws1Name, ws1);
    AnalysisDataService::Instance().addOrReplace(ws2Name, ws2);

    auto appendSpectra = Mantid::API::AlgorithmManager::Instance().create("AppendSpectra");
    doAppendSpectraWithWorkspacesTest(appendSpectra, ws1Name, ws2Name, ws1Name, false, false, combineLogs, number);

    TS_ASSERT_THROWS_NOTHING(
        out = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(ws1Name));)
    TS_ASSERT(out);
    if (!out)
      return;

    TS_ASSERT_EQUALS(out->getNumberHistograms(), 10 + 5 * number);
    TS_ASSERT_EQUALS(out->blocksize(), numBins);

    for (size_t wi = 0; wi < out->getNumberHistograms(); wi++) {
      TS_ASSERT_EQUALS(out->getSpectrum(wi).getSpectrumNo(), specnum_t(wi));
      TS_ASSERT(!out->getSpectrum(wi).getDetectorIDs().empty());
      const auto &y = out->y(wi);
      for (const auto value : y)
        TS_ASSERT_DELTA(value, 2.0, 1e-5);
    }

    if (combineLogs) {
      TS_ASSERT_EQUALS(out->run().getTimeSeriesProperty<std::string>("aLog")->size(), 2)
    } else {
      TS_ASSERT_EQUALS(out->run().getTimeSeriesProperty<std::string>("aLog")->size(), 1)
    }
  }

  void doAppendSpectraWithWorkspacesTest(const IAlgorithm_sptr &appendSpectra, const std::string &inputWorkspace1,
                                         const std::string &inputWorkspace2, const std::string &outputWorkspace,
                                         const bool shoudThrow = false, const bool appendYAxis = false,
                                         const bool combineLogs = false, const int number = 1,
                                         const bool rewriteSpMap = false) {
    if (!appendSpectra->isInitialized())
      appendSpectra->initialize();
    TS_ASSERT_THROWS_NOTHING(appendSpectra->setRethrows(true));
    TS_ASSERT_THROWS_NOTHING(appendSpectra->setProperty("InputWorkspace1", inputWorkspace1));
    TS_ASSERT_THROWS_NOTHING(appendSpectra->setProperty("InputWorkspace2", inputWorkspace2));
    TS_ASSERT_THROWS_NOTHING(appendSpectra->setProperty("AppendYAxisLabels", appendYAxis));
    TS_ASSERT_THROWS_NOTHING(appendSpectra->setProperty("Number", number));
    TS_ASSERT_THROWS_NOTHING(appendSpectra->setProperty("MergeLogs", combineLogs));
    TS_ASSERT_THROWS_NOTHING(appendSpectra->setProperty("OutputWorkspace", outputWorkspace));
    TS_ASSERT_THROWS_NOTHING(appendSpectra->setProperty("RewriteSpectraMap", rewriteSpMap));

    if (shoudThrow) {
      TS_ASSERT_THROWS_ANYTHING(appendSpectra->execute());
    } else {
      TS_ASSERT_THROWS_NOTHING(appendSpectra->execute());
      TS_ASSERT(appendSpectra->isExecuted());
    }
  }

  void doAppendWorkspacesWithNumericAxisTest(bool appendYAxis, bool isBinEdgeAxis, bool overlappedWS,
                                             bool differentTypes = false) {
    const std::string outputWorkspace = "appended";
    if (overlappedWS || differentTypes) {
      createWorkspaceWithAxisAndLabel(TOP_WS_NAME, "Time", "1.0");
      createWorkspaceWithAxisAndLabel(BOTTOM_WS_NAME, differentTypes ? "Text" : "Time", "1.0");
    } else {
      loadTestWorkspaces(TOP_WS_NAME, BOTTOM_WS_NAME);
    }

    MatrixWorkspace_sptr inputWS1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(TOP_WS_NAME);
    MatrixWorkspace_sptr inputWS2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(BOTTOM_WS_NAME);

    if (!differentTypes) {
      appendNumericAxis(inputWS1, inputWS2, isBinEdgeAxis);
    }

    auto appendSpectra = Mantid::API::AlgorithmManager::Instance().create("AppendSpectra");
    doAppendSpectraWithWorkspacesTest(appendSpectra, TOP_WS_NAME, BOTTOM_WS_NAME, outputWorkspace, differentTypes,
                                      appendYAxis);

    if (!differentTypes) {
      bool isNotOverlappedAndDisabledAppend = !appendYAxis && !overlappedWS;
      MatrixWorkspace_const_sptr outputWS =
          AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWorkspace);
      auto outputAxis = outputWS->getAxis(1);
      for (size_t i = 1; i < outputWS->getNumberHistograms(); ++i) {
        TS_ASSERT_EQUALS(outputAxis->getValue(i), isNotOverlappedAndDisabledAppend ? 0 : i);
      }
    }
  }

  void appendNumericAxis(MatrixWorkspace_sptr ws1, MatrixWorkspace_sptr ws2, const bool isBinEdges = false) {
    const std::size_t ws1num = ws1->getNumberHistograms() + (isBinEdges ? 1 : 0);
    const std::size_t ws2num = ws2->getNumberHistograms() + (isBinEdges ? 1 : 0);

    std::vector<double> ws1Values(ws1num);
    std::vector<double> ws2Values(ws2num);

    size_t i, j;

    for (i = 0; i < ws1num; ++i) {
      ws1Values[i] = static_cast<double>(i);
    }

    for (j = i; j < (ws1num + ws2num); j++) {
      ws2Values[j - ws1num] = static_cast<double>(j - (isBinEdges ? 1 : 0));
    }

    std::unique_ptr<API::NumericAxis> ws1Axis;
    std::unique_ptr<API::NumericAxis> ws2Axis;

    if (isBinEdges) {
      ws1Axis = std::make_unique<API::BinEdgeAxis>(ws1Values);
      ws2Axis = std::make_unique<API::BinEdgeAxis>(ws2Values);
    } else {
      ws1Axis = std::make_unique<API::NumericAxis>(ws1Values);
      ws2Axis = std::make_unique<API::NumericAxis>(ws2Values);
    }

    ws1->replaceAxis(1, std::move(ws1Axis));
    ws2->replaceAxis(1, std::move(ws2Axis));
  }

  /** Creates a 2D workspace with 5 histograms
   */
  void createWorkspaceWithAxisAndLabel(const std::string &outputName, const std::string &axisType,
                                       const std::string &axisValue) {
    int nspec = 5;
    std::vector<std::string> YVals;
    std::vector<double> dataX;
    std::vector<double> dataY;

    for (auto i = 0; i < nspec; ++i) {
      YVals.emplace_back(axisValue);
    }

    for (int i = 0; i < 100; ++i) {
      dataX.emplace_back(static_cast<double>(i));
      dataY.emplace_back(static_cast<double>(i));
    }

    auto createWS = Mantid::API::AlgorithmManager::Instance().create("CreateWorkspace");
    TS_ASSERT_THROWS_NOTHING(createWS->setProperty("OutputWorkspace", "we"));
    TS_ASSERT_THROWS_NOTHING(createWS->setProperty("DataX", dataX));
    TS_ASSERT_THROWS_NOTHING(createWS->setProperty("DataY", dataY));
    TS_ASSERT_THROWS_NOTHING(createWS->setProperty("NSpec", nspec));
    TS_ASSERT_THROWS_NOTHING(createWS->setProperty("UnitX", "Wavelength"));
    TS_ASSERT_THROWS_NOTHING(createWS->setProperty("VerticalAxisUnit", axisType));
    TS_ASSERT_THROWS_NOTHING(createWS->setProperty("VerticalAxisValues", YVals));
    TS_ASSERT_THROWS_NOTHING(createWS->setProperty("YUnitLabel", "Counts"));
    TS_ASSERT_THROWS_NOTHING(createWS->execute());

    // we do a rebin so we can have nice bins
    auto rebin = Mantid::API::AlgorithmManager::Instance().create("Rebin");
    TS_ASSERT_THROWS_NOTHING(rebin->setProperty("InputWorkspace", "we"));
    TS_ASSERT_THROWS_NOTHING(rebin->setProperty("Params", std::vector<double>{1}));
    TS_ASSERT_THROWS_NOTHING(rebin->setProperty("OutputWorkspace", outputName));
    TS_ASSERT_THROWS_NOTHING(rebin->execute());
    TS_ASSERT(rebin->isExecuted());
  }

  void loadTestWorkspaces(const std::string &ws1, const std::string &ws2, const bool nonOverlapping = true) {
    const auto loader = AlgorithmManager::Instance().create("LoadRaw", 3);
    loader->initialize();
    loader->setPropertyValue("Filename", "OSI11886.raw");
    loader->setPropertyValue("OutputWorkspace", ws1);
    if (nonOverlapping) {
      loader->setPropertyValue("SpectrumMin", "1");
      loader->setPropertyValue("SpectrumMax", "10");
    }
    TS_ASSERT_THROWS_NOTHING(loader->execute());
    TS_ASSERT(loader->isExecuted());

    loader->setPropertyValue("Filename", "OSI11886.raw");
    loader->setPropertyValue("OutputWorkspace", ws2);
    if (nonOverlapping) {
      loader->setPropertyValue("SpectrumMin", "11");
      loader->setPropertyValue("SpectrumMax", "25");
    }
    TS_ASSERT_THROWS_NOTHING(loader->execute());
    TS_ASSERT(loader->isExecuted());
  }

  void groupDetectorsInWs(const std::string &wsName, const std::string &groupingPattern) {
    const auto grouper = AlgorithmManager::Instance().create("GroupDetectors");
    grouper->initialize();
    grouper->setPropertyValue("InputWorkspace", wsName);
    grouper->setPropertyValue("OutputWorkspace", wsName);
    grouper->setPropertyValue("GroupingPattern", groupingPattern);

    TS_ASSERT_THROWS_NOTHING(grouper->execute());
    TS_ASSERT(grouper->isExecuted());
  }

  void createOverlappingGroupedWorkspaces() {
    // taking 3 first osiris banks as groups to get 3 spectrum.
    const std::string groupingPattern = "2-121,122-241,242-361";
    loadTestWorkspaces(TOP_WS_NAME, BOTTOM_WS_NAME, false);
    const auto loader = AlgorithmManager::Instance().create("LoadRaw", 3);

    auto ws1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(TOP_WS_NAME);
    auto BOTTOM_WS_NAMEWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(BOTTOM_WS_NAME);
    // we move bank 0 on osiris instrument on Z
    const std::string osirisBankName = "mod0";
    const auto moveAlg = AlgorithmManager::Instance().create("MoveInstrumentComponent");
    moveAlg->initialize();
    moveAlg->setPropertyValue("Workspace", BOTTOM_WS_NAME);
    moveAlg->setPropertyValue("ComponentName", osirisBankName);
    moveAlg->setProperty("Z", 0.1);
    moveAlg->execute();

    groupDetectorsInWs(TOP_WS_NAME, groupingPattern);
    groupDetectorsInWs(BOTTOM_WS_NAME, groupingPattern);
  }
};
