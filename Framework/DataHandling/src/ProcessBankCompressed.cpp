// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/ProcessBankCompressed.h"
#include "MantidDataHandling/BankPulseTimes.h"
#include "MantidDataHandling/DefaultEventLoader.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidDataHandling/PulseIndexer.h"
#include "MantidKernel/Timer.h"

#include "tbb/parallel_for.h"
#include <iostream>

namespace Mantid {
namespace DataHandling {
ProcessBankCompressed::ProcessBankCompressed(DefaultEventLoader &m_loader, const std::string &entry_name,
                                             API::Progress *prog, std::shared_ptr<std::vector<uint32_t>> event_detid,
                                             std::shared_ptr<std::vector<float>> event_tof, size_t startAt,
                                             std::shared_ptr<std::vector<uint64_t>> event_index,
                                             std::shared_ptr<BankPulseTimes> bankPulseTimes, detid_t min_detid,
                                             detid_t max_detid,
                                             std::shared_ptr<std::vector<double>> histogram_bin_edges,
                                             const double divisor)
    : Task(), m_loader(m_loader), m_entry_name(entry_name), m_prog(prog), m_event_detid(std::move(event_detid)),
      m_event_tof(std::move(event_tof)), m_firstEventIndex(startAt), m_event_index(std::move(event_index)),
      m_bankPulseTimes(std::move(bankPulseTimes)), m_detid_min(min_detid), m_detid_max(max_detid),
      m_tof_min(static_cast<float>(histogram_bin_edges->front())),
      m_tof_max(static_cast<float>(histogram_bin_edges->back())) {
  const auto bin_mode = (divisor >= 0) ? CompressBinningMode::LINEAR : CompressBinningMode::LOGARITHMIC;

  m_cost = static_cast<double>(m_event_detid->size());

  const auto divisor_abs = abs(divisor);

  // create the spetcra accumulators
  m_factory = std::make_unique<CompressEventAccumulatorFactory>(histogram_bin_edges, divisor_abs, bin_mode);
}

namespace {
size_t estimate_avg_events(const size_t num_events, const size_t num_dets, const size_t num_periods) {
  double result = static_cast<double>(num_events) / static_cast<double>(num_dets) / static_cast<double>(num_periods);
  return static_cast<size_t>(result);
}
} // namespace

void ProcessBankCompressed::createAccumulators(const bool precount) {
  const auto NUM_PERIODS = m_loader.m_ws.nPeriods();
  const auto NUM_DETS = static_cast<size_t>(m_detid_max - m_detid_min) + 1;
  const auto NUM_EVENTS_AVG = estimate_avg_events(m_event_detid->size(), NUM_DETS, NUM_PERIODS);

  std::vector<size_t> counts;
  if (precount) {
    const auto detid_min = static_cast<size_t>(m_detid_min);
    const auto detid_max = static_cast<size_t>(m_detid_max);
    counts.assign(NUM_DETS + 1, 0);
    for (const auto &detid : *m_event_detid) {
      if (!(detid < detid_min || detid > detid_max)) // or allows for skipping out early
        counts[detid - detid_min]++;
    }
  }

  m_spectra_accum.resize(NUM_PERIODS);
  for (size_t periodIndex = 0; periodIndex < NUM_PERIODS; ++periodIndex) {
    m_spectra_accum[periodIndex].reserve(NUM_DETS);
    for (size_t det_index = 0; det_index <= NUM_DETS; ++det_index) {
      if (precount)
        m_spectra_accum[periodIndex].push_back(m_factory->create(counts[det_index]));
      else
        m_spectra_accum[periodIndex].push_back(m_factory->create(NUM_EVENTS_AVG));
    }
  }

  // the factory is no longer needed so drop the pointer
  m_factory.reset();
}

void ProcessBankCompressed::addEvent(const size_t period_index, const detid_t detid, const float tof) {
  // comparing to integers is cheapest
  if ((detid < m_detid_min) || detid > m_detid_max) {
    // std::cout << "Skipping detid: " << m_detid_min << " < " << detid << " < " << m_detid_max << "\n";
    return; // early
  }

  // check if the tof is within range
  if (((tof - m_tof_min) * (tof - m_tof_max) > 0.)) {
    // std::cout << "Skipping tof: " << tof_f << "\n";
    return; // early
  }

  const auto det_index = static_cast<size_t>(detid - m_detid_min);
  m_spectra_accum[period_index][det_index]->addEvent(tof);
}

void ProcessBankCompressed::collectEvents() {
  Kernel::Timer timer;
  const auto NUM_EVENTS = m_event_detid->size();

  // iterate through all events in a single pulse
  if (m_event_index || m_loader.m_ws.nPeriods() > 1) {
    const auto NUM_PULSES = m_event_index->size();
    const PulseIndexer pulseIndexer(m_event_index, m_firstEventIndex, NUM_EVENTS, m_entry_name);
    for (std::size_t pulseIndex = pulseIndexer.getFirstPulseIndex(); pulseIndex < NUM_PULSES; pulseIndex++) {
      const int logPeriodNumber = m_bankPulseTimes->periodNumber(pulseIndex);
      const auto periodIndex = static_cast<size_t>(logPeriodNumber - 1);

      // determine range of events for the pulse
      const auto eventIndexRange = pulseIndexer.getEventIndexRange(pulseIndex);
      if (eventIndexRange.first > NUM_EVENTS)
        break;
      else if (eventIndexRange.first == eventIndexRange.second)
        continue;

      // add all events in this pulse
      for (std::size_t eventIndex = eventIndexRange.first; eventIndex < eventIndexRange.second; ++eventIndex) {
        this->addEvent(periodIndex, static_cast<detid_t>(m_event_detid->operator[](eventIndex)),
                       m_event_tof->operator[](eventIndex));
      }
    }
  } else {
    // add all events in the list
    for (std::size_t eventIndex = 0; eventIndex < NUM_EVENTS; ++eventIndex) {
      this->addEvent(0, static_cast<detid_t>(m_event_detid->operator[](eventIndex)),
                     m_event_tof->operator[](eventIndex));
    }
  }

  m_event_detid.reset();
  m_event_tof.reset();
  m_event_index.reset();
  m_bankPulseTimes.reset();
  std::cout << "Time to collectEvents:          " << m_entry_name << " " << timer << "\n";
}

/*
 * A side effect of this is that the CompressEventAccumulator is converted to a nullptr after the events have been added
 * to the EventList.
 */
void ProcessBankCompressed::createWeightedEvents(const size_t period_index, const detid_t detid,
                                                 std::vector<Mantid::DataObjects::WeightedEventNoTime> *raw_events) {
  if ((detid < m_detid_min) || detid > m_detid_max) {
    std::stringstream msg;
    msg << "Encountered invalid detid=" << detid;
    throw std::runtime_error(msg.str());
  }

  const auto det_index = static_cast<size_t>(detid - m_detid_min);
  m_spectra_accum[period_index][det_index]->createWeightedEvents(raw_events);

  // let go of the unique_ptr to free up memory
  m_spectra_accum[period_index][det_index].reset();
}

namespace { // anonymous
// private class to allow TBB do a more controlled spreading out of event creation
// since each detid/spectrum/EventList is already independent, these can be done at once
class EventCreationTask {
public:
  EventCreationTask(std::vector<std::unique_ptr<DataHandling::CompressEventAccumulator>> *accumulators,
                    std::vector<std::vector<Mantid::DataObjects::WeightedEventNoTime> *> *eventlists,
                    const detid_t detid_min)
      : m_accumulators(accumulators), m_eventlists(eventlists), m_detid_min(static_cast<size_t>(detid_min)) {}

