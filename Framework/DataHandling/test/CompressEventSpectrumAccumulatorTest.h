// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <NeXusFile.hpp>
#include <algorithm>
#include <cxxtest/TestSuite.h>
#include <iostream>

#include "MantidDataHandling/CompressEventSpectrumAccumulator.h"
#include "MantidKernel/Timer.h"

using namespace Mantid;
using Mantid::DataHandling::CompressEventSpectrumAccumulator;
using Mantid::DataObjects::EventList;

class CompressEventSpectrumAccumulatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CompressEventSpectrumAccumulatorTest *createSuite() { return new CompressEventSpectrumAccumulatorTest(); }
  static void destroySuite(CompressEventSpectrumAccumulatorTest *suite) { delete suite; }

  void run_linear_test(const double tof_min, const double tof_delta_hist) {
    constexpr double TOF_MAX{10000000}; // 1e7

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

    // create the accumulator
    CompressEventSpectrumAccumulator accumulator(tof_fine_bins, tof_delta_hist,
                                                 DataHandling::CompressBinningMode::LINEAR);

    // check that the starting condition is correct
    TS_ASSERT_EQUALS(accumulator.numberHistBins(), NUM_HIST_BINS);
    TS_ASSERT_EQUALS(accumulator.numberWeightedEvents(), 0);

    // add a bunch of events
    const size_t NUM_RAW_EVENTS = static_cast<size_t>(TOF_MAX - tof_min);
    for (size_t i = 0; i < NUM_RAW_EVENTS; ++i) {
      accumulator.addEvent(static_cast<float>(i) + static_cast<float>(tof_min));
    }
    const size_t NUM_WGHT_EVENTS = static_cast<size_t>((TOF_MAX - tof_min) / tof_delta_hist);
    TS_ASSERT_EQUALS(accumulator.numberWeightedEvents(), NUM_WGHT_EVENTS);

    // set up an EventList to add weighted events to
    EventList event_list;
    event_list.switchTo(Mantid::API::EventType::WEIGHTED_NOTIME);
    std::vector<Mantid::DataObjects::WeightedEventNoTime> *raw_events;
    getEventsFrom(event_list, raw_events);

    // write the events
    accumulator.createWeightedEvents(raw_events);
    TS_ASSERT_EQUALS(raw_events->size(), accumulator.numberWeightedEvents());

    // the first event has the weight of the fine histogram width
    TS_ASSERT_DELTA(raw_events->front().weight(), tof_delta_hist, .1);

    // confim that all events were added
    const double total_weight =
        std::accumulate(raw_events->cbegin(), raw_events->cend(), 0.,
                        [](const auto &current, const auto &value) { return current + value.weight(); });
    TS_ASSERT_DELTA(total_weight, static_cast<double>(NUM_RAW_EVENTS), .1);
  }

  void test_accumulator_linear_delta10() {
    constexpr double TOF_MIN{0};
    constexpr double TOF_DELTA_HIST{10}; // this puts 10 events in each bin
    run_linear_test(TOF_MIN, TOF_DELTA_HIST);
  }

  void test_accumulator_linear_offset10_delta10() {
    constexpr double TOF_MIN{10};
    constexpr double TOF_DELTA_HIST{10}; // this puts 10 events in each bin
    run_linear_test(TOF_MIN, TOF_DELTA_HIST);
  }

  void test_accumulator_linear_delta20() {
    constexpr double TOF_MIN{0};
    constexpr double TOF_DELTA_HIST{20}; // this puts 20 events in each bin
    run_linear_test(TOF_MIN, TOF_DELTA_HIST);
  }

  // ====================================== BEGIN OF REMOVE
  std::unique_ptr<std::vector<float>> getTof(::NeXus::File &filehandle, const std::string &nxspath) {
    filehandle.openPath(nxspath);
    const std::string FIELD_NAME("event_time_offset");

    filehandle.openData(FIELD_NAME); // time-of-flight
    const auto field_info = filehandle.getInfo();
    const int64_t dim0 = static_cast<int64_t>(field_info.dims[0]); // assume 1d
    auto time_of_flight = std::make_unique<std::vector<float>>(dim0);
    filehandle.readData(FIELD_NAME, *(time_of_flight.get()));

    return time_of_flight;
  }

  std::unique_ptr<std::vector<Mantid::Types::Core::DateAndTime>> getPulseTimes(::NeXus::File &filehandle,
                                                                               const std::string &nxspath) {
    filehandle.openPath(nxspath);
    const std::string FIELD_NAME("event_time_zero");

    filehandle.openData(FIELD_NAME);

    // get the offset
    std::string startTimeStr;
    filehandle.getAttr("offset", startTimeStr);
    Mantid::Types::Core::DateAndTime startTime(startTimeStr);

    const auto field_info = filehandle.getInfo();
    const int64_t dim0 = static_cast<int64_t>(field_info.dims[0]); // assume 1d
    auto pulsetime_raw = std::make_unique<std::vector<double>>(dim0);
    filehandle.readData(FIELD_NAME, *(pulsetime_raw.get()));

    // convert to DateAndTime
    auto pulsetime = std::make_unique<std::vector<Mantid::Types::Core::DateAndTime>>(dim0);
    std::transform(pulsetime_raw->cbegin(), pulsetime_raw->cend(), pulsetime->begin(),
                   [startTime](double incremental_time) { return startTime + incremental_time; });

    return pulsetime;
  }

  std::unique_ptr<std::vector<uint64_t>> getPulseIndex(::NeXus::File &filehandle, const std::string &nxspath) {
    filehandle.openPath(nxspath);
    const std::string FIELD_NAME("event_index");

    filehandle.openData(FIELD_NAME);

    const auto field_info = filehandle.getInfo();
    const int64_t dim0 = static_cast<int64_t>(field_info.dims[0]); // assume 1d
    auto event_index = std::make_unique<std::vector<uint64_t>>(dim0);
    filehandle.readData(FIELD_NAME, *(event_index.get()));

    return event_index;
  }

  void test_prototype() {
    const std::string FILENAME_SNAP("/home/pf9/build/mantid/snapperf/SNAP_57514.nxs.h5");
    const std::string NXSPATH_SNAP("/entry/bank52_events");

    constexpr double DELTA{0.1}; // microseconds

    Kernel::Timer snap_timer;
    auto snap_handle = ::NeXus::File(FILENAME_SNAP, NXACC_READ);
    auto snap_tof = getTof(snap_handle, NXSPATH_SNAP);
    auto snap_pulse_time = getPulseTimes(snap_handle, NXSPATH_SNAP);
    auto snap_pulse_index = getPulseIndex(snap_handle, NXSPATH_SNAP);
    snap_handle.close();
    std::cout << "\nREAD in " << snap_timer.elapsed() << "s\n";

    std::cout << "SNAP TOF[size=" << snap_tof->size() << "] " << snap_tof->front() << " ... " << snap_tof->back()
              << "\n";
    std::cout << "     PULSE[size=" << snap_pulse_time->size() << "] " << snap_pulse_time->front() << " ... "
              << snap_pulse_time->back() << "\n";
    std::cout << "     INDEX[size=" << snap_pulse_index->size() << "] " << snap_pulse_index->front() << " ... "
              << snap_pulse_index->back() << "\n";

    const auto [snap_min, snap_max] = std::minmax_element(snap_tof->cbegin(), snap_tof->cend());
    std::cout << "MIN=" << *snap_min << " MAX=" << *snap_max << " DELTA=" << DELTA << " <- linear bins\n";
    std::cout << "   RANGE " << (((*snap_max - *snap_min) / DELTA) + 1) << "\n";
    const size_t snap_num_bins =
        static_cast<size_t>(((*snap_max - *snap_min) / DELTA) + .5); // 1 for extra right bin boundary
    std::cout << "BINS " << snap_num_bins << "\n";

    auto tof_fine_bins = std::make_shared<std::vector<double>>();
    for (size_t i = 0; i < snap_num_bins + 1; ++i) { // to convert to edges
      tof_fine_bins->push_back(static_cast<double>(*snap_min + (static_cast<double>(i) * DELTA)));
    }

    const size_t MAX_EVENTS = snap_tof->size(); //  / 10;
    std::cout << "Parsing " << MAX_EVENTS << " events\n";

    // -------------------- accumulator
    snap_timer.reset();

    CompressEventSpectrumAccumulator accumulator(tof_fine_bins, DELTA, DataHandling::CompressBinningMode::LINEAR);
    for (size_t i = 0; i < MAX_EVENTS; ++i) {
      accumulator.addEvent(snap_tof->at(i));
    }

    EventList event_list;
    event_list.switchTo(Mantid::API::EventType::WEIGHTED_NOTIME);
    std::vector<Mantid::DataObjects::WeightedEventNoTime> *raw_events;
    getEventsFrom(event_list, raw_events);

    accumulator.createWeightedEvents(raw_events);

    {
      auto seconds = snap_timer.elapsed();
      std::cout << "Accumulator             in " << seconds << "s | rate=" << (static_cast<float>(MAX_EVENTS) / seconds)
                << "E/s\n"
                << "                      numWeighted=" << accumulator.numberWeightedEvents()
                << " numHist=" << accumulator.numberHistBins() << " unused="
                << (100. * double(accumulator.numberHistBins() - accumulator.numberWeightedEvents()) /
                    double(accumulator.numberHistBins()))
                << "%\n"
                << "                    elements.size=" << raw_events->size()
                << " memory=" << (event_list.getMemorySize() / 1024) << "kB\n";
    }

    // -------------------- prototype
    snap_timer.reset();

    std::vector<float> snap_vec_tof(snap_num_bins, 0.);
    std::vector<uint32_t> snap_vec_count(snap_num_bins, 0.);
    for (size_t i = 0; i < MAX_EVENTS; ++i) {
      const auto tof = static_cast<double>(snap_tof->at(i));
      const auto stdbin = EventList::findLinearBin(*tof_fine_bins.get(), tof, DELTA, *snap_min);
      if (stdbin) {
        const auto bin = stdbin.get();
        snap_vec_tof[bin] += static_cast<float>(tof);
        snap_vec_count[bin] += 1;
      } else {
        std::cout << "????????????????????? " << tof << " not in range of fine histogram\n";
      }
    }

    // pre-count how much to allocate for the output
    std::size_t numWgtEvents = std::accumulate(snap_vec_count.cbegin(), snap_vec_count.cend(), static_cast<size_t>(0),
                                               [](const auto &current, const auto &value) {
                                                 if (value > 0)
                                                   return current + 1;
                                                 else
                                                   return current;
                                               });

    EventList snap_events_wgt1;
    snap_events_wgt1.switchTo(Mantid::API::EventType::WEIGHTED_NOTIME);
    std::vector<Mantid::DataObjects::WeightedEventNoTime> *snap_wgt_events1;
    getEventsFrom(snap_events_wgt1, snap_wgt_events1);
    snap_wgt_events1->reserve(numWgtEvents);
    for (size_t i = 0; i < snap_num_bins; ++i) {
      const auto counts = snap_vec_count[i];
      if (counts > 0) {
        const double weight = static_cast<double>(counts);
        const double tof = static_cast<double>(snap_vec_tof[i]) / weight;
        snap_wgt_events1->emplace_back(tof, weight, weight);
      }
    }
    snap_events_wgt1.setSortOrder(Mantid::DataObjects::EventSortType::TOF_SORT);
    {
      auto seconds = snap_timer.elapsed();
      std::cout << "WeightedEventNoTime VEC in " << seconds << "s | rate=" << (static_cast<float>(MAX_EVENTS) / seconds)
                << "E/s\n"
                << "                    elements.size=" << snap_wgt_events1->size()
                << " memory=" << (snap_events_wgt1.getMemorySize() / 1024) << "kB\n"
                << "                    unused temporary fine bins=" << (snap_num_bins - snap_wgt_events1->size())
                << "\n";
    }
  }
  // ====================================== BEGIN OF REMOVE
};
