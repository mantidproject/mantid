// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/AlignAndFocusPowderSlim.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/TimeROI.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/Unit.h"

#include <gtest/gtest.h>
#include <numbers>

using Mantid::API::AlgorithmManager;
using Mantid::API::AnalysisDataService;
using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::Workspace;
using Mantid::API::Workspace_sptr;
using Mantid::API::WorkspaceGroup;
using Mantid::API::WorkspaceGroup_sptr;
using Mantid::DataHandling::AlignAndFocusPowderSlim::AlignAndFocusPowderSlim;
using Mantid::DataObjects::TableWorkspace_sptr;

namespace {
// used in most tests
const std::string VULCAN_218062("VULCAN_218062.nxs.h5");

// place where the disabled tests at the bottom are
// test_exec1GB, test_exec10GB, test_exec18GB
const std::string DATA_LOCATION{"/home/pf9/build/mantid/vulcanperf/"};

/// struct to make it easier to configure the test
struct TestConfig {
  std::vector<double> xmin = {};
  std::vector<double> xmax = {};
  std::vector<double> xdelta = {};
  std::string binning = "Logarithmic";
  std::string binningUnits = "dSpacing";
  double timeMin = -1.;
  double timeMax = -1.;
  TableWorkspace_sptr tablesplitter = nullptr;
  bool relativeTime = false;
  bool filterBadPulses = false;
  std::string logListBlock = "";
  std::string logListAllow = "";
  int outputSpecNum = -10;
};
} // namespace

class AlignAndFocusPowderSlimTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AlignAndFocusPowderSlimTest *createSuite() { return new AlignAndFocusPowderSlimTest(); }
  static void destroySuite(AlignAndFocusPowderSlimTest *suite) { delete suite; }

  void test_Init() {
    AlignAndFocusPowderSlim alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  // run the algorithm do some common checks and return output workspace name
  Workspace_sptr run_algorithm(const std::string &filename, const TestConfig &configuration,
                               const bool should_throw = false) {
    // simplify setting property names
    using namespace Mantid::DataHandling::AlignAndFocusPowderSlim::PropertyNames;

    const std::string wksp_name("VULCAN");

    std::cout << "==================> " << filename << '\n';
    Mantid::Kernel::Timer timer;
    AlignAndFocusPowderSlim alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(FILENAME, filename));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(OUTPUT_WKSP, wksp_name));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(BINMODE, configuration.binning));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(BIN_UNITS, configuration.binningUnits));
    if (!configuration.xmin.empty())
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(X_MIN, configuration.xmin));
    if (!configuration.xmax.empty())
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(X_MAX, configuration.xmax));
    if (!configuration.xdelta.empty())
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(X_DELTA, configuration.xdelta));
    if (!configuration.logListBlock.empty())
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(BLOCK_LOGS, configuration.logListBlock));
    if (!configuration.logListAllow.empty())
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(ALLOW_LOGS, configuration.logListAllow));
    if (configuration.timeMin > 0.)
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(FILTER_TIMESTART, configuration.timeMin));
    if (configuration.timeMax > 0.)
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(FILTER_TIMESTOP, configuration.timeMax));
    if (configuration.tablesplitter != nullptr) {
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(SPLITTER_WS, configuration.tablesplitter));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(SPLITTER_RELATIVE, configuration.relativeTime));
    }
    if (configuration.filterBadPulses) {
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(FILTER_BAD_PULSES, configuration.filterBadPulses));
    }
    if (configuration.outputSpecNum != -10) {
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(OUTPUT_SPEC_NUM, configuration.outputSpecNum));
    }

    if (should_throw) {
      TS_ASSERT_THROWS(alg.execute(), const std::invalid_argument &);
      return Workspace_sptr();
    }

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
    std::cout << "==================> " << timer << '\n';

    Workspace_sptr outputWS = alg.getProperty(OUTPUT_WKSP);
    TS_ASSERT(outputWS);
    return outputWS;
  }

  void test_defaults() {
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(run_algorithm(VULCAN_218062, TestConfig()));

    constexpr size_t NUM_Y{1874}; // observed value

    // verify the output
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 6);
    TS_ASSERT_EQUALS(outputWS->blocksize(), NUM_Y);
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->unitID(), "TOF");
    // default values in algorithm
    TS_ASSERT_DELTA(outputWS->readX(0).front(), 1646., 1);
    TS_ASSERT_DELTA(outputWS->readX(0).back(), 32925., 1);
    // observed values from running
    const auto y_values = outputWS->readY(0);
    TS_ASSERT_EQUALS(y_values.size(), NUM_Y);
    TS_ASSERT_EQUALS(y_values[0], 0.);
    TS_ASSERT_EQUALS(y_values[NUM_Y / 2], 0.);
    TS_ASSERT_EQUALS(y_values[NUM_Y - 1], 4744.);

    // do not need to cleanup because workspace did not go into the ADS

    // The default chunk size will load VULCAN_218062.nxs.h5 in 1 chunk so try loading with ReadSizeFromDisk=1000000
    // which will load the banks in 9 to 27 chunks. The output should be the same as with the default chunk size.
    AlignAndFocusPowderSlim alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", VULCAN_218062));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "unused"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ReadSizeFromDisk", 1000000));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););

    Workspace_sptr outputWS2 = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS2);

    // Run CompareWorkspaces algorithm to verify the output
    auto compareAlg = alg.createChildAlgorithm("CompareWorkspaces");
    compareAlg->setProperty("Workspace1", outputWS);
    compareAlg->setProperty("Workspace2", outputWS2);
    compareAlg->execute();
    bool result = compareAlg->getProperty("Result");
    TS_ASSERT(result);
  }

  void test_common_x() {
    TestConfig configuration({13000.}, {36000.}, {}, "Logarithmic", "TOF");
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(run_algorithm(VULCAN_218062, configuration));

    constexpr size_t NUM_Y{637}; // observed value

    // verify the output
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 6);
    TS_ASSERT_EQUALS(outputWS->blocksize(), NUM_Y);
    TS_ASSERT(outputWS->isCommonBins());
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->unitID(), "TOF");
    // default values in algorithm
    TS_ASSERT_EQUALS(outputWS->readX(0).front(), configuration.xmin[0]);
    TS_ASSERT_EQUALS(outputWS->readX(0).back(), configuration.xmax[0]);
    // observed values from running
    const auto y_values = outputWS->readY(0);
    TS_ASSERT_EQUALS(y_values.size(), NUM_Y);
    TS_ASSERT_EQUALS(y_values[0], 0.);
    TS_ASSERT_EQUALS(y_values[NUM_Y / 2], 55374.); // observed
    TS_ASSERT_EQUALS(y_values[NUM_Y - 1], 0.);

    // do not need to cleanup because workspace did not go into the ADS
  }

  void test_ragged_bins_x_min_max() {
    TestConfig configuration({13000., 14000., 15000., 16000., 17000., 18000.},
                             {36000., 37000., 38000., 39000., 40000., 41000.}, {}, "Logarithmic", "TOF");
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(run_algorithm(VULCAN_218062, configuration));

    // verify the output
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 6);

    // check the x-values
    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      const auto &x_values = outputWS->readX(i);
      TS_ASSERT_EQUALS(x_values.front(), configuration.xmin[i]);
      TS_ASSERT_EQUALS(x_values.back(), configuration.xmax[i]);
    }

    // do not need to cleanup because workspace did not go into the ADS
  }

  void test_ragged_bins_x_delta() {
    TestConfig configuration({13000.}, {36000.}, {1000., 2000., 3000., 4000., 5000., 6000.}, "Linear", "TOF");
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(run_algorithm(VULCAN_218062, configuration));

    // verify the output
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 6);

    // check the x-values
    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      const auto &x_values = outputWS->readX(i);
      TS_ASSERT_EQUALS(x_values.front(), configuration.xmin[0]);
      TS_ASSERT_EQUALS(x_values.back(), configuration.xmax[0]);
      TS_ASSERT_EQUALS(x_values.size(),
                       std::round((configuration.xmax[0] - configuration.xmin[0]) / configuration.xdelta[i] + 1));
    }

    // do not need to cleanup because workspace did not go into the ADS
  }

  void test_different_units() {
    const double l1{43.755};
    const std::vector<double> polars{90, 90, 120, 150, 157, 65.5}; // two-theta
    const std::vector<double> l2s{2.296, 2.296, 2.070, 2.070, 2.070, 2.530};

    // test TOF
    const std::vector<double> tof_mins(6, 13000);
    const std::vector<double> tof_maxs(6, 36000);
    const std::vector<double> tof_deltas(6, (36000 - 13000) / 20.);
    run_test_with_different_units(tof_mins, tof_maxs, tof_deltas, "TOF");

    std::vector<double> dmin;
    std::vector<double> dmax;
    std::vector<double> ddelta;
    std::vector<double> qmin;
    std::vector<double> qmax;
    std::vector<double> qdelta;

    const double deg2rad = std::numbers::pi_v<double> / 180.;
    const double pi2 = 2 * std::numbers::pi_v<double>;

    // setup dSpacing and Q vectors so that we get the same output TOF range of 13000 to 36000 with 20
    // bins
    for (size_t i = 0; i < 6; ++i) {
      const double tofToD = Mantid::Kernel::Units::tofToDSpacingFactor(l1, l2s[i], deg2rad * polars[i], 0.);
      dmin.push_back(tof_mins[i] * tofToD);
      dmax.push_back(tof_maxs[i] * tofToD);
      ddelta.push_back((dmax[i] - dmin[i]) / 20.);
      qmin.push_back(pi2 / dmax[i]);
      qmax.push_back(pi2 / dmin[i]);
      qdelta.push_back((qmax[i] - qmin[i]) / 20.);
    }

    // test dSpacing
    run_test_with_different_units(dmin, dmax, ddelta, "dSpacing");
    // test Q
    run_test_with_different_units(qmin, qmax, qdelta, "MomentumTransfer");
  }

  void run_test_with_different_units(std::vector<double> xmin, std::vector<double> xmax, std::vector<double> xdelta,
                                     std::string units) {
    TestConfig configuration(xmin, xmax, xdelta, "Linear", units);
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(run_algorithm(VULCAN_218062, configuration));

    // verify the output
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 6);
    TS_ASSERT(outputWS->isCommonBins());
    TS_ASSERT_EQUALS(outputWS->blocksize(), 20);
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->unitID(), "TOF");
    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      TS_ASSERT_DELTA(outputWS->readX(i).front(), 13000., 1e-5);
      TS_ASSERT_DELTA(outputWS->readX(i).back(), 36000., 1e-5);
    }
  }

  void test_load_nexus_logs() {
    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF");
    configuration.timeMin = 0.;
    configuration.timeMax = 300.;
    configuration.logListBlock = "skf*";
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(run_algorithm(VULCAN_218062, configuration));

    // verify the output
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 6);
    TS_ASSERT(outputWS->isCommonBins());
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1);
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->unitID(), "TOF");
    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      TS_ASSERT_DELTA(outputWS->readX(i).front(), configuration.xmin[0], 1e-5);
      TS_ASSERT_DELTA(outputWS->readX(i).back(), configuration.xmax[0], 1e-5);
    }
    // check some logs
    TS_ASSERT(outputWS->run().hasProperty("run_number"));
    TS_ASSERT(!outputWS->run().hasProperty("skf2"));
    TS_ASSERT(!outputWS->run().hasProperty("skf3"));
  }

  void test_start_stop_time_filtering() {
    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF");
    configuration.timeMin = 200.;
    configuration.timeMax = 300.;
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(run_algorithm(VULCAN_218062, configuration));

    /* expected results came from running

    ws = LoadEventNexus(VULCAN_218062, FilterByTimeStart=200, FilterByTimeStop=300, NumberOfBins=1)
    grp = CreateGroupingWorkspace(ws, GroupDetectorsBy='bank')
    ws = GroupDetectors(ws, CopyGroupingFromWorkspace="grp")
    print(ws.extractY())
    */

    TS_ASSERT_EQUALS(outputWS->readY(0).front(), 3742475);
    TS_ASSERT_EQUALS(outputWS->readY(1).front(), 3735653);
    TS_ASSERT_EQUALS(outputWS->readY(2).front(), 4295302);
    TS_ASSERT_EQUALS(outputWS->readY(3).front(), 4244796);
    TS_ASSERT_EQUALS(outputWS->readY(4).front(), 1435593);
    TS_ASSERT_EQUALS(outputWS->readY(5).front(), 2734113);

    // check the time ROI
    const auto &run_timeroi = outputWS->run().getTimeROI();
    TS_ASSERT_EQUALS(run_timeroi.numberOfRegions(), 1);
    TS_ASSERT_EQUALS(run_timeroi.timeAtIndex(0), outputWS->run().startTime() + 200.);
    TS_ASSERT_EQUALS(run_timeroi.timeAtIndex(1), outputWS->run().startTime() + 300.);

    // check that the logs are filtered corerctly by checking first and last pulse times
    TS_ASSERT_DELTA(outputWS->run().getFirstPulseTime().totalNanoseconds(),
                    (outputWS->run().startTime() + 200.).totalNanoseconds(), 1e8 /* 0.1 sec */);
    TS_ASSERT_DELTA(outputWS->run().getLastPulseTime().totalNanoseconds(),
                    (outputWS->run().startTime() + 300.).totalNanoseconds(), 1e8 /* 0.1 sec */);
  }

  void test_start_time_filtering() {
    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF");
    configuration.timeMin = 200.;
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(run_algorithm(VULCAN_218062, configuration));

    /* expected results came from running

    ws = LoadEventNexus(VULCAN_218062, FilterByTimeStart=200, NumberOfBins=1)
    grp = CreateGroupingWorkspace(ws, GroupDetectorsBy='bank')
    ws = GroupDetectors(ws, CopyGroupingFromWorkspace="grp")
    print(ws.extractY())
    */

    TS_ASSERT_EQUALS(outputWS->readY(0).front(), 16370014);
    TS_ASSERT_EQUALS(outputWS->readY(1).front(), 16353116);
    TS_ASSERT_EQUALS(outputWS->readY(2).front(), 18782610);
    TS_ASSERT_EQUALS(outputWS->readY(3).front(), 18572804);
    TS_ASSERT_EQUALS(outputWS->readY(4).front(), 6275399);
    TS_ASSERT_EQUALS(outputWS->readY(5).front(), 11972050);
  }

  void test_stop_time_filtering() {
    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF");
    configuration.timeMax = 300.;
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(run_algorithm(VULCAN_218062, configuration));

    /* expected results came from running

    ws = LoadEventNexus(VULCAN_218062, FilterByTimeStop=300, NumberOfBins=1)
    grp = CreateGroupingWorkspace(ws, GroupDetectorsBy='bank')
    ws = GroupDetectors(ws, CopyGroupingFromWorkspace="grp")
    print(ws.extractY())
    */

    TS_ASSERT_EQUALS(outputWS->readY(0).front(), 10348627);
    TS_ASSERT_EQUALS(outputWS->readY(1).front(), 10328566);
    TS_ASSERT_EQUALS(outputWS->readY(2).front(), 11877182);
    TS_ASSERT_EQUALS(outputWS->readY(3).front(), 11734382);
    TS_ASSERT_EQUALS(outputWS->readY(4).front(), 3969153);
    TS_ASSERT_EQUALS(outputWS->readY(5).front(), 7567195);
  }

  void test_all_time_filtering() {
    // run is only ~600 seconds long so this include all events
    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF");
    configuration.timeMax = 3000.;
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(run_algorithm(VULCAN_218062, configuration));

    /* expected results came from running

    ws = LoadEventNexus(VULCAN_218062, NumberOfBins=1)
    grp = CreateGroupingWorkspace(ws, GroupDetectorsBy='bank')
    ws = GroupDetectors(ws, CopyGroupingFromWorkspace="grp")
    print(ws.extractY())
    */

    TS_ASSERT_EQUALS(outputWS->readY(0).front(), 22976166);
    TS_ASSERT_EQUALS(outputWS->readY(1).front(), 22946029);
    TS_ASSERT_EQUALS(outputWS->readY(2).front(), 26364490);
    TS_ASSERT_EQUALS(outputWS->readY(3).front(), 26062390);
    TS_ASSERT_EQUALS(outputWS->readY(4).front(), 8808959);
    TS_ASSERT_EQUALS(outputWS->readY(5).front(), 16805132);
  }

  void test_invalid_time_filtering() {
    // start time > stop time
    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF");
    configuration.timeMin = 300.;
    configuration.timeMax = 200.;
    run_algorithm(VULCAN_218062, configuration, true);
    // start time longer than run time of ~600 seconds
    configuration.timeMin = 1000.;
    configuration.timeMax = 2000.;
    run_algorithm(VULCAN_218062, configuration, true);
  }

  void test_splitter_table() {
    Mantid::DataObjects::TableWorkspace_sptr tablesplitter = create_splitter_table();
    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF");
    configuration.relativeTime = true;
    configuration.tablesplitter = create_splitter_table(configuration.relativeTime);
    auto outputWS = std::dynamic_pointer_cast<WorkspaceGroup>(run_algorithm(VULCAN_218062, configuration));

    /* expected results came from running

    splitter = CreateEmptyTableWorkspace()
    splitter.addColumn('float', 'start')
    splitter.addColumn('float', 'stop')
    splitter.addColumn('str', 'target')
    splitter.addRow((10,20, '0'))
    splitter.addRow((200,210, '0'))
    splitter.addRow((400,410, '0'))

    ws = LoadEventNexus(VULCAN_218062, NumberOfBins=1)
    grp = CreateGroupingWorkspace(ws, GroupDetectorsBy='bank')
    ws = GroupDetectors(ws, CopyGroupingFromWorkspace="grp")

    FilterEvents(ws, SplitterWorkspace=splitter, RelativeTime=True, FilterByPulseTime=True,
    OutputWorkspaceBaseName="filtered")

    print(mtd["filtered_0"].extractY())
    */

    auto outputWS0 = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(0));
    TS_ASSERT_EQUALS(outputWS0->readY(0).front(), 807206);
    TS_ASSERT_EQUALS(outputWS0->readY(1).front(), 805367);
    TS_ASSERT_EQUALS(outputWS0->readY(2).front(), 920983);
    TS_ASSERT_EQUALS(outputWS0->readY(3).front(), 909955);
    TS_ASSERT_EQUALS(outputWS0->readY(4).front(), 310676);
    TS_ASSERT_EQUALS(outputWS0->readY(5).front(), 590230);
  }

  void test_splitter_table_absolute_time() {
    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF");
    configuration.relativeTime = false;
    configuration.tablesplitter = create_splitter_table(configuration.relativeTime);
    auto outputWS = std::dynamic_pointer_cast<WorkspaceGroup>(run_algorithm(VULCAN_218062, configuration));

    /* expected results should be the same as test_splitter_table but produced with absolute time */

    auto outputWS0 = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(0));
    TS_ASSERT_EQUALS(outputWS0->readY(0).front(), 807206);
    TS_ASSERT_EQUALS(outputWS0->readY(1).front(), 805367);
    TS_ASSERT_EQUALS(outputWS0->readY(2).front(), 920983);
    TS_ASSERT_EQUALS(outputWS0->readY(3).front(), 909955);
    TS_ASSERT_EQUALS(outputWS0->readY(4).front(), 310676);
    TS_ASSERT_EQUALS(outputWS0->readY(5).front(), 590230);
  }

  void test_splitter_table_multiple_targets() {
    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF");
    configuration.relativeTime = true;
    configuration.tablesplitter = create_splitter_table(configuration.relativeTime, false);
    auto outputWS = std::dynamic_pointer_cast<WorkspaceGroup>(run_algorithm(VULCAN_218062, configuration));

    /* expected results came from running

    ws = LoadEventNexus(VULCAN_218062, NumberOfBins=1, FilterByTimeStart=10, FilterByTimeStop=20)
    grp = CreateGroupingWorkspace(ws, GroupDetectorsBy='bank')
    ws = GroupDetectors(ws, CopyGroupingFromWorkspace="grp")
    print(ws.extractY())
    */

    auto outputWS0 = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(0));

    TS_ASSERT_EQUALS(outputWS0->readY(0).front(), 59561);
    TS_ASSERT_EQUALS(outputWS0->readY(1).front(), 59358);
    TS_ASSERT_EQUALS(outputWS0->readY(2).front(), 63952);
    TS_ASSERT_EQUALS(outputWS0->readY(3).front(), 63299);
    TS_ASSERT_EQUALS(outputWS0->readY(4).front(), 22917);
    TS_ASSERT_EQUALS(outputWS0->readY(5).front(), 43843);

    /* expected results came from running

    ws = LoadEventNexus(VULCAN_218062, NumberOfBins=1, FilterByTimeStart=200, FilterByTimeStop=210)
    grp = CreateGroupingWorkspace(ws, GroupDetectorsBy='bank')
    ws = GroupDetectors(ws, CopyGroupingFromWorkspace="grp")
    print(ws.extractY())
    */

    auto outputWS1 = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(1));

    TS_ASSERT_EQUALS(outputWS1->readY(0).front(), 373262);
    TS_ASSERT_EQUALS(outputWS1->readY(1).front(), 372186);
    TS_ASSERT_EQUALS(outputWS1->readY(2).front(), 428220);
    TS_ASSERT_EQUALS(outputWS1->readY(3).front(), 423472);
    TS_ASSERT_EQUALS(outputWS1->readY(4).front(), 143703);
    TS_ASSERT_EQUALS(outputWS1->readY(5).front(), 273072);

    /* expected results came from running

    ws = LoadEventNexus(VULCAN_218062, NumberOfBins=1, FilterByTimeStart=400, FilterByTimeStop=410)
    grp = CreateGroupingWorkspace(ws, GroupDetectorsBy='bank')
    ws = GroupDetectors(ws, CopyGroupingFromWorkspace="grp")
    print(ws.extractY())
    */

    auto outputWS2 = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(2));
    TS_ASSERT_EQUALS(outputWS2->readY(0).front(), 374383);
    TS_ASSERT_EQUALS(outputWS2->readY(1).front(), 373823);
    TS_ASSERT_EQUALS(outputWS2->readY(2).front(), 428811);
    TS_ASSERT_EQUALS(outputWS2->readY(3).front(), 423184);
    TS_ASSERT_EQUALS(outputWS2->readY(4).front(), 144056);
    TS_ASSERT_EQUALS(outputWS2->readY(5).front(), 273315);
  }

  void test_splitter_table_and_time_start_stop() {
    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF");
    configuration.timeMin = 15;
    configuration.timeMax = 300;
    configuration.relativeTime = true;
    configuration.tablesplitter = create_splitter_table(configuration.relativeTime);
    auto outputWS = std::dynamic_pointer_cast<WorkspaceGroup>(run_algorithm(VULCAN_218062, configuration));

    /* expected results came from running

    splitter = CreateEmptyTableWorkspace()
    splitter.addColumn('float', 'start')
    splitter.addColumn('float', 'stop')
    splitter.addColumn('str', 'target')
    splitter.addRow((15,20, '0'))
    splitter.addRow((200,210, '0'))

    ws = LoadEventNexus(VULCAN_218062, NumberOfBins=1)
    grp = CreateGroupingWorkspace(ws, GroupDetectorsBy='bank')
    ws = GroupDetectors(ws, CopyGroupingFromWorkspace="grp")

    FilterEvents(ws, SplitterWorkspace=splitter, RelativeTime=True, FilterByPulseTime=True,
    OutputWorkspaceBaseName="filtered")

    print(mtd["filtered_0"].extractY())
    */

    auto outputWS0 = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(0));
    TS_ASSERT_EQUALS(outputWS0->readY(0).front(), 415525);
    TS_ASSERT_EQUALS(outputWS0->readY(1).front(), 414435);
    TS_ASSERT_EQUALS(outputWS0->readY(2).front(), 476903);
    TS_ASSERT_EQUALS(outputWS0->readY(3).front(), 471846);
    TS_ASSERT_EQUALS(outputWS0->readY(4).front(), 160000);
    TS_ASSERT_EQUALS(outputWS0->readY(5).front(), 304167);
  }

  TableWorkspace_sptr create_splitter_table(const bool relativeTime = true, const bool sameTarget = true) {
    // create splitter table
    TableWorkspace_sptr tablesplitter = std::make_shared<Mantid::DataObjects::TableWorkspace>();
    tablesplitter->addColumn("double", "start");
    tablesplitter->addColumn("double", "stop");
    tablesplitter->addColumn("str", "target");

    // start time was 2022-05-31T02:57:22.028123667 which is 1022813842.0281236 seconds since epoch
    const double offset = relativeTime ? 0. : 1022813842.0281236;

    tablesplitter->appendRow();
    tablesplitter->cell<double>(0, 0) = 10.0 + offset;
    tablesplitter->cell<double>(0, 1) = 20.0 + offset;
    tablesplitter->cell<std::string>(0, 2) = "0";

    tablesplitter->appendRow();
    tablesplitter->cell<double>(1, 0) = 200.0 + offset;
    tablesplitter->cell<double>(1, 1) = 210.0 + offset;
    tablesplitter->cell<std::string>(1, 2) = sameTarget ? "0" : "1";

    tablesplitter->appendRow();
    tablesplitter->cell<double>(2, 0) = 400.0 + offset;
    tablesplitter->cell<double>(2, 1) = 410.0 + offset;
    tablesplitter->cell<std::string>(2, 2) = sameTarget ? "0" : "2";
    return tablesplitter;
  }

  void test_splitter_from_GenerateEventsFilter() {
    // load only the CaveTemperature log from the nexus file
    auto load = AlgorithmManager::Instance().createUnmanaged("LoadEventNexus");
    load->initialize();
    load->setProperty("Filename", "VULCAN_218062.nxs.h5");
    load->setProperty("MetaDataOnly", true);
    load->setProperty("AllowList", std::vector<std::string>{"CaveTemperature"});
    load->setProperty("OutputWorkspace", "logs");
    load->execute();

    // GenereateEventsFilter should create 3 different output targets
    auto gen = AlgorithmManager::Instance().createUnmanaged("GenerateEventsFilter");
    gen->initialize();
    gen->setProperty("InputWorkspace", "logs");
    gen->setProperty("LogName", "CaveTemperature");
    gen->setProperty("MinimumLogValue", 70.1);
    gen->setProperty("MaximumLogValue", 70.15);
    gen->setProperty("LogValueInterval", 0.025);
    gen->setProperty("OutputWorkspace", "splitter");
    gen->setProperty("InformationWorkspace", "info");
    gen->execute();

    auto tablesplitter = std::dynamic_pointer_cast<Mantid::DataObjects::SplittersWorkspace>(
        AnalysisDataService::Instance().retrieve("splitter"));

    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF");
    configuration.relativeTime = true;
    configuration.tablesplitter = tablesplitter;
    auto outputWS = std::dynamic_pointer_cast<WorkspaceGroup>(run_algorithm(VULCAN_218062, configuration));

    /* expected results came from running

    ws = LoadEventNexus("VULCAN_218062.nxs.h5")
    GenerateEventsFilter(ws,
                         OutputWorkspace='splitter',
                         InformationWorkspace='info',
                         LogName='CaveTemperature',
                         LogValueInterval=0.025,
                         MinimumLogValue=70.10,
                         MaximumLogValue=70.15)
    grp = CreateGroupingWorkspace(ws, GroupDetectorsBy='bank')
    ws = GroupDetectors(ws, CopyGroupingFromWorkspace="grp")
    FilterEvents(ws, SplitterWorkspace='splitter',
                 InformationWorkspace='info',
                 OutputWorkspaceBaseName="eventFiltered",
                 FilterByPulseTime=True,
                 GroupWorkspaces=True)
    Rebin("eventFiltered", "0,50000,50000", PreserveEvents=False, OutputWorkspace="eventFiltered")
    for ws in mtd['eventFiltered']:
        print(str(ws), ws.extractY())
    */

    auto outputWS0 = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(0));
    TS_ASSERT_EQUALS(outputWS0->readY(0).front(), 2729042);
    TS_ASSERT_EQUALS(outputWS0->readY(1).front(), 2726901);
    TS_ASSERT_EQUALS(outputWS0->readY(2).front(), 3133867);
    TS_ASSERT_EQUALS(outputWS0->readY(3).front(), 3098887);
    TS_ASSERT_EQUALS(outputWS0->readY(4).front(), 1045181);
    TS_ASSERT_EQUALS(outputWS0->readY(5).front(), 1997189);

    auto outputWS1 = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(1));
    TS_ASSERT_EQUALS(outputWS1->readY(0).front(), 2567255);
    TS_ASSERT_EQUALS(outputWS1->readY(1).front(), 2566070);
    TS_ASSERT_EQUALS(outputWS1->readY(2).front(), 2947152);
    TS_ASSERT_EQUALS(outputWS1->readY(3).front(), 2913240);
    TS_ASSERT_EQUALS(outputWS1->readY(4).front(), 983897);
    TS_ASSERT_EQUALS(outputWS1->readY(5).front(), 1877851);

    auto outputWS2 = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(2));
    TS_ASSERT_EQUALS(outputWS2->readY(0).front(), 1346290);
    TS_ASSERT_EQUALS(outputWS2->readY(1).front(), 1343588);
    TS_ASSERT_EQUALS(outputWS2->readY(2).front(), 1541892);
    TS_ASSERT_EQUALS(outputWS2->readY(3).front(), 1526538);
    TS_ASSERT_EQUALS(outputWS2->readY(4).front(), 516351);
    TS_ASSERT_EQUALS(outputWS2->readY(5).front(), 984359);
  }

  void test_filter_bad_pulses() {
    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF");
    configuration.filterBadPulses = true;
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(run_algorithm(VULCAN_218062, configuration));

    /* expected results came from running

    ws = LoadEventNexus(VULCAN_218062, NumberOfBins=1)
    grp = CreateGroupingWorkspace(ws, GroupDetectorsBy='bank')
    ws = GroupDetectors(ws, CopyGroupingFromWorkspace="grp")
    ws = FilterBadPulses(ws)
    print(ws.extractY())
    */

    TS_ASSERT_EQUALS(outputWS->readY(0).front(), 22668454);
    TS_ASSERT_EQUALS(outputWS->readY(1).front(), 22639565);
    TS_ASSERT_EQUALS(outputWS->readY(2).front(), 26014789);
    TS_ASSERT_EQUALS(outputWS->readY(3).front(), 25716703);
    TS_ASSERT_EQUALS(outputWS->readY(4).front(), 8690549);
    TS_ASSERT_EQUALS(outputWS->readY(5).front(), 16577786);
  }

  void test_filter_bad_pulses_and_time_start_stop() {
    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF");
    configuration.filterBadPulses = true;
    configuration.timeMin = 200.;
    configuration.timeMax = 300.;
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(run_algorithm(VULCAN_218062, configuration));

    /* expected results came from running

    ws = LoadEventNexus(VULCAN_218062, NumberOfBins=1, FilterByTimeStart=200, FilterByTimeStop=300)
    grp = CreateGroupingWorkspace(ws, GroupDetectorsBy='bank')
    ws = GroupDetectors(ws, CopyGroupingFromWorkspace="grp")
    ws = FilterBadPulses(ws)
    print(ws.extractY())
    */

    // values should be slightly smaller than in test_start_stop_time_filtering
    TS_ASSERT_EQUALS(outputWS->readY(0).front(), 3736146);
    TS_ASSERT_EQUALS(outputWS->readY(1).front(), 3729398);
    TS_ASSERT_EQUALS(outputWS->readY(2).front(), 4288311);
    TS_ASSERT_EQUALS(outputWS->readY(3).front(), 4237608);
    TS_ASSERT_EQUALS(outputWS->readY(4).front(), 1433200);
    TS_ASSERT_EQUALS(outputWS->readY(5).front(), 2729481);
  }

  void test_output_specnum_validation() {
    using namespace Mantid::DataHandling::AlignAndFocusPowderSlim::PropertyNames;
    AlignAndFocusPowderSlim alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS(alg.setProperty(OUTPUT_SPEC_NUM, -1), std::invalid_argument const &);
    TS_ASSERT_THROWS(alg.setProperty(OUTPUT_SPEC_NUM, 0), std::invalid_argument const &);
    for (int i = 1; i <= 6; i++) {
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(OUTPUT_SPEC_NUM, i));
    }
  }

  void test_output_specnum() {
    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF"); // bins set for single bin
    constexpr int NUM_HIST{6};
    for (int i = 1; i <= NUM_HIST; i++) {
      configuration.outputSpecNum = i;
      auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(run_algorithm(VULCAN_218062, configuration));

      // verify the output -- all spectra exist
      TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), NUM_HIST);
      for (int j = 0; j < NUM_HIST; j++) {
        // check all spectra have bins
        TS_ASSERT_EQUALS(outputWS->readX(j).front(), 0.);
        TS_ASSERT_EQUALS(outputWS->readX(j).back(), 50000.);
        if (j == i - 1) {
          // the indicated spectra has values
          const auto y_values = outputWS->y(j);
          TS_ASSERT_EQUALS(y_values.size(), 1);
          TS_ASSERT_DIFFERS(y_values.front(), 0); // NONZERO
        } else {
          // non-specified spectra should have all-zero values
          const auto y_values = outputWS->y(j);
          TS_ASSERT_EQUALS(y_values.size(), 1);
          TS_ASSERT_EQUALS(y_values.front(), 0); // ZERO
        }
      }
    }
  }

  // ==================================
  // TODO things below this point are for benchmarking and will be removed later
  // ==================================

  void run_test(const std::string &filename) {
    Workspace_sptr outputWS = run_algorithm(filename, TestConfig());
    auto WS = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS);

    // LoadEventNexus 4 seconds
    // tof 6463->39950

    // verify the output
    TS_ASSERT_EQUALS(WS->getNumberHistograms(), 6);
    TS_ASSERT_EQUALS(WS->blocksize(), 3349); // observed value

    // do not need to cleanup because workspace did not go into the ADS
  }

  void xtest_exec1GB() { run_test(DATA_LOCATION + "VULCAN_218075.nxs.h5"); }

  void xtest_exec10GB() { run_test(DATA_LOCATION + "VULCAN_218092.nxs.h5"); }

  void xtest_exec18GB() { run_test(DATA_LOCATION + "VULCAN_217967.nxs.h5"); }
};