  void operator()(const tbb::blocked_range<size_t> &range) const {
    for (size_t index = range.begin(); index < range.end(); ++index) {
      auto &accumulator = m_accumulators->operator[](index);
      if (accumulator->m_numevents > 0) {
        // create the events on the correct event list
        m_accumulators->operator[](index)->createWeightedEvents(m_eventlists->operator[](index + m_detid_min));
      }
      // let go of the unique_ptr to free up memory
      m_accumulators->operator[](index).reset();
    }
  }

private:
  std::vector<std::unique_ptr<DataHandling::CompressEventAccumulator>> *m_accumulators;
  std::vector<std::vector<Mantid::DataObjects::WeightedEventNoTime> *> *m_eventlists;
  const size_t m_detid_min;
};

} // namespace

void ProcessBankCompressed::addToEventLists() {
  Kernel::Timer timer;
  const auto num_periods = m_loader.m_ws.nPeriods();
  const auto num_dets = static_cast<size_t>(m_detid_max - m_detid_min + 1);
  // loop over periods
  for (size_t period_index = 0; period_index < num_periods; ++period_index) {
    // create the events and add them to the EventLists
    EventCreationTask create_task(&(m_spectra_accum[period_index]), &m_loader.weightedNoTimeEventVectors[period_index],
                                  m_detid_min);
    // grainsize selected to balance overhead of creating threads with how much work is done in a thread
    constexpr size_t grainsize{20};
    tbb::parallel_for(tbb::blocked_range<size_t>(0, num_dets, grainsize), create_task);
  }

  std::cout << "Time to addToEventLists.append: " << m_entry_name << " " << timer << "\n";
}

void ProcessBankCompressed::run() {
  // timer for performance
  Kernel::Timer timer;
  auto *alg = m_loader.alg;

  this->createAccumulators(m_loader.precount);
  m_prog->report();

  // parse the events
  this->collectEvents();
  m_prog->report(m_entry_name + ": accumulated events");

  // create weighted events on the workspace
  this->addToEventLists();
  m_prog->report(m_entry_name + ": created events");

  /* TODO need to coordinate with accumulators to find out if they were sorted
  // set sort order on all of the EventLists since they were sorted by TOF
  const size_t numEventLists = m_loader.m_ws.getNumberHistograms();
  for (size_t wi = 0; wi < numEventLists; ++wi) {
    auto &eventList = m_loader.m_ws.getSpectrum(wi);
    eventList.setSortOrder(DataObjects::TOF_SORT);
  }
  */

  std::cout << "Time to ProcessBankCompressed   " << m_entry_name << " " << timer << "\n";
  // log performance in debug mode
#ifndef _WIN32
  if (alg->getLogger().isDebug())
    alg->getLogger().debug() << "Time to ProcessBankCompressed " << m_entry_name << " " << timer << "\n";
#endif
} // END-OF-RUN()

double ProcessBankCompressed::totalWeight() const {
  double totalWeightedEvents = std::accumulate(
      m_spectra_accum.cbegin(), m_spectra_accum.cend(), 0., [](const auto &current, const auto &period_spectra) {
        return current + std::accumulate(period_spectra.cbegin(), period_spectra.cend(), 0.,
                                         [](const auto &current, const auto &spectra_accum) {
                                           return current + spectra_accum->totalWeight();
                                         });
      });
  return totalWeightedEvents;
}

} // namespace DataHandling
} // namespace Mantid
