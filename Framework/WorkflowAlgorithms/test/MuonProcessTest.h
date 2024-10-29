// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataHandling/Load.h"
#include "MantidDataObjects/TableWorkspace.h"

#include "MantidWorkflowAlgorithms/MuonProcess.h"

using Mantid::WorkflowAlgorithms::MuonProcess;

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

/// Holds data loaded from file
struct LoadedData {
  Workspace_sptr workspace;
  double timeZero;
  Workspace_sptr grouping;
};

/**
 * Tests for MuonProcess. Note that testing of the calculation (group counts,
 * group asymmetry, pair asymmetry) is covered in the tests of
 * IMuonAsymmetryCalculator,
 * so these tests are for the other parts of MuonProcess.
 */
class MuonProcessTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonProcessTest *createSuite() { return new MuonProcessTest(); }
  static void destroySuite(MuonProcessTest *suite) { delete suite; }

  void test_Init() {
    MuonProcess alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_simpleLoad() {
    ScopedWorkspace output;

    std::vector<int> group1, group2;

    for (int i = 1; i <= 16; ++i)
      group1.emplace_back(i);

    for (int i = 17; i <= 32; ++i)
      group2.emplace_back(i);

    TableWorkspace_sptr grouping = createGroupingTable(group1, group2);

    auto data = loadEMU();

    MuonProcess alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", data->workspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SummedPeriodSet", std::vector<int>{1}));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Mode", "Combined"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorGroupingTable", grouping));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LoadedTimeZero", data->timeZero));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "GroupCounts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupIndex", 0));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", output.name()));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(output.retrieve());

    TS_ASSERT(ws);
    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 2000);

      TS_ASSERT_EQUALS(ws->readY(0)[0], 461);
      TS_ASSERT_EQUALS(ws->readY(0)[1000], 192);
      TS_ASSERT_EQUALS(ws->readY(0)[1752], 5);

      TS_ASSERT_DELTA(ws->readE(0)[0], 21.471, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[1000], 13.856, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[1752], 2.236, 0.001);

      TS_ASSERT_DELTA(ws->readX(0)[0], -0.254, 0.001);
      TS_ASSERT_DELTA(ws->readX(0)[1000], 15.746, 0.001);
      TS_ASSERT_DELTA(ws->readX(0)[1752], 27.778, 0.001);
    }
  }

  void test_Cropping() {
    ScopedWorkspace output;

    std::vector<int> group1, group2;

    for (int i = 1; i <= 16; ++i)
      group1.emplace_back(i);

    for (int i = 17; i <= 32; ++i)
      group2.emplace_back(i);

    TableWorkspace_sptr grouping = createGroupingTable(group1, group2);

    auto data = loadEMU();

    MuonProcess alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", data->workspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SummedPeriodSet", std::vector<int>{1}));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Mode", "Combined"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorGroupingTable", grouping));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LoadedTimeZero", data->timeZero));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "GroupCounts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupIndex", 0));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", output.name()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CropWorkspace", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Xmin", 3.0));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(output.retrieve());

    TS_ASSERT(ws);
    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);

      TS_ASSERT_DELTA(ws->readX(0)[0], 3.0100, 0.001);
      TS_ASSERT_DELTA(ws->readX(0)[1000], 19.0100, 0.001);
      TS_ASSERT_DELTA(ws->readX(0)[1752], 31.0420, 0.001);
    }
  }

  void test_noCropping() {
    ScopedWorkspace output;

    std::vector<int> group1, group2;

    for (int i = 1; i <= 16; ++i)
      group1.emplace_back(i);

    for (int i = 17; i <= 32; ++i)
      group2.emplace_back(i);

    TableWorkspace_sptr grouping = createGroupingTable(group1, group2);

    auto data = loadEMU();

    MuonProcess alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", data->workspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SummedPeriodSet", std::vector<int>{1}));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Mode", "Combined"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorGroupingTable", grouping));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LoadedTimeZero", data->timeZero));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "GroupCounts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupIndex", 0));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", output.name()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CropWorkspace", false));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Xmin", 3.0));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(output.retrieve());

    TS_ASSERT(ws);
    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);

      TS_ASSERT_DELTA(ws->readX(0)[0], 3.010, 0.001);
      TS_ASSERT_DELTA(ws->readX(0)[1000], 19.010, 0.001);
      TS_ASSERT_DELTA(ws->readX(0)[1752], 31.042, 0.001);
    }
  }

  void test_multiPeriod() {
    ScopedWorkspace output;

    std::vector<int> group1, group2;

    for (int i = 33; i <= 64; ++i)
      group1.emplace_back(i);
    for (int i = 1; i <= 32; ++i)
      group2.emplace_back(i);

    TableWorkspace_sptr grouping = createGroupingTable(group1, group2);

    auto data = loadMUSR();

    MuonProcess alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", data->workspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SummedPeriodSet", "1,2"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LoadedTimeZero", data->timeZero));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Mode", "Combined"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorGroupingTable", grouping));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "GroupCounts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupIndex", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", output.name()));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(output.retrieve());

    TS_ASSERT(ws);
    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 2000);

      TS_ASSERT_EQUALS(ws->readY(0)[0], 23);
      TS_ASSERT_EQUALS(ws->readY(0)[1000], 3);
      TS_ASSERT_EQUALS(ws->readY(0)[1701], 1);

      TS_ASSERT_DELTA(ws->readE(0)[0], 4.796, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[1000], 1.732, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[1701], 1.000, 0.001);

      TS_ASSERT_DELTA(ws->readX(0)[0], -0.550, 0.001);
      TS_ASSERT_DELTA(ws->readX(0)[1000], 15.450, 0.001);
      TS_ASSERT_DELTA(ws->readX(0)[1701], 26.666, 0.001);
    }
  }

  void test_binCorrectionParams() {
    ScopedWorkspace output;

    std::vector<int> group1, group2;

    for (int i = 1; i <= 16; ++i)
      group1.emplace_back(i);
    for (int i = 17; i <= 32; ++i)
      group2.emplace_back(i);

    TableWorkspace_sptr grouping = createGroupingTable(group1, group2);

    auto data = loadEMU();

    MuonProcess alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", data->workspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LoadedTimeZero", data->timeZero));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Mode", "Combined"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SummedPeriodSet", std::vector<int>{1}));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorGroupingTable", grouping));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "GroupCounts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupIndex", 0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeZero", 0.5));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Xmin", 0.1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Xmax", 16.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RebinParams", "0.08"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", output.name()));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(output.retrieve());

    TS_ASSERT(ws);
    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 198);

      TS_ASSERT_DELTA(ws->readX(0)[0], 0.102, 0.001);
      TS_ASSERT_DELTA(ws->readX(0)[100], 8.102, 0.001);
      TS_ASSERT_DELTA(ws->readX(0)[198], 15.942, 0.001);

      TS_ASSERT_DELTA(ws->readY(0)[0], 1024372.2, 0.1);
      TS_ASSERT_DELTA(ws->readY(0)[100], 24589.0, 0.1);
      TS_ASSERT_DELTA(ws->readY(0)[197], 730.0, 0.1);

      TS_ASSERT_DELTA(ws->readE(0)[0], 1012.113, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[100], 156.809, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[197], 27.019, 0.001);
    }
  }

  void test_deadTimeCorrection() {
    ScopedWorkspace output;

    std::vector<int> group1, group2;

    for (int i = 1; i <= 16; ++i)
      group1.emplace_back(i);
    for (int i = 17; i <= 32; ++i)
      group2.emplace_back(i);

    TableWorkspace_sptr grouping = createGroupingTable(group1, group2);

    auto deadTimes = std::make_shared<TableWorkspace>();
    deadTimes->addColumn("int", "spectrum");
    deadTimes->addColumn("double", "dead-time");

    for (int i = 0; i < 32; ++i) {
      TableRow newRow = deadTimes->appendRow();
      newRow << (i + 1) << 1.0;
    }

    auto data = loadEMU();

    MuonProcess alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", data->workspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LoadedTimeZero", data->timeZero));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Mode", "Combined"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SummedPeriodSet", std::vector<int>{1}));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorGroupingTable", grouping));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ApplyDeadTimeCorrection", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DeadTimeTable", deadTimes));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "GroupCounts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupIndex", 0));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", output.name()));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(output.retrieve());

    TS_ASSERT(ws);
    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 2000);

      TS_ASSERT_DELTA(ws->readY(0)[0], 463.383, 0.001);
      TS_ASSERT_DELTA(ws->readY(0)[1000], 192.468, 0.001);
      TS_ASSERT_DELTA(ws->readY(0)[1752], 5.00075, 0.00001);

      TS_ASSERT_DELTA(ws->readE(0)[0], 21.471, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[1000], 13.856, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[1752], 2.236, 0.001);

      TS_ASSERT_DELTA(ws->readX(0)[0], -0.254, 0.001);
      TS_ASSERT_DELTA(ws->readX(0)[1000], 15.746, 0.001);
      TS_ASSERT_DELTA(ws->readX(0)[1752], 27.778, 0.001);
    }
  }

  void test_errorReporting_emptyGrouping() {
    ScopedWorkspace output;

    auto emptyGrouping = createGroupingTable(std::vector<int>(), std::vector<int>());

    MuonProcess alg;
    alg.setRethrows(true);

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    // Single-period input
    auto data = loadEMU();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", data->workspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LoadedTimeZero", data->timeZero));

    // Test empty grouping
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SummedPeriodSet", std::vector<int>{1}));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SubtractedPeriodSet", std::vector<int>{}));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorGroupingTable", emptyGrouping));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Mode", "Combined"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "GroupCounts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupIndex", 0));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", output.name()));

    TS_ASSERT_THROWS(alg.execute(), const std::invalid_argument &);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_errorReporting_emptyWS() {
    MuonProcess alg;
    alg.setRethrows(true);
    Workspace_sptr emptyWS;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS(alg.setProperty("InputWorkspace", emptyWS), const std::invalid_argument &);
  }

  void test_errorReporting_badWSType() {
    ScopedWorkspace output;
    std::vector<int> group1, group2;
    for (int i = 1; i <= 16; ++i)
      group1.emplace_back(i);
    for (int i = 17; i <= 32; ++i)
      group2.emplace_back(i);
    TableWorkspace_sptr grouping = createGroupingTable(group1, group2);

    MuonProcess alg;
    alg.setRethrows(true);
    Workspace_sptr badWS = std::make_shared<TableWorkspace>();
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", badWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SummedPeriodSet", std::vector<int>{1}));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorGroupingTable", grouping));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LoadedTimeZero", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Mode", "Combined"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "GroupCounts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupIndex", 0));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", output.name()));

    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_errorReporting_invalidPeriodNumbers() {
    ScopedWorkspace output;
    std::vector<int> group1, group2;
    for (int i = 1; i <= 16; ++i)
      group1.emplace_back(i);
    for (int i = 17; i <= 32; ++i)
      group2.emplace_back(i);
    TableWorkspace_sptr grouping = createGroupingTable(group1, group2);

    MuonProcess alg;
    alg.setRethrows(true);
    auto data = loadEMU();
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", data->workspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SummedPeriodSet", std::vector<int>{1, 9}));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorGroupingTable", grouping));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LoadedTimeZero", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Mode", "Combined"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "GroupCounts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupIndex", 0));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", output.name()));

    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_errorReporting_noPeriodsSpecified() {
    ScopedWorkspace output;
    std::vector<int> group1, group2;
    for (int i = 1; i <= 16; ++i)
      group1.emplace_back(i);
    for (int i = 17; i <= 32; ++i)
      group2.emplace_back(i);
    TableWorkspace_sptr grouping = createGroupingTable(group1, group2);

    MuonProcess alg;
    alg.setRethrows(true);
    auto data = loadEMU();
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", data->workspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorGroupingTable", grouping));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LoadedTimeZero", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Mode", "Combined"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "GroupCounts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupIndex", 0));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", output.name()));

    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_errorReporting_noDeadTimes() {
    ScopedWorkspace output;
    std::vector<int> group1, group2;
    for (int i = 1; i <= 16; ++i)
      group1.emplace_back(i);
    for (int i = 17; i <= 32; ++i)
      group2.emplace_back(i);
    TableWorkspace_sptr grouping = createGroupingTable(group1, group2);

    MuonProcess alg;
    alg.setRethrows(true);
    auto data = loadEMU();
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", data->workspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SummedPeriodSet", std::vector<int>{1}));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorGroupingTable", grouping));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ApplyDeadTimeCorrection", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LoadedTimeZero", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Mode", "Combined"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "GroupCounts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupIndex", 0));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", output.name()));

    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_autoGrouping() {
    ScopedWorkspace output;

    try {
      auto load = loadEMU();
      MuonProcess alg;
      alg.setRethrows(true);
      alg.initialize();
      alg.setProperty("InputWorkspace", load->workspace);
      alg.setProperty("SummedPeriodSet", std::vector<int>{1});
      alg.setProperty("LoadedTimeZero", load->timeZero);
      alg.setProperty("DetectorGroupingTable", load->grouping);
      alg.setProperty("Mode", "Combined");
      alg.setProperty("OutputType", "GroupCounts");
      alg.setProperty("GroupIndex", 0);
      alg.setPropertyValue("OutputWorkspace", output.name());
      alg.execute();
    } catch (std::exception &e) {
      TS_FAIL(e.what());
      return;
    }

    MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(output.retrieve());

    TS_ASSERT(ws);
    if (!ws)
      return; // Nothing to check

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);

    TS_ASSERT_EQUALS(ws->readY(0)[0], 461);
    TS_ASSERT_EQUALS(ws->readY(0)[1000], 192);
    TS_ASSERT_EQUALS(ws->readY(0)[1998], 1);
  }

