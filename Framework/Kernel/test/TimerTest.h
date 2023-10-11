// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

class TimerTest : public CxxTest::TestSuite {
public:
  void sleepSec(const int sec) {
#if defined _WIN64
    Sleep(sec);
#elif defined _WIN32
    // 32-bit windows doesn't seem to have brilliant resolution
    // as the Sleep(200) can quite often cause the elapsed
    // time to be less than 0.19 seconds!
    Sleep(sec);
#else
    usleep(sec * 1000);
#endif
  }

  void testTimer() {
    // Instantiating the object starts the timer
    Mantid::Kernel::Timer timer;
    sleepSec(200);
    TS_ASSERT_LESS_THAN(0.18, timer.elapsed());
    // Calling elapsed above should reset the timer
    sleepSec(100);
    TS_ASSERT_LESS_THAN(0.08, timer.elapsed());
  }

  void testCodeBlockTimer() {
    std::ostringstream out;
    {
      Mantid::Kernel::CodeBlockTimer("test_timer", out);
      sleepSec(200);
    }
    // TS_ASSERT
  }
};
