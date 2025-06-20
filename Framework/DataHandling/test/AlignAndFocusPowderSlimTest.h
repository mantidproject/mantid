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
#include "MantidDataHandling/AlignAndFocusPowderSlim.h"
#include "MantidKernel/Timer.h"

using Mantid::API::MatrixWorkspace_sptr;
using Mantid::DataHandling::AlignAndFocusPowderSlim;

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
  MatrixWorkspace_sptr run_algorithm(const std::string &filename, const double xmin = -1., const double xmax = -1.) {
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
    if (xmin >= 0.)
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMin", std::vector<double>{xmin}));
    if (xmax >= 0.)
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMax", std::vector<double>{xmax}));
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

    constexpr size_t NUM_Y{4641}; // observed value

    // verify the output
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 6);
    TS_ASSERT_EQUALS(outputWS->blocksize(), NUM_Y);
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->unitID(), "TOF");
    // default values in algorithm
    TS_ASSERT_EQUALS(outputWS->readX(0).front(), 10.);
    TS_ASSERT_EQUALS(outputWS->readX(0).back(), 16667.);
    // observed values from running
    const auto y_values = outputWS->readY(0);
    TS_ASSERT_EQUALS(y_values.size(), NUM_Y);
    TS_ASSERT_EQUALS(y_values[0], 0.);
    TS_ASSERT_EQUALS(y_values[NUM_Y / 2], 0.);
    TS_ASSERT_EQUALS(y_values[NUM_Y - 1], 20719);

    // do not need to cleanup because workspace did not go into the ADS
  }

  void test_common_x() {
    constexpr double xmin{13000};
    constexpr double xmax{36000};
    const std::string filename("VULCAN_218062.nxs.h5");
    MatrixWorkspace_sptr outputWS = run_algorithm(filename, xmin, xmax);

    constexpr size_t NUM_Y{637}; // observed value

    // verify the output
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 6);
    TS_ASSERT_EQUALS(outputWS->blocksize(), NUM_Y);
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->unitID(), "TOF");
    // default values in algorithm
    TS_ASSERT_EQUALS(outputWS->readX(0).front(), xmin);
    TS_ASSERT_EQUALS(outputWS->readX(0).back(), xmax);
    // observed values from running
    const auto y_values = outputWS->readY(0);
    TS_ASSERT_EQUALS(y_values.size(), NUM_Y);
    TS_ASSERT_EQUALS(y_values[0], 0.);
    TS_ASSERT_EQUALS(y_values[NUM_Y / 2], 55374.); // observed
    TS_ASSERT_EQUALS(y_values[NUM_Y - 1], 0.);

    // do not need to cleanup because workspace did not go into the ADS
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
