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
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/CalculateEfficiency2.h"
#include "MantidDataHandling/Load.h"
#include "MantidDataHandling/LoadSpice2D.h"
#include "MantidDataHandling/MoveInstrumentComponent.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/Unit.h"
#include "MantidTestHelpers/SANSInstrumentCreationHelper.h"

#include <cxxtest/TestSuite.h>
using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class CalculateEfficiency2Test : public CxxTest::TestSuite {
public:
  /*
   * Generate fake data for which we know what the result should be
   */
  void setUpWorkspace(bool asEventWorkspace = false) {
    inputWS = "sampledata";

    Mantid::DataObjects::Workspace2D_sptr ws = SANSInstrumentCreationHelper::createSANSInstrumentWorkspace(inputWS);

    // Set up the X bin for the monitor channels
    for (int i = 0; i < SANSInstrumentCreationHelper::nMonitors; i++) {
      auto &X = ws->mutableX(i);
      X[0] = 1;
      X[1] = 2;
    }

    for (int ix = 0; ix < SANSInstrumentCreationHelper::nBins; ix++) {
      for (int iy = 0; iy < SANSInstrumentCreationHelper::nBins; iy++) {
        int i = ix * SANSInstrumentCreationHelper::nBins + iy + SANSInstrumentCreationHelper::nMonitors;
        auto &X = ws->mutableX(i);
        auto &Y = ws->mutableY(i);
        auto &E = ws->mutableE(i);
        X[0] = 1;
        X[1] = 2;
        Y[0] = 2.0;
        E[0] = 1;
      }
    }
    // Change one of the bins so that it will be excluded for having a high
    // signal
    auto &Y = ws->mutableY(SANSInstrumentCreationHelper::nMonitors + 5);
    Y[0] = 202.0;

    if (asEventWorkspace) {
      auto convertToEvents = AlgorithmManager::Instance().create("ConvertToEventWorkspace");
      convertToEvents->initialize();
      convertToEvents->setProperty("InputWorkspace", inputWS);
      convertToEvents->setProperty("OutputWorkspace", inputWS);
      convertToEvents->execute();
    }
  }
  /*
   * Generate fake data workspace group for which we know what the result should
   * be
   */
  void setUpWorkspaceGroup() {
    inputWS = "sampledata";

    std::vector<std::string> toGroup;
    auto wsName = inputWS + "_1";
    Mantid::DataObjects::Workspace2D_sptr ws1 = SANSInstrumentCreationHelper::createSANSInstrumentWorkspace(wsName);
    toGroup.emplace_back(wsName);

    wsName = inputWS + "_2";
    Mantid::DataObjects::Workspace2D_sptr ws2 = SANSInstrumentCreationHelper::createSANSInstrumentWorkspace(wsName);
    toGroup.emplace_back(wsName);

    // Set up the X bin for the monitor channels
    for (int i = 0; i < SANSInstrumentCreationHelper::nMonitors; i++) {
      auto &X = ws1->mutableX(i);
      X[0] = 1;
      X[1] = 2;
      auto &X2 = ws2->mutableX(i);
      X2 = X;
    }

    for (int ix = 0; ix < SANSInstrumentCreationHelper::nBins; ix++) {
      for (int iy = 0; iy < SANSInstrumentCreationHelper::nBins; iy++) {
        int i = ix * SANSInstrumentCreationHelper::nBins + iy + SANSInstrumentCreationHelper::nMonitors;
        auto &X = ws1->mutableX(i);
        auto &Y = ws1->mutableY(i);
        auto &E = ws1->mutableE(i);
        X[0] = 1;
        X[1] = 2;
        Y[0] = 1.5;
        E[0] = 0.1;
        auto &X2 = ws2->mutableX(i);
        auto &Y2 = ws2->mutableY(i);
        auto &E2 = ws2->mutableE(i);
        X2 = X;
        Y2[0] = 1.0;
        E2[0] = 0.2;
      }
    }
    // mask certain spectra to test merging
    auto info1 = ws1->spectrumInfo();
    auto info2 = ws2->spectrumInfo();
    info1.setMasked(0, true);
    info2.setMasked(0, true);
    info1.setMasked(1, true);
    info2.setMasked(2, true);
    info1.setMasked(4, true);
    info2.setMasked(4, true);

    auto groupAlg = AlgorithmManager::Instance().create("GroupWorkspaces");
    groupAlg->initialize();
    groupAlg->setAlwaysStoreInADS(true);
    groupAlg->setProperty("InputWorkspaces", toGroup);
    groupAlg->setProperty("OutputWorkspace", inputWS);
    groupAlg->execute();
  }

  void testName() { TS_ASSERT_EQUALS(correction.name(), "CalculateEfficiency") }

  void testVersion() { TS_ASSERT_EQUALS(correction.version(), 2) }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(correction.initialize())
    TS_ASSERT(correction.isInitialized())
  }

  void testExecDefault() {
    setUpWorkspace(true); // create event workspace
    if (!correction.isInitialized())
      correction.initialize();

    const std::string outputWS("testExecDefault_result");
    TS_ASSERT_THROWS_NOTHING(correction.setPropertyValue("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(correction.setPropertyValue("OutputWorkspace", outputWS))

    TS_ASSERT_THROWS_NOTHING(correction.execute())
    TS_ASSERT(correction.isExecuted())

    Mantid::API::Workspace_sptr ws_out;
    TS_ASSERT_THROWS_NOTHING(ws_out = Mantid::API::AnalysisDataService::Instance().retrieve(outputWS));
    Mantid::DataObjects::Workspace2D_sptr ws2d_out =
        std::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws_out);

    double tolerance(1e-03);
    TS_ASSERT_DELTA(ws2d_out->y(1 + SANSInstrumentCreationHelper::nMonitors)[0], 1.0, tolerance);
    TS_ASSERT_DELTA(ws2d_out->y(15 + SANSInstrumentCreationHelper::nMonitors)[0], 1.0, tolerance);
    TS_ASSERT_DELTA(ws2d_out->y(6 + SANSInstrumentCreationHelper::nMonitors)[0], 1.0, tolerance);
    TS_ASSERT_DELTA(ws2d_out->e(1 + SANSInstrumentCreationHelper::nMonitors)[0], 0.5, tolerance);
    TS_ASSERT_DELTA(ws2d_out->e(15 + SANSInstrumentCreationHelper::nMonitors)[0], 0.5, tolerance);
    TS_ASSERT_DELTA(ws2d_out->e(6 + SANSInstrumentCreationHelper::nMonitors)[0], 0.5, tolerance);

    // Check that pixels that were out of range were masked
    const auto &oSpecInfo = ws2d_out->spectrumInfo();
    TS_ASSERT(!oSpecInfo.isMasked(5 + SANSInstrumentCreationHelper::nMonitors));
    TS_ASSERT(!oSpecInfo.isMasked(1 + SANSInstrumentCreationHelper::nMonitors));
  }

  void testExecEvent() {
    setUpWorkspace();
    if (!correction.isInitialized())
      correction.initialize();

    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING(correction.setPropertyValue("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(correction.setPropertyValue("OutputWorkspace", outputWS))

    TS_ASSERT_THROWS_NOTHING(correction.execute())
    TS_ASSERT(correction.isExecuted())

    Mantid::API::Workspace_sptr ws_out;
    TS_ASSERT_THROWS_NOTHING(ws_out = Mantid::API::AnalysisDataService::Instance().retrieve(outputWS));
    Mantid::DataObjects::Workspace2D_sptr ws2d_out =
        std::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws_out);

    double tolerance(1e-03);
    TS_ASSERT_DELTA(ws2d_out->y(1 + SANSInstrumentCreationHelper::nMonitors)[0], 1.0, tolerance);
    TS_ASSERT_DELTA(ws2d_out->y(15 + SANSInstrumentCreationHelper::nMonitors)[0], 1.0, tolerance);
    TS_ASSERT_DELTA(ws2d_out->y(6 + SANSInstrumentCreationHelper::nMonitors)[0], 1.0, tolerance);
    TS_ASSERT_DELTA(ws2d_out->e(1 + SANSInstrumentCreationHelper::nMonitors)[0], 0.5, tolerance);
    TS_ASSERT_DELTA(ws2d_out->e(15 + SANSInstrumentCreationHelper::nMonitors)[0], 0.5, tolerance);
    TS_ASSERT_DELTA(ws2d_out->e(6 + SANSInstrumentCreationHelper::nMonitors)[0], 0.5, tolerance);

    // Check that pixels that were out of range were masked
    const auto &oSpecInfo = ws2d_out->spectrumInfo();
    TS_ASSERT(!oSpecInfo.isMasked(5 + SANSInstrumentCreationHelper::nMonitors));
    TS_ASSERT(!oSpecInfo.isMasked(1 + SANSInstrumentCreationHelper::nMonitors));
  }

  void testExecWithPixelsExcluded() {
    // Repeat the calculation by excluding high/low pixels
    setUpWorkspace();
    if (!correction.isInitialized())
      correction.initialize();

    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING(correction.setPropertyValue("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(correction.setPropertyValue("OutputWorkspace", outputWS))
    TS_ASSERT_THROWS_NOTHING(correction.setProperty<double>("MinThreshold", 0.5))
    TS_ASSERT_THROWS_NOTHING(correction.setProperty<double>("MaxThreshold", 1.50))

    TS_ASSERT_THROWS_NOTHING(correction.execute())
    TS_ASSERT(correction.isExecuted())

    Mantid::API::Workspace_sptr ws_out;
    TS_ASSERT_THROWS_NOTHING(ws_out = Mantid::API::AnalysisDataService::Instance().retrieve(outputWS));
    Mantid::DataObjects::Workspace2D_sptr ws2d_out =
        std::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws_out);

    double tolerance(1e-03);
    TS_ASSERT_DELTA(ws2d_out->x(1 + SANSInstrumentCreationHelper::nMonitors)[0], 1.0, tolerance);
    TS_ASSERT_DELTA(ws2d_out->x(1 + SANSInstrumentCreationHelper::nMonitors)[1], 2.0, tolerance);

    TS_ASSERT_DELTA(ws2d_out->y(1 + SANSInstrumentCreationHelper::nMonitors)[0], 1.0, tolerance);
    TS_ASSERT_DELTA(ws2d_out->y(15 + SANSInstrumentCreationHelper::nMonitors)[0], 1.0, tolerance);
    TS_ASSERT_DELTA(ws2d_out->y(6 + SANSInstrumentCreationHelper::nMonitors)[0], 1.0, tolerance);

    TS_ASSERT_DELTA(ws2d_out->e(1 + SANSInstrumentCreationHelper::nMonitors)[0], 0.5, tolerance);
    TS_ASSERT_DELTA(ws2d_out->e(15 + SANSInstrumentCreationHelper::nMonitors)[0], 0.5, tolerance);
    TS_ASSERT_DELTA(ws2d_out->e(6 + SANSInstrumentCreationHelper::nMonitors)[0], 0.5, tolerance);

    // Check that pixels that were out of range where EMPTY_DBL
    TS_ASSERT_DELTA(ws2d_out->y(5 + SANSInstrumentCreationHelper::nMonitors)[0], EMPTY_DBL(), tolerance);

    const auto &oSpecInfo2 = ws2d_out->spectrumInfo();
    TS_ASSERT(!oSpecInfo2.isMasked(1 + SANSInstrumentCreationHelper::nMonitors));

    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

  void testProcessGroupsMerge() {
    setUpWorkspaceGroup();
    if (!correction.isInitialized())
      correction.initialize();

    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING(correction.setPropertyValue("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(correction.setPropertyValue("OutputWorkspace", outputWS))
    TS_ASSERT_THROWS_NOTHING(correction.setProperty("MergeGroup", true))
    TS_ASSERT_THROWS_NOTHING(correction.execute())
    TS_ASSERT(correction.isExecuted())

    Mantid::API::Workspace_const_sptr ws_out;
    TS_ASSERT_THROWS_NOTHING(ws_out = Mantid::API::AnalysisDataService::Instance().retrieve(outputWS));
    auto ws2d_out = std::static_pointer_cast<const MatrixWorkspace>(ws_out);

    double tolerance(1e-02);
    const auto &oSpecInfo = ws2d_out->spectrumInfo();
    // spectrum not masked in any input
    TS_ASSERT_DELTA(ws2d_out->x(3)[0], 1.0, tolerance);
    TS_ASSERT_DELTA(ws2d_out->x(3)[1], 2.0, tolerance);
    TS_ASSERT_DELTA(ws2d_out->y(3)[0], 1.0, tolerance);
    TS_ASSERT(!oSpecInfo.isMasked(3));

    // spectra masked in one but not the other
    TS_ASSERT_DELTA(ws2d_out->y(1)[0], 1.0, tolerance);
    TS_ASSERT_DELTA(ws2d_out->e(1)[0], 0.0, tolerance);
    TS_ASSERT(!oSpecInfo.isMasked(1));
    TS_ASSERT_DELTA(ws2d_out->y(2)[0], 1.5, tolerance);
    TS_ASSERT_DELTA(ws2d_out->e(2)[0], 0.1, tolerance);
    TS_ASSERT(!oSpecInfo.isMasked(2));

    // the first and last spectra should stay masked
    TS_ASSERT(oSpecInfo.isMasked(0));
    TS_ASSERT(oSpecInfo.isMasked(4));

    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

  void testProcessGroupsIndividual() {
    setUpWorkspaceGroup();
    if (!correction.isInitialized())
      correction.initialize();

    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING(correction.setPropertyValue("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(correction.setPropertyValue("OutputWorkspace", outputWS))
    TS_ASSERT_THROWS_NOTHING(correction.setProperty("MergeGroup", false))
    TS_ASSERT_THROWS_NOTHING(correction.execute())
    TS_ASSERT(correction.isExecuted())

    Mantid::API::Workspace_const_sptr ws_out;
    TS_ASSERT_THROWS_NOTHING(ws_out = Mantid::API::AnalysisDataService::Instance().retrieve(outputWS));
    auto wsGroup_out = std::static_pointer_cast<const WorkspaceGroup>(ws_out);

    double tolerance(1e-02);
    auto nEntries = wsGroup_out->getNumberOfEntries();
    TS_ASSERT_EQUALS(nEntries, 2)
    for (auto entryNo = 0; entryNo < nEntries; entryNo++) {
      auto entry = std::static_pointer_cast<const MatrixWorkspace>(wsGroup_out->getItem(entryNo));
      const auto &oSpecInfo = entry->spectrumInfo();
      // spectrum not masked in any input
      TS_ASSERT_DELTA(entry->x(3)[0], 1, tolerance);
      TS_ASSERT_DELTA(entry->x(3)[1], 2, tolerance);
      TS_ASSERT_DELTA(entry->y(3)[0], 1.0, tolerance);
      TS_ASSERT(!oSpecInfo.isMasked(3));

      // spectra masked in one but not the other, should stay masked
      if (entryNo == 0) {
        TS_ASSERT(oSpecInfo.isMasked(1));
        TS_ASSERT_DELTA(entry->y(2)[0], 1.0, tolerance);
        TS_ASSERT_DELTA(entry->e(2)[0], 0.067, tolerance);
        TS_ASSERT(!oSpecInfo.isMasked(2));
      } else {
        TS_ASSERT_DELTA(entry->y(1)[0], 1.0, tolerance);
        TS_ASSERT_DELTA(entry->e(1)[0], 0.0, tolerance);
        TS_ASSERT(!oSpecInfo.isMasked(1));
        TS_ASSERT(oSpecInfo.isMasked(2));
      }
      // the first and last spectra should stay masked
      TS_ASSERT(oSpecInfo.isMasked(0));
      TS_ASSERT(oSpecInfo.isMasked(4));
    }
    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

  /*
   * Function that will validate results against known results found with
   * "standard" HFIR reduction package.
   */
  void validate() {
    Mantid::DataHandling::LoadSpice2D loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "BioSANS_exp61_scan0004_0001.xml");
    const std::string inputWS("wav");
    loader.setPropertyValue("OutputWorkspace", inputWS);
    loader.execute();

    if (!correction.isInitialized())
      correction.initialize();

    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING(correction.setPropertyValue("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(correction.setPropertyValue("OutputWorkspace", outputWS))
    TS_ASSERT_THROWS_NOTHING(correction.setProperty<double>("MinThreshold", 0.5))
    TS_ASSERT_THROWS_NOTHING(correction.setProperty<double>("MaxThreshold", 1.50))

    correction.execute();

    TS_ASSERT(correction.isExecuted())

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(result = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)))
    TS_ASSERT_EQUALS(result->getNumberHistograms(), 36866)

    TS_ASSERT_EQUALS(result->getAxis(0)->unit()->unitID(), "Wavelength")

    Mantid::API::Workspace_sptr ws_in;
    TS_ASSERT_THROWS_NOTHING(ws_in = Mantid::API::AnalysisDataService::Instance().retrieve(inputWS));
    Mantid::DataObjects::Workspace2D_sptr ws2d_in = std::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws_in);

    Mantid::API::Workspace_sptr ws_out;
    TS_ASSERT_THROWS_NOTHING(ws_out = Mantid::API::AnalysisDataService::Instance().retrieve(outputWS));
    Mantid::DataObjects::Workspace2D_sptr ws2d_out =
        std::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws_out);

    // Number of monitors
    int nmon = Mantid::DataHandling::LoadSpice2D::nMonitors;
    // Get the coordinate of the detector pixel

    double tolerance(1e-03);
    TS_ASSERT_DELTA(ws2d_out->y(1 + nmon)[0], 0.980083, tolerance);
    TS_ASSERT_DELTA(ws2d_out->y(193 + nmon)[0], 1.23006, tolerance);
    TS_ASSERT_DELTA(ws2d_out->y(6 + nmon)[0], 1.10898, tolerance);

    TS_ASSERT_DELTA(ws2d_out->e(1 + nmon)[0], 0.0990047, tolerance);
    TS_ASSERT_DELTA(ws2d_out->e(193 + nmon)[0], 0.110913, tolerance);
    TS_ASSERT_DELTA(ws2d_out->e(6 + nmon)[0], 0.105261, tolerance);

    // Check that pixels that were out of range were masked
    const auto &oSpecInfo = ws2d_out->spectrumInfo();
    TS_ASSERT(oSpecInfo.isMasked(1826));
    TS_ASSERT(oSpecInfo.isMasked(2014));
    TS_ASSERT(oSpecInfo.isMasked(2015));

    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

private:
  Mantid::Algorithms::CalculateEfficiency2 correction;
  std::string inputWS;
};
