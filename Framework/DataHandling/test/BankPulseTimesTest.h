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
typedef std::vector<Mantid::Types::Core::DateAndTime> PulseTimeVec;

namespace {
constexpr double SECONDS_DELTA{30.};
const static Mantid::Types::Core::DateAndTime START_TIME(0., 0.);
} // namespace

class BankPulseTimesTest : public CxxTest::TestSuite {
private:
  PulseTimeVec createPulseTimes(const size_t length) {
    PulseTimeVec pulse_times;
    for (size_t i = 0; i < length; ++i)
      pulse_times.emplace_back(START_TIME + (static_cast<double>(i) * SECONDS_DELTA));

    return pulse_times;
  }

  /// Create periods 1-5
  std::vector<int> createPeriodIndices(const size_t length) {
    std::vector<int> period_indices;
    for (int i = 0; i < static_cast<int>(length); ++i)
      period_indices.emplace_back((i % 5) + 1);

    return period_indices;
  }

  void doROITest(BankPulseTimes &bank_pulse_times, const std::vector<Mantid::Types::Core::DateAndTime> &starting,
                 const std::vector<Mantid::Types::Core::DateAndTime> &stopping, const std::vector<size_t> &roi_exp) {

    for (const auto &startVal : starting) {
      for (const auto &stopVal : stopping) {
        auto roi = bank_pulse_times.getPulseIndices(startVal, stopVal);
        TS_ASSERT_EQUALS(roi.size(), 2);
        TS_ASSERT_EQUALS(roi, roi_exp);
      }
    }
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BankPulseTimesTest *createSuite() { return new BankPulseTimesTest(); }
  static void destroySuite(BankPulseTimesTest *suite) { delete suite; }

  void test_noPeriods() {
    constexpr size_t NUM_TIMES{100};
    const auto pulse_times = createPulseTimes(NUM_TIMES);

    BankPulseTimes bank_pulse_times(pulse_times);

    TS_ASSERT_EQUALS(bank_pulse_times.startTime, pulse_times[0].toISO8601String());
    TS_ASSERT_EQUALS(bank_pulse_times.numberOfPulses(), pulse_times.size());

    for (std::size_t i = 0; i < pulse_times.size(); ++i) {
      TS_ASSERT_EQUALS(bank_pulse_times.pulseTime(i), pulse_times[i]);
      TS_ASSERT_EQUALS(bank_pulse_times.periodNumber(i), BankPulseTimes::FIRST_PERIOD);
    }

    // roi - include everything
    {
      const auto start = pulse_times.front() - SECONDS_DELTA;
      const auto stop = pulse_times.back() + SECONDS_DELTA;

      // past both ends
      auto roi = bank_pulse_times.getPulseIndices(start, stop);
      TS_ASSERT(roi.empty());

      // at the front and past the end
      roi = bank_pulse_times.getPulseIndices(pulse_times.front(), stop);
      TS_ASSERT(roi.empty());

      // before the front and at the end
      roi = bank_pulse_times.getPulseIndices(start, pulse_times.back());
      TS_ASSERT(roi.empty());
    }

    // roi - trim from the front
    {
      const auto start = pulse_times.front() + 2. * SECONDS_DELTA;
      const auto stop = pulse_times.back() + SECONDS_DELTA;

      // parameterize values to check - all have the same roi result
      std::vector<Mantid::Types::Core::DateAndTime> starting({start, start - .5 * SECONDS_DELTA});
      std::vector<Mantid::Types::Core::DateAndTime> stopping({stop, pulse_times.back()});

      const std::vector<size_t> roi_exp({2, pulse_times.size()});

      doROITest(bank_pulse_times, starting, stopping, roi_exp);
    }

    // roi - trim from the back
    {
      const auto start = pulse_times.front() - SECONDS_DELTA;
      const auto stop = pulse_times.back() - 2. * SECONDS_DELTA;

      // parameterize values to check - all have the same roi result
      std::vector<Mantid::Types::Core::DateAndTime> starting({start, pulse_times.front()});
      std::vector<Mantid::Types::Core::DateAndTime> stopping({stop, stop + .5 * SECONDS_DELTA});

      const std::vector<size_t> roi_exp({0, pulse_times.size() - 3});

      doROITest(bank_pulse_times, starting, stopping, roi_exp);
    }
  }

  void test_noPeriods_unsorted() {
    // generate a sawtooth function for pulse times - smallest value is not at the beginning
    PulseTimeVec pulse_times;
    for (int offset = 20; offset >= 0; offset -= 10)
      for (size_t i = 0; i < 33; ++i)
        pulse_times.emplace_back(START_TIME + static_cast<double>(offset) + (static_cast<double>(i) * SECONDS_DELTA));

    // cache the minimum value
    const auto startTimeObj = std::min(pulse_times.cbegin(), pulse_times.cend());

    BankPulseTimes bank_pulse_times(pulse_times);

    TS_ASSERT_EQUALS(bank_pulse_times.startTime, startTimeObj->toISO8601String());
    TS_ASSERT_EQUALS(bank_pulse_times.numberOfPulses(), pulse_times.size());

    for (std::size_t i = 0; i < pulse_times.size(); ++i) {
      TS_ASSERT_EQUALS(bank_pulse_times.pulseTime(i), pulse_times[i]);
      TS_ASSERT_EQUALS(bank_pulse_times.periodNumber(i), BankPulseTimes::FIRST_PERIOD);
    }
  }

  void test_periods() {
    const auto pulse_times = createPulseTimes(100);
    const auto period_indices = createPeriodIndices(100);

    BankPulseTimes bank_pulse_times(pulse_times, period_indices);

    TS_ASSERT_EQUALS(bank_pulse_times.startTime, pulse_times[0].toISO8601String());
    TS_ASSERT_EQUALS(bank_pulse_times.numberOfPulses(), pulse_times.size());

    for (std::size_t i = 0; i < pulse_times.size(); ++i) {
      TS_ASSERT_EQUALS(bank_pulse_times.pulseTime(i), pulse_times[i]);
      // coordinated with createPeriodIndices() above
      TS_ASSERT_EQUALS(bank_pulse_times.periodNumber(i), (i % 5) + 1);
    }
  }

  void test_empty() {
    const size_t NUM_PULSES{0};
    // empty vector of pulse times
    const auto pulse_times = createPulseTimes(NUM_PULSES);
    TS_ASSERT_EQUALS(pulse_times.size(), NUM_PULSES);

    BankPulseTimes bank_pulse_times(pulse_times);

    // should be zero length
    TS_ASSERT_EQUALS(bank_pulse_times.numberOfPulses(), NUM_PULSES);
    // default is unix epoch
    TS_ASSERT_EQUALS(bank_pulse_times.startTime, BankPulseTimes::DEFAULT_START_TIME);
  }

  void test_periods_not_parallel() {
    const auto pulse_times = createPulseTimes(100);
    const auto period_indices = createPeriodIndices(10);

    BankPulseTimes bank_pulse_times(pulse_times, period_indices);

    TS_ASSERT_EQUALS(bank_pulse_times.startTime, pulse_times[0].toISO8601String());
    TS_ASSERT_EQUALS(bank_pulse_times.numberOfPulses(), pulse_times.size());

    for (std::size_t i = 0; i < pulse_times.size(); ++i) {
      TS_ASSERT_EQUALS(bank_pulse_times.pulseTime(i), pulse_times[i]);
      TS_ASSERT_EQUALS(bank_pulse_times.periodNumber(i), BankPulseTimes::FIRST_PERIOD);
    }
  }
};
