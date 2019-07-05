// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/ProcessBankData.h"
#include "MantidDataHandling/DefaultEventLoader.h"
#include "MantidDataHandling/LoadEventNexus.h"

using namespace Mantid::DataObjects;

namespace Mantid {
namespace DataHandling {

ProcessBankData::ProcessBankData(
    DefaultEventLoader &m_loader, std::string entry_name, API::Progress *prog,
    boost::shared_array<uint32_t> event_id,
    boost::shared_array<float> event_time_of_flight, size_t numEvents,
    size_t startAt, boost::shared_ptr<std::vector<uint64_t>> event_index,
    boost::shared_ptr<BankPulseTimes> thisBankPulseTimes, bool have_weight,
    boost::shared_array<float> event_weight, detid_t min_event_id,
    detid_t max_event_id)
    : Task(), m_loader(m_loader), entry_name(entry_name),
      pixelID_to_wi_vector(m_loader.pixelID_to_wi_vector),
      pixelID_to_wi_offset(m_loader.pixelID_to_wi_offset), prog(prog),
      event_id(event_id), event_time_of_flight(event_time_of_flight),
      numEvents(numEvents), startAt(startAt), event_index(event_index),
      thisBankPulseTimes(thisBankPulseTimes), have_weight(have_weight),
      event_weight(event_weight), m_min_id(min_event_id),
      m_max_id(max_event_id) {
  // Cost is approximately proportional to the number of events to process.
  m_cost = static_cast<double>(numEvents);
}

/** Run the data processing
 * FIXME/TODO - split run() into readable methods
 */
void ProcessBankData::run() { // override {
  // Local tof limits
  double my_shortest_tof =
      static_cast<double>(std::numeric_limits<uint32_t>::max()) * 0.1;
  double my_longest_tof = 0.;
  // A count of "bad" TOFs that were too high
  size_t badTofs = 0;
  size_t my_discarded_events(0);

  prog->report(entry_name + ": precount");
  // ---- Pre-counting events per pixel ID ----
  auto &outputWS = m_loader.m_ws;
  auto *alg = m_loader.alg;
  if (m_loader.precount) {

    std::vector<size_t> counts(m_max_id - m_min_id + 1, 0);
    for (size_t i = 0; i < numEvents; i++) {
      detid_t thisId = detid_t(event_id[i]);
      if (thisId >= m_min_id && thisId <= m_max_id)
        counts[thisId - m_min_id]++;
    }

    // Now we pre-allocate (reserve) the vectors of events in each pixel
    // counted
    const size_t numEventLists = outputWS.getNumberHistograms();
    for (detid_t pixID = m_min_id; pixID <= m_max_id; pixID++) {
      if (counts[pixID - m_min_id] > 0) {
        size_t wi = getWorkspaceIndexFromPixelID(pixID);
        // Find the the workspace index corresponding to that pixel ID
        // Allocate it
        if (wi < numEventLists) {
          outputWS.reserveEventListAt(wi, counts[pixID - m_min_id]);
        }
        if (alg->getCancel())
          break; // User cancellation
      }
    }
  }

  // Check for canceled algorithm
  if (alg->getCancel()) {
    return;
  }

  // Default pulse time (if none are found)
  Mantid::Types::Core::DateAndTime pulsetime;
  int periodIndex = 0;
  Mantid::Types::Core::DateAndTime lastpulsetime(0);

  bool pulsetimesincreasing = true;

  // Index into the pulse array
  int pulse_i = 0;

  // And there are this many pulses
  int numPulses = static_cast<int>(thisBankPulseTimes->numPulses);
  if (numPulses > static_cast<int>(event_index->size())) {
    alg->getLogger().warning()
        << "Entry " << entry_name
        << "'s event_index vector is smaller than the event_time_zero field. "
           "This is inconsistent, so we cannot find pulse times for this "
           "entry.\n";
    // This'll make the code skip looking for any pulse times.
    pulse_i = numPulses + 1;
  }

  prog->report(entry_name + ": filling events");

  // Will we need to compress?
  bool compress = (alg->compressTolerance >= 0);

  // Which detector IDs were touched? - only matters if compress is on
  std::vector<bool> usedDetIds;
  if (compress)
    usedDetIds.assign(m_max_id - m_min_id + 1, false);

  // Go through all events in the list
  for (std::size_t i = 0; i < numEvents; i++) {
    //------ Find the pulse time for this event index ---------
    if (pulse_i < numPulses - 1) {
      bool breakOut = false;
      // Go through event_index until you find where the index increases to
      // encompass the current index. Your pulse = the one before.
      while ((i + startAt < event_index->operator[](pulse_i)) ||
             (i + startAt >= event_index->operator[](pulse_i + 1))) {
        pulse_i++;
        // Check once every new pulse if you need to cancel (checking on every
        // event might slow things down more)
        if (alg->getCancel())
          breakOut = true;
        if (pulse_i >= (numPulses - 1))
          break;
      }

      // Save the pulse time at this index for creating those events
      pulsetime = thisBankPulseTimes->pulseTimes[pulse_i];
      int logPeriodNumber = thisBankPulseTimes->periodNumbers[pulse_i];
      periodIndex = logPeriodNumber - 1;

      // Determine if pulse times continue to increase
      if (pulsetime < lastpulsetime)
        pulsetimesincreasing = false;
      else
        lastpulsetime = pulsetime;

      // Flag to break out of the event loop without using goto
      if (breakOut)
        break;
    }

    // We cached a pointer to the vector<tofEvent> -> so retrieve it and add
    // the event
    detid_t detId = event_id[i];
    if (detId >= m_min_id && detId <= m_max_id) {
      // Create the tofevent
      double tof = static_cast<double>(event_time_of_flight[i]);
      if ((tof >= alg->filter_tof_min) && (tof <= alg->filter_tof_max)) {
        // Handle simulated data if present
        if (have_weight) {
          double weight = static_cast<double>(event_weight[i]);
          double errorSq = weight * weight;
          auto *eventVector = m_loader.weightedEventVectors[periodIndex][detId];
          // NULL eventVector indicates a bad spectrum lookup
          if (eventVector) {
            eventVector->emplace_back(tof, pulsetime, weight, errorSq);
          } else {
            ++my_discarded_events;
          }
        } else {
          // We have cached the vector of events for this detector ID
          auto *eventVector = m_loader.eventVectors[periodIndex][detId];
          // NULL eventVector indicates a bad spectrum lookup
          if (eventVector) {
            eventVector->emplace_back(tof, pulsetime);
          } else {
            ++my_discarded_events;
          }
        }

        // Local tof limits
        if (tof < my_shortest_tof) {
          my_shortest_tof = tof;
        }
        // Skip any events that are the cause of bad DAS data (e.g. a negative
        // number in uint32 -> 2.4 billion * 100 nanosec = 2.4e8 microsec)
        if (tof < 2e8) {
          if (tof > my_longest_tof) {
            my_longest_tof = tof;
          }
        } else
          badTofs++;

        // Track all the touched wi (only necessary when compressing events,
        // for thread safety)
        if (compress)
          usedDetIds[detId - m_min_id] = true;
      } // valid time-of-flight

    } // valid detector IDs
  }   //(for each event)

  //------------ Compress Events (or set sort order) ------------------
  // Do it on all the detector IDs we touched
  if (compress) {
    for (detid_t pixID = m_min_id; pixID <= m_max_id; pixID++) {
      if (usedDetIds[pixID - m_min_id]) {
        // Find the the workspace index corresponding to that pixel ID
        size_t wi = getWorkspaceIndexFromPixelID(pixID);
        auto &el = outputWS.getSpectrum(wi);
        if (compress)
          el.compressEvents(alg->compressTolerance, &el);
        else {
          if (pulsetimesincreasing)
            el.setSortOrder(DataObjects::PULSETIME_SORT);
          else
            el.setSortOrder(DataObjects::UNSORTED);
        }
      }
    }
  }
  prog->report(entry_name + ": filled events");

  alg->getLogger().debug() << entry_name
                           << (pulsetimesincreasing ? " had "
                                                    : " DID NOT have ")
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
  alg->getLogger().debug() << "Time to process " << entry_name << " " << m_timer
                           << "\n";
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
  if (offset_pixID < 0 ||
      offset_pixID >= static_cast<int32_t>(pixelID_to_wi_vector.size())) {
    std::stringstream msg;
    msg << "Error finding workspace index; pixelID " << pixID << " with offset "
        << pixelID_to_wi_offset
        << " is out of range (length=" << pixelID_to_wi_vector.size() << ")";
    throw std::runtime_error(msg.str());
  }
  return pixelID_to_wi_vector[offset_pixID];
}
} // namespace DataHandling
} // namespace Mantid
