// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidKernel/CPUTimer.h"

using namespace Mantid::Kernel;

class CPUTimerTest : public CxxTest::TestSuite {
public:
  /** Since timer tests are difficult to make reliable,
   * simple tests for not throwing only.
   */
  void test_throws_nothing() {
    TS_ASSERT_THROWS_NOTHING(CPUTimer timer1;)
    CPUTimer tim1;
    TS_ASSERT_THROWS_NOTHING(tim1.reset();)
    TS_ASSERT_THROWS_NOTHING(tim1.elapsedCPU();)
    TS_ASSERT_THROWS_NOTHING(tim1.elapsedCPU(true);)
    TS_ASSERT_THROWS_NOTHING(tim1.elapsedCPU(false);)
    TS_ASSERT_THROWS_NOTHING(tim1.CPUfraction();)
    TS_ASSERT_THROWS_NOTHING(tim1.CPUfraction(true);)
    TS_ASSERT_THROWS_NOTHING(tim1.CPUfraction(false);)
  }
};
