#include "MantidDataHandling/ProcessBankData.h"

using namespace Mantid::DataObjects;

namespace Mantid {
namespace DataHandling {

ProcessBankData::ProcessBankData(
    LoadEventNexus *alg, std::string entry_name, API::Progress *prog,
    boost::shared_array<uint32_t> event_id,
    boost::shared_array<float> event_time_of_flight, size_t numEvents,
    size_t startAt, boost::shared_ptr<std::vector<uint64_t>> event_index,
    boost::shared_ptr<BankPulseTimes> thisBankPulseTimes, bool have_weight,
    boost::shared_array<float> event_weight, detid_t min_event_id,
    detid_t max_event_id)
    : Task(), alg(alg), entry_name(entry_name),
      pixelID_to_wi_vector(alg->pixelID_to_wi_vector),
      pixelID_to_wi_offset(alg->pixelID_to_wi_offset), prog(prog),
      event_id(event_id), event_time_of_flight(event_time_of_flight),
      numEvents(numEvents), startAt(startAt), event_index(event_index),
      thisBankPulseTimes(thisBankPulseTimes), have_weight(have_weight),
      event_weight(event_weight), m_min_id(min_event_id),
      m_max_id(max_event_id) {
  // Cost is approximately proportional to the number of events to process.
  m_cost = static_cast<double>(numEvents);
}

/** Pre-allocate the events-vector size for each spectrum
 * @brief ProcessBankData::preCount
 */
void ProcessBankData::preAllocate() {
  auto &outputWS = *(alg->m_ws);

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
      // Find the the workspace index corresponding to that pixel ID
      size_t wi = pixelID_to_wi_vector[pixID + pixelID_to_wi_offset];
      // Allocate it
      if (wi < numEventLists) {
        outputWS.reserveEventListAt(wi, counts[pixID - m_min_id]);
      }
      if (alg->getCancel())
        break; // User cancellation
    }
  }
}

/**
 * @brief ProcessBankData::processEvents
 * @param pulsetimesincreasing
 * @param my_discarded_events
 * @param my_shortest_tof
 * @param my_longest_tof
 * @param badTofs
 * @param compress
 * @param usedDetIds
 */
void ProcessBankData::processEvents(bool &pulsetimesincreasing,
                                    size_t &my_discarded_events,
                                    double &my_shortest_tof,
                                    double &my_longest_tof, size_t &badTofs,
                                    bool compress,
                                    std::vector<bool> &usedDetIds) {
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

  // Default pulse time (if none are found)
  Mantid::Kernel::DateAndTime pulsetime;
  int periodNumber = 1;
  int periodIndex = 0;
  Mantid::Kernel::DateAndTime lastpulsetime(0);

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
      periodNumber = logPeriodNumber > 0
                         ? logPeriodNumber
                         : periodNumber; // Some historic files have recorded
                                         // their logperiod numbers as zeros!
      periodIndex = periodNumber - 1;

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
          LoadEventNexus::WeightedEventVector_pt eventVector =
              alg->weightedEventVectors[periodIndex][detId];
          // NULL eventVector indicates a bad spectrum lookup
          if (eventVector) {
            eventVector->emplace_back(tof, pulsetime, weight, errorSq);
          } else {
            ++my_discarded_events;
          }
        } else {
          // We have cached the vector of events for this detector ID
          std::vector<Mantid::DataObjects::TofEvent> *eventVector =
              alg->eventVectors[periodIndex][detId];
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
}

/** compress events or set the order of events in a sepctrum
 * @brief ProcessBankData::compressEvents
 * @param compress
 * @param usedDetIds
 */
void ProcessBankData::compressEvents(bool compress,
                                     const std::vector<bool> &usedDetIds) {
  auto &outputWS = *(alg->m_ws);

  // Do it on all the detector IDs we touched
  if (compress) {
    for (detid_t pixID = m_min_id; pixID <= m_max_id; pixID++) {
      if (usedDetIds[pixID - m_min_id]) {
        // Find the the workspace index corresponding to that pixel ID
        size_t wi = pixelID_to_wi_vector[pixID + pixelID_to_wi_offset];
        auto &el = outputWS.getSpectrum(wi);

        el.compressEvents(alg->compressTolerance, &el);
      }
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Run the data processing on a subset of pixel IDs
*/
void ProcessBankData::run() {
  // Local tof limits
  double my_shortest_tof =
      static_cast<double>(std::numeric_limits<uint32_t>::max()) * 0.1;
  double my_longest_tof = 0.;
  // A count of "bad" TOFs that were too high
  size_t badTofs = 0;
  size_t my_discarded_events(0);
  prog->report(entry_name + ": precount");

  // Pre-counting events per pixel ID and allocate the memory ----
  // auto &outputWS = *(alg->m_ws);
  if (alg->precount) {
    preAllocate();
  }

  // Check for canceled algorithm
  if (alg->getCancel()) {
    return;
  }

  bool pulsetimesincreasing = true;

  prog->report(entry_name + ": filling events");

  // Will we need to compress?
  bool compress = (alg->compressTolerance >= 0);

  // Which detector IDs were touched? - only matters if compress is on
  std::vector<bool> usedDetIds;
  if (compress)
    usedDetIds.assign(m_max_id - m_min_id + 1, false);

  // Go through all events in the list
  // NEW NEW NEW .............................................
  processEvents(pulsetimesincreasing, my_discarded_events, my_shortest_tof,
                my_longest_tof, badTofs, compress, usedDetIds);

  //------------ Compress Events (or set sort order) ------------------
  compressEvents(compress, usedDetIds);

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

} // namespace Mantid{
} // namespace DataHandling{
