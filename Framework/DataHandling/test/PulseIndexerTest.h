// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/PulseIndexer.h"

using Mantid::DataHandling::PulseIndexer;

class PulseIndexerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PulseIndexerTest *createSuite() { return new PulseIndexerTest(); }
  static void destroySuite(PulseIndexerTest *suite) { delete suite; }

  void run_test(std::shared_ptr<std::vector<uint64_t>> eventIndices, const size_t start_event_index,
                const size_t total_events) {
    PulseIndexer indexer(eventIndices, start_event_index, total_events, "junk_name");

    // test locating the first pulse entirely containing the event index
    for (size_t pulse_index = 0; pulse_index < eventIndices->size(); ++pulse_index)
      TS_ASSERT_EQUALS(indexer.getFirstPulseIndex(eventIndices->operator[](pulse_index)), pulse_index);

    // test locating the first event index for the pulse
    // how start_event_index affects values is baked in from how code worked pre 2024
    for (size_t i = 0; i < eventIndices->size(); ++i)
      TS_ASSERT_EQUALS(indexer.getStartEventIndex(i), eventIndices->operator[](i) - start_event_index);

    // test locating the flast event index for the pulse
    // how start_event_index affects values is baked in from how code worked pre 2024
    for (size_t i = 0; i < eventIndices->size() - 1; ++i)
      TS_ASSERT_EQUALS(indexer.getStopEventIndex(i), eventIndices->operator[](i + 1) - start_event_index);
    TS_ASSERT_EQUALS(indexer.getStopEventIndex(eventIndices->size() - 1), total_events);
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
    eventIndices->push_back(16);

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

    constexpr size_t total_events{20};

    run_test(eventIndices, 2, total_events);
  }
};