private:
  TableWorkspace_sptr createGroupingTable(const std::vector<int> &group1, const std::vector<int> &group2) {
    auto t = std::make_shared<TableWorkspace>();

    t->addColumn("vector_int", "Detectors");

    TableRow row1 = t->appendRow();
    row1 << group1;

    TableRow row2 = t->appendRow();
    row2 << group2;

    return t;
  }

  /**
   * Use LoadMuonNexus to load data from file and return it
   * @param filename :: [input] Name of file to load
   * @returns LoadedData struct
   */
  std::unique_ptr<LoadedData> loadData(const std::string &filename) {
    auto data = std::make_unique<LoadedData>();
    Mantid::DataHandling::Load load;
    load.initialize();
    load.setChild(true);
    load.setPropertyValue("Filename", filename);
    load.setPropertyValue("DetectorGroupingTable", "__notused");
    load.execute();
    data->workspace = load.getProperty("OutputWorkspace");
    data->timeZero = load.getProperty("TimeZero");
    data->grouping = load.getProperty("DetectorGroupingTable");
    return data;
  }

  /**
   * Use LoadMuonNexus to load data from EMU file
   * @returns LoadedData struct
   */
  std::unique_ptr<LoadedData> loadEMU() { return loadData("emu00006473.nxs"); }

  /**
   * Use LoadMuonNexus to load data from MUSR file
   * @returns LoadedData struct
   */
  std::unique_ptr<LoadedData> loadMUSR() { return loadData("MUSR00015189.nxs"); }
};
