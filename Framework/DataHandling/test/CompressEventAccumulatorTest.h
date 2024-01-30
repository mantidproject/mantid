// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <algorithm>
#include <cxxtest/TestSuite.h>
#include <iostream>

#include "MantidDataHandling/CompressEventAccumulator.h"
#include "MantidKernel/Timer.h"

using namespace Mantid;
using Mantid::DataHandling::CompressBinningMode;
using Mantid::DataHandling::CompressEventAccumulator;
using Mantid::DataHandling::CompressEventAccumulatorFactory;
using Mantid::DataObjects::EventList;

namespace {
constexpr double TOF_MAX{10000000}; // 1e7
} // namespace

class CompressEventAccumulatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CompressEventAccumulatorTest *createSuite() { return new CompressEventAccumulatorTest(); }
  static void destroySuite(CompressEventAccumulatorTest *suite) { delete suite; }

  size_t addEvents(CompressEventAccumulator *accumulator, const float tof_min) {
    const auto tof_max = static_cast<float>(TOF_MAX);

    float tof = tof_min;
    while (tof < tof_max) {
      accumulator->addEvent(tof);
      tof += 1;
    }

    return static_cast<size_t>(tof_max - tof_min);
  }

  void run_general_test(std::shared_ptr<std::vector<double>> histogram_bin_edges, const double tof_min,
                        const double divisor, CompressBinningMode bin_mode, std::size_t num_wght_events) {
    // create the accumulator
    CompressEventAccumulatorFactory factory(histogram_bin_edges, divisor, bin_mode);
    auto accumulator = factory.create();

    // add a bunch of events
    const size_t NUM_RAW_EVENTS = addEvents(accumulator.get(), static_cast<float>(tof_min));

    // set up an EventList to add weighted events to
    EventList event_list;
    event_list.switchTo(Mantid::API::EventType::WEIGHTED_NOTIME);
    std::vector<Mantid::DataObjects::WeightedEventNoTime> *raw_events;
    getEventsFrom(event_list, raw_events);

    // write the events
    accumulator->createWeightedEvents(raw_events);
    TS_ASSERT_EQUALS(raw_events->size(), num_wght_events);

    // the first event has the weight of the fine histogram width
    // TS_ASSERT_DELTA(raw_events->front().weight(), divisor, .1);

    // confim that all events were added
    const double total_weight =
        std::accumulate(raw_events->cbegin(), raw_events->cend(), 0.,
                        [](const auto &current, const auto &value) { return current + value.weight(); });
    TS_ASSERT_DELTA(total_weight, static_cast<double>(NUM_RAW_EVENTS), .1);
  }

  void run_linear_test(const double tof_min, const double tof_delta_hist) {
    // set up the fine histogram
    const size_t NUM_HIST_BINS = static_cast<size_t>((TOF_MAX - tof_min) / tof_delta_hist);
    auto tof_fine_bins = std::make_shared<std::vector<double>>();
    for (size_t i = 0; i < NUM_HIST_BINS + 1; ++i) { // to convert to edges
      const double tof = static_cast<double>(tof_min + (static_cast<double>(i) * tof_delta_hist));
      tof_fine_bins->push_back(tof);
    }
    TS_ASSERT_EQUALS(tof_fine_bins->size(), NUM_HIST_BINS + 1);
    TS_ASSERT_EQUALS(tof_fine_bins->front(), tof_min);
    TS_ASSERT_EQUALS(tof_fine_bins->back(), TOF_MAX);

    // number of weighted events can be calculated
    const size_t NUM_WGHT_EVENTS = static_cast<size_t>((TOF_MAX - tof_min) / tof_delta_hist);

    run_general_test(tof_fine_bins, tof_min, tof_delta_hist, DataHandling::CompressBinningMode::LINEAR,
                     NUM_WGHT_EVENTS);
  }

  void test_linear_delta10() {
    constexpr double TOF_MIN{0};
    constexpr double TOF_DELTA_HIST{10}; // this puts 10 events in each bin
    run_linear_test(TOF_MIN, TOF_DELTA_HIST);
  }

  void test_linear_offset10_delta10() {
    constexpr double TOF_MIN{10};
    constexpr double TOF_DELTA_HIST{10}; // this puts 10 events in each bin
    run_linear_test(TOF_MIN, TOF_DELTA_HIST);
  }

  void test_linear_delta20() {
    constexpr double TOF_MIN{0};
    constexpr double TOF_DELTA_HIST{20}; // this puts 20 events in each bin
    run_linear_test(TOF_MIN, TOF_DELTA_HIST);
  }

  void run_logorithm_test(const double tof_min, const double tof_delta_hist, const size_t num_bins) {
    std::cout << "run_logorithm_test\n";
    if (tof_min <= 0.)
      throw std::runtime_error("Cannot have tof_min <= 0");

    // set up the fine histogram
    auto tof_fine_bins = std::make_shared<std::vector<double>>();
    {
      tof_fine_bins->push_back(tof_min);
      double tof = tof_min;
      while (tof < 10000000 + tof_delta_hist) {
        tof = (1 + tof_delta_hist) * tof;
        tof_fine_bins->push_back(tof);
      }
    }
    TS_ASSERT_EQUALS(tof_fine_bins->size(), num_bins + 1);
    TS_ASSERT_EQUALS(tof_fine_bins->front(), tof_min);
    TS_ASSERT_EQUALS(tof_fine_bins->at(1), tof_min * (1. + tof_delta_hist));
    TS_ASSERT_LESS_THAN(TOF_MAX, tof_fine_bins->back());

    run_general_test(tof_fine_bins, tof_min, tof_delta_hist, DataHandling::CompressBinningMode::LOGARITHMIC, num_bins);
  }

  void test_log_delta10() {
    constexpr double TOF_MIN{1};
    constexpr double TOF_DELTA_HIST{1};
    constexpr size_t NUM_BINS{24}; // this is observed
    run_logorithm_test(TOF_MIN, TOF_DELTA_HIST, NUM_BINS);
  }
};
