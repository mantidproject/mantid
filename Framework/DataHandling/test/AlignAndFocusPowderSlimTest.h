// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

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

  void run_test(const std::string &filename) {
    std::cout << "==================> " << filename << '\n';
    Mantid::Kernel::Timer timer;
    const std::string wksp_name("VULCAN");
    AlignAndFocusPowderSlim alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", wksp_name));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
    std::cout << "==================> " << timer << '\n';

    // LoadEventNexus 4 seconds
    // tof 6463->39950

    // verify the output
    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 6);
    TS_ASSERT_EQUALS(outputWS->blocksize(), 3349); // observed value

    // do not need to cleanup because workspace did not go into the ADS
  }

  void test_exec1GB() { run_test("/home/pf9/build/mantid/vulcanperf/VULCAN_218075.nxs.h5"); }

  void test_exec10GB() { run_test("/home/pf9/build/mantid/vulcanperf/VULCAN_218092.nxs.h5"); }

  void test_exec18GB() { run_test("/home/pf9/build/mantid/vulcanperf/VULCAN_217967.nxs.h5"); }

  //  void test_Something() { TS_FAIL("You forgot to write a test!"); }
};
