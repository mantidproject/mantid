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
#include <regex>
#include <sstream>

using namespace Mantid::Kernel;

class TimerTest : public CxxTest::TestSuite {
public:
  void sleep_ms(const int ms) {
#if defined _WIN64
    Sleep(ms);
#elif defined _WIN32
    // 32-bit windows doesn't seem to have brilliant resolution
    // as the Sleep(200) can quite often cause the elapsed
    // time to be less than 0.19 seconds!
    Sleep(ms);
#else
    usleep(ms * 1000); /*microseconds*/
#endif
  }

  void sleep_ms_with_timer(const int ms, CodeBlockMultipleTimer::TimeAccumulator &timeAccumulator) {
    CodeBlockMultipleTimer timer{timeAccumulator};
    sleep_ms(ms);
  }

  void testTimer() {
    // Instantiating the object starts the timer
    Timer timer;
    sleep_ms(200);
    TS_ASSERT_LESS_THAN(0.18, timer.elapsed());
    // Calling elapsed above should reset the timer
    sleep_ms(100);
    TS_ASSERT_LESS_THAN(0.08, timer.elapsed());
  }

  // Test a block of code that calls sleep function and measures the elapsed time
  void testCodeBlockTimer() {
    const int sleepTime_ms{200};
    const double minTimeFactor{
        0.9}; // The measured elapsed time is expected to be at least this portion of the input sleep time

    // Test this code block
    std::ostringstream out;
    {
      CodeBlockTimer timer("test_timer", out);
      sleep_ms(sleepTime_ms);
    }

    // Parse the timer output and find the elapsed time
    // Example of the timer output: "Elapsed time (sec) in "test_timer": 0.200135\n"
    const std::string str_output{out.str()};
    const std::regex str_expr{"Elapsed time \\(sec\\) in \"(.*)\": ([0-9]+(\\.[0-9]+)?)\\n$"};
    std::smatch sm;
    std::regex_match(str_output, sm, str_expr);
    // Expected pattern matches for a timer output "Elapsed time (sec) in "test_timer": 0.200135\n"
    // Match 0: "Elapsed time (sec) in "test_timer": 0.200135\n"
    // Match 1: "test_timer"
    // Match 2: "0.200135"
    // Match 3: ".200135"
    TS_ASSERT_LESS_THAN(2, sm.size());
    double elapsed_sec = stof(sm.str(2));
    TS_ASSERT_LESS_THAN(sleepTime_ms * minTimeFactor, elapsed_sec * sec2msec);
  }

  // Call sleep function multiple times and measure the total elapsed time
  void testCodeBlockMultipleTimer() {
    const std::string timer_name{"test_timer"};
    const int sleepTime_ms{200};
    const double minTimeFactor{
        0.9}; // The measured elapsed time is expected to be at least this portion of the input sleep time
    const int number_of_sleep_calls{4};

    // Call sleep function a few times with time accumulator
    CodeBlockMultipleTimer::TimeAccumulator ta{timer_name};
    for (size_t i = 0; i < number_of_sleep_calls; i++)
      sleep_ms_with_timer(sleepTime_ms, ta);

    // Test elapsed time and number of entrances
    double elapsed_sec = ta.getElapsed();
    TS_ASSERT_LESS_THAN(number_of_sleep_calls * sleepTime_ms * minTimeFactor, elapsed_sec * sec2msec);
    TS_ASSERT_EQUALS(ta.getNumberOfEntrances(), number_of_sleep_calls);

    // Test stream output
    std::ostringstream out;
    out << ta << '\n';
    std::ostringstream expected_out;
    expected_out << "Elapsed time (sec) in \"" << timer_name << "\": " << elapsed_sec
                 << "; Number of entrances: " << number_of_sleep_calls << '\n';
    TS_ASSERT_EQUALS(expected_out.str(), out.str());

    // Reset the accumulator and repeat the test
    ta.reset();
    for (size_t i = 0; i < number_of_sleep_calls; i++)
      sleep_ms_with_timer(sleepTime_ms, ta);

    // Test elapsed time and number of entrances
    elapsed_sec = ta.getElapsed();
    TS_ASSERT_LESS_THAN(number_of_sleep_calls * sleepTime_ms * minTimeFactor, elapsed_sec * sec2msec);
    TS_ASSERT_EQUALS(ta.getNumberOfEntrances(), number_of_sleep_calls);

    // Test stream output
    out.str("");
    out << ta << '\n';
    expected_out.str("");
    expected_out << "Elapsed time (sec) in \"" << timer_name << "\": " << elapsed_sec
                 << "; Number of entrances: " << number_of_sleep_calls << '\n';
    TS_ASSERT_EQUALS(expected_out.str(), out.str());
  }

private:
  const int sec2msec{1000}; // time conversion factor
};
