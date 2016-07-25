//==============================================================================================
// Class LoadBankFromDiskTask
//==============================================================================================
#include "MantidDataHandling/LoadEventNexusHelper.h"
#include "MantidAPI/Progress.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/Events.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Task.h"
#include "MantidKernel/ThreadScheduler.h"

using namespace Mantid;
using namespace Mantid::DataObjects;

//==============================================================================================
// Class ProcessBankData
//==============================================================================================
/** Constructor
*
* @param alg :: LoadEventNexus
* @param entry_name :: name of the bank
* @param prog :: Progress reporter
* @param event_id :: array with event IDs
* @param event_time_of_flight :: array with event TOFS
* @param numEvents :: how many events in the arrays
* @param startAt :: index of the first event from event_index
* @param event_index :: vector of event index (length of # of pulses)
* @param thisBankPulseTimes :: ptr to the pulse times for this particular
*bank.
* @param have_weight :: flag for handling simulated files
* @param event_weight :: array with weights for events
* @param min_event_id ;: minimum detector ID to load
* @param max_event_id :: maximum detector ID to load
* @return
*/
ProcessBankData::ProcessBankData(
    Mantid::DataHandling::LoadEventNexus *alg, std::string entry_name,
    Mantid::API::Progress *prog, boost::shared_array<uint32_t> event_id,
    boost::shared_array<float> event_time_of_flight, size_t numEvents,
    size_t startAt, boost::shared_ptr<std::vector<uint64_t>> event_index,
    boost::shared_ptr<Mantid::DataHandling::BankPulseTimes> thisBankPulseTimes,
    bool have_weight, boost::shared_array<float> event_weight,
    detid_t min_event_id, detid_t max_event_id)
    : alg(alg), entry_name(entry_name),
      pixelID_to_wi_vector(alg->pixelID_to_wi_vector),
      pixelID_to_wi_offset(alg->pixelID_to_wi_offset), prog(prog),
      event_id(event_id), event_time_of_flight(event_time_of_flight),
      numEvents(numEvents), startAt(startAt), event_index(event_index),
      thisBankPulseTimes(thisBankPulseTimes), have_weight(have_weight),
      event_weight(event_weight), m_min_id(min_event_id),
      m_max_id(max_event_id) {}

//----------------------------------------------------------------------------------------------
/** compress events for each event lists
 * @brief ProcessBankData::compressEvents
 * @param compress
 * @param pulsetimesincreasing
 * @param usedDetIds
 */
