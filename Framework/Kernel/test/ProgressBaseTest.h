// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidKernel/ProgressBase.h"

using namespace Mantid::Kernel;

class ProgressBaseTest : public CxxTest::TestSuite {
public:
  /** Class for debugging progress reporting */
  class MyTestProgress : public ProgressBase {
  public:
    MyTestProgress(double start, double end, int64_t numSteps) : ProgressBase(start, end, numSteps) {}

    void doReport(const std::string &msg = "") override {
      last_report_message = msg;
      last_report_counter = m_i;
      double p = m_start + m_step * double(m_i - m_ifirst);
      last_report_value = p;
    }

  public:
    /// Index that was last set at doReport
    int64_t last_report_counter;
    double last_report_value;
    std::string last_report_message;
  };

  void test_copy_and_assign() {
    MyTestProgress prog1(0.1, 0.5, 10);
    prog1.report("Hello");

    MyTestProgress prog2(0.0, 1.0, 5);
    prog2 = prog1; // copy assign
    TS_ASSERT_EQUALS(prog2.last_report_counter, 1);
    TS_ASSERT_DELTA(prog2.last_report_value, 0.14, 1e-3);
    TS_ASSERT_EQUALS(prog2.last_report_message, "Hello");

    MyTestProgress prog3(prog1); // copy construct
    TS_ASSERT_EQUALS(prog3.last_report_counter, 1);
    TS_ASSERT_DELTA(prog3.last_report_value, 0.14, 1e-3);
    TS_ASSERT_EQUALS(prog3.last_report_message, "Hello");
  }

  void test_report_and_reportIncrement() {
    // 8 steps from 0.1 to 0.9
    MyTestProgress p(0.1, 0.9, 8);

    // First report goes to 0.2
    p.report("Hello");
    TS_ASSERT_EQUALS(p.last_report_counter, 1);
    TS_ASSERT_DELTA(p.last_report_value, 0.2, 1e-3);
    TS_ASSERT_EQUALS(p.last_report_message, "Hello");

    // Now let's increment
    p.reportIncrement(2, "Hi-oh!");
    TS_ASSERT_EQUALS(p.last_report_counter, 3);
    TS_ASSERT_DELTA(p.last_report_value, 0.4, 1e-3);
    TS_ASSERT_EQUALS(p.last_report_message, "Hi-oh!");

    // Report to go directly to a value (little-used)
    p.report(6);
    TS_ASSERT_EQUALS(p.last_report_counter, 6);
    TS_ASSERT_DELTA(p.last_report_value, 0.7, 1e-3);
  }

  void test_setNumSteps() {
    MyTestProgress p(0.0, 1.0, 10);
    p.setNumSteps(100);

    // First report goes to 0.01, since there are now 100 steps
    p.report("One percent");
    TS_ASSERT_EQUALS(p.last_report_counter, 1);
    TS_ASSERT_DELTA(p.last_report_value, 0.01, 1e-3);
    TS_ASSERT_EQUALS(p.last_report_message, "One percent");

    // Back to ten steps with counter > 0 will make things odd so should be
    // avoided
    p.setNumSteps(10);
    p.report();
    TS_ASSERT_EQUALS(p.last_report_counter, 2);
    TS_ASSERT_DELTA(p.last_report_value, 0.2, 1e-3);
  }

  void test_notifyStep() {
    // 500 steps, default = only notify every 1 % = 5 calls
    MyTestProgress p(0.0, 1.0, 500);
    p.last_report_counter = -1;
    TS_ASSERT_EQUALS(p.last_report_counter, -1);
    // The first notify always does the report!
    p.report();
    TS_ASSERT_EQUALS(p.last_report_counter, 1);
    // But no more until you reach 5% MORE
    p.report();
    TS_ASSERT_EQUALS(p.last_report_counter, 1);
    p.report();
    TS_ASSERT_EQUALS(p.last_report_counter, 1);
    p.report();
    TS_ASSERT_EQUALS(p.last_report_counter, 1);
    p.report();
    TS_ASSERT_EQUALS(p.last_report_counter, 1);
    p.report();
    TS_ASSERT_EQUALS(p.last_report_counter, 6);
  }

  void test_setNotifyStep() {
    // Make a progress reporter that will report each time, even though it is
    // less than 1 percent
    MyTestProgress p(0.0, 1.0, 500);
    p.setNotifyStep(0.1);
    p.report();
    TS_ASSERT_EQUALS(p.last_report_counter, 1);
    p.report();
    TS_ASSERT_EQUALS(p.last_report_counter, 2);
    p.report();
    TS_ASSERT_EQUALS(p.last_report_counter, 3);
    p.report();
    TS_ASSERT_EQUALS(p.last_report_counter, 4);
  }

  /** Progress report would work incorrectly for ridiculously large integer # of
   * steps. */
  void test_setNumSteps_forRidiculouslyLargeNumbers() {
    int64_t reallyBig = int64_t(1000 * 1000) * int64_t(1000 * 1000);
    MyTestProgress p(0.0, 1.0, 10);
    p.last_report_counter = -1;
    p.setNumSteps(reallyBig);

    p.report("");
    TS_ASSERT_EQUALS(p.last_report_counter, -1); // Not reported yet

    // Need 10*1e9 reports before you even reach 1%
    for (size_t i = 0; i < 10; i++)
      p.reportIncrement(1000000000, "");

    TS_ASSERT_EQUALS(p.last_report_counter, 10000000001);
    TS_ASSERT_DELTA(p.last_report_value, 1e-2, 1e-6);
  }
};
