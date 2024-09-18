// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <utility>

#include "MantidDataHandling/DefaultEventLoader.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidDataHandling/ProcessBankData.h"
#include "MantidDataHandling/PulseIndexer.h"
#include "MantidKernel/Timer.h"

using namespace Mantid::DataObjects;

namespace Mantid::DataHandling {

ProcessBankData::ProcessBankData(DefaultEventLoader &m_loader, const std::string &entry_name, API::Progress *prog,
                                 std::shared_ptr<std::vector<uint32_t>> event_id,
                                 std::shared_ptr<std::vector<float>> event_time_of_flight, size_t numEvents,
                                 size_t startAt, std::shared_ptr<std::vector<uint64_t>> event_index,
                                 std::shared_ptr<BankPulseTimes> thisBankPulseTimes, bool have_weight,
                                 std::shared_ptr<std::vector<float>> event_weight, detid_t min_event_id,
                                 detid_t max_event_id)
    : Task(), m_loader(m_loader), entry_name(std::move(entry_name)),
      pixelID_to_wi_vector(m_loader.pixelID_to_wi_vector), pixelID_to_wi_offset(m_loader.pixelID_to_wi_offset),
      prog(prog), event_detid(std::move(event_id)), event_time_of_flight(std::move(event_time_of_flight)),
      numEvents(numEvents), startAt(startAt), event_index(std::move(event_index)),
      thisBankPulseTimes(std::move(thisBankPulseTimes)), have_weight(have_weight),
      event_weight(std::move(event_weight)), m_min_detid(min_event_id), m_max_detid(max_event_id) {
  // Cost is approximately proportional to the number of events to process.
  m_cost = static_cast<double>(numEvents);

  if (m_max_detid < m_min_detid) {
    std::stringstream msg;
    msg << "max detid (" << m_max_detid << ") < min (" << m_min_detid << ")";
    throw std::runtime_error(msg.str());
  }
}

/*
 * Pre-counting the events per pixel ID allows for allocating the proper amount of memory in each output event vector
 */
void ProcessBankData::preCountAndReserveMem() {
  // ---- Pre-counting events per pixel ID ----
  std::vector<size_t> counts(m_max_detid - m_min_detid + 1, 0);
  for (size_t i = 0; i < numEvents; i++) {
    const auto thisId = static_cast<detid_t>((*event_detid)[i]);
    if (!(thisId < m_min_detid || thisId > m_max_detid)) // or allows for skipping out early
      counts[thisId - m_min_detid]++;
  }

  // Now we pre-allocate (reserve) the vectors of events in each pixel
  // counted
  auto &outputWS = m_loader.m_ws;
  const auto *alg = m_loader.alg;
  const size_t numEventLists = outputWS.getNumberHistograms();
  for (detid_t pixID = m_min_detid; pixID <= m_max_detid; ++pixID) {
    const auto pixelIndex = pixID - m_min_detid; // index from zero
    if (counts[pixelIndex] > 0) {
      const size_t wi = getWorkspaceIndexFromPixelID(pixID);
      // Find the workspace index corresponding to that pixel ID
      // Allocate it
      if (wi < numEventLists) {
        outputWS.reserveEventListAt(wi, counts[pixelIndex]);
      }
      if ((wi % 20 == 0) && alg->getCancel())
        return; // User cancellation
    }
  }
}

/** Run the data processing
 * FIXME/TODO - split run() into readable methods
 */
void ProcessBankData::run() {
  // timer for performance
  Mantid::Kernel::Timer timer;

  // Local tof limits
  double my_shortest_tof = static_cast<double>(std::numeric_limits<uint32_t>::max()) * 0.1;
  double my_longest_tof = 0.;
  // A count of "bad" TOFs that were too high
  size_t badTofs = 0;
  size_t my_discarded_events(0);

  prog->report(entry_name + ": precount");
  // ---- Pre-counting events per pixel ID ----
  if (m_loader.precount) {
    this->preCountAndReserveMem();
    if (m_loader.alg->getCancel())
      return; // User cancellation
  }

  // this assumes that pulse indices are sorted
  if (!std::is_sorted(event_index->cbegin(), event_index->cend()))
    throw std::runtime_error("Event index is not sorted");

  // And there are this many pulses
  prog->report(entry_name + ": filling events");

  auto *alg = m_loader.alg;

  // Will we need to compress?
  const bool compress = (alg->compressEvents);

  // Which detector IDs were touched?
  std::vector<bool> usedDetIds(m_max_detid - m_min_detid + 1, false);

  const double TOF_MIN = alg->filter_tof_min;
  const double TOF_MAX = alg->filter_tof_max;
  const bool NO_TOF_FILTERING = !(alg->filter_tof_range);

  // set up wall-clock filtering if it was requested
  std::vector<size_t> pulseROI;
  if (alg->m_is_time_filtered) {
    pulseROI = thisBankPulseTimes->getPulseIndices(alg->filter_time_start, alg->filter_time_stop);
  }

  if (alg->filter_bad_pulses) {
    pulseROI = Mantid::Kernel::ROI::calculate_intersection(
        pulseROI, thisBankPulseTimes->getPulseIndices(alg->bad_pulses_timeroi->toTimeIntervals()));
  }

  const PulseIndexer pulseIndexer(event_index, startAt, numEvents, entry_name, pulseROI);

  // loop over all pulses
  for (const auto &pulseIter : pulseIndexer) {
    // Save the pulse time at this index for creating those events
    const auto &pulsetime = thisBankPulseTimes->pulseTime(pulseIter.pulseIndex);
    const int logPeriodNumber = thisBankPulseTimes->periodNumber(pulseIter.pulseIndex);
    const auto periodIndex = static_cast<size_t>(logPeriodNumber - 1);

    // loop through events associated with a single pulse
    for (std::size_t eventIndex = pulseIter.eventIndexStart; eventIndex < pulseIter.eventIndexStop; ++eventIndex) {
      // We cached a pointer to the vector<tofEvent> -> so retrieve it and add
      // the event
      const detid_t &detId = static_cast<detid_t>((*event_detid)[eventIndex]);
      if (detId >= m_min_detid && detId <= m_max_detid) {
        // Create the tofevent
        const auto tof = static_cast<double>((*event_time_of_flight)[eventIndex]);
        // this is fancy for check if value is in range
        if ((NO_TOF_FILTERING) || ((tof - TOF_MIN) * (tof - TOF_MAX) <= 0.)) {
          // Handle simulated data if present
          if (have_weight) {
            auto *eventVector = m_loader.weightedEventVectors[periodIndex][detId];
            // NULL eventVector indicates a bad spectrum lookup
            if (eventVector) {
              const auto weight = static_cast<double>((*event_weight)[eventIndex]);
              const double errorSq = weight * weight;
              eventVector->emplace_back(tof, pulsetime, weight, errorSq);
            } else {
              ++my_discarded_events;
            }
          } else {
            // We have cached the vector of events for this detector ID
            auto *eventVector = m_loader.eventVectors[periodIndex][detId];
            // NULL eventVector indicates a bad spectrum lookup
            if (eventVector) {
              eventVector->emplace_back(std::move(tof), pulsetime);
            } else {
              ++my_discarded_events;
            }
          }

          // Skip any events that are the cause of bad DAS data (e.g. a negative
          // number in uint32 -> 2.4 billion * 100 nanosec = 2.4e8 microsec)
          if (tof < 2e8) {
            // tof limits from things observed here
            if (tof > my_longest_tof) {
              my_longest_tof = tof;
            }
            if (tof < my_shortest_tof) {
              my_shortest_tof = tof;
            }
          } else
            badTofs++;

          // Track all the touched wi
          const auto detidIndex = detId - m_min_detid;
          if (!usedDetIds[detidIndex])
            usedDetIds[detidIndex] = true;
        } // valid time-of-flight

      } // valid detector IDs
    }   // for events in pulse
    // check if cancelled after each 100s of pulses (assumes 60Hz)
    if ((pulseIter.pulseIndex % 6000 == 0) && alg->getCancel())
      return;
  } // for pulses

  // Default pulse time (if none are found)
  const auto pulseSortingType =
      thisBankPulseTimes->arePulseTimesIncreasing() ? DataObjects::PULSETIME_SORT : DataObjects::UNSORTED;

  //------------ Compress Events (or set sort order) ------------------
  // Do it on all the detector IDs we touched
  auto &outputWS = m_loader.m_ws;
  const size_t numEventLists = outputWS.getNumberHistograms();
  for (detid_t pixID = m_min_detid; pixID <= m_max_detid; ++pixID) {
    if (usedDetIds[pixID - m_min_detid]) {
      // Find the workspace index corresponding to that pixel ID
      size_t wi = getWorkspaceIndexFromPixelID(pixID);
      if (wi < numEventLists) {
        auto &el = outputWS.getSpectrum(wi);
        // set the sort order based on what is known
        el.setSortOrder(pulseSortingType);
        // compress events if requested
        if (compress)
          el.compressEvents(alg->compressTolerance, &el);
      }
    }
  }
  prog->report(entry_name + ": filled events");

  alg->getLogger().debug() << entry_name << (thisBankPulseTimes->arePulseTimesIncreasing() ? " had " : " DID NOT have ")
                           << "monotonically increasing pulse times\n";

  // Join back up the tof limits to the global ones
  // This is not thread safe, so only one thread at a time runs this.
  {
    std::lock_guard<std::mutex> _lock(alg->m_tofMutex);
    if (my_shortest_tof < alg->shortest_tof) {
      alg->shortest_tof = my_shortest_tof;
    }
    if (my_longest_tof > alg->longest_tof) {
      alg->longest_tof = my_longest_tof;
    }
    alg->bad_tofs += badTofs;
    alg->discarded_events += my_discarded_events;
  }

#ifndef _WIN32
  if (alg->getLogger().isDebug())
    alg->getLogger().debug() << "Time to ProcessBankData " << entry_name << " " << timer << "\n";
#endif
} // END-OF-RUN()

/**
 * Get the workspace index for a given pixel ID. Throws if the pixel ID is
 * not in the expected range.
 *
 * @param pixID :: The pixel ID to look up
 * @return The workspace index for this pixel
 */
size_t ProcessBankData::getWorkspaceIndexFromPixelID(const detid_t pixID) {
  // Check that the vector index is not out of range
  const detid_t offset_pixID = pixID + pixelID_to_wi_offset;
  if (offset_pixID < 0 || offset_pixID >= static_cast<int32_t>(pixelID_to_wi_vector.size())) {
    std::stringstream msg;
    msg << "Error finding workspace index; pixelID " << pixID << " with offset " << pixelID_to_wi_offset
        << " is out of range (length=" << pixelID_to_wi_vector.size() << ")";
    throw std::runtime_error(msg.str());
  }
  return pixelID_to_wi_vector[offset_pixID];
}
} // namespace Mantid::DataHandling
