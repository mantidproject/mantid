// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidDataHandling/AlignAndFocusPowderSlim.h"
#include "MantidKernel/TimeROI.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/Unit.h"

#include <numbers>

using Mantid::API::MatrixWorkspace_sptr;
using Mantid::DataHandling::AlignAndFocusPowderSlim::AlignAndFocusPowderSlim;

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
  MatrixWorkspace_sptr run_algorithm(const std::string &filename, const std::vector<double> &xmin = {},
                                     const std::vector<double> &xmax = {}, const std::vector<double> &xdelta = {},
                                     const std::string binning = "Logarithmic",
                                     const std::string binningUnits = "dSpacing", const double timeMin = -1.,
                                     const double timeMax = -1., const bool should_throw = false) {
    const std::string wksp_name("VULCAN");

    std::cout << "==================> " << filename << '\n';
    Mantid::Kernel::Timer timer;
    AlignAndFocusPowderSlim alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", wksp_name));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BinningMode", binning));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BinningUnits", binningUnits));
    if (!xmin.empty())
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMin", xmin));
    if (!xmax.empty())
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMax", xmax));
    if (!xdelta.empty())
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("XDelta", xdelta));
    if (timeMin > 0.)
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("FilterByTimeStart", timeMin));
    if (timeMax > 0.)
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("FilterByTimeStop", timeMax));

    if (should_throw) {
      TS_ASSERT_THROWS(alg.execute(), const std::invalid_argument &);
      return MatrixWorkspace_sptr();
    }

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
    std::cout << "==================> " << timer << '\n';

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    return outputWS;
  }

  void test_defaults() {
    const std::string filename("VULCAN_218062.nxs.h5");
    MatrixWorkspace_sptr outputWS = run_algorithm(filename);

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
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "unused"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ReadSizeFromDisk", 1000000));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););

    MatrixWorkspace_sptr outputWS2 = alg.getProperty("OutputWorkspace");
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
    const std::vector<double> xmin{13000.};
    const std::vector<double> xmax{36000.};
    const std::vector<double> xdelta{};
    const std::string filename("VULCAN_218062.nxs.h5");
    MatrixWorkspace_sptr outputWS = run_algorithm(filename, xmin, xmax, xdelta, "Logarithmic", "TOF");

    constexpr size_t NUM_Y{637}; // observed value

    // verify the output
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 6);
    TS_ASSERT_EQUALS(outputWS->blocksize(), NUM_Y);
    TS_ASSERT(outputWS->isCommonBins());
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->unitID(), "TOF");
    // default values in algorithm
    TS_ASSERT_EQUALS(outputWS->readX(0).front(), xmin[0]);
    TS_ASSERT_EQUALS(outputWS->readX(0).back(), xmax[0]);
    // observed values from running
    const auto y_values = outputWS->readY(0);
    TS_ASSERT_EQUALS(y_values.size(), NUM_Y);
    TS_ASSERT_EQUALS(y_values[0], 0.);
    TS_ASSERT_EQUALS(y_values[NUM_Y / 2], 55374.); // observed
    TS_ASSERT_EQUALS(y_values[NUM_Y - 1], 0.);

    // do not need to cleanup because workspace did not go into the ADS
  }

  void test_ragged_bins_x_min_max() {
    const std::vector<double> xmin{13000., 14000., 15000., 16000., 17000., 18000.};
    const std::vector<double> xmax{36000., 37000., 38000., 39000., 40000., 41000.};
    const std::vector<double> xdelta{}; // empty means use the default binning
    const std::string filename("VULCAN_218062.nxs.h5");
    MatrixWorkspace_sptr outputWS = run_algorithm(filename, xmin, xmax, xdelta, "Logarithmic", "TOF");

    // verify the output
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 6);

    // check the x-values
    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      const auto &x_values = outputWS->readX(i);
      TS_ASSERT_EQUALS(x_values.front(), xmin[i]);
      TS_ASSERT_EQUALS(x_values.back(), xmax[i]);
    }

    // do not need to cleanup because workspace did not go into the ADS
  }

  void test_ragged_bins_x_delta() {
    const std::vector<double> xmin{13000.};
    const std::vector<double> xmax{36000.};
    const std::vector<double> xdelta{1000., 2000., 3000., 4000., 5000., 6000.};
    // this will create
    const std::string filename("VULCAN_218062.nxs.h5");
    MatrixWorkspace_sptr outputWS = run_algorithm(filename, xmin, xmax, xdelta, "Linear", "TOF");

    // verify the output
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 6);

    // check the x-values
    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      const auto &x_values = outputWS->readX(i);
      TS_ASSERT_EQUALS(x_values.front(), xmin[0]);
      TS_ASSERT_EQUALS(x_values.back(), xmax[0]);
      TS_ASSERT_EQUALS(x_values.size(), std::round((xmax[0] - xmin[0]) / xdelta[i] + 1));
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
    const std::string filename("VULCAN_218062.nxs.h5");

    MatrixWorkspace_sptr outputWS = run_algorithm(filename, xmin, xmax, xdelta, "Linear", units);

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

  void test_start_stop_time_filtering() {
    MatrixWorkspace_sptr outputWS =
        run_algorithm("VULCAN_218062.nxs.h5", std::vector<double>{0.}, std::vector<double>{50000.},
                      std::vector<double>{50000.}, "Linear", "TOF", 200., 300.);

    /* expected results came from running

    ws = LoadEventNexus("VULCAN_218062.nxs.h5", FilterByTimeStart=200, FilterByTimeStop=300, NumberOfBins=1)
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
    MatrixWorkspace_sptr outputWS =
        run_algorithm("VULCAN_218062.nxs.h5", std::vector<double>{0.}, std::vector<double>{50000.},
                      std::vector<double>{50000.}, "Linear", "TOF", 200., -1.);

    /* expected results came from running

    ws = LoadEventNexus("VULCAN_218062.nxs.h5", FilterByTimeStart=200, NumberOfBins=1)
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
    MatrixWorkspace_sptr outputWS =
        run_algorithm("VULCAN_218062.nxs.h5", std::vector<double>{0.}, std::vector<double>{50000.},
                      std::vector<double>{50000.}, "Linear", "TOF", -1., 300.);

    /* expected results came from running

    ws = LoadEventNexus("VULCAN_218062.nxs.h5", FilterByTimeStop=300, NumberOfBins=1)
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
    MatrixWorkspace_sptr outputWS =
        run_algorithm("VULCAN_218062.nxs.h5", std::vector<double>{0.}, std::vector<double>{50000.},
                      std::vector<double>{50000.}, "Linear", "TOF", 0., 3000.);

    /* expected results came from running

    ws = LoadEventNexus("VULCAN_218062.nxs.h5", NumberOfBins=1)
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
    run_algorithm("VULCAN_218062.nxs.h5", std::vector<double>{0.}, std::vector<double>{50000.},
                  std::vector<double>{50000.}, "Linear", "TOF", 300., 200., true);
    // start time longer than run time of ~600 seconds
    run_algorithm("VULCAN_218062.nxs.h5", std::vector<double>{0.}, std::vector<double>{50000.},
                  std::vector<double>{50000.}, "Linear", "TOF", 1000., 2000., true);
  }

  // ==================================
  // TODO things below this point are for benchmarking and will be removed later
  // ==================================

  void run_test(const std::string &filename) {
    MatrixWorkspace_sptr outputWS = run_algorithm(filename);

    // LoadEventNexus 4 seconds
    // tof 6463->39950

    // verify the output
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 6);
    TS_ASSERT_EQUALS(outputWS->blocksize(), 3349); // observed value

    // do not need to cleanup because workspace did not go into the ADS
  }

  void xtest_exec1GB() { run_test("/home/pf9/build/mantid/vulcanperf/VULCAN_218075.nxs.h5"); }

  void xtest_exec10GB() { run_test("/home/pf9/build/mantid/vulcanperf/VULCAN_218092.nxs.h5"); }

  void xtest_exec18GB() { run_test("/home/pf9/build/mantid/vulcanperf/VULCAN_217967.nxs.h5"); }
};
