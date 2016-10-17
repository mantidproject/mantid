#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidDataHandling/ProcessBankData.h"
#include "MantidKernel/Task.h"
#include "MantidKernel/ThreadScheduler.h"

#include <boost/shared_array.hpp>

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace Geometry;
using namespace API;
using namespace DataObjects;

//==============================================================================================
// Class LoadBankFromDiskTask
//==============================================================================================
/** This task does the disk IO from loading the NXS file,
* and so will be on a disk IO mutex */
class LoadBankFromDiskTask : public Task {

public:
  //---------------------------------------------------------------------------------------------------
  /** Constructor
  *
  * @param alg :: Handle to the main algorithm
  * @param entry_name :: The pathname of the bank to load
  * @param entry_type :: The classtype of the entry to load
  * @param numEvents :: The number of events in the bank.
  * @param oldNeXusFileNames :: Identify if file is of old variety.
  * @param prog :: an optional Progress object
  * @param ioMutex :: a mutex shared for all Disk I-O tasks
  * @param scheduler :: the ThreadScheduler that runs this task.
  * @param framePeriodNumbers :: Period numbers corresponding to each frame
  */
  LoadBankFromDiskTask(LoadEventNexus *alg, const std::string &entry_name,
                       const std::string &entry_type,
                       const std::size_t numEvents,
                       const bool oldNeXusFileNames, Progress *prog,
                       boost::shared_ptr<std::mutex> ioMutex,
                       ThreadScheduler *scheduler,
                       const std::vector<int> &framePeriodNumbers)
      : Task(), alg(alg), entry_name(entry_name), entry_type(entry_type),
        // prog(prog), scheduler(scheduler), thisBankPulseTimes(NULL),
        // m_loadError(false),
        prog(prog), scheduler(scheduler), m_loadError(false),
        m_oldNexusFileNames(oldNeXusFileNames), m_loadStart(), m_loadSize(),
        m_event_id(nullptr), m_event_time_of_flight(nullptr),
        m_have_weight(false), m_event_weight(nullptr),
        m_framePeriodNumbers(framePeriodNumbers) {
    setMutex(ioMutex);
    m_cost = static_cast<double>(numEvents);
    m_min_id = std::numeric_limits<uint32_t>::max();
    m_max_id = 0;
  }

