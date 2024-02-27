// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <iostream>

#include "MantidDataHandling/PulseIndexer.h"

using Mantid::DataHandling::PulseIndexer;

namespace {
const std::string entry_name("junk_name");
}

class PulseIndexerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PulseIndexerTest *createSuite() { return new PulseIndexerTest(); }
  static void destroySuite(PulseIndexerTest *suite) { delete suite; }

  void run_test(std::shared_ptr<std::vector<uint64_t>> eventIndices, const size_t start_event_index,
                const size_t total_events, const size_t firstPulseIndex = 0, const size_t lastPulseIndex = 0) {
    // rather than make all the tests supply a value, calculate it when it isn't specified
    const size_t myLastPulseIndex = (lastPulseIndex == 0) ? eventIndices->size() : lastPulseIndex;

    // create the object to test
    PulseIndexer indexer(eventIndices, start_event_index, total_events, entry_name);

    // test locating the range of pulse entirely containing the event indices
    TS_ASSERT_EQUALS(indexer.getFirstPulseIndex(), firstPulseIndex);
    TS_ASSERT_EQUALS(indexer.getLastPulseIndex(), myLastPulseIndex);

    // things before
    const auto exp_start_event = eventIndices->operator[](firstPulseIndex) - start_event_index;
    for (size_t i = 0; i < firstPulseIndex; ++i) {
      TS_ASSERT_EQUALS(indexer.getStartEventIndex(i), indexer.getStopEventIndex(i));
      TS_ASSERT_EQUALS(indexer.getStartEventIndex(i), exp_start_event); // TODO check
    }

    // test locating the first event index for the pulse
    // how start_event_index affects values is baked in from how code worked pre 2024
    for (size_t i = firstPulseIndex; i < myLastPulseIndex - 1; ++i) {
      TS_ASSERT_EQUALS(indexer.getStartEventIndex(i), eventIndices->operator[](i) - start_event_index);
      TS_ASSERT_EQUALS(indexer.getStopEventIndex(i), eventIndices->operator[](i + 1) - start_event_index);
    }

    const auto exp_stop_event = (total_events >= start_event_index) ? (total_events - start_event_index) : total_events;
    // last element is a little different
    {
      const size_t i = myLastPulseIndex - 1;
      TS_ASSERT_EQUALS(indexer.getStartEventIndex(i), eventIndices->operator[](i) - start_event_index);
      TS_ASSERT_EQUALS(indexer.getStopEventIndex(i), exp_stop_event);
    }

    // check past the end
    for (size_t i = myLastPulseIndex; i < eventIndices->size() + 2; ++i) {
      TS_ASSERT_EQUALS(indexer.getStartEventIndex(i), indexer.getStopEventIndex(i));
      TS_ASSERT_EQUALS(indexer.getStopEventIndex(i), exp_stop_event);
    }
  }

  void test_Simple() {
    auto eventIndices = std::make_shared<std::vector<uint64_t>>();
    for (uint64_t i = 0; i < 30; i += 10) {
      eventIndices->push_back(i);
    }

    TS_ASSERT_EQUALS(eventIndices->size(), 3);

    constexpr size_t total_events{40};
    constexpr size_t start_event_index{0};
    run_test(eventIndices, start_event_index, total_events);
  }

  std::shared_ptr<std::vector<uint64_t>> generate_nonConstant() {
    auto eventIndices = std::make_shared<std::vector<uint64_t>>();
    eventIndices->push_back(10);
    eventIndices->push_back(12);
    eventIndices->push_back(15);
    eventIndices->push_back(18);

    TS_ASSERT_EQUALS(eventIndices->size(), 4);

    return eventIndices;
  }

  void test_nonConstant() {
    auto eventIndices = generate_nonConstant();

    constexpr size_t total_events{20};

    run_test(eventIndices, 0, total_events);
  }

  void test_nonConstantWithOffset() {
    auto eventIndices = generate_nonConstant();

    constexpr size_t start_event_index{2};
    constexpr size_t total_events{20};
    constexpr size_t first_pulse_index{3};

    run_test(eventIndices, start_event_index, total_events, first_pulse_index);
  }

  void test_nonConstantWithOffset2() {
    auto eventIndices = generate_nonConstant();

    constexpr size_t first_pulse_index{1};
    const auto start_event_index = eventIndices->operator[](1);
    constexpr size_t total_events{20};

    run_test(eventIndices, start_event_index, total_events, first_pulse_index);
  }

  void test_nonConstantWithOffsetAndTrimStop() {
    auto eventIndices = generate_nonConstant();

    constexpr size_t first_pulse_index{1};
    const size_t last_pulse_index{eventIndices->size() - 1};
    const auto start_event_index = eventIndices->operator[](1);
    const size_t total_events{eventIndices->back() - eventIndices->operator[](first_pulse_index) - 1};

    run_test(eventIndices, start_event_index, total_events, first_pulse_index, last_pulse_index);
  }

  void test_repeatingZeros() {
    // the values are taken from LoadEventNexusTest::test_load_ILL_no_triggers
    // in essence, the pulse information isn't supplied
    auto eventIndices = std::make_shared<std::vector<uint64_t>>();
    eventIndices->push_back(0);
    eventIndices->push_back(0);

    constexpr size_t start_event_index{0};
    constexpr size_t total_events{1000};
    constexpr size_t first_pulse_index{1};

    run_test(eventIndices, start_event_index, total_events, first_pulse_index);
  }
};
