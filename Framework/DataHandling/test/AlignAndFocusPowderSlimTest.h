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
#include "MantidDataObjects/GroupingWorkspace.h"
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
using Mantid::DataObjects::GroupingWorkspace;
using Mantid::DataObjects::GroupingWorkspace_sptr;
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
  Workspace_sptr splitterWS = nullptr;
  bool relativeTime = false;
  bool filterBadPulses = false;
  GroupingWorkspace_sptr groupingWS = nullptr;
  std::string logListBlock = "";
  std::string logListAllow = "";
  int outputSpecNum = -10;
  bool processBankSplitTask = false;
  bool useFullTime = false;
  bool correctToSample = false;
  // focus positions
  double l1 = 43.755;
  std::vector<double> l2s = {2.296, 2.296, 2.070, 2.070, 2.070, 2.530};
  std::vector<double> twoTheta = {90, 90, 120, 150, 157, 65.5};
  std::vector<double> phi = {180, 0, 0, 0, 0, 0};
};
} // namespace

class AlignAndFocusPowderSlimTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AlignAndFocusPowderSlimTest *createSuite() { return new AlignAndFocusPowderSlimTest(); }
  static void destroySuite(AlignAndFocusPowderSlimTest *suite) { delete suite; }

  AlignAndFocusPowderSlimTest() {
    // CreateGroupingWorkspace(InstrumentName="VULCAN", GroupDetectorsBy="bank", OutputWorkspace="groups")
    auto gen = AlgorithmManager::Instance().createUnmanaged("CreateGroupingWorkspace");
    gen->initialize();
    gen->setProperty("InstrumentName", "VULCAN");
    gen->setProperty("GroupDetectorsBy", "bank");
    gen->setProperty("OutputWorkspace", "bank_groups");
    gen->execute();
    bank_grouping_ws =
        std::dynamic_pointer_cast<GroupingWorkspace>(AnalysisDataService::Instance().retrieve("bank_groups"));
  }

  ~AlignAndFocusPowderSlimTest() { AnalysisDataService::Instance().remove("bank_groups"); }

  void test_Init() {
    AlignAndFocusPowderSlim alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_focus_position_validators() {
    TestConfig defaults;
    AlignAndFocusPowderSlim alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    // l1 is mandatory and must be nonnegative
    TS_ASSERT_THROWS_ANYTHING(alg.setPropertyValue("L1", ""));
    TS_ASSERT_THROWS_ANYTHING(alg.setPropertyValue("L1", "-1."));
    // l2s is mandatory and must be nonnegative
    TS_ASSERT_THROWS_ANYTHING(alg.setPropertyValue("L2", ""));
    TS_ASSERT_THROWS_ANYTHING(alg.setPropertyValue("L2", "1., -1."));
    // twoTheta is mandatory and must be nonnegative
    TS_ASSERT_THROWS_ANYTHING(alg.setProperty("Polar", ""));
    TS_ASSERT_THROWS_ANYTHING(alg.setProperty("Polar", "1., -1."));
    // phi is optional, but if specified must be nonnegative
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Azimuthal", ""));
    TS_ASSERT_THROWS_ANYTHING(alg.setProperty("Azimuthal", "1., -1."));

    // set everything to valid value to move on
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", VULCAN_218062));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "outws"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("L1", defaults.l1));

    // // l2 and twoTheta must have the same length
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("L2", "1., 2."));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Polar", "1., 2., 3."));
    TS_ASSERT_THROWS_ASSERT(alg.execute(), std::runtime_error const &e,
                            TS_ASSERT(strstr(e.what(), "Polar has inconsistent length 3")));

    // // if phi is given, must have same length as l2and twoTheta
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("L2", "1., 2."));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Polar", "1., 2."));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Azimuthal", "1., 2., 3."));
    TS_ASSERT_THROWS_ASSERT(alg.execute(), std::runtime_error const &e,
                            TS_ASSERT(strstr(e.what(), "Azimuthal has inconsistent length 3")));
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
    if (configuration.splitterWS != nullptr) {
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(SPLITTER_WS, configuration.splitterWS));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(SPLITTER_RELATIVE, configuration.relativeTime));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(PROCESS_BANK_SPLIT_TASK, configuration.processBankSplitTask));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(FULL_TIME, configuration.useFullTime));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(CORRECTION_TO_SAMPLE, configuration.correctToSample));
    }
    if (configuration.filterBadPulses) {
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(FILTER_BAD_PULSES, configuration.filterBadPulses));
    }
    if (configuration.groupingWS != nullptr) {
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(GROUPING_WS, configuration.groupingWS));
    }
    if (configuration.outputSpecNum != -10) {
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(OUTPUT_SPEC_NUM, configuration.outputSpecNum));
    }
    // set focus positions
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(L1, configuration.l1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(L2, configuration.l2s));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(POLARS, configuration.twoTheta));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(AZIMUTHALS, configuration.phi));

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
    TestConfig config;
    config.groupingWS = bank_grouping_ws;
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(run_algorithm(VULCAN_218062, config));

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
    const auto e_values = outputWS->readE(0);
    TS_ASSERT_DELTA(e_values[0], 0., 1e-10);
    TS_ASSERT_DELTA(e_values[NUM_Y / 2], 0., 1e-10);
    TS_ASSERT_DELTA(e_values[NUM_Y - 1], std::sqrt(4744.), 1e-10);

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
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupingWorkspace", bank_grouping_ws));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("L1", config.l1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("L2", config.l2s));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Polar", config.twoTheta));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Azimuthal", config.phi));
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

  void test_no_grouping() {
    // this should result in 1 spectrum in the output when no grouping is given
    TestConfig config;
    config.l2s = {2.296};
    config.twoTheta = {90};
    config.phi = {0};
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(run_algorithm(VULCAN_218062, config));

    constexpr size_t NUM_Y{1874}; // observed value

    // verify the output
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1);
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
    TS_ASSERT_EQUALS(y_values[NUM_Y - 1], 34622.); // expect larger value then before since all counts go to 1 spectrum
    const auto e_values = outputWS->readE(0);
    TS_ASSERT_DELTA(e_values[0], 0., 1e-10);
    TS_ASSERT_DELTA(e_values[NUM_Y / 2], 0., 1e-10);
    TS_ASSERT_DELTA(e_values[NUM_Y - 1], std::sqrt(34622.), 1e-10);
  }

  void test_common_x() {
    TestConfig configuration({13000.}, {36000.}, {}, "Logarithmic", "TOF");
    configuration.groupingWS = bank_grouping_ws;
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
    const auto e_values = outputWS->readE(0);
    TS_ASSERT_DELTA(e_values[0], 0., 1e-10);
    TS_ASSERT_DELTA(e_values[NUM_Y / 2], std::sqrt(55374.), 1e-10);
    TS_ASSERT_DELTA(e_values[NUM_Y - 1], 0., 1e-10);

    // do not need to cleanup because workspace did not go into the ADS
  }

  void test_ragged_bins_x_min_max() {
    TestConfig configuration({13000., 14000., 15000., 16000., 17000., 18000.},
                             {36000., 37000., 38000., 39000., 40000., 41000.}, {}, "Logarithmic", "TOF");
    configuration.groupingWS = bank_grouping_ws;
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
    configuration.groupingWS = bank_grouping_ws;
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
    configuration.groupingWS = bank_grouping_ws;
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
    configuration.groupingWS = bank_grouping_ws;
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
    configuration.groupingWS = bank_grouping_ws;
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
    configuration.groupingWS = bank_grouping_ws;
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
    configuration.groupingWS = bank_grouping_ws;
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
    configuration.groupingWS = bank_grouping_ws;
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
    configuration.groupingWS = bank_grouping_ws;
    configuration.timeMin = 300.;
    configuration.timeMax = 200.;
    run_algorithm(VULCAN_218062, configuration, true);
    // start time longer than run time of ~600 seconds
    configuration.timeMin = 1000.;
    configuration.timeMax = 2000.;
    run_algorithm(VULCAN_218062, configuration, true);
  }

  void test_splitter_table() {
    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF");
    configuration.groupingWS = bank_grouping_ws;
    configuration.relativeTime = true;
    configuration.splitterWS = create_splitter_table(configuration.relativeTime);
    for (bool processBankSplitTask : {false, true}) {
      configuration.processBankSplitTask = processBankSplitTask;
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
      TS_ASSERT_DELTA(outputWS0->readE(0).front(), std::sqrt(807206), 1e-10);
      TS_ASSERT_DELTA(outputWS0->readE(1).front(), std::sqrt(805367), 1e-10);
      TS_ASSERT_DELTA(outputWS0->readE(2).front(), std::sqrt(920983), 1e-10);
      TS_ASSERT_DELTA(outputWS0->readE(3).front(), std::sqrt(909955), 1e-10);
      TS_ASSERT_DELTA(outputWS0->readE(4).front(), std::sqrt(310676), 1e-10);
      TS_ASSERT_DELTA(outputWS0->readE(5).front(), std::sqrt(590230), 1e-10);
    }
  }

  void test_splitter_table_absolute_time() {
    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF");
    configuration.groupingWS = bank_grouping_ws;
    configuration.relativeTime = false;
    configuration.splitterWS = create_splitter_table(configuration.relativeTime);
    for (bool processBankSplitTask : {false, true}) {
      configuration.processBankSplitTask = processBankSplitTask;
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
  }

  void test_splitter_table_multiple_targets() {
    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF");
    configuration.groupingWS = bank_grouping_ws;
    configuration.relativeTime = true;
    configuration.splitterWS = create_splitter_table(configuration.relativeTime, false);
    for (bool processBankSplitTask : {false, true}) {
      configuration.processBankSplitTask = processBankSplitTask;
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
      TS_ASSERT_DELTA(outputWS0->readE(0).front(), std::sqrt(59561), 1e-10);
      TS_ASSERT_DELTA(outputWS0->readE(1).front(), std::sqrt(59358), 1e-10);
      TS_ASSERT_DELTA(outputWS0->readE(2).front(), std::sqrt(63952), 1e-10);
      TS_ASSERT_DELTA(outputWS0->readE(3).front(), std::sqrt(63299), 1e-10);
      TS_ASSERT_DELTA(outputWS0->readE(4).front(), std::sqrt(22917), 1e-10);
      TS_ASSERT_DELTA(outputWS0->readE(5).front(), std::sqrt(43843), 1e-10);

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
  }

  void test_splitter_table_and_time_start_stop() {
    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF");
    configuration.groupingWS = bank_grouping_ws;
    configuration.timeMin = 15;
    configuration.timeMax = 300;
    configuration.relativeTime = true;
    configuration.splitterWS = create_splitter_table(configuration.relativeTime);
    for (bool processBankSplitTask : {false, true}) {
      configuration.processBankSplitTask = processBankSplitTask;
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

    auto splitterWS = std::dynamic_pointer_cast<Mantid::DataObjects::SplittersWorkspace>(
        AnalysisDataService::Instance().retrieve("splitter"));

    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF");
    configuration.groupingWS = bank_grouping_ws;
    configuration.relativeTime = true;
    configuration.splitterWS = splitterWS;

    for (bool processBankSplitTask : {false, true}) {
      configuration.processBankSplitTask = processBankSplitTask;
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
  }

  void test_split_full_time() {
    // create splitter with sub pulsetime ranges plus one that covers multiple pulses
    auto createSplitter = AlgorithmManager::Instance().createUnmanaged("CreateWorkspace");
    createSplitter->initialize();
    createSplitter->setProperty("DataX",
                                std::vector<double>{0.2, 0.202, 0.204, 0.206, 0.208, 0.21, 0.212, 0.55, 1.001, 1.002});
    createSplitter->setProperty("DataY", std::vector<double>{0, 1, 2, 0, 1, 2, 3, -1, 4});
    createSplitter->setProperty("NSpec", 1);
    createSplitter->setPropertyValue("OutputWorkspace", "split_matrix_ws");
    createSplitter->execute();

    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF");
    configuration.groupingWS = bank_grouping_ws;
    configuration.useFullTime = true;
    configuration.relativeTime = true;
    configuration.splitterWS =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("split_matrix_ws"));
    auto outputWS = std::dynamic_pointer_cast<WorkspaceGroup>(run_algorithm(VULCAN_218062, configuration));

    /* expected results came from running

    ws = LoadEventNexus("VULCAN_218062.nxs.h5", NumberOfBins=1)
    grp = CreateGroupingWorkspace(ws, GroupDetectorsBy='bank')
    ws = GroupDetectors(ws, CopyGroupingFromWorkspace="grp")

    times = [0.2, 0.202, 0.204, 0.206, 0.208, 0.21, 0.212, 0.55, 1.001, 1.002]
    targets = [0, 1, 2, 0, 1, 2, 3, -1, 4]
    split_matrix_ws = CreateWorkspace(DataX=times, DataY=targets, NSpec=1)

    FilterEvents(ws, SplitterWorkspace='split_matrix_ws', OutputWorkspaceBaseName="eventFiltered", GroupWorkspaces=True,
    FilterByPulseTime=False, RelativeTime=True)
    Rebin("eventFiltered", "0,50000,50000", PreserveEvents=False,
    OutputWorkspace="eventFiltered")
    for ws in mtd['eventFiltered']:
        print(str(ws), ws.extractY())
    */
    {
      auto outputWS0 = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(0));
      TS_ASSERT_EQUALS(outputWS0->readY(0).front(), 214);
      TS_ASSERT_EQUALS(outputWS0->readY(1).front(), 219);
      TS_ASSERT_EQUALS(outputWS0->readY(2).front(), 269);
      TS_ASSERT_EQUALS(outputWS0->readY(3).front(), 228);
      TS_ASSERT_EQUALS(outputWS0->readY(4).front(), 71);
      TS_ASSERT_EQUALS(outputWS0->readY(5).front(), 144);
      TS_ASSERT_DELTA(outputWS0->readE(0).front(), std::sqrt(214), 1e-10);
      TS_ASSERT_DELTA(outputWS0->readE(1).front(), std::sqrt(219), 1e-10);
      TS_ASSERT_DELTA(outputWS0->readE(2).front(), std::sqrt(269), 1e-10);
      TS_ASSERT_DELTA(outputWS0->readE(3).front(), std::sqrt(228), 1e-10);
      TS_ASSERT_DELTA(outputWS0->readE(4).front(), std::sqrt(71), 1e-10);
      TS_ASSERT_DELTA(outputWS0->readE(5).front(), std::sqrt(144), 1e-10);

      auto outputWS1 = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(1));
      TS_ASSERT_EQUALS(outputWS1->readY(0).front(), 171);
      TS_ASSERT_EQUALS(outputWS1->readY(1).front(), 163);
      TS_ASSERT_EQUALS(outputWS1->readY(2).front(), 188);
      TS_ASSERT_EQUALS(outputWS1->readY(3).front(), 182);
      TS_ASSERT_EQUALS(outputWS1->readY(4).front(), 68);
      TS_ASSERT_EQUALS(outputWS1->readY(5).front(), 135);

      auto outputWS2 = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(2));
      TS_ASSERT_EQUALS(outputWS2->readY(0).front(), 132);
      TS_ASSERT_EQUALS(outputWS2->readY(1).front(), 131);
      TS_ASSERT_EQUALS(outputWS2->readY(2).front(), 159);
      TS_ASSERT_EQUALS(outputWS2->readY(3).front(), 139);
      TS_ASSERT_EQUALS(outputWS2->readY(4).front(), 54);
      TS_ASSERT_EQUALS(outputWS2->readY(5).front(), 77);

      auto outputWS3 = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(3));
      TS_ASSERT_EQUALS(outputWS3->readY(0).front(), 12705);
      TS_ASSERT_EQUALS(outputWS3->readY(1).front(), 12668);
      TS_ASSERT_EQUALS(outputWS3->readY(2).front(), 14334);
      TS_ASSERT_EQUALS(outputWS3->readY(3).front(), 14313);
      TS_ASSERT_EQUALS(outputWS3->readY(4).front(), 4807);
      TS_ASSERT_EQUALS(outputWS3->readY(5).front(), 9179);

      auto outputWS4 = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(4));
      TS_ASSERT_EQUALS(outputWS4->readY(0).front(), 54);
      TS_ASSERT_EQUALS(outputWS4->readY(1).front(), 71);
      TS_ASSERT_EQUALS(outputWS4->readY(2).front(), 76);
      TS_ASSERT_EQUALS(outputWS4->readY(3).front(), 86);
      TS_ASSERT_EQUALS(outputWS4->readY(4).front(), 30);
      TS_ASSERT_EQUALS(outputWS4->readY(5).front(), 29);
    }

    // now repeat but with correction to sample
    configuration.correctToSample = true;

    outputWS = std::dynamic_pointer_cast<WorkspaceGroup>(run_algorithm(VULCAN_218062, configuration));

    /* expected results came from running same as above but with CorrectionToSample="Elastic" for FilterEvents

    FilterEvents(ws, SplitterWorkspace='split_matrix_ws', OutputWorkspaceBaseName="eventFiltered", GroupWorkspaces=True,
    FilterByPulseTime=False, RelativeTime=True, CorrectionToSample="Elastic")
    */

    {
      auto outputWS0 = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(0));
      TS_ASSERT_EQUALS(outputWS0->readY(0).front(), 207);
      TS_ASSERT_EQUALS(outputWS0->readY(1).front(), 196);
      TS_ASSERT_EQUALS(outputWS0->readY(2).front(), 241);
      TS_ASSERT_EQUALS(outputWS0->readY(3).front(), 206);
      TS_ASSERT_EQUALS(outputWS0->readY(4).front(), 69);
      TS_ASSERT_EQUALS(outputWS0->readY(5).front(), 151);

      auto outputWS1 = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(1));
      TS_ASSERT_EQUALS(outputWS1->readY(0).front(), 150);
      TS_ASSERT_EQUALS(outputWS1->readY(1).front(), 149);
      TS_ASSERT_EQUALS(outputWS1->readY(2).front(), 180);
      TS_ASSERT_EQUALS(outputWS1->readY(3).front(), 173);
      TS_ASSERT_EQUALS(outputWS1->readY(4).front(), 63);
      TS_ASSERT_EQUALS(outputWS1->readY(5).front(), 104);

      auto outputWS2 = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(2));
      TS_ASSERT_EQUALS(outputWS2->readY(0).front(), 119);
      TS_ASSERT_EQUALS(outputWS2->readY(1).front(), 123);
      TS_ASSERT_EQUALS(outputWS2->readY(2).front(), 147);
      TS_ASSERT_EQUALS(outputWS2->readY(3).front(), 133);
      TS_ASSERT_EQUALS(outputWS2->readY(4).front(), 50);
      TS_ASSERT_EQUALS(outputWS2->readY(5).front(), 78);

      auto outputWS3 = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(3));
      TS_ASSERT_EQUALS(outputWS3->readY(0).front(), 12742);
      TS_ASSERT_EQUALS(outputWS3->readY(1).front(), 12705);
      TS_ASSERT_EQUALS(outputWS3->readY(2).front(), 14375);
      TS_ASSERT_EQUALS(outputWS3->readY(3).front(), 14348);
      TS_ASSERT_EQUALS(outputWS3->readY(4).front(), 4813);
      TS_ASSERT_EQUALS(outputWS3->readY(5).front(), 9213);

      auto outputWS4 = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(4));
      TS_ASSERT_EQUALS(outputWS4->readY(0).front(), 66);
      TS_ASSERT_EQUALS(outputWS4->readY(1).front(), 67);
      TS_ASSERT_EQUALS(outputWS4->readY(2).front(), 90);
      TS_ASSERT_EQUALS(outputWS4->readY(3).front(), 67);
      TS_ASSERT_EQUALS(outputWS4->readY(4).front(), 18);
      TS_ASSERT_EQUALS(outputWS4->readY(5).front(), 57);
    }
  }

  void test_filter_bad_pulses() {
    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF");
    configuration.groupingWS = bank_grouping_ws;
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
    configuration.groupingWS = bank_grouping_ws;
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
    configuration.groupingWS = bank_grouping_ws;
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

  void test_grouping_workspace_12_groups() {
    // load VULCAN instrument
    auto load = AlgorithmManager::Instance().createUnmanaged("LoadEmptyInstrument");
    load->initialize();
    load->setProperty("InstrumentName", "VULCAN");
    load->setProperty("OutputWorkspace", "instrument");
    load->execute();

    // Use GenerateGroupingPowder to create grouping workspace. This will have a many-to-many relationship between banks
    // and output spectra. 10 degrees gives us 12 groups for VULCAN
    auto gen = AlgorithmManager::Instance().createUnmanaged("GenerateGroupingPowder");
    gen->initialize();
    gen->setProperty("InputWorkspace", "instrument");
    gen->setProperty("AngleStep", 10.);
    gen->setProperty("GroupingWorkspace", "grouping");
    gen->execute();

    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF");
    configuration.l2s = std::vector<double>(12, 2.0);
    configuration.twoTheta = std::vector<double>(12, 90.0);
    configuration.phi = std::vector<double>(12, 0.0);
    configuration.groupingWS = std::dynamic_pointer_cast<Mantid::DataObjects::GroupingWorkspace>(
        AnalysisDataService::Instance().retrieve("grouping"));
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(run_algorithm(VULCAN_218062, configuration));

    /* expected results came from running
    ws = LoadEventNexus("VULCAN_218062.nxs.h5", NumberOfBins=1)
    grp = GenerateGroupingPowder(ws, AngleStep=10.)
    ws = GroupDetectors(ws, CopyGroupingFromWorkspace="grp")
    print(ws.extractY())
    */

    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 12);
    TS_ASSERT_EQUALS(outputWS->readY(0).front(), 52699);
    TS_ASSERT_EQUALS(outputWS->readY(1).front(), 15037626);
    TS_ASSERT_EQUALS(outputWS->readY(2).front(), 3776091);
    TS_ASSERT_EQUALS(outputWS->readY(3).front(), 20332502);
    TS_ASSERT_EQUALS(outputWS->readY(4).front(), 21215268);
    TS_ASSERT_EQUALS(outputWS->readY(5).front(), 3819719);
    TS_ASSERT_EQUALS(outputWS->readY(6).front(), 11720729);
    TS_ASSERT_EQUALS(outputWS->readY(7).front(), 12322917);
    TS_ASSERT_EQUALS(outputWS->readY(8).front(), 2784939);
    TS_ASSERT_EQUALS(outputWS->readY(9).front(), 11921456);
    TS_ASSERT_EQUALS(outputWS->readY(10).front(), 19044631);
    TS_ASSERT_EQUALS(outputWS->readY(11).front(), 1934589);
  }

  void test_grouping_workspace_3_groups() {
    // load VULCAN instrument
    auto load = AlgorithmManager::Instance().createUnmanaged("LoadEmptyInstrument");
    load->initialize();
    load->setProperty("InstrumentName", "VULCAN");
    load->setProperty("OutputWorkspace", "instrument");
    load->execute();

    // Use GenerateGroupingPowder to create grouping workspace. This will have a many-to-many relationship between banks
    // and output spectra. 45 degrees gives us 3 groups for VULCAN
    auto gen = AlgorithmManager::Instance().createUnmanaged("GenerateGroupingPowder");
    gen->initialize();
    gen->setProperty("InputWorkspace", "instrument");
    gen->setProperty("AngleStep", 45.);
    gen->setProperty("GroupingWorkspace", "grouping");
    gen->execute();

    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF");
    configuration.l2s = std::vector<double>(3, 2.0);
    configuration.twoTheta = std::vector<double>(3, 90.0);
    configuration.phi = std::vector<double>(3, 0.0);
    configuration.groupingWS = std::dynamic_pointer_cast<Mantid::DataObjects::GroupingWorkspace>(
        AnalysisDataService::Instance().retrieve("grouping"));
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(run_algorithm(VULCAN_218062, configuration));

    /* expected results came from running
    ws = LoadEventNexus("VULCAN_218062.nxs.h5", NumberOfBins=1)
    grp = GenerateGroupingPowder(ws, AngleStep=45.)
    ws = GroupDetectors(ws, CopyGroupingFromWorkspace="grp")
    print(ws.extractY())
    */

    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 3);
    TS_ASSERT_EQUALS(outputWS->readY(0).front(), 39198918);
    TS_ASSERT_EQUALS(outputWS->readY(1).front(), 49892899);
    TS_ASSERT_EQUALS(outputWS->readY(2).front(), 34871349);
  }

  void test_grouping_workspace_sparse() {
    // create grouping workspace for VULCAN instrument, default is no grouping (all 0)
    auto createGroups = AlgorithmManager::Instance().createUnmanaged("CreateGroupingWorkspace");
    createGroups->initialize();
    createGroups->setProperty("InstrumentName", "VULCAN");
    createGroups->setProperty("OutputWorkspace", "grouping");
    createGroups->execute();

    auto groupingWS = std::dynamic_pointer_cast<Mantid::DataObjects::GroupingWorkspace>(
        AnalysisDataService::Instance().retrieve("grouping"));

    // Create 2 groups using only 3 detectors
    groupingWS->setValue(30, 1);
    groupingWS->setValue(40, 2);
    groupingWS->setValue(50, 1);

    TestConfig configuration({0.}, {50000.}, {50000.}, "Linear", "TOF");
    configuration.l2s = std::vector<double>(2, 2.0);
    configuration.twoTheta = std::vector<double>(2, 90.0);
    configuration.phi = std::vector<double>(2, 0.0);
    configuration.groupingWS = groupingWS;
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(run_algorithm(VULCAN_218062, configuration));

    /* expected results came from running
    ws = LoadEventNexus("VULCAN_218062.nxs.h5", NumberOfBins=1)
    print(ws.readY(30) + ws.readY(50))
    print(ws.readY(40))
    */

    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(outputWS->readY(0).front(), 543);
    TS_ASSERT_EQUALS(outputWS->readY(1).front(), 260);
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

private:
  GroupingWorkspace_sptr bank_grouping_ws;
};