  //---------------------------------------------------------------------------------------------------
  /** Load the pulse times, if needed. This sets
  * thisBankPulseTimes to the right pointer.
  * */
  void loadPulseTimes(::NeXus::File &file) {
    try {
      // First, get info about the event_time_zero field in this bank
      file.openData("event_time_zero");
    } catch (::NeXus::Exception &) {
      // Field not found error is most likely.
      // Use the "proton_charge" das logs.
      thisBankPulseTimes = alg->m_allBanksPulseTimes;
      return;
    }
    std::string thisStartTime;
    size_t thisNumPulses = 0;
    file.getAttr("offset", thisStartTime);
    if (!file.getInfo().dims.empty())
      thisNumPulses = file.getInfo().dims[0];
    file.closeData();

    // Now, we look through existing ones to see if it is already loaded
    // thisBankPulseTimes = NULL;
    for (auto &bankPulseTime : alg->m_bankPulseTimes) {
      if (bankPulseTime->equals(thisNumPulses, thisStartTime)) {
        thisBankPulseTimes = bankPulseTime;
        return;
      }
    }

    // Not found? Need to load and add it
    thisBankPulseTimes = boost::make_shared<BankPulseTimes>(
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
  void loadEventIndex(::NeXus::File &file, std::vector<uint64_t> &event_index) {
    // Get the event_index (a list of size of # of pulses giving the index in
    // the event list for that pulse)
    file.openData("event_index");
    // Must be uint64
    if (file.getInfo().type == ::NeXus::UINT64)
      file.getData(event_index);
    else {
      alg->getLogger().warning()
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
        alg->getLogger().debug() << "Bank " << entry_name << " is empty.\n";
      }
    }
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
  void prepareEventId(::NeXus::File &file, size_t &start_event,
                      size_t &stop_event, std::vector<uint64_t> &event_index) {
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
      alg->getLogger().warning()
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
    if (alg->chunk != EMPTY_INT()) {
      start_event = (alg->chunk - alg->firstChunkForBank) * alg->eventsPerChunk;
      // Don't change stop_event for the final chunk
      if (start_event + alg->eventsPerChunk < stop_event)
        stop_event = start_event + alg->eventsPerChunk;
    }

    // Make sure it is within range
    if (stop_event > static_cast<size_t>(dim0))
      stop_event = dim0;

    alg->getLogger().debug() << entry_name << ": start_event " << start_event
                             << " stop_event " << stop_event << "\n";
  }

  //---------------------------------------------------------------------------------------------------
  /** Load the event_id field, which has been open
  */
  void loadEventId(::NeXus::File &file) {
    // This is the data size
    ::NeXus::Info id_info = file.getInfo();
    int64_t dim0 = recalculateDataSize(id_info.dims[0]);

    // Now we allocate the required arrays
    m_event_id = new uint32_t[m_loadSize[0]];

    // Check that the required space is there in the file.
    if (dim0 < m_loadSize[0] + m_loadStart[0]) {
      alg->getLogger().warning() << "Entry " << entry_name
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
        alg->getLogger().warning()
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
  }

  //---------------------------------------------------------------------------------------------------
  /** Open and load the times-of-flight data
  */
  void loadTof(::NeXus::File &file) {
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
      alg->getLogger().warning() << "Entry " << entry_name
                                 << "'s event_time_offset field is too small "
                                    "to load the desired data.\n";
      m_loadError = true;
    }

    // Check that the type is what it is supposed to be
    if (tof_info.type == ::NeXus::FLOAT32)
      file.getSlab(m_event_time_of_flight, m_loadStart, m_loadSize);
    else {
      alg->getLogger().warning()
          << "Entry " << entry_name
          << "'s event_time_offset field is not FLOAT32! It will be skipped.\n";
      m_loadError = true;
    }

    if (!m_loadError) {
      std::string units;
      file.getAttr("units", units);
      if (units != "microsecond") {
        alg->getLogger().warning() << "Entry " << entry_name
                                   << "'s event_time_offset field's units are "
                                      "not microsecond. It will be skipped.\n";
        m_loadError = true;
      }
      file.closeData();
    } // no error
  }

  //----------------------------------------------------------------------------------------------
  /** Load weight of weigthed events
  */
  void loadEventWeights(::NeXus::File &file) {
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
      alg->getLogger().warning()
          << "Entry " << entry_name
          << "'s event_weight field is too small to load the desired data.\n";
      m_loadError = true;
    }

    // Check that the type is what it is supposed to be
    if (weight_info.type == ::NeXus::FLOAT32)
      file.getSlab(m_event_weight, m_loadStart, m_loadSize);
    else {
      alg->getLogger().warning()
          << "Entry " << entry_name
          << "'s event_weight field is not FLOAT32! It will be skipped.\n";
      m_loadError = true;
    }

    if (!m_loadError) {
      file.closeData();
    }
  }

  //---------------------------------------------------------------------------------------------------
  void run() override {
    // The vectors we will be filling
    auto event_index_ptr = new std::vector<uint64_t>();
    std::vector<uint64_t> &event_index = *event_index_ptr;

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
          alg->getLogger().warning()
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
      alg->getLogger().error() << "Error while loading bank " << entry_name
                               << ":\n";
      alg->getLogger().error() << e.what() << '\n';
      m_loadError = true;
    } catch (...) {
      alg->getLogger().error() << "Unspecified error while loading bank "
                               << entry_name << '\n';
      m_loadError = true;
    }

    // Close up the file even if errors occured.
    file.closeGroup();
    file.close();

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

    const auto bank_size = m_max_id - m_min_id;
    const uint32_t minSpectraToLoad = static_cast<uint32_t>(alg->m_specMin);
    const uint32_t maxSpectraToLoad = static_cast<uint32_t>(alg->m_specMax);
    const uint32_t emptyInt = static_cast<uint32_t>(EMPTY_INT());
    // check that if a range of spectra were requested that these fit within
    // this bank
    if (minSpectraToLoad != emptyInt && m_min_id < minSpectraToLoad) {
      if (minSpectraToLoad > m_max_id) { // the minimum spectra to load is more
                                         // than the max of this bank
        return;
      }
      // the min spectra to load is higher than the min for this bank
      m_min_id = minSpectraToLoad;
    }
    if (maxSpectraToLoad != emptyInt && m_max_id > maxSpectraToLoad) {
      if (maxSpectraToLoad < m_min_id) {
        // the maximum spectra to load is less than the minimum of this bank
        return;
      }
      // the max spectra to load is lower than the max for this bank
      m_max_id = maxSpectraToLoad;
    }
    if (m_min_id > m_max_id) {
      // the min is now larger than the max, this means the entire block of
      // spectra to load is outside this bank
      return;
    }

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
    boost::shared_array<float> event_time_of_flight_shrd(
        m_event_time_of_flight);
    boost::shared_array<float> event_weight_shrd(m_event_weight);
    boost::shared_ptr<std::vector<uint64_t>> event_index_shrd(event_index_ptr);

    ProcessBankData *newTask1 = new ProcessBankData(
        alg, entry_name, prog, event_id_shrd, event_time_of_flight_shrd,
        numEvents, startAt, event_index_shrd, thisBankPulseTimes, m_have_weight,
        event_weight_shrd, m_min_id, mid_id);
    scheduler->push(newTask1);
    if (alg->splitProcessing && (mid_id < m_max_id)) {
      ProcessBankData *newTask2 = new ProcessBankData(
          alg, entry_name, prog, event_id_shrd, event_time_of_flight_shrd,
          numEvents, startAt, event_index_shrd, thisBankPulseTimes,
          m_have_weight, event_weight_shrd, (mid_id + 1), m_max_id);
      scheduler->push(newTask2);
    }
  }