void ProcessBankData::compressEvents(bool compress, bool pulsetimesincreasing,
                                     const std::vector<bool> &usedDetIds) {
  // return if it is not selected to be compressed
  if (!compress)
    return;

  auto &outputWS = *(alg->m_ws);

  for (detid_t pixID = m_min_id; pixID <= m_max_id; pixID++) {
    if (usedDetIds[pixID - m_min_id]) {
      // Find the the workspace index corresponding to that pixel ID
      size_t wi = pixelID_to_wi_vector[pixID + pixelID_to_wi_offset];
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

  return;
}

//----------------------------------------------------------------------------------------------
// Pre-counting events per pixel ID and allocate the memory for each spectrum's
// events list
void ProcessBankData::precountEvents() {
  // return if LoadEventNexus is not selected to pre-count events
  if (!alg->precount)
    return;

  auto &outputWS = *(alg->m_ws);

  // create count vector with size of detector IDs
  std::vector<size_t> counts(m_max_id - m_min_id + 1, 0);

  // register the counts of events on each
  for (size_t i = 0; i < numEvents; i++) {
    detid_t pixID = detid_t(event_id[i]);
    if (pixID >= m_min_id && pixID <= m_max_id)
      counts[pixID - m_min_id]++;
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

  return;
}

void ProcessBankData::processEvents(int numPulses, int pulse_i, bool compress,
                                    bool &pulsetimesincreasing,
                                    double &my_shortest_tof,
                                    double &my_longest_tof, size_t &badTofs,
                                    size_t &my_discarded_events,
                                    std::vector<bool> &usedDetIds) {

  // Default pulse time (if none are found)
  Mantid::Kernel::DateAndTime pulsetime;
  int periodNumber = 1;
  int periodIndex = 0;
  Mantid::Kernel::DateAndTime lastpulsetime(0);

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
          Mantid::DataHandling::LoadEventNexus::WeightedEventVector_pt
              eventVector = alg->weightedEventVectors[periodIndex][detId];
          // NULL eventVector indicates a bad spectrum lookup
          if (eventVector) {
            // This avoids a copy constructor call but is only available with
            // GCC (requires variadic templates)
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
            // This avoids a copy constructor call but is only available with
            // GCC (requires variadic templates)
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

//----------------------------------------------------------------------------------------------
/** Run the data processing
*/
void ProcessBankData::run() {
  // Local tof limits
  double my_shortest_tof =
      static_cast<double>(std::numeric_limits<uint32_t>::max()) * 0.1;
  double my_longest_tof = 0.;
  // A count of "bad" TOFs that were too high
  size_t badTofs = 0;
  size_t my_discarded_events(0);

  // pre-count events to reserve memeory of events lists
  prog->report(entry_name + ": precount");
  this->precountEvents();
  // Check for canceled algorithm
  if (alg->getCancel()) {
    return;
  }

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

  processEvents(numPulses, pulse_i, compress, pulsetimesincreasing,
                my_shortest_tof, my_longest_tof, badTofs, my_discarded_events,
                usedDetIds);

  //------------ Compress Events (or set sort order) ------------------
  // Do it on all the detector IDs we touched
  compressEvents(compress, pulsetimesincreasing, usedDetIds);

  prog->report(entry_name + ": filled events");

  alg->getLogger().debug() << entry_name
                           << (pulsetimesincreasing ? " had "
                                                    : " DID NOT have ")
                           << "monotonically increasing pulse times"
                           << std::endl;

  // Join back up the tof limits to the global ones
  // This is not thread safe, so only one thread at a time runs this.
  {
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
} // END (run)

//---------------------------------------------------------------------------------------------------
/** Constructor
*
* @param input_alg :: Handle to the main algorithm
* @param entry_name :: The pathname of the bank to load
* @param entry_type :: The classtype of the entry to load
* @param oldNeXusFileNames :: Identify if file is of old variety.
* @param prog :: an optional Progress object
* @param framePeriodNumbers :: Period numbers corresponding to each frame
* @param logger :: reference to Mantid log
*/
LoadBankFromDiskTask::LoadBankFromDiskTask(
    Mantid::DataHandling::LoadEventNexus *input_alg,
    const std::string &entry_name, const std::string &entry_type,
    const bool oldNeXusFileNames, Mantid::API::Progress *prog,
    const std::vector<int> &framePeriodNumbers, Mantid::Kernel::Logger &logger)
    : alg(input_alg), entry_name(entry_name), entry_type(entry_type),
      prog(prog), m_loadError(false), m_oldNexusFileNames(oldNeXusFileNames),
      m_loadStart(), m_loadSize(), m_event_id(nullptr),
      m_event_time_of_flight(nullptr), m_have_weight(false),
      m_event_weight(nullptr), m_framePeriodNumbers(framePeriodNumbers),
      alg_Logger(logger) {
  m_min_id = std::numeric_limits<uint32_t>::max();
  m_max_id = 0;
}

//---------------------------------------------------------------------------------------------------
/** Load the pulse times, if needed. This sets
* thisBankPulseTimes to the right pointer.
* */
void LoadBankFromDiskTask::loadPulseTimes(::NeXus::File &file) {
  try {
    // First, get info about the event_time_zero field in this bank
    file.openData("event_time_zero");
  } catch (::NeXus::Exception &) {
    // Field not found error is most likely.
    // Use the "proton_charge" das logs.
    thisBankPulseTimes =
        alg->m_allBanksPulseTimes; // alg->m_allBanksPulseTimes;
    return;
  }
  std::string thisStartTime = "";
  size_t thisNumPulses = 0;
  file.getAttr("offset", thisStartTime);
  if (!file.getInfo().dims.empty())
    thisNumPulses = file.getInfo().dims[0];
  file.closeData();

  // Now, we look through existing ones to see if it is already loaded
  // thisBankPulseTimes = NULL;
  for (auto &bankPulseTime : alg->m_bankPulseTimes) { // alg.m_bankPulseTimes
    if (bankPulseTime->equals(thisNumPulses, thisStartTime)) {
      thisBankPulseTimes = bankPulseTime;
      return;
    }
  }

  // Not found? Need to load and add it
  thisBankPulseTimes = boost::make_shared<Mantid::DataHandling::BankPulseTimes>(
      boost::ref(file), m_framePeriodNumbers);
  alg->m_bankPulseTimes.push_back(thisBankPulseTimes);
}

//---------------------------------------------------------------------------------------------------
/** Load the event_index field
(a list of size of # of pulses giving the index in the event list for that
pulse)

* @param file :: File handle for the NeXus file
* @param event_index :: ref to the vector
*/
void LoadBankFromDiskTask::loadEventIndex(::NeXus::File &file,
                                          std::vector<uint64_t> &event_index) {
  // Get the event_index (a list of size of # of pulses giving the index in
  // the event list for that pulse)
  file.openData("event_index");
  // Must be uint64
  if (file.getInfo().type == ::NeXus::UINT64)
    file.getData(event_index);
  else {
    alg_Logger.warning() // alg_Logger
        << "Entry " << entry_name
        << "'s event_index field is not UINT64! It will be skipped.\n";
    m_loadError = true;
  }
  file.closeData();

  // Look for the sign that the bank is empty
  if (event_index.size() == 1) {
    if (event_index[0] == 0) {
      // One entry, only zero. This means NO events in this bank.
      m_loadError = true;
      alg_Logger.debug() << "Bank " << entry_name << " is empty.\n";
    }
  }

  return;
}

//---------------------------------------------------------------------------------------------------
/** Open the event_id field and validate the contents
*
* @param file :: File handle for the NeXus file
* @param start_event :: set to the index of the first event
* @param stop_event :: set to the index of the last event + 1
* @param event_index ::  (a list of size of # of pulses giving the index in
*the event list for that pulse)
*/
void LoadBankFromDiskTask::prepareEventId(::NeXus::File &file,
                                          size_t &start_event,
                                          size_t &stop_event,
                                          std::vector<uint64_t> &event_index) {
  // Get the list of pixel ID's
  if (m_oldNexusFileNames)
    file.openData("event_pixel_id");
  else
    file.openData("event_id");

  // By default, use all available indices
  start_event = 0;
  ::NeXus::Info id_info = file.getInfo();
  // dims[0] can be negative in ISIS meaning 2^32 + dims[0]. Take that into
  // account
  int64_t dim0 = recalculateDataSize(id_info.dims[0]);
  stop_event = static_cast<size_t>(dim0);

  // Handle the time filtering by changing the start/end offsets.
  for (size_t i = 0; i < thisBankPulseTimes->numPulses; i++) {
    if (thisBankPulseTimes->pulseTimes[i] >= alg->filter_time_start) {
      start_event = event_index[i];
      break; // stop looking
    }
  }

  if (start_event > static_cast<size_t>(dim0)) {
    // If the frame indexes are bad then we can't construct the times of the
    // events properly and filtering by time
    // will not work on this data
    alg_Logger.warning()
        << this->entry_name
        << "'s field 'event_index' seems to be invalid (start_index > than "
           "the number of events in the bank)."
        << "All events will appear in the same frame and filtering by time "
           "will not be possible on this data.\n";
    start_event = 0;
    stop_event = static_cast<size_t>(dim0);
  } else {
    for (size_t i = 0; i < thisBankPulseTimes->numPulses; i++) {
      if (thisBankPulseTimes->pulseTimes[i] > alg->filter_time_stop) {
        stop_event = event_index[i];
        break;
      }
    }
  }
  // We are loading part - work out the event number range
  if (alg->chunk != Mantid::EMPTY_INT()) {
    start_event = (alg->chunk - alg->firstChunkForBank) * alg->eventsPerChunk;
    // Don't change stop_event for the final chunk
    if (start_event + alg->eventsPerChunk < stop_event)
      stop_event = start_event + alg->eventsPerChunk;
  }

  // Make sure it is within range
  if (stop_event > static_cast<size_t>(dim0))
    stop_event = dim0;

  alg_Logger.debug() << entry_name << ": start_event " << start_event
                     << " stop_event " << stop_event << "\n";

  return;
}

//---------------------------------------------------------------------------------------------------
/** Load the event_id field, which has been open
*/
void LoadBankFromDiskTask::loadEventId(::NeXus::File &file) {
  // This is the data size
  ::NeXus::Info id_info = file.getInfo();
  int64_t dim0 = recalculateDataSize(id_info.dims[0]);

  // Now we allocate the required arrays
  m_event_id = new uint32_t[m_loadSize[0]];

  // Check that the required space is there in the file.
  if (dim0 < m_loadSize[0] + m_loadStart[0]) {
    alg_Logger.warning() << "Entry " << entry_name
                         << "'s event_id field is too small (" << dim0
                         << ") to load the desired data size ("
                         << m_loadSize[0] + m_loadStart[0] << ").\n";
    m_loadError = true;
  }

  if (alg->getCancel())
    m_loadError = true; // To allow cancelling the algorithm

  if (!m_loadError) {
    // Must be uint32
    if (id_info.type == ::NeXus::UINT32)
      file.getSlab(m_event_id, m_loadStart, m_loadSize);
    else {
      alg_Logger.warning()
          << "Entry " << entry_name
          << "'s event_id field is not UINT32! It will be skipped.\n";
      m_loadError = true;
    }
    file.closeData();

    // determine the range of pixel ids
    for (auto i = 0; i < m_loadSize[0]; ++i) {
      uint32_t temp = m_event_id[i];
      if (temp < m_min_id)
        m_min_id = temp;
      if (temp > m_max_id)
        m_max_id = temp;
    }

    if (m_min_id > static_cast<uint32_t>(alg->eventid_max)) {
      // All the detector IDs in the bank are higher than the highest 'known'
      // (from the IDF)
      // ID. Setting this will abort the loading of the bank.
      m_loadError = true;
    }
    // fixup the maximum pixel id in the case that it's higher than the
    // highest 'known' id
    if (m_max_id > static_cast<uint32_t>(alg->eventid_max))
      m_max_id = static_cast<uint32_t>(alg->eventid_max);
  }

  return;
}

//---------------------------------------------------------------------------------------------------
/** Open and load the times-of-flight data
*/
void LoadBankFromDiskTask::loadTof(::NeXus::File &file) {
  // Allocate the array
  auto temp = new float[m_loadSize[0]];
  delete[] m_event_time_of_flight;
  m_event_time_of_flight = temp;

  // Get the list of event_time_of_flight's
  if (!m_oldNexusFileNames)
    file.openData("event_time_offset");
  else
    file.openData("event_time_of_flight");

  // Check that the required space is there in the file.
  ::NeXus::Info tof_info = file.getInfo();
  int64_t tof_dim0 = recalculateDataSize(tof_info.dims[0]);
  if (tof_dim0 < m_loadSize[0] + m_loadStart[0]) {
    alg_Logger.warning() << "Entry " << entry_name
                         << "'s event_time_offset field is too small "
                            "to load the desired data.\n";
    m_loadError = true;
  }

  // Check that the type is what it is supposed to be
  if (tof_info.type == ::NeXus::FLOAT32)
    file.getSlab(m_event_time_of_flight, m_loadStart, m_loadSize);
  else {
    alg_Logger.warning()
        << "Entry " << entry_name
        << "'s event_time_offset field is not FLOAT32! It will be skipped.\n";
    m_loadError = true;
  }

  if (!m_loadError) {
    std::string units;
    file.getAttr("units", units);
    if (units != "microsecond") {
      alg_Logger.warning() << "Entry " << entry_name
                           << "'s event_time_offset field's units are "
                              "not microsecond. It will be skipped.\n";
      m_loadError = true;
    }
    file.closeData();
  } // no error

  return;
}

//----------------------------------------------------------------------------------------------
/** Load weight of weigthed events
*/
void LoadBankFromDiskTask::loadEventWeights(::NeXus::File &file) {
  try {
    // First, get info about the event_weight field in this bank
    file.openData("event_weight");
  } catch (::NeXus::Exception &) {
    // Field not found error is most likely.
    m_have_weight = false;
    return;
  }
  // OK, we've got them
  m_have_weight = true;

  // Allocate the array
  auto temp = new float[m_loadSize[0]];
  delete[] m_event_weight;
  m_event_weight = temp;

  ::NeXus::Info weight_info = file.getInfo();
  int64_t weight_dim0 = recalculateDataSize(weight_info.dims[0]);
  if (weight_dim0 < m_loadSize[0] + m_loadStart[0]) {
    Kernel::Logger &logger = alg_Logger;
    logger.warning()
        << "Entry " << entry_name
        << "'s event_weight field is too small to load the desired data.\n";
    m_loadError = true;
  }

  // Check that the type is what it is supposed to be
  if (weight_info.type == ::NeXus::FLOAT32)
    file.getSlab(m_event_weight, m_loadStart, m_loadSize);
  else {
    alg_Logger.warning()
        << "Entry " << entry_name
        << "'s event_weight field is not FLOAT32! It will be skipped.\n";
    m_loadError = true;
  }

  if (!m_loadError) {
    file.closeData();
  }

  return;
}

void LoadBankFromDiskTask::readFile(std::vector<uint64_t> *event_index_ptr) {
  std::vector<uint64_t> &event_index = *event_index_ptr;

  // Open the file
  ::NeXus::File file(alg->m_filename);
  try {
    // Navigate into the file
    file.openGroup(alg->m_top_entry_name, "NXentry");
    // Open the bankN_event group
    file.openGroup(entry_name, entry_type);

    // Load the event_index field.
    this->loadEventIndex(file, event_index);

    if (!m_loadError) {
      // Load and validate the pulse times
      this->loadPulseTimes(file);

      // The event_index should be the same length as the pulse times from DAS
      // logs.
      if (event_index.size() != thisBankPulseTimes->numPulses)
        alg_Logger.warning()
            << "Bank " << entry_name
            << " has a mismatch between the number of event_index entries "
               "and the number of pulse times in event_time_zero.\n";

      // Open and validate event_id field.
      size_t start_event = 0;
      size_t stop_event = 0;
      this->prepareEventId(file, start_event, stop_event, event_index);

      // These are the arguments to getSlab()
      m_loadStart[0] = static_cast<int>(start_event);
      m_loadSize[0] = static_cast<int>(stop_event - start_event);

      if ((m_loadSize[0] > 0) && (m_loadStart[0] >= 0)) {
        // Load pixel IDs
        this->loadEventId(file);
        if (alg->getCancel())
          m_loadError = true; // To allow cancelling the algorithm

        // And TOF.
        if (!m_loadError) {
          this->loadTof(file);
          if (m_have_weight) {
            this->loadEventWeights(file);
          }
        }
      } // Size is at least 1
      else {
        // Found a size that was 0 or less; stop processing
        m_loadError = true;
      }

    } // no error

  } // try block
  catch (std::exception &e) {
    alg_Logger.error() << "Error while loading bank " << entry_name << ":"
                       << std::endl;
    alg_Logger.error() << e.what() << std::endl;
    m_loadError = true;
  } catch (...) {
    alg_Logger.error() << "Unspecified error while loading bank " << entry_name
                       << std::endl;
    m_loadError = true;
  }

  // Close up the file even if errors occured.
  file.closeGroup();
  file.close();
}

bool LoadBankFromDiskTask::checkSpectra() {
  const uint32_t minSpectraToLoad = static_cast<uint32_t>(alg->m_specMin);
  const uint32_t maxSpectraToLoad = static_cast<uint32_t>(alg->m_specMax);
  const uint32_t emptyInt = static_cast<uint32_t>(Mantid::EMPTY_INT());
  // check that if a range of spectra were requested that these fit within
  // this bank
  if (minSpectraToLoad != emptyInt && m_min_id < minSpectraToLoad) {
    if (minSpectraToLoad > m_max_id) { // the minimum spectra to load is more
                                       // than the max of this bank
      return false;
    }
    // the min spectra to load is higher than the min for this bank
    m_min_id = minSpectraToLoad;
  }
  if (maxSpectraToLoad != emptyInt && m_max_id > maxSpectraToLoad) {
    if (maxSpectraToLoad < m_min_id) {
      // the maximum spectra to load is less than the minimum of this bank
      return false;
    }
    // the max spectra to load is lower than the max for this bank
    m_max_id = maxSpectraToLoad;
  }
  if (m_min_id > m_max_id) {
    // the min is now larger than the max, this means the entire block of
    // spectra to load is outside this bank
    return false;
  }

  return true;
}

//---------------------------------------------------------------------------------------------------
/** Main method to load bank from disk
 * @brief LoadBankFromDiskTask::run
 */
void LoadBankFromDiskTask::run() {
  // The vectors we will be filling
  auto event_index_ptr = new std::vector<uint64_t>();

  // These give the limits in each file as to which events we actually load
  // (when filtering by time).
  m_loadStart.resize(1, 0);
  m_loadSize.resize(1, 0);

  // Data arrays
  m_event_id = nullptr;
  m_event_time_of_flight = nullptr;
  m_event_weight = nullptr;

  m_loadError = false;
  m_have_weight = alg->m_haveWeights;

  prog->report(entry_name + ": load from disk");

  // Read file and load the data into memeory
  this->readFile(event_index_ptr);

  // Abort if anything failed
  if (m_loadError) {
    prog->reportIncrement(4, entry_name + ": skipping");
    delete[] m_event_id;
    delete[] m_event_time_of_flight;
    if (m_have_weight) {
      delete[] m_event_weight;
    }
    delete event_index_ptr;
    return;
  }

  // calculate bank size.  m_max_id and m_min_id may be changed in the next
  // method
  const auto bank_size = m_max_id - m_min_id;

  // check bank and spectra IDs.  If failed the check, return
  bool file_valid = checkSpectra();
  if (!file_valid)
    return;

  // schedule the job to generate the event lists
  auto mid_id = m_max_id;
  if (alg->splitProcessing && m_max_id > (m_min_id + (bank_size / 4)))
    // only split if told to and the section to load is at least 1/4 the size
    // of the whole bank
    mid_id = (m_max_id + m_min_id) / 2;

  // No error? Launch a new task to process that data.
  size_t numEvents = m_loadSize[0];
  size_t startAt = m_loadStart[0];

  // convert things to shared_arrays
  boost::shared_array<uint32_t> event_id_shrd(m_event_id);
  boost::shared_array<float> event_time_of_flight_shrd(m_event_time_of_flight);
  boost::shared_array<float> event_weight_shrd(m_event_weight);
  boost::shared_ptr<std::vector<uint64_t>> event_index_shrd(event_index_ptr);

  ProcessBankData newTask1(alg, entry_name, prog, event_id_shrd,
                           event_time_of_flight_shrd, numEvents, startAt,
                           event_index_shrd, thisBankPulseTimes, m_have_weight,
                           event_weight_shrd, m_min_id, mid_id);
  newTask1.run();

  if (alg->splitProcessing && (mid_id < m_max_id)) {
    ProcessBankData newTask2(
        alg, entry_name, prog, event_id_shrd, event_time_of_flight_shrd,
        numEvents, startAt, event_index_shrd, thisBankPulseTimes, m_have_weight,
        event_weight_shrd, (mid_id + 1), m_max_id);
    newTask2.run();
  }
}

//---------------------------------------------------------------------------------------------------
/**
* Interpret the value describing the number of events. If the number is
* positive return it unchanged.
* If the value is negative (can happen at ISIS) add 2^32 to it.
* @param size :: The size of events value.
*/
int64_t LoadBankFromDiskTask::recalculateDataSize(const int64_t &size) {
  if (size < 0) {
    const int64_t shift = int64_t(1) << 32;
    return shift + size;
  }
  return size;
}
