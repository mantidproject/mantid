// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/BankPulseTimes.h"

using Mantid::DataHandling::BankPulseTimes;

class BankPulseTimesTest : public CxxTest::TestSuite {
private:
  std::vector<Mantid::Types::Core::DateAndTime> createPulseTimes(const size_t length) {
    const static Mantid::Types::Core::DateAndTime START_TIME(0., 0.);
    constexpr double SECONDS_DELTA{30.};

    std::vector<Mantid::Types::Core::DateAndTime> pulse_times;
    for (size_t i = 0; i < length; ++i)
      pulse_times.emplace_back(START_TIME + static_cast<double>(i) + SECONDS_DELTA);

    return pulse_times;
  }

  /// Create periods 1-5
  std::vector<int> createPeriodIndices(const size_t length) {
    std::vector<int> period_indices;
    for (int i = 0; i < static_cast<int>(length); ++i)
      period_indices.emplace_back((i % 5) + 1);

    return period_indices;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BankPulseTimesTest *createSuite() { return new BankPulseTimesTest(); }
  static void destroySuite(BankPulseTimesTest *suite) { delete suite; }

  void test_noPeriods() {
    const auto pulse_times = createPulseTimes(100);

    BankPulseTimes bank_pulse_times(pulse_times);

    // TODO should have a method to get number of pulses
    TS_ASSERT_EQUALS(bank_pulse_times.pulseTimes.size(), pulse_times.size());
    TS_ASSERT_EQUALS(bank_pulse_times.periodNumbers.size(), pulse_times.size());

    // TODO should have a method to get this information
    for (std::size_t i = 0; i < pulse_times.size(); ++i) {
      TS_ASSERT_EQUALS(bank_pulse_times.pulseTimes[i], pulse_times[i]);
      TS_ASSERT_EQUALS(bank_pulse_times.periodNumbers[i], BankPulseTimes::FirstPeriod);
    }
  }

  void test_periods() {
    const auto pulse_times = createPulseTimes(100);
    const auto period_indices = createPeriodIndices(100);

    BankPulseTimes bank_pulse_times(pulse_times, period_indices);

    // TODO should have a method to get number of pulses
    TS_ASSERT_EQUALS(bank_pulse_times.pulseTimes.size(), pulse_times.size());
    TS_ASSERT_EQUALS(bank_pulse_times.periodNumbers.size(), pulse_times.size());

    // TODO should have a method to get this information
    for (std::size_t i = 0; i < pulse_times.size(); ++i) {
      TS_ASSERT_EQUALS(bank_pulse_times.pulseTimes[i], pulse_times[i]);
      // coordinated with createPeriodIndices() above
      TS_ASSERT_EQUALS(bank_pulse_times.periodNumbers[i], (i % 5) + 1);
    }
  }

  void test_empty() {
    // empty vector of pulse times
    const auto pulse_times = createPulseTimes(0);
    TS_ASSERT_EQUALS(pulse_times.size(), 0);

    BankPulseTimes bank_pulse_times(pulse_times);

    // TODO should have a method to get number of pulses
    TS_ASSERT_EQUALS(bank_pulse_times.pulseTimes.size(), pulse_times.size());
    TS_ASSERT_EQUALS(bank_pulse_times.periodNumbers.size(), pulse_times.size());

    // TODO should have a method to get this information
    for (std::size_t i = 0; i < pulse_times.size(); ++i) {
      TS_ASSERT_EQUALS(bank_pulse_times.pulseTimes[i], pulse_times[i]);
      TS_ASSERT_EQUALS(bank_pulse_times.periodNumbers[i], BankPulseTimes::FirstPeriod);
    }
  }

  void test_periods_not_parallel() {
    const auto pulse_times = createPulseTimes(100);
    const auto period_indices = createPeriodIndices(10);

    BankPulseTimes bank_pulse_times(pulse_times, period_indices);

    // TODO should have a method to get number of pulses
    TS_ASSERT_EQUALS(bank_pulse_times.pulseTimes.size(), pulse_times.size());
    TS_ASSERT_EQUALS(bank_pulse_times.periodNumbers.size(), pulse_times.size());

    // TODO should have a method to get this information
    for (std::size_t i = 0; i < pulse_times.size(); ++i) {
      TS_ASSERT_EQUALS(bank_pulse_times.pulseTimes[i], pulse_times[i]);
      TS_ASSERT_EQUALS(bank_pulse_times.periodNumbers[i], BankPulseTimes::FirstPeriod);
    }
  }
};