  //---------------------------------------------------------------------------------------------------
  /**
  * Interpret the value describing the number of events. If the number is
  * positive return it unchanged.
  * If the value is negative (can happen at ISIS) add 2^32 to it.
  * @param size :: The size of events value.
  */
  int64_t recalculateDataSize(const int64_t &size) {
    if (size < 0) {
      const int64_t shift = int64_t(1) << 32;
      return shift + size;
    }
    return size;
  }

private:
  /// Algorithm being run
  LoadEventNexus *alg;
  /// NXS path to bank
  std::string entry_name;
  /// NXS type
  std::string entry_type;
  /// Progress reporting
  Progress *prog;
  /// ThreadScheduler running this task
  ThreadScheduler *scheduler;
  /// Object with the pulse times for this bank
  boost::shared_ptr<BankPulseTimes> thisBankPulseTimes;
  /// Did we get an error in loading
  bool m_loadError;
  /// Old names in the file?
  bool m_oldNexusFileNames;
  /// Index to load start at in the file
  std::vector<int> m_loadStart;
  /// How much to load in the file
  std::vector<int> m_loadSize;
  /// Event pixel ID data
  uint32_t *m_event_id;
  /// Minimum pixel ID in this data
  uint32_t m_min_id;
  /// Maximum pixel ID in this data
  uint32_t m_max_id;
  /// TOF data
  float *m_event_time_of_flight;
  /// Flag for simulated data
  bool m_have_weight;
  /// Event weights
  float *m_event_weight;
  /// Frame period numbers
  const std::vector<int> m_framePeriodNumbers;
}; // END-DEF-CLASS LoadBankFromDiskTask
}
}
