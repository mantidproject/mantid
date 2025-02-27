// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/PulseIndexer.h"
#include <cxxtest/TestSuite.h>
#include <iostream>

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

private:
  void run_test(std::shared_ptr<std::vector<uint64_t>> eventIndices, const size_t start_event_index,
                const size_t total_events, const size_t firstPulseIndex = 0, const size_t lastPulseIndex = 0) {
    // rather than make all the tests supply a value, calculate it when it isn't specified
    const size_t myLastPulseIndex = (lastPulseIndex == 0) ? eventIndices->size() : lastPulseIndex;

    // create the object to test
    PulseIndexer indexer(eventIndices, start_event_index, total_events, entry_name, std::vector<size_t>());

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
      assert_indices_equal(indexer, i, eventIndices->operator[](i) - start_event_index,
                           eventIndices->operator[](i + 1) - start_event_index);
    }

    const auto exp_stop_event = (total_events >= start_event_index) ? (total_events - start_event_index) : total_events;
    // last element is a little different
    {
      const size_t i = myLastPulseIndex - 1;
      assert_indices_equal(indexer, i, eventIndices->operator[](i) - start_event_index, exp_stop_event);
    }

    // check past the end
    for (size_t i = myLastPulseIndex; i < eventIndices->size() + 2; ++i) {
      TS_ASSERT_EQUALS(indexer.getStartEventIndex(i), indexer.getStopEventIndex(i));
      TS_ASSERT_EQUALS(indexer.getStopEventIndex(i), total_events);
    }

    // check the iterator
    TS_ASSERT_DIFFERS(indexer.cbegin(), indexer.cend());

    { // explicit for loop
      size_t count{0};
      for (auto iter = indexer.cbegin(); iter != indexer.cend(); ++iter)
        count++;
      TS_ASSERT_EQUALS(count, indexer.getLastPulseIndex() - indexer.getFirstPulseIndex());
    }

    { // range based for loop
      size_t count{0};

      for (const auto &iter : indexer) { // requires begin() and end()
        (void)iter;                      // to quiet unused arg warning
        count++;
      }
      TS_ASSERT_EQUALS(count, indexer.getLastPulseIndex() - indexer.getFirstPulseIndex());
    }
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

  void assert_indices_equal(PulseIndexer &indexer, const size_t pulseIndex, const size_t startEventIndex,
                            const size_t stopEventIndex) {
    TS_ASSERT_EQUALS(indexer.getStartEventIndex(pulseIndex), startEventIndex);
    TS_ASSERT_EQUALS(indexer.getStopEventIndex(pulseIndex), stopEventIndex);
  }

