// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ConvertToPointData.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/MaskBins.h"
#include "MantidAlgorithms/RebinRagged2.h"
#include "MantidDataHandling/LoadNexusProcessed.h"

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::DataHandling;

using Mantid::Algorithms::ConvertToPointData;
using Mantid::Algorithms::CreateSampleWorkspace;
using Mantid::Algorithms::MaskBins;
using Mantid::Algorithms::RebinRagged;

class RebinRagged2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RebinRagged2Test *createSuite() { return new RebinRagged2Test(); }
  static void destroySuite(RebinRagged2Test *suite) { delete suite; }

  void test_Init() {
    RebinRagged alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_nomad_inplace() {
    Mantid::DataHandling::LoadNexusProcessed loader;
    loader.initialize();
    loader.setProperty("Filename", "NOM_91796_banks.nxs");
    loader.setProperty("OutputWorkspace", "NOM_91796_banks");
    loader.execute();

    RebinRagged alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "NOM_91796_banks"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    const std::vector<double> xmins = {0.67, 1.20, 2.42, 3.70, 4.12, 0.39};
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMin", xmins));
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const std::vector<double> xmaxs = {10.20, 20.8, nan, nan, nan, 9.35};
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMax", xmaxs));
    const std::vector<double> deltas = {0.02};
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Delta", deltas));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr result = alg.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(result->getNumberHistograms(), 6);

    const std::vector<double> expected_len = {478, 981, 1880, 1816, 1795, 449};

    for (size_t i = 0; i < 6; i++) {
      TS_ASSERT_EQUALS(result->readX(i).size(), expected_len[i]);
    }
  }

  void test_nomad_no_mins() {
    Mantid::DataHandling::LoadNexusProcessed loader;
    loader.initialize();
    loader.setProperty("Filename", "NOM_91796_banks.nxs");
    loader.setProperty("OutputWorkspace", "NOM_91796_banks");
    loader.execute();

    RebinRagged alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "NOM_91796_banks"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const std::vector<double> xmaxs = {10.20, 20.8, std::numeric_limits<double>::infinity(), nan, nan, 9.35};
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMax", xmaxs));
    const std::vector<double> deltas = {0.04}; // double original data bin size
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Delta", deltas));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr result = alg.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(result->getNumberHistograms(), 6);

    const std::vector<double> expected_len = {256, 521, 1001, 1001, 1001, 235};

    for (size_t i = 0; i < 6; i++) {
      TS_ASSERT_EQUALS(result->readX(i).size(), expected_len[i]);
    }
  }

  void test_hist_workspace() {
    std::vector<double> xmins(200, 2600.);
    xmins[11] = 3000.;
    std::vector<double> xmaxs(200, 6200.);
    xmaxs[12] = 5000.;
    std::vector<double> deltas(200, 400.);
    deltas[13] = 600.;

    CreateSampleWorkspace createSampleWorkspaceAlgo;
    createSampleWorkspaceAlgo.setChild(true);
    createSampleWorkspaceAlgo.initialize();
    createSampleWorkspaceAlgo.setPropertyValue("WorkspaceType", "Histogram");
    createSampleWorkspaceAlgo.setPropertyValue("OutputWorkspace", "_unused_for_child");
    createSampleWorkspaceAlgo.execute();
    MatrixWorkspace_sptr ws = createSampleWorkspaceAlgo.getProperty("OutputWorkspace");

    RebinRagged alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMin", xmins));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMax", xmaxs));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Delta", deltas));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr result = alg.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(result->getNumberHistograms(), 200);

    for (size_t i = 0; i < result->getNumberHistograms(); i++) {
      const auto X = result->readX(i);
      if (i == 11)
        TS_ASSERT_EQUALS(X.size(), 9)
      else if (i == 12 || i == 13)
        TS_ASSERT_EQUALS(X.size(), 7)
      else
        TS_ASSERT_EQUALS(X.size(), 10)

      const auto Y = result->readY(i);
      if (i == 13)
        for (const auto y : Y)
          TS_ASSERT_DELTA(y, 0.9, 1e-9)
      else
        for (const auto y : Y)
          TS_ASSERT_DELTA(y, 0.6, 1e-9)
    }
  }

  void test_event_workspace() {
    std::vector<double> xmins(200, 2600.);
    xmins[11] = 3000.;
    std::vector<double> xmaxs(200, 6200.);
    xmaxs[12] = 5000.;
    std::vector<double> deltas(200, 400.);
    deltas[13] = 600.;

    CreateSampleWorkspace createSampleWorkspaceAlgo;
    createSampleWorkspaceAlgo.setChild(true);
    createSampleWorkspaceAlgo.initialize();
    createSampleWorkspaceAlgo.setPropertyValue("WorkspaceType", "Event");
    createSampleWorkspaceAlgo.setPropertyValue("OutputWorkspace", "_unused_for_child");
    createSampleWorkspaceAlgo.execute();
    MatrixWorkspace_sptr ws = createSampleWorkspaceAlgo.getProperty("OutputWorkspace");

    RebinRagged alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMin", xmins));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMax", xmaxs));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Delta", deltas));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr result = alg.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(result->getNumberHistograms(), 200);

    for (size_t i = 0; i < result->getNumberHistograms(); i++) {
      const auto X = result->readX(i);
      if (i == 11)
        TS_ASSERT_EQUALS(X.size(), 9)
      else if (i == 12 || i == 13)
        TS_ASSERT_EQUALS(X.size(), 7)
      else
        TS_ASSERT_EQUALS(X.size(), 10)

      const auto Y = result->readY(i);
      if (i == 13)
        for (const auto y : Y)
          TS_ASSERT_EQUALS(y, 21)
      else
        for (const auto y : Y)
          TS_ASSERT_EQUALS(y, 14)
    }
  }

  void test_event_workspace_preserverEvents_false() {
    std::vector<double> xmins(200, 2600.);
    xmins[11] = 3000.;
    std::vector<double> xmaxs(200, 6200.);
    xmaxs[12] = 5000.;
    std::vector<double> deltas(200, 400.);
    deltas[13] = 600.;

    CreateSampleWorkspace createSampleWorkspaceAlgo;
    createSampleWorkspaceAlgo.setChild(true);
    createSampleWorkspaceAlgo.initialize();
    createSampleWorkspaceAlgo.setPropertyValue("WorkspaceType", "Event");
    createSampleWorkspaceAlgo.setPropertyValue("OutputWorkspace", "_unused_for_child");
    createSampleWorkspaceAlgo.execute();
    MatrixWorkspace_sptr ws = createSampleWorkspaceAlgo.getProperty("OutputWorkspace");

    RebinRagged alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMin", xmins));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMax", xmaxs));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Delta", deltas));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PreserveEvents", false));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr result = alg.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(result->getNumberHistograms(), 200);

    for (size_t i = 0; i < result->getNumberHistograms(); i++) {
      const auto X = result->readX(i);
      if (i == 11)
        TS_ASSERT_EQUALS(X.size(), 9)
      else if (i == 12 || i == 13)
        TS_ASSERT_EQUALS(X.size(), 7)
      else
        TS_ASSERT_EQUALS(X.size(), 10)

      const auto Y = result->readY(i);
      if (i == 13)
        for (const auto y : Y)
          TS_ASSERT_EQUALS(y, 21)
      else
        for (const auto y : Y)
          TS_ASSERT_EQUALS(y, 14)
    }
  }

  void test_bin_mask_propagation() {
    // based on the MaskBin usage example and expected output from RebinRagged Version=1

    CreateSampleWorkspace createSampleWorkspaceAlgo;
    createSampleWorkspaceAlgo.setChild(true);
    createSampleWorkspaceAlgo.initialize();
    createSampleWorkspaceAlgo.setPropertyValue("WorkspaceType", "Histogram");
    createSampleWorkspaceAlgo.setPropertyValue("OutputWorkspace", "_unused_for_child");
    createSampleWorkspaceAlgo.setProperty("BankPixelWidth", 1);
    createSampleWorkspaceAlgo.setProperty("XMax", 100.);
    createSampleWorkspaceAlgo.setProperty("BinWidth", 10.);
    createSampleWorkspaceAlgo.execute();
    MatrixWorkspace_sptr ws = createSampleWorkspaceAlgo.getProperty("OutputWorkspace");

    MaskBins maskBins;
    maskBins.setChild(true);
    maskBins.initialize();
    maskBins.setProperty("InputWorkspace", ws);
    maskBins.setPropertyValue("OutputWorkspace", "_unused_for_child");
    maskBins.setProperty("XMin", 16.);
    maskBins.setProperty("XMax", 32.);
    maskBins.execute();
    ws = maskBins.getProperty("OutputWorkspace");

    // check bin mask before RebinRagged

    TS_ASSERT_EQUALS(ws->readX(0).size(), 11);
    TS_ASSERT_EQUALS(ws->readX(1).size(), 11);

    auto hist0BinMasked = ws->maskedBinsIndices(0);
    auto hist1BinMasked = ws->maskedBinsIndices(1);

    TS_ASSERT_EQUALS(hist0BinMasked.size(), 3);
    TS_ASSERT_EQUALS(hist1BinMasked.size(), 3);

    for (size_t i = 0; i < 3; i++) {
      TS_ASSERT_EQUALS(hist0BinMasked[i], i + 1);
      TS_ASSERT_EQUALS(hist1BinMasked[i], i + 1);
    }

    std::vector<double> xmins = {-20., 20.};
    std::vector<double> deltas = {10.};

    RebinRagged alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMin", xmins));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Delta", deltas));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr result = alg.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(result->getNumberHistograms(), 2);

    TS_ASSERT_EQUALS(result->readX(0).size(), 13);
    TS_ASSERT_EQUALS(result->readX(1).size(), 9);

    hist0BinMasked = result->maskedBinsIndices(0);
    hist1BinMasked = result->maskedBinsIndices(1);

    TS_ASSERT_EQUALS(hist0BinMasked.size(), 3);
    TS_ASSERT_EQUALS(hist1BinMasked.size(), 2);

    for (size_t i = 0; i < 3; i++)
      TS_ASSERT_EQUALS(hist0BinMasked[i], i + 3);

    for (size_t i = 0; i < 2; i++)
      TS_ASSERT_EQUALS(hist1BinMasked[i], i);
  }

  void test_point_data() {
    std::vector<double> xmins(200, 2600.);
    xmins[11] = 3000.;
    std::vector<double> xmaxs(200, 6200.);
    xmaxs[12] = 5000.;
    std::vector<double> deltas(200, 400.);
    deltas[13] = 600.;

    CreateSampleWorkspace createSampleWorkspaceAlgo;
    createSampleWorkspaceAlgo.setChild(true);
    createSampleWorkspaceAlgo.initialize();
    createSampleWorkspaceAlgo.setPropertyValue("OutputWorkspace", "_unused_for_child");
    createSampleWorkspaceAlgo.execute();
    MatrixWorkspace_sptr ws = createSampleWorkspaceAlgo.getProperty("OutputWorkspace");

    ConvertToPointData ctpd;
    ctpd.setChild(true);
    ctpd.initialize();
    ctpd.setProperty("InputWorkspace", ws);
    ctpd.setProperty("OutputWorkspace", "_unused_for_child");
    ctpd.execute();

    ws = ctpd.getProperty("OutputWorkspace");

    RebinRagged alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMin", xmins));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMax", xmaxs));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Delta", deltas));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr result = alg.getProperty("OutputWorkspace");

    TS_ASSERT(!ws->isHistogramData());     // check input is still point data
    TS_ASSERT(!result->isHistogramData()); // check output is point data
    TS_ASSERT_EQUALS(result->getNumberHistograms(), 200);

    for (size_t i = 0; i < result->getNumberHistograms(); i++) {
      const auto X = result->readX(i);
      if (i == 11)
        TS_ASSERT_EQUALS(X.size(), 8)
      else if (i == 12 || i == 13)
        TS_ASSERT_EQUALS(X.size(), 6)
      else
        TS_ASSERT_EQUALS(X.size(), 9)

      const auto Y = result->readY(i);
      if (i == 13)
        for (const auto y : Y)
          TS_ASSERT_DELTA(y, 0.9, 1e-9)
      else
        for (const auto y : Y)
          TS_ASSERT_DELTA(y, 0.6, 1e-9)
    }
  }
};
