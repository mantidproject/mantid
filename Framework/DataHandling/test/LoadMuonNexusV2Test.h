// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadMuonNexusV2.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/UnitLabelTypes.h"

#include <cxxtest/TestSuite.h>

#include <cmath>
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadMuonNexusV2Test : public CxxTest::TestSuite {
private:
  // helper methods
  std::string createSpectraList(const std::vector<int> &spectraList) {
    std::ostringstream oss;
    std::copy(std::begin(spectraList), std::end(spectraList), std::ostream_iterator<int>(oss, ","));
    std::string spectraStringList(oss.str());
    return spectraStringList;
  }

public:
  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void testExec() {
    LoadMuonNexusV2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "EMU00102347.nxs_v2");
    ld.setPropertyValue("OutputWorkspace", "outWS");

    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());
    // Verify that the output workspace exists
    MatrixWorkspace_sptr output_ws;
    TS_ASSERT_THROWS_NOTHING(output_ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS"));

    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output_ws);

    const Mantid::API::Run &run = output2D->run();
    int goodfrm = run.getPropertyAsIntegerValue("goodfrm");
    TS_ASSERT_EQUALS(goodfrm, 14320);
    double firstGoodData = ld.getProperty("FirstGoodData");
    TS_ASSERT_EQUALS(firstGoodData, 0.384);
    double lastGoodData = ld.getProperty("LastGoodData");
    TS_ASSERT_EQUALS(lastGoodData, 32.768);
    double timeZero = ld.getProperty("TimeZero");
    TS_ASSERT_DELTA(timeZero, 0.1599999, 1e-5);
    std::vector<double> timeZeroVector = ld.getProperty("TimeZeroList");
    TS_ASSERT_EQUALS(timeZeroVector.size(), 96);
    TS_ASSERT_DELTA(timeZeroVector[0], 0.1599999, 1e-5);

    // Check that timeZero has been applied to the output spectra
    // as LoadISISNexus does not do this.
    // First time reading should be shifted by time zero.
    TS_ASSERT_DELTA(output2D->x(3)[0], -0.1599999, 1e-5);
    TS_ASSERT_DELTA(output2D->x(67)[0], -0.1599999, 1e-5);
    TS_ASSERT_DELTA(output2D->x(81)[0], -0.1599999, 1e-5);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS(output2D->getAxis(0)->unit()->unitID(), "Label");
    TS_ASSERT(!output2D->isDistribution());

    // Check that sample temp and field set
    double temperature = run.getPropertyAsSingleValue("sample_temp");
    TS_ASSERT_EQUALS(10.0, temperature);
    double field = run.getPropertyAsSingleValue("sample_magn_field");
    TS_ASSERT_EQUALS(20.0, field);
  }

  void testExecWithDeadtimeTable() {

    LoadMuonNexusV2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "EMU00102347.nxs_v2");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    const std::string deadTimeWSName = "LoadMuonNexusV2Test_DeadTimes";
    ld.setPropertyValue("DeadTimeTable", deadTimeWSName);

    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());
    // Verify that the output workspace exists
    MatrixWorkspace_sptr output_ws;
    TS_ASSERT_THROWS_NOTHING(output_ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS"));

    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output_ws);

    // Check detector grouping table
    TableWorkspace_sptr deadTimeTable;
    TS_ASSERT_THROWS_NOTHING(deadTimeTable =
                                 AnalysisDataService::Instance().retrieveWS<TableWorkspace>(deadTimeWSName));
    TS_ASSERT(deadTimeTable);
    // Check number of rows and columns
    TS_ASSERT_EQUALS(deadTimeTable->columnCount(), 2);
    TS_ASSERT_EQUALS(deadTimeTable->rowCount(), output2D->getNumberHistograms());
    // Check Deadtimes
    TS_ASSERT_DELTA(deadTimeTable->Double(0, 1), -0.0095861498, 1e-6);
    TS_ASSERT_DELTA(deadTimeTable->Double(20, 1), 0.0067306999, 1e-6);
    TS_ASSERT_DELTA(deadTimeTable->Double(62, 1), 0.0073113599, 1e-6);
  }

  void testExecWithTimeZeroTable() {
    LoadMuonNexusV2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "EMU00102347.nxs_v2");
    ld.setPropertyValue("OutputWorkspace", "outWs");
    ld.setPropertyValue("TimeZeroTable", "tzt");
    ld.setRethrows(true);
    auto &ads = AnalysisDataService::Instance();

    ld.execute();
    // Verify that the output workspaces exist
    TS_ASSERT(ads.doesExist("outWs"));
    TS_ASSERT(ads.doesExist("tzt"));

    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(ads.retrieveWS<MatrixWorkspace>("outWs"));
    TableWorkspace_sptr tbl = ads.retrieveWS<TableWorkspace>("tzt");

    // Check number of rows and columns
    TS_ASSERT_EQUALS(tbl->columnCount(), 1);
    TS_ASSERT_EQUALS(tbl->rowCount(), output2D->getNumberHistograms());
    TS_ASSERT_DELTA(tbl->Double(0, 0), 0.16, 0.001);
    TS_ASSERT_DELTA(tbl->Double(47, 0), 0.16, 0.001);
    TS_ASSERT_DELTA(tbl->Double(95, 0), 0.16, 0.001);
  }

  void testExecWithGroupingTable() {

    LoadMuonNexusV2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "EMU00102347.nxs_v2");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    const std::string groupingWSName = "LoadMuonNexusV2Test_Grouping";
    ld.setPropertyValue("DetectorGroupingTable", groupingWSName);
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());
    // Verify that the output workspace exists
    MatrixWorkspace_sptr output_ws;
    TS_ASSERT_THROWS_NOTHING(output_ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS"));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output_ws);

    // Check detector grouping table
    TableWorkspace_sptr groupingTable;
    TS_ASSERT_THROWS_NOTHING(groupingTable =
                                 AnalysisDataService::Instance().retrieveWS<TableWorkspace>(groupingWSName));
    TS_ASSERT(groupingTable);
    // Check number of rows and columns
    TS_ASSERT_EQUALS(groupingTable->columnCount(), 1);
    TS_ASSERT_EQUALS(groupingTable->rowCount(), 2);
    // Check grouping
    std::vector<int> testGroupingVec;
    // Half the detectors are in the first group
    for (int i = 1; i < 49; ++i)
      testGroupingVec.emplace_back(i);
    TS_ASSERT_EQUALS(groupingTable->cell<std::vector<int>>(0, 0), testGroupingVec);
    testGroupingVec.clear();
    for (int i = 49; i < static_cast<int>(output2D->getNumberHistograms() + 1); ++i)
      testGroupingVec.emplace_back(i);
    TS_ASSERT_EQUALS(groupingTable->cell<std::vector<int>>(1, 0), testGroupingVec);
  }

  void testExecWithSpectraList() {
    std::vector<int> spectraIntegerList = {1, 21, 63};
    auto spectraList = createSpectraList(spectraIntegerList);

    LoadMuonNexusV2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "EMU00102347.nxs_v2");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("SpectrumList", spectraList);
    const std::string deadTimeWSName = "LoadMuonNexusV2Test_DeadTimes";
    ld.setPropertyValue("DeadTimeTable", deadTimeWSName);
    const std::string groupingWSName = "LoadMuonNexusV2Test_Grouping";
    ld.setPropertyValue("DetectorGroupingTable", groupingWSName);

    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());
    // Verify that the output workspace exists
    MatrixWorkspace_sptr output_ws;
    TS_ASSERT_THROWS_NOTHING(output_ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS"));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output_ws);

    // Test correct spectra loaded
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 3);
    // Check that spectra maps to correct detector
    for (size_t i = 0; i < 3; ++i) {
      auto detectorgroup = output2D->getSpectrum(i).getDetectorIDs();
      TS_ASSERT_EQUALS(detectorgroup.size(), 1);
      TS_ASSERT_EQUALS(*detectorgroup.begin(), spectraIntegerList[i]);
    }

    // Check detector grouping table
    TableWorkspace_sptr deadTimeTable;
    TS_ASSERT_THROWS_NOTHING(deadTimeTable =
                                 AnalysisDataService::Instance().retrieveWS<TableWorkspace>(deadTimeWSName));
    TS_ASSERT(deadTimeTable);
    // Check number of rows and columns
    TS_ASSERT_EQUALS(deadTimeTable->columnCount(), 2);
    TS_ASSERT_EQUALS(deadTimeTable->rowCount(), 3);
    // Check Deadtimes
    TS_ASSERT_DELTA(deadTimeTable->Double(0, 1), -0.0095861498, 1e-6);
    TS_ASSERT_DELTA(deadTimeTable->Double(1, 1), 0.0067306999, 1e-6);
    TS_ASSERT_DELTA(deadTimeTable->Double(2, 1), 0.0073113599, 1e-6);
  }
  void testExecWithSpectraMax() {

    int specMax = 24;
    LoadMuonNexusV2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "EMU00102347.nxs_v2");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("SpectrumMax", "24");
    const std::string deadTimeWSName = "LoadMuonNexusV2Test_DeadTimes";
    ld.setPropertyValue("DeadTimeTable", deadTimeWSName);
    const std::string groupingWSName = "LoadMuonNexusV2Test_Grouping";
    ld.setPropertyValue("DetectorGroupingTable", groupingWSName);

    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());
    // Verify that the output workspace exists
    MatrixWorkspace_sptr output_ws;
    TS_ASSERT_THROWS_NOTHING(output_ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS"));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output_ws);

    // Test correct spectra loaded
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), specMax);
    // Check that spectra maps to correct detector
    auto detectorgroup = output2D->getSpectrum(specMax - 1).getDetectorIDs();
    TS_ASSERT_EQUALS(detectorgroup.size(), 1);
    TS_ASSERT_EQUALS(*detectorgroup.begin(), specMax);
  }

  void testLoadFailsIfEntryNumberOutOfRange() {
    LoadMuonNexusV2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "EMU00102347.nxs_v2");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("EntryNumber", "10");

    TS_ASSERT(!ld.isExecuted())
  }

  void testLoadFailsIfInvalidSpectraProperties() {
    std::vector<int> spectraIntegerList = {1, 123, 157};
    auto spectraList = createSpectraList(spectraIntegerList);
    LoadMuonNexusV2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "EMU00102347.nxs_v2");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("SpectrumList", spectraList);

    TS_ASSERT(!ld.isExecuted())
  }

  void testMaxThreadsRestoredWhenAlgorithmFinished() {
    int maxThreads = PARALLEL_GET_MAX_THREADS;
    LoadMuonNexusV2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "EMU00102347.nxs_v2");
    ld.setPropertyValue("OutputWorkspace", "outWS");

    ld.execute();

    TS_ASSERT_EQUALS(maxThreads, PARALLEL_GET_MAX_THREADS)
  }

  void test_when_load_uncorrected_time_is_true_that_uncorrected_time_is_loaded() {
    LoadMuonNexusV2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "EMU00102347.nxs_v2");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setProperty("CorrectTime", false);

    ld.execute();

    auto output_ws = AnalysisDataService::Instance().retrieveWS<Workspace2D>("outWS");

    // Check that timeZero has not been applied yet.
    TS_ASSERT_DELTA(output_ws->x(3)[0], 0.0, 1e-5);
    TS_ASSERT_DELTA(output_ws->x(67)[0], 0.0, 1e-5);
    TS_ASSERT_DELTA(output_ws->x(81)[0], 0.0, 1e-5);
  }

  void test_time_zero_list_is_loaded_correctly_when_only_single_time_zero_in_file() {
    LoadMuonNexusV2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "EMU00102347.nxs_v2");
    ld.setPropertyValue("OutputWorkspace", "outWS");

    ld.execute();

    std::vector<double> timeZeroVector = ld.getProperty("TimeZeroList");

    TS_ASSERT_EQUALS(timeZeroVector.size(), 96);
    for (auto value : timeZeroVector) {
      TS_ASSERT_DELTA(value, 0.1599999, 1e-5);
    }
  }
  /* Multi period tests using EMU00103767.nxs_v2
  In this file there are pulses fed into histograms 49 and 50 that form a
  pattern depending on the period
   Period 1 - 49, 50 -> (1, 1) - pulse in 49th and 50th histogram
   Period 2 - 49, 50 -> (0, 1)
   Period 3 - 49, 50 -> (1, 0)
   Period 4 - 49, 50 -> (0, 0)
  */

  void testExecMultiPeriodSinglePeriod() {
    LoadMuonNexusV2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "EMU00103767.nxs_v2");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("EntryNumber", "3");

    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());
    // Verify that the output workspace exists
    MatrixWorkspace_sptr output_ws;
    TS_ASSERT_THROWS_NOTHING(output_ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS"));

    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output_ws);

    const Mantid::API::Run &run = output2D->run();
    int goodfrm = run.getPropertyAsIntegerValue("goodfrm");
    TS_ASSERT_EQUALS(goodfrm, 25000);
    double firstGoodData = ld.getProperty("FirstGoodData");
    TS_ASSERT_EQUALS(firstGoodData, 0.384);
    double lastGoodData = ld.getProperty("LastGoodData");
    TS_ASSERT_EQUALS(lastGoodData, 32.768);
    double timeZero = ld.getProperty("TimeZero");
    TS_ASSERT_DELTA(timeZero, 0.1599999, 1e-5);

    // Check that timeZero has been applied to the output spectra
    // as LoadISISNexus does not do this.
    // First time reading should be shifted by time zero.
    TS_ASSERT_DELTA(output2D->x(3)[0], -0.1599999, 1e-5);
    TS_ASSERT_DELTA(output2D->x(67)[0], -0.1599999, 1e-5);
    TS_ASSERT_DELTA(output2D->x(81)[0], -0.1599999, 1e-5);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS(output2D->getAxis(0)->unit()->unitID(), "Label");
    TS_ASSERT(!output2D->isDistribution());

    // Check that sample temp and field set
    double temperature = run.getPropertyAsSingleValue("sample_temp");
    TS_ASSERT_EQUALS(80.0, temperature);
    double field = run.getPropertyAsSingleValue("sample_magn_field");
    TS_ASSERT_EQUALS(10.0, field);

    // Check spectrum 49 is none zero
    TS_ASSERT_DELTA(output2D->y(48).sum(), 25000, 1e-5);
    TS_ASSERT_DELTA(output2D->y(49).sum(), 0, 1e-5);

    // check loaded period info
    std::string labels = run.getProperty("period_labels")->value();
    TS_ASSERT_EQUALS(labels, "ch49 1 ch50 1;ch49 1 ch50 0;ch49 0 ch50 1;ch49 0 ch50 0");

    std::string sequences = run.getProperty("period_sequences")->value();
    TS_ASSERT_EQUALS(sequences, "50;50;50;50");

    std::string type = run.getProperty("period_type")->value();
    TS_ASSERT_EQUALS(type, "1;1;1;1");

    std::string requested = run.getProperty("frames_period_requested")->value();
    TS_ASSERT_EQUALS(requested, "500;500;500;500");

    std::string raw = run.getProperty("frames_period_raw")->value();
    TS_ASSERT_EQUALS(raw, "25002;25000;25000;25004");

    std::string output = run.getProperty("period_output")->value();
    TS_ASSERT_EQUALS(output, "0;1;2;3");

    std::string counts = run.getProperty("total_counts_period")->value();
    TS_ASSERT_EQUALS(counts, "0.050002;0.025000;0.025000;0.000000");
  }
  void testExecMultiPeriodAllPeriods() {
    LoadMuonNexusV2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "EMU00103767.nxs_v2");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("EntryNumber", "0");

    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());
    // Verify that the output workspace exists
    WorkspaceGroup_sptr outputGroup;
    TS_ASSERT_THROWS_NOTHING(outputGroup = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("outWS"));

    TS_ASSERT_EQUALS(outputGroup->getNumberOfEntries(), 4);

    // Nexus entries to check
    int goodFrames[4] = {25001, 25000, 25000, 25001};
    double histogram_49_data[4] = {25001, 0, 25000, 0};
    double histogram_50_data[4] = {25001, 25000, 0, 0};

    for (int i = 0; i < outputGroup->getNumberOfEntries(); i++) {
      auto output2D = std::dynamic_pointer_cast<Workspace2D>(outputGroup->getItem(i));

      const Mantid::API::Run &run = output2D->run();
      int goodfrm = run.getPropertyAsIntegerValue("goodfrm");
      TS_ASSERT_EQUALS(goodfrm, goodFrames[i]);
      double firstGoodData = ld.getProperty("FirstGoodData");
      TS_ASSERT_EQUALS(firstGoodData, 0.384);
      double lastGoodData = ld.getProperty("LastGoodData");
      TS_ASSERT_EQUALS(lastGoodData, 32.768);
      double timeZero = ld.getProperty("TimeZero");
      TS_ASSERT_DELTA(timeZero, 0.1599999, 1e-5);

      // Check that timeZero has been applied to the output spectra
      // as LoadISISNexus does not do this.
      // First time reading should be shifted by time zero.
      TS_ASSERT_DELTA(output2D->x(3)[0], -0.1599999, 1e-5);
      TS_ASSERT_DELTA(output2D->x(67)[0], -0.1599999, 1e-5);
      TS_ASSERT_DELTA(output2D->x(81)[0], -0.1599999, 1e-5);

      // Check the unit has been set correctly
      TS_ASSERT_EQUALS(output2D->getAxis(0)->unit()->unitID(), "Label");
      TS_ASSERT(!output2D->isDistribution());

      // Check that sample temp and field set
      double temperature = run.getPropertyAsSingleValue("sample_temp");
      TS_ASSERT_EQUALS(80.0, temperature);
      double field = run.getPropertyAsSingleValue("sample_magn_field");
      TS_ASSERT_EQUALS(10.0, field);

      // Check data with respect to the expected binary pattern, see comments
      // above
      TS_ASSERT_DELTA(output2D->y(48).sum(), histogram_49_data[i], 1e-5);
      TS_ASSERT_DELTA(output2D->y(49).sum(), histogram_50_data[i], 1e-5);

      // check loaded period info
      std::string labels = run.getProperty("period_labels")->value();
      TS_ASSERT_EQUALS(labels, "ch49 1 ch50 1;ch49 1 ch50 0;ch49 0 ch50 1;ch49 0 ch50 0");

      std::string sequences = run.getProperty("period_sequences")->value();
      TS_ASSERT_EQUALS(sequences, "50;50;50;50");

      std::string type = run.getProperty("period_type")->value();
      TS_ASSERT_EQUALS(type, "1;1;1;1");

      std::string requested = run.getProperty("frames_period_requested")->value();
      TS_ASSERT_EQUALS(requested, "500;500;500;500");

      std::string raw = run.getProperty("frames_period_raw")->value();
      TS_ASSERT_EQUALS(raw, "25002;25000;25000;25004");

      std::string output = run.getProperty("period_output")->value();
      TS_ASSERT_EQUALS(output, "0;1;2;3");

      std::string counts = run.getProperty("total_counts_period")->value();
      TS_ASSERT_EQUALS(counts, "0.050002;0.025000;0.025000;0.000000");
    }
  }
  void testExecMultiPeriodWithDeadtimeTable() {

    LoadMuonNexusV2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "EMU00103767.nxs_v2");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    const std::string deadTimeWSName = "LoadMuonNexusV2Test_DeadTimes";
    ld.setPropertyValue("DeadTimeTable", deadTimeWSName);

    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());
    // Verify that the output workspace exists
    WorkspaceGroup_sptr outputGroup;
    TS_ASSERT_THROWS_NOTHING(outputGroup = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("outWS"));

    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(outputGroup->getItem(0));

    // Check detector grouping table
    WorkspaceGroup_sptr deadTimeTableGroup;
    TS_ASSERT_THROWS_NOTHING(deadTimeTableGroup =
                                 AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(deadTimeWSName));
    TS_ASSERT_EQUALS(deadTimeTableGroup->getNumberOfEntries(), 4);

    for (int i = 0; i < deadTimeTableGroup->getNumberOfEntries(); ++i) {
      auto deadTimeTable = std::dynamic_pointer_cast<TableWorkspace>(deadTimeTableGroup->getItem(i));
      TS_ASSERT(deadTimeTable);
      // Check number of rows and columns
      TS_ASSERT_EQUALS(deadTimeTable->columnCount(), 2);
      TS_ASSERT_EQUALS(deadTimeTable->rowCount(), output2D->getNumberHistograms());
      // Check Deadtimes
      TS_ASSERT_DELTA(deadTimeTable->Double(0, 1), -0.0095861498, 1e-6);
      TS_ASSERT_DELTA(deadTimeTable->Double(20, 1), 0.0067306999, 1e-6);
      TS_ASSERT_DELTA(deadTimeTable->Double(62, 1), 0.0073113599, 1e-6);
    }
  }
  void testExecMultiPeriodWithGroupingTable() {

    LoadMuonNexusV2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "EMU00103767.nxs_v2");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    const std::string groupingWSName = "LoadMuonNexusV2Test_Grouping";
    ld.setPropertyValue("DetectorGroupingTable", groupingWSName);
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    // Verify that the output workspace exists
    WorkspaceGroup_sptr outputGroup;
    TS_ASSERT_THROWS_NOTHING(outputGroup = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("outWS"));

    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(outputGroup->getItem(0));
    // Check detector grouping table
    TableWorkspace_sptr groupingTable;
    TS_ASSERT_THROWS_NOTHING(groupingTable =
                                 AnalysisDataService::Instance().retrieveWS<TableWorkspace>(groupingWSName));
    TS_ASSERT(groupingTable);
    // Check number of rows and columns
    TS_ASSERT_EQUALS(groupingTable->columnCount(), 1);
    TS_ASSERT_EQUALS(groupingTable->rowCount(), 2);
    // Check grouping
    std::vector<int> testGroupingVec;
    // Half the detectors are in the first group
    for (int i = 1; i < 49; ++i)
      testGroupingVec.emplace_back(i);
    TS_ASSERT_EQUALS(groupingTable->cell<std::vector<int>>(0, 0), testGroupingVec);
    testGroupingVec.clear();
    for (int i = 49; i < static_cast<int>(output2D->getNumberHistograms() + 1); ++i)
      testGroupingVec.emplace_back(i);
    TS_ASSERT_EQUALS(groupingTable->cell<std::vector<int>>(1, 0), testGroupingVec);
  }

  void test_loading_data_with_three_periods_but_only_two_histograms_gives_expected_period_counts_property() {
    LoadMuonNexusV2 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "HIFI00183810.nxs");
    loader.setPropertyValue("OutputWorkspace", "outWS");

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    auto const output = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("outWS");
    auto const workspace2D = std::dynamic_pointer_cast<Workspace2D>(output->getItem(0));
    auto const &run = workspace2D->run();

    TS_ASSERT_EQUALS("5.033640;5.026534", run.getProperty("total_counts_period")->value());
  }

  void test_loading_detector_grouping_table_when_grouping_info_is_empty_will_load_default_group_from_IDF() {
    LoadMuonNexusV2 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "ARGUS00073601.nxs");
    loader.setPropertyValue("DetectorGroupingTable", "detector_grouping");
    loader.setPropertyValue("DeadTimeTable", "deadtime_table");
    loader.setPropertyValue("OutputWorkspace", "outWS");

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    auto const detTable = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("detector_grouping");
    // When the grouping info is not provided, it should load the grouping from the IDF. The IDF has two groups.
    TS_ASSERT_EQUALS(2, detTable->rowCount());
  }
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class LoadMuonNexusV2TestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    loader.initialize();
    loader.setPropertyValue("Filename", "EMU00102347.nxs_v2");
    loader.setPropertyValue("OutputWorkspace", "ws");
  }

  void tearDown() override { AnalysisDataService::Instance().remove("ws"); }

  void testDefaultLoad() { loader.execute(); }

private:
  LoadMuonNexusV2 loader;
};