public:
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

  void test_zerosEvents() {
    // test that the pulse indexer can handle no events
    auto eventIndices = std::make_shared<std::vector<uint64_t>>();
    eventIndices->push_back(0);
    eventIndices->push_back(0);

    constexpr size_t start_event_index{0};
    constexpr size_t total_events{0};
    constexpr size_t first_pulse_index{1};

    run_test(eventIndices, start_event_index, total_events, first_pulse_index);
  }

  void test_invalidPulseROI() {
    auto eventIndices = generate_nonConstant();

    std::vector<size_t> roi;
    roi.push_back(1); // must be even number

    TS_ASSERT_THROWS(PulseIndexer indexer(eventIndices, 0, eventIndices->back() + 10, entry_name, roi),
                     const std::runtime_error &);
  }

  // this exercises the functionality of filtering pulse times using BankPulseTimes
  void test_pulseROI() {
    // add to an example above
    auto eventIndices = generate_nonConstant();
    for (uint64_t i = 50; i < 100; i += 13)
      eventIndices->push_back(i);

    constexpr size_t first_pulse_index{1};
    const size_t last_pulse_index{eventIndices->size() - 2};
    const auto start_event_index = eventIndices->operator[](1);
    const size_t total_events{eventIndices->back() - eventIndices->operator[](first_pulse_index) - 1};
    std::vector<size_t> roi;

    // test having the roi being entirely included
    {
      roi.clear();
      roi.push_back(first_pulse_index);
      roi.push_back(last_pulse_index);
      PulseIndexer indexer(eventIndices, start_event_index, total_events, entry_name, roi);

      const auto firstPulseIndex = indexer.getFirstPulseIndex();

      TS_ASSERT_EQUALS(firstPulseIndex, roi.front());
      TS_ASSERT_EQUALS(indexer.getLastPulseIndex(), roi.back());

      TS_ASSERT_EQUALS(indexer.getStartEventIndex(firstPulseIndex),
                       eventIndices->operator[](firstPulseIndex) - start_event_index);
      TS_ASSERT_EQUALS(indexer.getStopEventIndex(firstPulseIndex),
                       eventIndices->operator[](firstPulseIndex + 1) - start_event_index);
    }

    // test chopping off the front
    {
      roi.clear();
      roi.push_back(first_pulse_index + 1);
      roi.push_back(eventIndices->size());
      PulseIndexer indexer(eventIndices, start_event_index, total_events, entry_name, roi);

      const auto firstPulseIndex = indexer.getFirstPulseIndex();

      TS_ASSERT_EQUALS(firstPulseIndex, roi.front());
      TS_ASSERT_EQUALS(indexer.getLastPulseIndex(), last_pulse_index);

      TS_ASSERT_EQUALS(indexer.getStartEventIndex(firstPulseIndex),
                       eventIndices->operator[](firstPulseIndex) - start_event_index);
      TS_ASSERT_EQUALS(indexer.getStopEventIndex(firstPulseIndex),
                       eventIndices->operator[](firstPulseIndex + 1) - start_event_index);
    }

    // test a more interesting roi
    {
      roi.clear();
      // one frame after the first pulse index
      roi.push_back(first_pulse_index + 1);
      roi.push_back(first_pulse_index + 3);
      // two frames just before the end
      roi.push_back(eventIndices->size() - 2);
      roi.push_back(eventIndices->size());

      PulseIndexer indexer(eventIndices, start_event_index, total_events, entry_name, roi);

      TS_ASSERT_EQUALS(indexer.getFirstPulseIndex(), roi.front());
      TS_ASSERT_EQUALS(indexer.getLastPulseIndex(), last_pulse_index - 2); // roi gets rid of more

      auto toEventIndex = [eventIndices, start_event_index](const size_t pulseIndex) {
        return eventIndices->operator[](pulseIndex) - start_event_index;
      };

      const auto exp_start_event = eventIndices->operator[](indexer.getFirstPulseIndex()) - start_event_index;
      const auto exp_total_event = toEventIndex(4) - toEventIndex(2);

      // check the individual event indices
      assert_indices_equal(indexer, 0, exp_start_event, exp_start_event); // exclude before
      assert_indices_equal(indexer, 1, exp_start_event, exp_start_event); // exclude before
      assert_indices_equal(indexer, 2, toEventIndex(2), toEventIndex(3)); // include
      assert_indices_equal(indexer, 3, toEventIndex(3), toEventIndex(4)); // include
      assert_indices_equal(indexer, 4, total_events, total_events);       // exclude
      assert_indices_equal(indexer, 5, total_events, total_events);       // exclude
      assert_indices_equal(indexer, 6, total_events, total_events);       // exclude
      assert_indices_equal(indexer, 7, total_events, total_events);       // exclude due to number of events
      assert_indices_equal(indexer, 8, total_events, total_events);       // exclude after
      assert_indices_equal(indexer, 9, total_events, total_events);       // exclude after
      assert_indices_equal(indexer, 10, total_events, total_events);      // exclude out of range

      // check the iterator
      TS_ASSERT_DIFFERS(indexer.begin(), indexer.end());
      TS_ASSERT_DIFFERS(indexer.cbegin(), indexer.cend());

      // range based for loop
      size_t num_steps{0};
      size_t num_events{0};
      for (const auto &iter : indexer) { // requires begin() and end()
        num_events += (iter.eventIndexStop - iter.eventIndexStart);
        num_steps++;
      }
      TS_ASSERT_EQUALS(num_events, exp_total_event);
      TS_ASSERT_EQUALS(num_steps, 2); // calculated by hand
    }
  }
};
