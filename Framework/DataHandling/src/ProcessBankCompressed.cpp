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

  m_cost = static_cast<double>(m_event_detid->size());

  // setup the vector with sorting information
  const auto NUM_DETS = static_cast<size_t>(m_detid_max - m_detid_min) + 1;
  m_sorting.clear();
  m_sorting.resize(NUM_DETS, DataObjects::UNSORTED);

  // create the spetcra accumulators
  const auto bin_mode = (divisor >= 0) ? CompressBinningMode::LINEAR : CompressBinningMode::LOGARITHMIC;
  const auto divisor_abs = abs(divisor);
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

void ProcessBankCompressed::addEvent(const size_t period_index, const size_t event_index) {
  // comparing to integers is cheapest
  const auto detid = static_cast<detid_t>(m_event_detid->operator[](event_index));
  if ((detid < m_detid_min) || (detid > m_detid_max)) {
    return;
  }

  // check if the tof is within range
  const auto tof = m_event_tof->operator[](event_index);
  if (((tof - m_tof_min) * (tof - m_tof_max) > 0.)) {
    return;
  }

  // accumulators are zero indexed
  const auto det_index = static_cast<size_t>(detid - m_detid_min);
  m_spectra_accum[period_index][det_index]->addEvent(tof);
}

void ProcessBankCompressed::collectEvents() {
  Kernel::Timer timer;
  const auto NUM_EVENTS = m_event_detid->size();

  const auto *alg = m_loader.alg;

  // iterate through all events in a single pulse
  if (m_event_index || m_loader.m_ws.nPeriods() > 1 || alg->m_is_time_filtered || alg->filter_bad_pulses) {
    // set up wall-clock filtering if it was requested
    std::vector<size_t> pulseROI;
    if (alg->m_is_time_filtered) {
      pulseROI = m_bankPulseTimes->getPulseIndices(alg->filter_time_start, alg->filter_time_stop);
    }

    if (alg->filter_bad_pulses) {
      pulseROI = Mantid::Kernel::ROI::calculate_intersection(
          pulseROI, m_bankPulseTimes->getPulseIndices(alg->bad_pulses_timeroi->toTimeIntervals()));
    }

    const PulseIndexer pulseIndexer(m_event_index, m_firstEventIndex, NUM_EVENTS, m_entry_name, pulseROI);
    for (const auto &pulseIter : pulseIndexer) {
      const int logPeriodNumber = m_bankPulseTimes->periodNumber(pulseIter.pulseIndex);
      const auto periodIndex = static_cast<size_t>(logPeriodNumber - 1);

      // add all events in this pulse
      for (std::size_t eventIndex = pulseIter.eventIndexStart; eventIndex < pulseIter.eventIndexStop; ++eventIndex) {
        this->addEvent(periodIndex, eventIndex);
      }
    }
  } else {
    // add all events in the list which are all in the first period
    constexpr size_t periodIndex{0};
    for (std::size_t eventIndex = 0; eventIndex < NUM_EVENTS; ++eventIndex) {
      this->addEvent(periodIndex, eventIndex);
    }
  }

  m_event_detid.reset();
  m_event_tof.reset();
  m_event_index.reset();
  m_bankPulseTimes.reset();

#ifndef _WIN32
  if (m_loader.alg->getLogger().isDebug())
    m_loader.alg->getLogger().debug() << "Time to collectEvents: " << m_entry_name << " " << timer << "\n";
#endif
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
                    const detid_t detid_min, std::vector<DataObjects::EventSortType> *sorting)
      : m_accumulators(accumulators), m_eventlists(eventlists), m_sorting(sorting),
        m_detid_min(static_cast<size_t>(detid_min)) {}

  void operator()(const tbb::blocked_range<size_t> &range) const {
    for (size_t index = range.begin(); index < range.end(); ++index) {
      auto &accumulator = m_accumulators->operator[](index);
      if (accumulator->totalWeight() > 0.) {
        auto *raw_events = m_eventlists->operator[](index + m_detid_min);
        // create the events on the correct event list
        m_accumulators->operator[](index)->createWeightedEvents(raw_events);
        // drop extra space if the capacity is more than 10% of what is needed
        if (static_cast<double>(raw_events->capacity()) > 1.1 * static_cast<double>(raw_events->size()))
          raw_events->shrink_to_fit();
      }
      // get the sorting type back
      m_sorting->operator[](index) = m_accumulators->operator[](index)->getSortType();
      // let go of the unique_ptr to free up memory
      m_accumulators->operator[](index).reset();
    }
  }

private:
  std::vector<std::unique_ptr<DataHandling::CompressEventAccumulator>> *m_accumulators;
  std::vector<std::vector<Mantid::DataObjects::WeightedEventNoTime> *> *m_eventlists;
  std::vector<DataObjects::EventSortType> *m_sorting;
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
                                  m_detid_min, &m_sorting);
    // grainsize selected to balance overhead of creating threads with how much work is done in a thread
    const size_t grainsize = std::min<size_t>(20, num_dets / 20);
    tbb::parallel_for(tbb::blocked_range<size_t>(0, num_dets, grainsize), create_task);
  }

#ifndef _WIN32
  if (m_loader.alg->getLogger().isDebug())
    m_loader.alg->getLogger().debug() << "Time to addToEventLists.append: " << m_entry_name << " " << timer << "\n";
#endif
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

  // TODO need to coordinate with accumulators to find out if they were sorted
  // set sort order on all of the EventLists since they were sorted by TOF
  const auto pixelID_to_wi_offset = m_loader.pixelID_to_wi_offset;
  auto &outputWS = m_loader.m_ws;
  const size_t numEventLists = m_loader.m_ws.getNumberHistograms();
  const detid_t pixelIDtoWSVec_size = static_cast<detid_t>(m_loader.pixelID_to_wi_vector.size());
  for (detid_t detid = m_detid_min; detid <= m_detid_max; ++detid) {
    const detid_t detid_offset = detid + pixelID_to_wi_offset;
    if (!(detid_offset < 0 || detid_offset >= pixelIDtoWSVec_size)) {
      const auto wi = m_loader.pixelID_to_wi_vector[detid_offset];
      if (wi < numEventLists) {
        const auto sortOrder = m_sorting[static_cast<size_t>(detid - m_detid_min)];
        outputWS.getSpectrum(wi).setSortOrder(sortOrder);
      }
    }
  }

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
