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
  const auto num_dets = static_cast<size_t>(m_detid_max - m_detid_min) + 1;
  const auto bin_mode = (divisor >= 0) ? CompressBinningMode::LINEAR : CompressBinningMode::LOGARITHMIC;

  m_cost = static_cast<double>(m_event_detid->size());

  const auto divisor_abs = abs(divisor);

  // create the spetcra accumulators
  const auto NUM_PERIODS = m_loader.m_ws.nPeriods();
  m_spectra_accum.resize(NUM_PERIODS);
  for (size_t periodIndex = 0; periodIndex < NUM_PERIODS; ++periodIndex) {
    m_spectra_accum[periodIndex].reserve(num_dets);
    for (size_t det_index = 0; det_index <= num_dets; ++det_index) {
      m_spectra_accum[periodIndex].emplace_back(histogram_bin_edges, divisor_abs, bin_mode);
    }
  }
}

void ProcessBankCompressed::addEvent(const size_t period_index, const detid_t detid, const float tof) {
  // comparing to integers is cheapest
  if ((detid < m_detid_min) || detid > m_detid_max) {
    // std::cout << "Skipping detid: " << m_detid_min << " < " << detid << " < " << m_detid_max << "\n";
    return; // early
  }

  // check if the tof is within range
  const auto tof_f = static_cast<float>(tof);
  if (((tof_f - m_tof_min) * (tof_f - m_tof_max) > 0.)) {
    // std::cout << "Skipping tof: " << tof_f << "\n";
    return; // early
  }

  const auto det_index = static_cast<size_t>(detid - m_detid_min);
  m_spectra_accum[period_index][det_index].addEvent(tof_f);
}

void ProcessBankCompressed::collectEvents() {
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
}

void ProcessBankCompressed::createWeightedEvents(const size_t period_index, const detid_t detid,
                                                 std::vector<Mantid::DataObjects::WeightedEventNoTime> *raw_events) {
  if ((detid < m_detid_min) || detid > m_detid_max) {
    std::stringstream msg;
    msg << "Encountered invalid detid=" << detid;
    throw std::runtime_error(msg.str());
  }

  const auto det_index = static_cast<size_t>(detid - m_detid_min);
  m_spectra_accum[period_index][det_index].createWeightedEvents(raw_events);
  // m_spectra_accum[period_index][det_index].clear();
}

namespace { // anonymous
// private class to allow TBB do a more controlled spreading out of sorting
class TofSortingTask {
public:
  TofSortingTask(std::vector<CompressEventAccumulator> *accumulators) : m_accumulators(accumulators) {}

  void operator()(const tbb::blocked_range<size_t> &range) const {
    for (size_t index = range.begin(); index < range.end(); ++index) {
      m_accumulators->operator[](index).sort();
    }
  }

private:
  std::vector<CompressEventAccumulator> *m_accumulators;
};
} // namespace

void ProcessBankCompressed::addToEventLists() {
  const auto num_periods = m_loader.m_ws.nPeriods();
  const auto num_dets = static_cast<size_t>(m_detid_max - m_detid_min + 1);
  // sort all of the tofs
  for (size_t period_index = 0; period_index < num_periods; ++period_index) {
    TofSortingTask task(&(m_spectra_accum[period_index]));
    tbb::parallel_for(tbb::blocked_range<size_t>(0, num_dets), task);
  }

  // add create the events
  for (size_t period_index = 0; period_index < num_periods; ++period_index) {
    for (detid_t detid = m_detid_max; detid >= m_detid_min; --detid) {
      this->createWeightedEvents(period_index, detid,
                                 m_loader.weightedNoTimeEventVectors[period_index][static_cast<size_t>(detid)]);
      m_spectra_accum[period_index].pop_back(); // delete the accumulator
    }
  }
}

void ProcessBankCompressed::run() {
  // timer for performance
  Kernel::Timer timer;
  auto *alg = m_loader.alg;

  m_prog->report();

  // parse the events
  this->collectEvents();
  m_prog->report(m_entry_name + ": accumulated events");

  // create weighted events on the workspace
  this->addToEventLists();
  m_prog->report(m_entry_name + ": created events");

  // set sort order on all of the EventLists since they were sorted by TOF
  const size_t numEventLists = m_loader.m_ws.getNumberHistograms();
  for (size_t wi = 0; wi < numEventLists; ++wi) {
    auto &eventList = m_loader.m_ws.getSpectrum(wi);
    eventList.setSortOrder(DataObjects::TOF_SORT);
  }

  // log performance in debug mode
#ifndef _WIN32
  if (alg->getLogger().isDebug())
    alg->getLogger().debug() << "Time to ProcessBankData " << m_entry_name << " " << timer << "\n";
#endif
} // END-OF-RUN()

double ProcessBankCompressed::totalWeight() const {
  double totalWeightedEvents = std::accumulate(
      m_spectra_accum.cbegin(), m_spectra_accum.cend(), 0., [](const auto &current, const auto &period_spectra) {
        return current + std::accumulate(period_spectra.cbegin(), period_spectra.cend(), 0.,
                                         [](const auto &current, const auto &spectra_accum) {
                                           return current + spectra_accum.totalWeight();
                                         });
      });
  return totalWeightedEvents;
}

} // namespace DataHandling
} // namespace Mantid
