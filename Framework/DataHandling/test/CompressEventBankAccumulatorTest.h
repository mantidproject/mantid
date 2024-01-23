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

#include "MantidDataHandling/CompressEventBankAccumulator.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/VectorHelper.h"

using namespace Mantid;
using Mantid::DataHandling::CompressEventBankAccumulator;
using Mantid::DataHandling::CompressEventSpectrumAccumulator;
using Mantid::DataObjects::EventList;

class CompressEventBankAccumulatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CompressEventBankAccumulatorTest *createSuite() { return new CompressEventBankAccumulatorTest(); }
  static void destroySuite(CompressEventBankAccumulatorTest *suite) { delete suite; }

  /*
  void xtest_single_bank() {
    const size_t num_spectra{50};
    const detid_t min_detid{10};
    const detid_t max_detid{min_detid + num_spectra};
    const size_t num_events_per_spectrum{10000};
    const double tof_min{10};
    const double tof_delta{(17000 - tof_min) / static_cast<double>(num_events_per_spectrum)};
    const double tof_bin_width = 10.;

    // create fine histogram bin edges
    auto tof_fine_bins = std::make_shared<std::vector<double>>();
    {
      tof_fine_bins->push_back(tof_min);

      double tof = tof_min;
      while (tof < 16666) {
        tof += tof_bin_width;
        tof_fine_bins->push_back(tof);
      }
    }

    CompressEventBankAccumulator accumulator(min_detid, max_detid, tof_fine_bins, tof_bin_width);

    // this is intentionally the slow way around to make sure the logic treats each event separately
    size_t events_added = 0;
    for (size_t event_num = 0; event_num < num_events_per_spectrum; ++event_num) {
      // create tof
      const float tof = static_cast<float>((static_cast<float>(event_num) * tof_delta) + tof_min);
      // count up how many are actually in range
      if (tof >= tof_fine_bins->front() && tof < tof_fine_bins->back())
        events_added++;
      for (detid_t detid = min_detid - 1; detid <= max_detid; ++detid) {
        accumulator.addEvent(detid, tof);
      }
    }

    // verify total number of weighted events created
    // this is less than the number of events created to help verify that filtering is working
    double numWghtExpected = static_cast<double>(num_spectra + 1) * (events_added);
    TS_ASSERT_DELTA(accumulator.totalWeight(), numWghtExpected, .1);

    { // this should generate an exception
      // set up an EventList to add weighted events to
      EventList event_list(Mantid::API::EventType::WEIGHTED_NOTIME);
      std::vector<Mantid::DataObjects::WeightedEventNoTime> *raw_events;
      getEventsFrom(event_list, raw_events);

      TS_ASSERT_THROWS(accumulator.createWeightedEvents(min_detid - 1, raw_events), std::runtime_error const &);
    }

    // all event lists will be identical because of how fake data was made
    for (detid_t detid = min_detid; detid <= max_detid; ++detid) {
      // set up an EventList to add weighted events to
      EventList event_list(Mantid::API::EventType::WEIGHTED_NOTIME);
      std::vector<Mantid::DataObjects::WeightedEventNoTime> *raw_events;
      getEventsFrom(event_list, raw_events);

      // write the events and verify
      accumulator.createWeightedEvents(detid, raw_events);
      TS_ASSERT_EQUALS(raw_events->size(), (tof_fine_bins->size() - 1));

      // confim that the correct number events were added
      const double total_weight =
          std::accumulate(raw_events->cbegin(), raw_events->cend(), 0.,
                          [](const auto &current, const auto &value) { return current + value.weight(); });
      TS_ASSERT_DELTA(total_weight, 9806., .1); // observed value, but could be calculated
    }
  }
  */

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

  std::unique_ptr<std::vector<detid_t>> getDetIDs(::NeXus::File &filehandle, const std::string &nxspath) {
    // auto detids = std::make_unique<std::vector<detid_t>>();

    filehandle.openPath(nxspath);
    const std::string FIELD_NAME("event_id");

    filehandle.openData(FIELD_NAME); // detector_id
    const auto field_info = filehandle.getInfo();
    const int64_t dim0 = static_cast<int64_t>(field_info.dims[0]); // assume 1d

    auto raw_data = std::make_unique<std::vector<uint32_t>>(dim0);
    filehandle.readData(FIELD_NAME, *(raw_data.get()));

    auto detids = std::make_unique<std::vector<detid_t>>(raw_data->cbegin(), raw_data->cend());

    return detids;
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
    auto snap_detids = getDetIDs(snap_handle, NXSPATH_SNAP);
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
    Mantid::Kernel::VectorHelper::createAxisFromRebinParams({*snap_min, DELTA, (*snap_max + DELTA)}, *tof_fine_bins);

    const size_t MAX_EVENTS = snap_tof->size(); //  / 10;
    std::cout << "Parsing " << MAX_EVENTS << " events\n";

    // -------------------- spectrum accumulator
    snap_timer.reset();

    CompressEventSpectrumAccumulator accumulator(tof_fine_bins, DELTA, DataHandling::CompressBinningMode::LINEAR);
    for (size_t i = 0; i < MAX_EVENTS; ++i) {
      accumulator.addEvent(snap_tof->at(i));
    }

    EventList event_list(Mantid::API::EventType::WEIGHTED_NOTIME);
    std::vector<Mantid::DataObjects::WeightedEventNoTime> *raw_events;
    getEventsFrom(event_list, raw_events);

    accumulator.createWeightedEvents(raw_events);

    {
      auto seconds = snap_timer.elapsed();
      std::cout << "Spectrum Accumulator  in " << seconds << "s | rate=" << (static_cast<float>(MAX_EVENTS) / seconds)
                << "E/s\n"
                << "                      numWeighted=" << accumulator.totalWeight()
                << " numHist=" << accumulator.numberHistBins()
                << " unused="
                //  << (100. * double(accumulator.numberHistBins() - accumulator.totalWeight()) /
                //    double(accumulator.numberHistBins()))
                << "%\n"
                << "                     elements.size=" << raw_events->size()
                << " memory=" << (event_list.getMemorySize() / 1024) << "kB\n";
    }

    // -------------------- bank accumulator
    /*
        {
          // get detid range
          const auto [snap_min_detid, snap_max_detid] = std::minmax_element(snap_detids->cbegin(), snap_detids->cend());
          std::cout << "DET ID range " << (*snap_min_detid) << " to " << (*snap_max_detid) << "\n"
                    << "       for " << (*snap_max_detid - *snap_min_detid) << " pixels\n";
          size_t temp_size = tof_fine_bins->size() * static_cast<size_t>(*snap_max_detid - *snap_min_detid);
          std::cout << "       num bins " << tof_fine_bins->size() << "\n"
                    << "       temp size " << (temp_size * 8 / 1024 / 1024 / 1024) << "GiB\n";
          snap_timer.reset();

          CompressEventBankAccumulator accumulator(*snap_min_detid, *snap_max_detid, tof_fine_bins, DELTA);
          // loop through all of the events and add them
          for (size_t i = 0; i < MAX_EVENTS; ++i) {
            accumulator.addEvent(snap_detids->at(i), snap_tof->at(i));
          }

          // create the output event lists and change the types
          const size_t num_det = static_cast<size_t>(snap_max_detid - snap_min_detid + 1);
          std::vector<EventList> event_lists(num_det, Mantid::API::EventType::WEIGHTED_NOTIME);
          // for (auto &event_list : event_lists)
          // event_list.switchTo(Mantid::API::EventType::WEIGHTED_NOTIME);

          // create the weighted events
          for (detid_t detid = *snap_min_detid; detid <= *snap_max_detid; ++detid) {
            std::vector<Mantid::DataObjects::WeightedEventNoTime> *raw_events;
            getEventsFrom(event_lists[detid - *snap_min_detid], raw_events);

            accumulator.createWeightedEvents(detid, raw_events);
          }

          {
            auto seconds = snap_timer.elapsed();
            std::cout << "Bank Accumulator      in " << seconds << "s | rate=" << (static_cast<float>(MAX_EVENTS) /
       seconds)
                      << "E/s\n"
                      << "                      numWeighted="
                      << accumulator.totalWeight()
                      //<< " numHist=" << accumulator.numberHistBins() << " unused="
                      //<< (100. * double(accumulator.numberHistBins() - accumulator.numberWeightedEvents()) /
                      //    double(accumulator.numberHistBins()))
                      //<< "%\n"
                      << "\n"
                //<< "                     elements.size=" << raw_events->size()
                //<< " memory=" << (event_lists.getMemorySize() / 1024) << "kB\n"
                ;

            //size_t num_events =
            //    std::accumulate(event_lists.cbegin(), event_lists.cend(), 0, [](const auto &current, const auto
       &value) {
            //      const auto &events = value.getWeightedEventsNoTime();
            //      // double weight =
            //      return current + events.size();
            //    });
            //std::cout << "     TOTAL_WGHT=" << num_events << "\n";
          }
        }
        */

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

    EventList snap_events_wgt1(Mantid::API::EventType::WEIGHTED_NOTIME);
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
