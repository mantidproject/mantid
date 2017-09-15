#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidDataHandling/EventWorkspaceCollection.h"
#include "MantidDataHandling/ProcessBankData.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/ThreadSchedulerMutexes.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VisibleWhenProperty.h"

#include <boost/function.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>

#include <functional>

using std::map;
using std::string;
using std::vector;

using namespace ::NeXus;

namespace Mantid {
namespace DataHandling {

DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadEventNexus)

using namespace Kernel;
using namespace Geometry;
using namespace API;
using namespace DataObjects;

namespace {

/**
 * Copy all logData properties from the 'from' workspace to the 'to'
 * workspace. Does not use CopyLogs as a child algorithm (this is a
 * simple copy and the workspace is not yet in the ADS).
 *
 * @param from source of log entries
 * @param to workspace where to add the log entries
 */
void copyLogs(const Mantid::DataHandling::EventWorkspaceCollection_sptr &from,
              EventWorkspace_sptr &to) {
  // from the logs, get all the properties that don't overwrite any
  // prop. already set in the sink workspace (like 'filename').
  auto props = from->mutableRun().getLogData();
  for (auto &prop : props) {
    if (!to->mutableRun().hasProperty(prop->name())) {
      to->mutableRun().addLogData(prop->clone());
    }
  }
}
}

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
      // fixup the minimum pixel id in the case that it's lower than the lowest
      // 'known' id. We test this by checking that when we add the offset we
      // would not get a negative index into the vector. Note that m_min_id is
      // a uint so we have to be cautious about adding it to an int which may be
      // negative.
      if (static_cast<int32_t>(m_min_id) + alg->pixelID_to_wi_offset < 0) {
        m_min_id = static_cast<uint32_t>(abs(alg->pixelID_to_wi_offset));
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

//===============================================================================================
// LoadEventNexus
//===============================================================================================

//----------------------------------------------------------------------------------------------
/** Empty default constructor
*/
LoadEventNexus::LoadEventNexus()
    : IFileLoader<Kernel::NexusDescriptor>(), m_filename(), filter_tof_min(0),
      filter_tof_max(0), m_specList(), m_specMin(0), m_specMax(0),
      filter_time_start(), filter_time_stop(), chunk(0), totalChunks(0),
      firstChunkForBank(0), eventsPerChunk(0), m_tofMutex(), longest_tof(0),
      shortest_tof(0), bad_tofs(0), discarded_events(0), precount(0),
      compressTolerance(0), eventVectors(), m_eventVectorMutex(),
      eventid_max(0), pixelID_to_wi_vector(), pixelID_to_wi_offset(),
      m_bankPulseTimes(), m_allBanksPulseTimes(), m_top_entry_name(),
      m_file(nullptr), splitProcessing(false), m_haveWeights(false),
      weightedEventVectors(), m_instrument_loaded_correctly(false),
      loadlogs(false), m_logs_loaded_correctly(false), event_id_is_spec(false) {
}

//----------------------------------------------------------------------------------------------
/** Destructor */
LoadEventNexus::~LoadEventNexus() {
  if (m_file)
    delete m_file;
}

//----------------------------------------------------------------------------------------------
/**
* Return the confidence with with this algorithm can load the file
* @param descriptor A descriptor for the file
* @returns An integer specifying the confidence level. 0 indicates it will not
* be used
*/
int LoadEventNexus::confidence(Kernel::NexusDescriptor &descriptor) const {
  int confidence(0);
  if (descriptor.classTypeExists("NXevent_data")) {
    if (descriptor.pathOfTypeExists("/entry", "NXentry") ||
        descriptor.pathOfTypeExists("/raw_data_1", "NXentry")) {
      confidence = 80;
    }
  }
  return confidence;
}

//----------------------------------------------------------------------------------------------
/** Initialisation method.
*/
void LoadEventNexus::init() {
  const std::vector<std::string> exts{"_event.nxs", ".nxs.h5", ".nxs"};
  this->declareProperty(
      Kernel::make_unique<FileProperty>("Filename", "", FileProperty::Load,
                                        exts),
      "The name of the Event NeXus file to read, including its full or "
      "relative path. "
      "The file name is typically of the form INST_####_event.nxs (N.B. case "
      "sensitive if running on Linux).");

  this->declareProperty(
      make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "",
                                                Direction::Output),
      "The name of the output EventWorkspace or WorkspaceGroup in which to "
      "load the EventNexus file.");

  declareProperty(
      make_unique<PropertyWithValue<string>>("NXentryName", "",
                                             Direction::Input),
      "Optional: Name of the NXentry to load if it's not the default.");

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "FilterByTofMin", EMPTY_DBL(), Direction::Input),
                  "Optional: To exclude events that do not fall within a range "
                  "of times-of-flight. "
                  "This is the minimum accepted value in microseconds. Keep "
                  "blank to load all events.");

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "FilterByTofMax", EMPTY_DBL(), Direction::Input),
                  "Optional: To exclude events that do not fall within a range "
                  "of times-of-flight. "
                  "This is the maximum accepted value in microseconds. Keep "
                  "blank to load all events.");

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "FilterByTimeStart", EMPTY_DBL(), Direction::Input),
                  "Optional: To only include events after the provided start "
                  "time, in seconds (relative to the start of the run).");

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "FilterByTimeStop", EMPTY_DBL(), Direction::Input),
                  "Optional: To only include events before the provided stop "
                  "time, in seconds (relative to the start of the run).");

  std::string grp1 = "Filter Events";
  setPropertyGroup("FilterByTofMin", grp1);
  setPropertyGroup("FilterByTofMax", grp1);
  setPropertyGroup("FilterByTimeStart", grp1);
  setPropertyGroup("FilterByTimeStop", grp1);

  declareProperty(
      make_unique<ArrayProperty<string>>("BankName", Direction::Input),
      "Optional: To only include events from one bank. Any bank "
      "whose name does not match the given string will have no "
      "events.");

  declareProperty(make_unique<PropertyWithValue<bool>>("SingleBankPixelsOnly",
                                                       true, Direction::Input),
                  "Optional: Only applies if you specified a single bank to "
                  "load with BankName. "
                  "Only pixels in the specified bank will be created if true; "
                  "all of the instrument's pixels will be created otherwise.");
  setPropertySettings("SingleBankPixelsOnly", make_unique<VisibleWhenProperty>(
                                                  "BankName", IS_NOT_DEFAULT));

  std::string grp2 = "Loading a Single Bank";
  setPropertyGroup("BankName", grp2);
  setPropertyGroup("SingleBankPixelsOnly", grp2);

  declareProperty(
      make_unique<PropertyWithValue<bool>>("Precount", true, Direction::Input),
      "Pre-count the number of events in each pixel before allocating memory "
      "(optional, default True). "
      "This can significantly reduce memory use and memory fragmentation; it "
      "may also speed up loading.");

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "CompressTolerance", -1.0, Direction::Input),
                  "Run CompressEvents while loading (optional, leave blank or "
                  "negative to not do). "
                  "This specified the tolerance to use (in microseconds) when "
                  "compressing.");

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(1);
  declareProperty("ChunkNumber", EMPTY_INT(), mustBePositive,
                  "If loading the file by sections ('chunks'), this is the "
                  "section number of this execution of the algorithm.");
  declareProperty("TotalChunks", EMPTY_INT(), mustBePositive,
                  "If loading the file by sections ('chunks'), this is the "
                  "total number of sections.");
  // TotalChunks is only meaningful if ChunkNumber is set
  // Would be nice to be able to restrict ChunkNumber to be <= TotalChunks at
  // validation
  setPropertySettings("TotalChunks", make_unique<VisibleWhenProperty>(
                                         "ChunkNumber", IS_NOT_DEFAULT));

  std::string grp3 = "Reduce Memory Use";
  setPropertyGroup("Precount", grp3);
  setPropertyGroup("CompressTolerance", grp3);
  setPropertyGroup("ChunkNumber", grp3);
  setPropertyGroup("TotalChunks", grp3);

  declareProperty(make_unique<PropertyWithValue<bool>>("LoadMonitors", false,
                                                       Direction::Input),
                  "Load the monitors from the file (optional, default False).");

  declareProperty(
      make_unique<PropertyWithValue<bool>>("MonitorsAsEvents", false,
                                           Direction::Input),
      "If present, load the monitors as events. '''WARNING:''' WILL "
      "SIGNIFICANTLY INCREASE MEMORY USAGE (optional, default False). ");

  declareProperty("LoadEventMonitors", true,
                  "Load event monitor in NeXus file both event monitor and "
                  "histogram monitor found in NeXus file."
                  "If both of LoadEventMonitor and LoadHistoMonitor are true, "
                  "or both of them are false,"
                  "then it is in the auto mode such that any existing monitor "
                  "will be loaded.");

  declareProperty("LoadHistoMonitors", true,
                  "Load histogram monitor in NeXus file both event monitor and "
                  "histogram monitor found in NeXus file."
                  "If both of LoadEventMonitor and LoadHistoMonitor are true, "
                  "or both of them are false,"
                  "then it is in the auto mode such that any existing monitor "
                  "will be loaded.");

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "FilterMonByTofMin", EMPTY_DBL(), Direction::Input),
                  "Optional: To exclude events from monitors that do not fall "
                  "within a range of times-of-flight. "
                  "This is the minimum accepted value in microseconds.");

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "FilterMonByTofMax", EMPTY_DBL(), Direction::Input),
                  "Optional: To exclude events from monitors that do not fall "
                  "within a range of times-of-flight. "
                  "This is the maximum accepted value in microseconds.");

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "FilterMonByTimeStart", EMPTY_DBL(), Direction::Input),
                  "Optional: To only include events from monitors after the "
                  "provided start time, in seconds (relative to the start of "
                  "the run).");

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "FilterMonByTimeStop", EMPTY_DBL(), Direction::Input),
                  "Optional: To only include events from monitors before the "
                  "provided stop time, in seconds (relative to the start of "
                  "the run).");

  setPropertySettings(
      "MonitorsAsEvents",
      make_unique<VisibleWhenProperty>("LoadMonitors", IS_EQUAL_TO, "1"));
  setPropertySettings(
      "LoadEventMonitors",
      make_unique<VisibleWhenProperty>("LoadMonitors", IS_EQUAL_TO, "1"));
  setPropertySettings(
      "LoadHistoMonitors",
      make_unique<VisibleWhenProperty>("LoadMonitors", IS_EQUAL_TO, "1"));
  auto asEventsIsOn = [] {
    std::unique_ptr<IPropertySettings> prop =
        make_unique<VisibleWhenProperty>("MonitorsAsEvents", IS_EQUAL_TO, "1");
    return prop;
  };
  setPropertySettings("FilterMonByTofMin", asEventsIsOn());
  setPropertySettings("FilterMonByTofMax", asEventsIsOn());
  setPropertySettings("FilterMonByTimeStart", asEventsIsOn());
  setPropertySettings("FilterMonByTimeStop", asEventsIsOn());

  std::string grp4 = "Monitors";
  setPropertyGroup("LoadMonitors", grp4);
  setPropertyGroup("MonitorsAsEvents", grp4);
  setPropertyGroup("LoadEventMonitors", grp4);
  setPropertyGroup("LoadHistoMonitors", grp4);
  setPropertyGroup("FilterMonByTofMin", grp4);
  setPropertyGroup("FilterMonByTofMax", grp4);
  setPropertyGroup("FilterMonByTimeStart", grp4);
  setPropertyGroup("FilterMonByTimeStop", grp4);

  declareProperty("SpectrumMin", EMPTY_INT(), mustBePositive,
                  "The number of the first spectrum to read.");
  declareProperty("SpectrumMax", EMPTY_INT(), mustBePositive,
                  "The number of the last spectrum to read.");
  declareProperty(make_unique<ArrayProperty<int32_t>>("SpectrumList"),
                  "A comma-separated list of individual spectra to read.");

  declareProperty(
      make_unique<PropertyWithValue<bool>>("MetaDataOnly", false,
                                           Direction::Input),
      "If true, only the meta data and sample logs will be loaded.");

  declareProperty(
      make_unique<PropertyWithValue<bool>>("LoadLogs", true, Direction::Input),
      "Load the Sample/DAS logs from the file (default True).");
}

//----------------------------------------------------------------------------------------------
/** set the name of the top level NXentry m_top_entry_name
*/
void LoadEventNexus::setTopEntryName() {
  std::string nxentryProperty = getProperty("NXentryName");
  if (!nxentryProperty.empty()) {
    m_top_entry_name = nxentryProperty;
    return;
  }
  typedef std::map<std::string, std::string> string_map_t;
  try {
    string_map_t::const_iterator it;
    // assume we're at the top, otherwise: m_file->openPath("/");
    string_map_t entries = m_file->getEntries();

    // Choose the first entry as the default
    m_top_entry_name = entries.begin()->first;

    for (it = entries.begin(); it != entries.end(); ++it) {
      if (((it->first == "entry") || (it->first == "raw_data_1")) &&
          (it->second == "NXentry")) {
        m_top_entry_name = it->first;
        break;
      }
    }
  } catch (const std::exception &) {
    g_log.error() << "Unable to determine name of top level NXentry - assuming "
                     "\"entry\".\n";
    m_top_entry_name = "entry";
  }
}

template <typename T> void LoadEventNexus::filterDuringPause(T workspace) {
  try {
    if ((!ConfigService::Instance().hasProperty(
            "loadeventnexus.keeppausedevents")) &&
        (m_ws->run().getLogData("pause")->size() > 1)) {
      g_log.notice("Filtering out events when the run was marked as paused. "
                   "Set the loadeventnexus.keeppausedevents configuration "
                   "property to override this.");

      auto filter = createChildAlgorithm("FilterByLogValue");
      filter->setProperty("InputWorkspace", workspace);
      filter->setProperty("OutputWorkspace", workspace);
      filter->setProperty("LogName", "pause");
      // The log value is set to 1 when the run is paused, 0 otherwise.
      filter->setProperty("MinimumValue", 0.0);
      filter->setProperty("MaximumValue", 0.0);
      filter->setProperty("LogBoundary", "Left");
      filter->execute();
    }
  } catch (Exception::NotFoundError &) {
    // No "pause" log, just carry on
  }
}

template <>
void LoadEventNexus::filterDuringPause<EventWorkspaceCollection_sptr>(
    EventWorkspaceCollection_sptr workspace) {
  // We provide a function pointer to the filter method of the object
  boost::function<void(MatrixWorkspace_sptr)> func = std::bind1st(
      std::mem_fun(&LoadEventNexus::filterDuringPause<MatrixWorkspace_sptr>),
      this);
  workspace->applyFilter(func);
}

//------------------------------------------------------------------------------------------------
/** Executes the algorithm. Reading in the file and creating and populating
*  the output workspace
*/
void LoadEventNexus::exec() {
  // Retrieve the filename from the properties
  m_filename = getPropertyValue("Filename");

  precount = getProperty("Precount");
  compressTolerance = getProperty("CompressTolerance");

  loadlogs = getProperty("LoadLogs");

  // Check to see if the monitors need to be loaded later
  bool load_monitors = this->getProperty("LoadMonitors");

  // this must make absolutely sure that m_file is a valid (and open)
  // NeXus::File object
  safeOpenFile(m_filename);

  setTopEntryName();

  // Initialize progress reporting.
  int reports = 3;
  if (load_monitors)
    reports++;
  Progress prog(this, 0.0, 0.3, reports);

  // Load the detector events
  m_ws = boost::make_shared<EventWorkspaceCollection>(); // Algorithm currently
                                                         // relies on an
  // object-level workspace ptr
  loadEvents(&prog, false); // Do not load monitor blocks

  if (discarded_events > 0) {
    g_log.information() << discarded_events
                        << " events were encountered coming from pixels which "
                           "are not in the Instrument Definition File."
                           "These events were discarded.\n";
  }

  // If the run was paused at any point, filter out those events (SNS only, I
  // think)
  filterDuringPause(m_ws->getSingleHeldWorkspace());

  // add filename
  m_ws->mutableRun().addProperty("Filename", m_filename);
  // Save output
  this->setProperty("OutputWorkspace", m_ws->combinedWorkspace());
  // Load the monitors
  if (load_monitors) {
    prog.report("Loading monitors");
    const bool monitorsAsEvents = getProperty("MonitorsAsEvents");

    if (monitorsAsEvents && !this->hasEventMonitors()) {
      g_log.warning()
          << "The property MonitorsAsEvents has been enabled but "
             "this file does not seem to have monitors with events.\n";
    }
    if (monitorsAsEvents) {
      // no matter whether the file has events or not, the user has requested to
      // load events from monitors
      if (m_ws->nPeriods() > 1) {
        throw std::runtime_error(
            "Loading multi-period monitors in event mode is not supported.");
      }
      this->runLoadMonitorsAsEvents(&prog);
    } else {
      // this resorts to child algorithm 'LoadNexusMonitors', passing the
      // property 'MonitorsAsEvents'
      this->runLoadMonitors();
    }
  }
}

//-----------------------------------------------------------------------------
/** Generate a look-up table where the index = the pixel ID of an event
* and the value = a pointer to the EventList in the workspace
* @param vectors :: the array to create the map on
*/
template <class T>
void LoadEventNexus::makeMapToEventLists(std::vector<std::vector<T>> &vectors) {
  vectors.resize(m_ws->nPeriods());
  if (this->event_id_is_spec) {
    // Find max spectrum no
    Axis *ax1 = m_ws->getAxis(1);
    specnum_t maxSpecNo =
        -std::numeric_limits<specnum_t>::max(); // So that any number will be
                                                // greater than this
    for (size_t i = 0; i < ax1->length(); i++) {
      specnum_t spec = ax1->spectraNo(i);
      if (spec > maxSpecNo)
        maxSpecNo = spec;
    }

    // These are used by the bank loader to figure out where to put the events
    // The index of eventVectors is a spectrum number so it is simply resized to
    // the maximum
    // possible spectrum number
    eventid_max = maxSpecNo;
    for (size_t i = 0; i < vectors.size(); ++i) {
      vectors[i].resize(maxSpecNo + 1, nullptr);
    }
    for (size_t period = 0; period < m_ws->nPeriods(); ++period) {
      for (size_t i = 0; i < m_ws->getNumberHistograms(); ++i) {
        const auto &spec = m_ws->getSpectrum(i);
        getEventsFrom(m_ws->getSpectrum(i, period),
                      vectors[period][spec.getSpectrumNo()]);
      }
    }
  } else {
    // To avoid going out of range in the vector, this is the MAX index that can
    // go into it
    eventid_max = static_cast<int32_t>(pixelID_to_wi_vector.size()) +
                  pixelID_to_wi_offset;

    // Make an array where index = pixel ID
    // Set the value to NULL by default
    for (size_t i = 0; i < vectors.size(); ++i) {
      vectors[i].resize(eventid_max + 1, nullptr);
    }

    for (size_t j = size_t(pixelID_to_wi_offset);
         j < pixelID_to_wi_vector.size(); j++) {
      size_t wi = pixelID_to_wi_vector[j];
      // Save a POINTER to the vector
      if (wi < m_ws->getNumberHistograms()) {
        for (size_t period = 0; period < m_ws->nPeriods(); ++period) {
          getEventsFrom(m_ws->getSpectrum(wi, period),
                        vectors[period][j - pixelID_to_wi_offset]);
        }
      }
    }
  }
}

/**
* Get the number of events in the currently opened group.
*
* @param file The handle to the nexus file opened to the group to look at.
* @param hasTotalCounts Whether to try looking at the total_counts field. This
* variable will be changed if the field is not there.
* @param oldNeXusFileNames Whether to try using old names. This variable will
* be changed if it is determined that old names are being used.
*
* @return The number of events.
*/
std::size_t numEvents(::NeXus::File &file, bool &hasTotalCounts,
                      bool &oldNeXusFileNames) {
  // try getting the value of total_counts
  if (hasTotalCounts) {
    try {
      uint64_t numEvents;
      file.readData("total_counts", numEvents);
      return numEvents;
    } catch (::NeXus::Exception &) {
      hasTotalCounts = false; // carry on with the field not existing
    }
  }

  // just get the length of the event pixel ids
  try {
    if (oldNeXusFileNames)
      file.openData("event_pixel_id");
    else
      file.openData("event_id");
  } catch (::NeXus::Exception &) {
    // Older files (before Nov 5, 2010) used this field.
    try {
      file.openData("event_pixel_id");
      oldNeXusFileNames = true;
    } catch (::NeXus::Exception &) {
      // Some groups have neither indicating there are not events here
      return 0;
    }
  }

  size_t numEvents = static_cast<std::size_t>(file.getInfo().dims[0]);
  file.closeData();
  return numEvents;
}

void LoadEventNexus::createWorkspaceIndexMaps(
    const bool monitors, const std::vector<std::string> &bankNames) {
  // Create the required spectra mapping so that the workspace knows what to pad
  // to
  createSpectraMapping(m_filename, monitors, bankNames);

  // This map will be used to find the workspace index
  if (this->event_id_is_spec)
    pixelID_to_wi_vector =
        m_ws->getSpectrumToWorkspaceIndexVector(pixelID_to_wi_offset);
  else
    pixelID_to_wi_vector =
        m_ws->getDetectorIDToWorkspaceIndexVector(pixelID_to_wi_offset, true);
}

/** Load the instrument from the nexus file
*
* @param nexusfilename :: The name of the nexus file being loaded
* @param localWorkspace :: Templated workspace in which to put the instrument
*geometry
* @param alg :: Handle of the algorithm
* @param returnpulsetimes :: flag to return shared pointer for BankPulseTimes,
*otherwise NULL.
* @param nPeriods : Number of periods (write to)
* @param periodLog : Period logs DateAndTime to int map.
*
* @return Pulse times given in the DAS logs
*/
template <typename T>
boost::shared_ptr<BankPulseTimes> LoadEventNexus::runLoadNexusLogs(
    const std::string &nexusfilename, T localWorkspace, API::Algorithm &alg,
    bool returnpulsetimes, int &nPeriods,
    std::unique_ptr<const TimeSeriesProperty<int>> &periodLog) {
  // --------------------- Load DAS Logs -----------------
  // The pulse times will be empty if not specified in the DAS logs.
  // BankPulseTimes * out = NULL;
  boost::shared_ptr<BankPulseTimes> out;
  API::IAlgorithm_sptr loadLogs = alg.createChildAlgorithm("LoadNexusLogs");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    alg.getLogger().information() << "Loading logs from NeXus file..."
                                  << "\n";
    loadLogs->setPropertyValue("Filename", nexusfilename);
    loadLogs->setProperty<API::MatrixWorkspace_sptr>("Workspace",
                                                     localWorkspace);
    try {
      loadLogs->setPropertyValue("NXentryName",
                                 alg.getPropertyValue("NXentryName"));
    } catch (...) {
    }

    loadLogs->execute();

    const Run &run = localWorkspace->run();
    // Get the number of periods
    if (run.hasProperty("nperiods")) {
      nPeriods = run.getPropertyValueAsType<int>("nperiods");
    }
    // Get the period log. Map of DateAndTime to Period int values.
    if (run.hasProperty("period_log")) {
      auto *temp = run.getProperty("period_log");
      periodLog.reset(dynamic_cast<TimeSeriesProperty<int> *>(temp->clone()));
    }

    // If successful, we can try to load the pulse times
    Kernel::TimeSeriesProperty<double> *log =
        dynamic_cast<Kernel::TimeSeriesProperty<double> *>(
            localWorkspace->mutableRun().getProperty("proton_charge"));
    std::vector<Kernel::DateAndTime> temp;
    if (log)
      temp = log->timesAsVector();
    // if (returnpulsetimes) out = new BankPulseTimes(temp);
    if (returnpulsetimes)
      out = boost::make_shared<BankPulseTimes>(temp);

    // Use the first pulse as the run_start time.
    if (!temp.empty()) {
      if (temp[0] < Kernel::DateAndTime("1991-01-01T00:00:00"))
        alg.getLogger().warning() << "Found entries in the proton_charge "
                                     "sample log with invalid pulse time!\n";

      Kernel::DateAndTime run_start = localWorkspace->getFirstPulseTime();
      // add the start of the run as a ISO8601 date/time string. The start =
      // first non-zero time.
      // (this is used in LoadInstrument to find the right instrument file to
      // use).
      localWorkspace->mutableRun().addProperty(
          "run_start", run_start.toISO8601String(), true);
    } else {
      alg.getLogger().warning() << "Empty proton_charge sample log. You will "
                                   "not be able to filter by time.\n";
    }
    /// Attempt to make a gonoimeter from the logs
    try {
      Geometry::Goniometer gm;
      gm.makeUniversalGoniometer();
      localWorkspace->mutableRun().setGoniometer(gm, true);
    } catch (std::runtime_error &) {
    }
  } catch (...) {
    alg.getLogger().error() << "Error while loading Logs from SNS Nexus. Some "
                               "sample logs may be missing."
                            << "\n";
    return out;
  }
  return out;
}

/** Load the instrument from the nexus file
*
* @param nexusfilename :: The name of the nexus file being loaded
* @param localWorkspace :: EventWorkspaceCollection in which to put the
*instrument
*geometry
* @param alg :: Handle of the algorithm
* @param returnpulsetimes :: flag to return shared pointer for BankPulseTimes,
*otherwise NULL.
* @param nPeriods : Number of periods (write to)
* @param periodLog : Period logs DateAndTime to int map.
*
* @return Pulse times given in the DAS logs
*/
template <>
boost::shared_ptr<BankPulseTimes>
LoadEventNexus::runLoadNexusLogs<EventWorkspaceCollection_sptr>(
    const std::string &nexusfilename,
    EventWorkspaceCollection_sptr localWorkspace, API::Algorithm &alg,
    bool returnpulsetimes, int &nPeriods,
    std::unique_ptr<const TimeSeriesProperty<int>> &periodLog) {
  auto ws = localWorkspace->getSingleHeldWorkspace();
  auto ret = runLoadNexusLogs<MatrixWorkspace_sptr>(
      nexusfilename, ws, alg, returnpulsetimes, nPeriods, periodLog);
  return ret;
}

//-----------------------------------------------------------------------------
/**
* Load events from the file.
* @param prog :: A pointer to the progress reporting object
* @param monitors :: If true the events from the monitors are loaded and not the
* main banks
*
* This also loads the instrument, but only if it has not been set in the
*workspace
* being used as input (m_ws data member). Same applies to the logs.
*/
void LoadEventNexus::loadEvents(API::Progress *const prog,
                                const bool monitors) {
  bool metaDataOnly = getProperty("MetaDataOnly");

  // Get the time filters
  setTimeFilters(monitors);

  // The run_start will be loaded from the pulse times.
  DateAndTime run_start(0, 0);
  // Initialize the counter of bad TOFs
  bad_tofs = 0;
  int nPeriods = 1;
  auto periodLog = make_unique<const TimeSeriesProperty<int>>("period_log");
  if (!m_logs_loaded_correctly) {
    if (loadlogs) {
      prog->doReport("Loading DAS logs");

      m_allBanksPulseTimes = runLoadNexusLogs<EventWorkspaceCollection_sptr>(
          m_filename, m_ws, *this, true, nPeriods, periodLog);

      run_start = m_ws->getFirstPulseTime();
      m_logs_loaded_correctly = true;
    } else {
      g_log.information() << "Skipping the loading of sample logs!\n"
                          << "Reading the start time directly from /"
                          << m_top_entry_name << "/start_time\n";
      // start_time is read and set
      m_file->openPath("/");
      m_file->openGroup(m_top_entry_name, "NXentry");
      std::string tmp;
      m_file->readData("start_time", tmp);
      m_file->closeGroup();
      run_start = DateAndTime(tmp);
      m_ws->mutableRun().addProperty("run_start", run_start.toISO8601String(),
                                     true);
    }
  }
  m_ws->setNPeriods(
      nPeriods, periodLog); // This is how many workspaces we are going to make.

  // Make sure you have a non-NULL m_allBanksPulseTimes
  if (m_allBanksPulseTimes == nullptr) {
    std::vector<DateAndTime> temp;
    // m_allBanksPulseTimes = new BankPulseTimes(temp);
    m_allBanksPulseTimes = boost::make_shared<BankPulseTimes>(temp);
  }

  if (!m_ws->getInstrument() || !m_instrument_loaded_correctly) {
    // Load the instrument (if not loaded before)
    prog->report("Loading instrument");
    m_instrument_loaded_correctly =
        loadInstrument(m_filename, m_ws, m_top_entry_name, this);

    if (!m_instrument_loaded_correctly)
      throw std::runtime_error(
          "Instrument was not initialized correctly! Loading cannot continue.");
  }

  // top level file information
  m_file->openPath("/");
  // Start with the base entry
  m_file->openGroup(m_top_entry_name, "NXentry");

  // Now we want to go through all the bankN_event entries
  vector<string> bankNames;
  vector<std::size_t> bankNumEvents;
  size_t total_events = 0;
  map<string, string> entries = m_file->getEntries();
  map<string, string>::const_iterator it = entries.begin();
  std::string classType = monitors ? "NXmonitor" : "NXevent_data";
  ::NeXus::Info info;
  bool oldNeXusFileNames(false);
  bool hasTotalCounts(true);
  m_haveWeights = false;
  for (; it != entries.end(); ++it) {
    std::string entry_name(it->first);
    std::string entry_class(it->second);
    if (entry_class == classType) {
      // open the group
      m_file->openGroup(entry_name, classType);

      // get the number of events
      std::size_t num = numEvents(*m_file, hasTotalCounts, oldNeXusFileNames);
      bankNames.push_back(entry_name);
      bankNumEvents.push_back(num);
      total_events += num;

      // Look for weights in simulated file
      try {
        m_file->openData("event_weight");
        m_haveWeights = true;
        m_file->closeData();
      } catch (::NeXus::Exception &) {
        // Swallow exception since flag is already false;
      }

      m_file->closeGroup();
    }
  }

  loadSampleDataISIScompatibility(*m_file, *m_ws);

  // Close the 'top entry' group (raw_data_1 for NexusProcessed, etc.)
  m_file->closeGroup();

  // Delete the output workspace name if it existed
  std::string outName = getPropertyValue("OutputWorkspace");
  if (AnalysisDataService::Instance().doesExist(outName))
    AnalysisDataService::Instance().remove(outName);

  // set more properties on the workspace
  try {
    // this is a static method that is why it is passing the
    // file object and the file path
    loadEntryMetadata<EventWorkspaceCollection_sptr>(m_filename, m_ws,
                                                     m_top_entry_name);
  } catch (std::runtime_error &e) {
    // Missing metadata is not a fatal error. Log and go on with your life
    g_log.error() << "Error loading metadata: " << e.what() << '\n';
  }

  // --------------------------- Time filtering
  // ------------------------------------
  double filter_time_start_sec, filter_time_stop_sec;
  filter_time_start_sec = getProperty("FilterByTimeStart");
  filter_time_stop_sec = getProperty("FilterByTimeStop");
  chunk = getProperty("ChunkNumber");
  totalChunks = getProperty("TotalChunks");

  // Default to ALL pulse times
  bool is_time_filtered = false;
  filter_time_start = Kernel::DateAndTime::minimum();
  filter_time_stop = Kernel::DateAndTime::maximum();

  if (m_allBanksPulseTimes->numPulses > 0) {
    // If not specified, use the limits of doubles. Otherwise, convert from
    // seconds to absolute PulseTime
    if (filter_time_start_sec != EMPTY_DBL()) {
      filter_time_start = run_start + filter_time_start_sec;
      is_time_filtered = true;
    }

    if (filter_time_stop_sec != EMPTY_DBL()) {
      filter_time_stop = run_start + filter_time_stop_sec;
      is_time_filtered = true;
    }

    // Silly values?
    if (filter_time_stop < filter_time_start) {
      std::string msg = "Your ";
      if (monitors)
        msg += "monitor ";
      msg += "filter for time's Stop value is smaller than the Start value.";
      throw std::invalid_argument(msg);
    }
  }

  if (is_time_filtered) {
    // Now filter out the run, using the DateAndTime type.
    m_ws->mutableRun().filterByTime(filter_time_start, filter_time_stop);
  }

  if (metaDataOnly) {
    // Now, create a default X-vector for histogramming, with just 2 bins.
    auto axis = HistogramData::BinEdges{
        1, static_cast<double>(std::numeric_limits<uint32_t>::max()) * 0.1 - 1};
    // Set the binning axis using this.
    m_ws->setAllX(axis);

    createWorkspaceIndexMaps(monitors, std::vector<std::string>());
    return;
  }

  // --------- Loading only one bank ? ----------------------------------
  std::vector<std::string> someBanks = getProperty("BankName");
  bool SingleBankPixelsOnly = getProperty("SingleBankPixelsOnly");
  if ((!someBanks.empty()) && (!monitors)) {
    // check that all of the requested banks are in the file
    for (auto &someBank : someBanks) {
      bool foundIt = false;
      for (auto &bankName : bankNames) {
        if (bankName == someBank + "_events") {
          foundIt = true;
          break;
        }
      }
      if (!foundIt) {
        throw std::invalid_argument("No entry named '" + someBank +
                                    "' was found in the .NXS file.\n");
      }
    }

    // change the number of banks to load
    bankNames.clear();
    for (auto &someBank : someBanks)
      bankNames.push_back(someBank + "_events");

    // how many events are in a bank
    bankNumEvents.clear();
    bankNumEvents.assign(someBanks.size(),
                         1); // TODO this equally weights the banks

    if (!SingleBankPixelsOnly)
      someBanks.clear(); // Marker to load all pixels
  } else {
    someBanks.clear();
  }

  prog->report("Initializing all pixels");

  // Remove unused banks if parameter is set
  if (m_ws->getInstrument()->hasParameter("remove-unused-banks")) {
    std::vector<double> instrumentUnused =
        m_ws->getInstrument()->getNumberParameter("remove-unused-banks", true);
    if (!instrumentUnused.empty()) {
      const int unused = static_cast<int>(instrumentUnused.front());
      if (unused == 1)
        deleteBanks(m_ws, bankNames);
    }
  }
  //----------------- Pad Empty Pixels -------------------------------
  createWorkspaceIndexMaps(monitors, someBanks);

  // Cache a map for speed.
  if (!m_haveWeights) {
    this->makeMapToEventLists<EventVector_pt>(eventVectors);
  } else {
    // Convert to weighted events
    for (size_t i = 0; i < m_ws->getNumberHistograms(); i++) {
      m_ws->getSpectrum(i).switchTo(API::WEIGHTED);
    }
    this->makeMapToEventLists<WeightedEventVector_pt>(weightedEventVectors);
  }

  // Set all (empty) event lists as sorted by pulse time. That way, calling
  // SortEvents will not try to sort these empty lists.
  for (size_t i = 0; i < m_ws->getNumberHistograms(); i++)
    m_ws->getSpectrum(i).setSortOrder(DataObjects::PULSETIME_SORT);

  // Count the limits to time of flight
  shortest_tof =
      static_cast<double>(std::numeric_limits<uint32_t>::max()) * 0.1;
  longest_tof = 0.;

  // Make the thread pool
  ThreadScheduler *scheduler = new ThreadSchedulerMutexes();
  ThreadPool pool(scheduler);
  auto diskIOMutex = boost::make_shared<std::mutex>();
  size_t bank0 = 0;
  size_t bankn = bankNames.size();

  if (chunk !=
      EMPTY_INT()) // We are loading part - work out the bank number range
  {
    eventsPerChunk = total_events / totalChunks;
    // Sort banks by size
    size_t tmp;
    string stmp;
    for (size_t i = 0; i < bankn; i++)
      for (size_t j = 0; j < bankn - 1; j++)
        if (bankNumEvents[j] < bankNumEvents[j + 1]) {
          tmp = bankNumEvents[j];
          bankNumEvents[j] = bankNumEvents[j + 1];
          bankNumEvents[j + 1] = tmp;
          stmp = bankNames[j];
          bankNames[j] = bankNames[j + 1];
          bankNames[j + 1] = stmp;
        }
    int bigBanks = 0;
    for (size_t i = 0; i < bankn; i++)
      if (bankNumEvents[i] > eventsPerChunk)
        bigBanks++;
    // Each chunk is part of bank or multiple whole banks
    // 0.5 for last chunk of a bank with multiple chunks
    // 0.1 for multiple whole banks not completely filled
    eventsPerChunk +=
        static_cast<size_t>((static_cast<double>(bigBanks) /
                                 static_cast<double>(totalChunks) * 0.5 +
                             0.05) *
                            static_cast<double>(eventsPerChunk));
    double partialChunk = 0.;
    firstChunkForBank = 1;
    for (int chunki = 1; chunki <= chunk; chunki++) {
      if (partialChunk > 1.) {
        partialChunk = 0.;
        firstChunkForBank = chunki;
        bank0 = bankn;
      }
      if (bankNumEvents[bank0] > 1) {
        partialChunk += static_cast<double>(eventsPerChunk) /
                        static_cast<double>(bankNumEvents[bank0]);
      }
      if (chunki < totalChunks)
        bankn = bank0 + 1;
      else
        bankn = bankNames.size();
      if (chunki == firstChunkForBank && partialChunk > 1.0)
        bankn += static_cast<size_t>(partialChunk) - 1;
      if (bankn > bankNames.size())
        bankn = bankNames.size();
    }
    for (size_t i = bank0; i < bankn; i++) {
      size_t start_event = (chunk - firstChunkForBank) * eventsPerChunk;
      size_t stop_event = bankNumEvents[i];
      // Don't change stop_event for the final chunk
      if (start_event + eventsPerChunk < stop_event)
        stop_event = start_event + eventsPerChunk;
      bankNumEvents[i] = stop_event - start_event;
    }
  }

  // split banks up if the number of cores is more than twice the number of
  // banks
  splitProcessing =
      bool(bankNames.size() * 2 < ThreadPool::getNumPhysicalCores());

  // set up progress bar for the rest of the (multi-threaded) process
  size_t numProg = bankNames.size() * (1 + 3); // 1 = disktask, 3 = proc task
  if (splitProcessing)
    numProg += bankNames.size() * 3; // 3 = second proc task
  auto prog2 = make_unique<Progress>(this, 0.3, 1.0, numProg);

  const std::vector<int> periodLogVec = periodLog->valuesAsVector();

  for (size_t i = bank0; i < bankn; i++) {
    // We make tasks for loading
    if (bankNumEvents[i] > 0)
      pool.schedule(new LoadBankFromDiskTask(
          this, bankNames[i], classType, bankNumEvents[i], oldNeXusFileNames,
          prog2.get(), diskIOMutex, scheduler, periodLogVec));
  }
  // Start and end all threads
  pool.joinAll();
  diskIOMutex.reset();

  // Info reporting
  const std::size_t eventsLoaded = m_ws->getNumberEvents();
  g_log.information() << "Read " << eventsLoaded << " events"
                      << ". Shortest TOF: " << shortest_tof
                      << " microsec; longest TOF: " << longest_tof
                      << " microsec.\n";

  if (shortest_tof < 0)
    g_log.warning() << "The shortest TOF was negative! At least 1 event has an "
                       "invalid time-of-flight.\n";
  if (bad_tofs > 0)
    g_log.warning() << "Found " << bad_tofs << " events with TOF > 2e8. This "
                                               "may indicate errors in the raw "
                                               "TOF data.\n";

  // Use T0 offset from TOPAZ Parameter file if it exists
  if (m_ws->getInstrument()->hasParameter("T0")) {
    std::vector<double> instrumentT0 =
        m_ws->getInstrument()->getNumberParameter("T0", true);
    if (!instrumentT0.empty()) {
      const double mT0 = instrumentT0.front();
      if (mT0 != 0.0) {
        int64_t numHistograms =
            static_cast<int64_t>(m_ws->getNumberHistograms());
        PARALLEL_FOR_IF(Kernel::threadSafe(*m_ws))
        for (int64_t i = 0; i < numHistograms; ++i) {
          PARALLEL_START_INTERUPT_REGION
          // Do the offsetting
          m_ws->getSpectrum(i).addTof(mT0);
          PARALLEL_END_INTERUPT_REGION
        }
        PARALLEL_CHECK_INTERUPT_REGION
        // set T0 in the run parameters
        API::Run &run = m_ws->mutableRun();
        run.addProperty<double>("T0", mT0, true);
      }
    }
  }
  // Now, create a default X-vector for histogramming, with just 2 bins.
  if (eventsLoaded > 0)
    m_ws->setAllX(HistogramData::BinEdges{shortest_tof - 1, longest_tof + 1});
  else
    m_ws->setAllX(HistogramData::BinEdges{0.0, 1.0});

  // if there is time_of_flight load it
  loadTimeOfFlight(m_ws, m_top_entry_name, classType);
}

//-----------------------------------------------------------------------------
/** Load the instrument from the nexus file
*
*  @param nexusfilename :: The name of the nexus file being loaded
*  @param localWorkspace :: EventWorkspaceCollection in which to put the
*instrument
*geometry
*  @param top_entry_name :: entry name at the top of the Nexus file
*  @param alg :: Handle of the algorithm
*  @return true if successful
*/
template <>
bool LoadEventNexus::runLoadIDFFromNexus<EventWorkspaceCollection_sptr>(
    const std::string &nexusfilename,
    EventWorkspaceCollection_sptr localWorkspace,
    const std::string &top_entry_name, Algorithm *alg) {
  auto ws = localWorkspace->getSingleHeldWorkspace();
  auto hasLoaded = runLoadIDFFromNexus<MatrixWorkspace_sptr>(
      nexusfilename, ws, top_entry_name, alg);
  localWorkspace->setInstrument(ws->getInstrument());
  return hasLoaded;
}

/** method used to return instrument name for some old ISIS files where it is
* not written properly within the instrument
* @param hFile :: A reference to the NeXus file opened at the root entry
*/
std::string
LoadEventNexus::readInstrumentFromISIS_VMSCompat(::NeXus::File &hFile) {
  std::string instrumentName;
  try {
    hFile.openGroup("isis_vms_compat", "IXvms");
  } catch (std::runtime_error &) {
    return instrumentName;
  }
  try {
    hFile.openData("NAME");
  } catch (std::runtime_error &) {
    hFile.closeGroup();
    return instrumentName;
  }

  instrumentName = hFile.getStrData();
  hFile.closeData();
  hFile.closeGroup();

  return instrumentName;
}

//-----------------------------------------------------------------------------
/** Load the instrument definition file specified by info in the NXS file for
* a EventWorkspaceCollection
*
*  @param nexusfilename :: Used to pick the instrument.
*  @param localWorkspace :: EventWorkspaceCollection in which to put the
*instrument
*geometry
*  @param top_entry_name :: entry name at the top of the NXS file
*  @param alg :: Handle of the algorithm
*  @return true if successful
*/
template <>
bool LoadEventNexus::runLoadInstrument<EventWorkspaceCollection_sptr>(
    const std::string &nexusfilename,
    EventWorkspaceCollection_sptr localWorkspace,
    const std::string &top_entry_name, Algorithm *alg) {
  auto ws = localWorkspace->getSingleHeldWorkspace();
  auto hasLoaded = runLoadInstrument<MatrixWorkspace_sptr>(nexusfilename, ws,
                                                           top_entry_name, alg);
  localWorkspace->setInstrument(ws->getInstrument());
  return hasLoaded;
}

//-----------------------------------------------------------------------------
/**
* Deletes banks for a workspace given the bank names.
* @param workspace :: The workspace to contain the spectra mapping
* @param bankNames :: Bank names that are in Nexus file
*/
void LoadEventNexus::deleteBanks(EventWorkspaceCollection_sptr workspace,
                                 std::vector<std::string> bankNames) {
  Instrument_sptr inst = boost::const_pointer_cast<Instrument>(
      workspace->getInstrument()->baseInstrument());
  // Build a list of Rectangular Detectors
  std::vector<boost::shared_ptr<RectangularDetector>> detList;
  for (int i = 0; i < inst->nelements(); i++) {
    boost::shared_ptr<RectangularDetector> det;
    boost::shared_ptr<ICompAssembly> assem;
    boost::shared_ptr<ICompAssembly> assem2;

    det = boost::dynamic_pointer_cast<RectangularDetector>((*inst)[i]);
    if (det) {
      detList.push_back(det);
    } else {
      // Also, look in the first sub-level for RectangularDetectors (e.g. PG3).
      // We are not doing a full recursive search since that will be very long
      // for lots of pixels.
      assem = boost::dynamic_pointer_cast<ICompAssembly>((*inst)[i]);
      if (assem) {
        for (int j = 0; j < assem->nelements(); j++) {
          det = boost::dynamic_pointer_cast<RectangularDetector>((*assem)[j]);
          if (det) {
            detList.push_back(det);

          } else {
            // Also, look in the second sub-level for RectangularDetectors (e.g.
            // PG3).
            // We are not doing a full recursive search since that will be very
            // long for lots of pixels.
            assem2 = boost::dynamic_pointer_cast<ICompAssembly>((*assem)[j]);
            if (assem2) {
              for (int k = 0; k < assem2->nelements(); k++) {
                det = boost::dynamic_pointer_cast<RectangularDetector>(
                    (*assem2)[k]);
                if (det) {
                  detList.push_back(det);
                }
              }
            }
          }
        }
      }
    }
  }
  if (detList.empty())
    return;
  for (auto &det : detList) {
    bool keep = false;
    std::string det_name = det->getName();
    for (auto &bankName : bankNames) {
      size_t pos = bankName.find("_events");
      if (det_name == bankName.substr(0, pos))
        keep = true;
      if (keep)
        break;
    }
    if (!keep) {
      boost::shared_ptr<const IComponent> parent =
          inst->getComponentByName(det_name);
      std::vector<Geometry::IComponent_const_sptr> children;
      boost::shared_ptr<const Geometry::ICompAssembly> asmb =
          boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(parent);
      asmb->getChildren(children, false);
      for (auto &col : children) {
        boost::shared_ptr<const Geometry::ICompAssembly> asmb2 =
            boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(col);
        std::vector<Geometry::IComponent_const_sptr> grandchildren;
        asmb2->getChildren(grandchildren, false);

        for (auto &row : grandchildren) {
          Detector *d =
              dynamic_cast<Detector *>(const_cast<IComponent *>(row.get()));
          if (d)
            inst->removeDetector(d);
        }
      }
      IComponent *comp = dynamic_cast<IComponent *>(det.get());
      inst->remove(comp);
    }
  }
}
//-----------------------------------------------------------------------------
/**
* Create the required spectra mapping. If the file contains an isis_vms_compat
* block then
* the mapping is read from there, otherwise a 1:1 map with the instrument is
* created (along
* with the associated spectra axis)
* @param nxsfile :: The name of a nexus file to load the mapping from
* @param monitorsOnly :: Load only the monitors is true
* @param bankNames :: An optional bank name for loading specified banks
*/
void LoadEventNexus::createSpectraMapping(
    const std::string &nxsfile, const bool monitorsOnly,
    const std::vector<std::string> &bankNames) {
  bool spectramap = false;
  m_specMin = getProperty("SpectrumMin");
  m_specMax = getProperty("SpectrumMax");
  m_specList = getProperty("SpectrumList");

  // set up the
  if (!monitorsOnly && !bankNames.empty()) {
    std::vector<IDetector_const_sptr> allDets;

    for (const auto &bankName : bankNames) {
      // Only build the map for the single bank
      std::vector<IDetector_const_sptr> dets;
      m_ws->getInstrument()->getDetectorsInBank(dets, bankName);
      if (dets.empty())
        throw std::runtime_error("Could not find the bank named '" + bankName +
                                 "' as a component assembly in the instrument "
                                 "tree; or it did not contain any detectors."
                                 " Try unchecking SingleBankPixelsOnly.");
      allDets.insert(allDets.end(), dets.begin(), dets.end());
    }
    if (!allDets.empty()) {
      m_ws->resizeTo(allDets.size());
      // Make an event list for each.
      for (size_t wi = 0; wi < allDets.size(); wi++) {
        const detid_t detID = allDets[wi]->getID();
        m_ws->setDetectorIdsForAllPeriods(wi, detID);
      }
      spectramap = true;
      g_log.debug() << "Populated spectra map for select banks\n";
    }

  } else {
    spectramap = loadSpectraMapping(nxsfile, monitorsOnly, m_top_entry_name);
    // Did we load one? If so then the event ID is the spectrum number and not
    // det ID
    if (spectramap)
      this->event_id_is_spec = true;
  }

  if (!spectramap) {
    g_log.debug() << "No custom spectra mapping found, continuing with default "
                     "1:1 mapping of spectrum:detectorID\n";
    auto specList = m_ws->getInstrument()->getDetectorIDs(true);
    createSpectraList(*std::min_element(specList.begin(), specList.end()),
                      *std::max_element(specList.begin(), specList.end()));
    // The default 1:1 will suffice but exclude the monitors as they are always
    // in a separate workspace
    m_ws->padSpectra(m_specList);
    g_log.debug() << "Populated 1:1 spectra map for the whole instrument \n";
  }
}

//-----------------------------------------------------------------------------
/**
* Returns whether the file contains monitors with events in them
* @returns True if the file contains monitors with event data, false otherwise
*/
bool LoadEventNexus::hasEventMonitors() {
  bool result(false);
  // Determine whether to load histograms or events
  try {
    m_file->openPath("/" + m_top_entry_name);
    // Start with the base entry
    typedef std::map<std::string, std::string> string_map_t;
    // Now we want to go through and find the monitors
    string_map_t entries = m_file->getEntries();
    for (string_map_t::const_iterator it = entries.begin(); it != entries.end();
         ++it) {
      if (it->second == "NXmonitor") {
        m_file->openGroup(it->first, it->second);
        break;
      }
    }
    m_file->openData("event_id");
    m_file->closeGroup();
    result = true;
  } catch (::NeXus::Exception &) {
    result = false;
  }
  return result;
}

/**
* Load the Monitors from the NeXus file into an event workspace. A
* new event workspace is created and associated to the data
* workspace. The name of the new event workspace is contructed by
* appending '_monitors' to the base workspace name.
*
* This is used when the property "MonitorsAsEvents" is enabled, and
* there are monitors with events.
*
* @param prog :: progress reporter
*/
void LoadEventNexus::runLoadMonitorsAsEvents(API::Progress *const prog) {
  try {
    // Note the reuse of the m_ws member variable below. Means I need to grab a
    // copy of its current value.
    auto dataWS = m_ws;
    m_ws = boost::make_shared<EventWorkspaceCollection>(); // Algorithm
                                                           // currently relies
                                                           // on an
    // object-level workspace ptr
    // add filename
    m_ws->mutableRun().addProperty("Filename", m_filename);

    // Re-use instrument, which has probably been loaded into the data
    // workspace (this happens in the first call to loadEvents() (inside
    // LoadEventNexuss::exec()). The second call to loadEvents(), immediately
    // below, can re-use it.
    if (m_instrument_loaded_correctly) {
      m_ws->setInstrument(dataWS->getInstrument());
      g_log.information() << "Instrument data copied into monitors workspace "
                             " from the data workspace.\n";
    }

    // Perform the load (only events from monitor)
    loadEvents(prog, true);

    // and re-use log entries (but only after loading metadata in loadEvents()
    // this is not strictly needed for the load to work (like the instrument is)
    // so it can be done after loadEvents, and it doesn't throw
    if (m_logs_loaded_correctly) {
      g_log.information()
          << "Copying log data into monitors workspace from the "
          << "data workspace.\n";
      try {
        auto to = m_ws->getSingleHeldWorkspace();
        copyLogs(dataWS, to);
        g_log.information() << "Log data copied.\n";
      } catch (std::runtime_error &) {
        g_log.error()
            << "Could not copy log data into monitors workspace. Some "
               " logs may be wrong and/or missing in the output "
               "monitors workspace.\n";
      }
    }

    std::string mon_wsname = this->getProperty("OutputWorkspace");
    mon_wsname.append("_monitors");
    this->declareProperty(
        Kernel::make_unique<WorkspaceProperty<IEventWorkspace>>(
            "MonitorWorkspace", mon_wsname, Direction::Output),
        "Monitors from the Event NeXus file");
    this->setProperty<IEventWorkspace_sptr>("MonitorWorkspace",
                                            m_ws->getSingleHeldWorkspace());
    // Set the internal monitor workspace pointer as well
    dataWS->setMonitorWorkspace(m_ws->getSingleHeldWorkspace());
    // If the run was paused at any point, filter out those events (SNS only, I
    // think)
    filterDuringPause(m_ws);
  } catch (const std::exception &e) {
    g_log.error() << "Error while loading monitors as events from file: ";
    g_log.error() << e.what() << '\n';
  }
}

//-----------------------------------------------------------------------------
/**
* Load the Monitors from the NeXus file into a workspace. The original
* workspace name is used and appended with _monitors.
*
* This is used when the property "MonitorsAsEvents" is not
* enabled, and uses LoadNexusMonitors to load monitor data into a
* Workspace2D.
*/
void LoadEventNexus::runLoadMonitors() {
  std::string mon_wsname = this->getProperty("OutputWorkspace");
  mon_wsname.append("_monitors");

  IAlgorithm_sptr loadMonitors =
      this->createChildAlgorithm("LoadNexusMonitors");
  try {
    g_log.information("Loading monitors from NeXus file...");
    loadMonitors->setPropertyValue("Filename", m_filename);
    g_log.information() << "New workspace name for monitors: " << mon_wsname
                        << '\n';
    loadMonitors->setPropertyValue("OutputWorkspace", mon_wsname);
    loadMonitors->setPropertyValue("MonitorsAsEvents",
                                   this->getProperty("MonitorsAsEvents"));
    loadMonitors->setPropertyValue("LoadEventMonitors",
                                   this->getProperty("LoadEventMonitors"));
    loadMonitors->setPropertyValue("LoadHistoMonitors",
                                   this->getProperty("LoadHistoMonitors"));
    loadMonitors->execute();
    Workspace_sptr monsOut = loadMonitors->getProperty("OutputWorkspace");
    this->declareProperty(
        Kernel::make_unique<WorkspaceProperty<Workspace>>(
            "MonitorWorkspace", mon_wsname, Direction::Output),
        "Monitors from the Event NeXus file");
    this->setProperty("MonitorWorkspace", monsOut);
    // The output will either be a group workspace or a matrix workspace
    MatrixWorkspace_sptr mons =
        boost::dynamic_pointer_cast<MatrixWorkspace>(monsOut);
    if (mons) {
      // Set the internal monitor workspace pointer as well
      m_ws->setMonitorWorkspace(mons);

      filterDuringPause(mons);
    } else {
      WorkspaceGroup_sptr monsGrp =
          boost::dynamic_pointer_cast<WorkspaceGroup>(monsOut);
      if (monsGrp) {
        // declare a property for each member of the group
        for (int i = 0; i < monsGrp->getNumberOfEntries(); i++) {
          std::stringstream ssWsName;
          ssWsName << mon_wsname << "_" << i + 1;
          std::stringstream ssPropName;
          ssPropName << "MonitorWorkspace"
                     << "_" << i + 1;
          this->declareProperty(
              Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                  ssPropName.str(), ssWsName.str(), Direction::Output),
              "Monitors from the Event NeXus file");
          this->setProperty(ssPropName.str(), monsGrp->getItem(i));
        }
      }
    }
  } catch (...) {
    g_log.error("Error while loading the monitors from the file. File may "
                "contain no monitors.");
  }
}

//
/**
* Load a spectra mapping from the given file. This currently checks for the
* existence of
* an isis_vms_compat block in the file, if it exists it pulls out the spectra
* mapping listed there
* @param filename :: A filename
* @param monitorsOnly :: If true then only the monitor spectra are loaded
* @param entry_name :: name of the NXentry to open.
* @returns True if the mapping was loaded or false if the block does not exist
*/
bool LoadEventNexus::loadSpectraMapping(const std::string &filename,
                                        const bool monitorsOnly,
                                        const std::string &entry_name) {
  const std::string vms_str = "/isis_vms_compat";
  try {
    g_log.debug() << "Attempting to load custom spectra mapping from '"
                  << entry_name << vms_str << "'.\n";
    m_file->openPath("/" + entry_name + vms_str);
  } catch (::NeXus::Exception &) {
    return false; // Doesn't exist
  }

  // The ISIS spectrum mapping is defined by 2 arrays in isis_vms_compat block:
  //   UDET - An array of detector IDs
  //   SPEC - An array of spectrum numbers
  // There sizes must match. Hardware allows more than one detector ID to be
  // mapped to a single spectrum
  // and this is encoded in the SPEC/UDET arrays by repeating the spectrum
  // number in the array
  // for each mapped detector, e.g.
  //
  // 1 1001
  // 1 1002
  // 2 2001
  // 3 3001
  //
  // defines 3 spectra, where the first spectrum contains 2 detectors

  // UDET
  m_file->openData("UDET");
  std::vector<int32_t> udet;
  m_file->getData(udet);
  m_file->closeData();
  // SPEC
  m_file->openData("SPEC");
  std::vector<int32_t> spec;
  m_file->getData(spec);
  m_file->closeData();
  // Go up/back. this assumes one level for entry name and a second level
  // for /isis_vms_compat, typically: /raw_data_1/isis_vms_compat
  m_file->closeGroup();
  m_file->closeGroup();

  // The spec array will contain a spectrum number for each udet but the
  // spectrum number
  // may be the same for more that one detector
  const size_t ndets(udet.size());
  if (ndets != spec.size()) {
    std::ostringstream os;
    os << "UDET/SPEC list size mismatch. UDET=" << udet.size()
       << ", SPEC=" << spec.size() << "\n";
    throw std::runtime_error(os.str());
  }
  // Monitor filtering/selection
  const std::vector<detid_t> monitors = m_ws->getInstrument()->getMonitors();
  const size_t nmons(monitors.size());
  if (monitorsOnly) {
    g_log.debug() << "Loading only monitor spectra from " << filename << "\n";
    // Find the det_ids in the udet array.
    m_ws->resizeTo(nmons);
    for (size_t i = 0; i < nmons; ++i) {
      // Find the index in the udet array
      const detid_t &id = monitors[i];
      std::vector<int32_t>::const_iterator it =
          std::find(udet.begin(), udet.end(), id);
      if (it != udet.end()) {
        const specnum_t &specNo = spec[it - udet.begin()];
        m_ws->setSpectrumNumberForAllPeriods(i, specNo);
        m_ws->setDetectorIdsForAllPeriods(i, id);
      }
    }
  } else {
    g_log.debug() << "Loading only detector spectra from " << filename << "\n";

    // If optional spectra are provided, if so, m_specList is initialized. spec
    // is used if necessary
    createSpectraList(*std::min_element(spec.begin(), spec.end()),
                      *std::max_element(spec.begin(), spec.end()));

    if (!m_specList.empty()) {
      int i = 0;
      std::vector<int32_t> spec_temp, udet_temp;
      for (auto &element : spec) {
        if (find(m_specList.begin(), m_specList.end(), element) !=
            m_specList.end()) // spec element *it is not in spec_list
        {
          spec_temp.push_back(element);
          udet_temp.push_back(udet.at(i));
        }
        i++;
      }
      spec = spec_temp;
      udet = udet_temp;
    }

    SpectrumDetectorMapping mapping(spec, udet, monitors);
    m_ws->resizeTo(mapping.getMapping().size());
    // Make sure spectrum numbers are correct
    auto uniqueSpectra = mapping.getSpectrumNumbers();
    m_ws->setSpectrumNumbersFromUniqueSpectra(uniqueSpectra);
    // Fill detectors based on this mapping
    m_ws->updateSpectraUsing(mapping);
  }
  return true;
}

/**
* Set the filters on TOF.
* @param monitors :: If true check the monitor properties else use the standard
* ones
*/
void LoadEventNexus::setTimeFilters(const bool monitors) {
  // Get the limits to the filter
  std::string prefix("Filter");
  if (monitors)
    prefix += "Mon";

  filter_tof_min = getProperty(prefix + "ByTofMin");
  filter_tof_max = getProperty(prefix + "ByTofMax");
  if ((filter_tof_min == EMPTY_DBL()) && (filter_tof_max == EMPTY_DBL())) {
    // Nothing specified. Include everything
    filter_tof_min = -1e20;
    filter_tof_max = +1e20;
  } else if ((filter_tof_min != EMPTY_DBL()) &&
             (filter_tof_max != EMPTY_DBL())) {
    // Both specified. Keep these values
  } else {
    std::string msg("You must specify both min & max or neither TOF filters");
    if (monitors)
      msg = " for the monitors.";
    throw std::invalid_argument(msg);
  }
}

//-----------------------------------------------------------------------------
//               ISIS event corrections
//-----------------------------------------------------------------------------
/**
* Check if time_of_flight can be found in the file and load it
*
* @param WS :: The event workspace collection which events will be modified.
* @param entry_name :: An NXentry tag in the file
* @param classType :: The type of the events: either detector or monitor
*/
void LoadEventNexus::loadTimeOfFlight(EventWorkspaceCollection_sptr WS,
                                      const std::string &entry_name,
                                      const std::string &classType) {
  bool done = false;
  // Go to the root, and then top entry
  m_file->openPath("/");
  m_file->openGroup(entry_name, "NXentry");

  typedef std::map<std::string, std::string> string_map_t;
  string_map_t entries = m_file->getEntries();

  if (entries.find("detector_1_events") == entries.end()) { // not an ISIS file
    return;
  }

  // try if monitors have their own bins
  if (classType == "NXmonitor") {
    std::vector<std::string> bankNames;
    for (string_map_t::const_iterator it = entries.begin(); it != entries.end();
         ++it) {
      std::string entry_name(it->first);
      std::string entry_class(it->second);
      if (entry_class == classType) {
        bankNames.push_back(entry_name);
      }
    }
    for (size_t i = 0; i < bankNames.size(); ++i) {
      const std::string &mon = bankNames[i];
      m_file->openGroup(mon, classType);
      entries = m_file->getEntries();
      string_map_t::const_iterator bins = entries.find("event_time_bins");
      if (bins == entries.end()) {
        // bins = entries.find("time_of_flight"); // I think time_of_flight
        // doesn't work here
        // if (bins == entries.end())
        //{
        done = false;
        m_file->closeGroup();
        break; // done == false => use bins from the detectors
               //}
      }
      done = true;
      loadTimeOfFlightData(*m_file, WS, bins->first, i, i + 1);
      m_file->closeGroup();
    }
  }

  if (!done) {
    // first check detector_1_events
    m_file->openGroup("detector_1_events", "NXevent_data");
    entries = m_file->getEntries();
    for (string_map_t::const_iterator it = entries.begin(); it != entries.end();
         ++it) {
      if (it->first == "time_of_flight" || it->first == "event_time_bins") {
        loadTimeOfFlightData(*m_file, WS, it->first);
        done = true;
      }
    }
    m_file->closeGroup(); // detector_1_events

    if (!done) // if time_of_flight was not found try
               // instrument/dae/time_channels_#
    {
      m_file->openGroup("instrument", "NXinstrument");
      m_file->openGroup("dae", "IXdae");
      entries = m_file->getEntries();
      size_t time_channels_number = 0;
      for (string_map_t::const_iterator it = entries.begin();
           it != entries.end(); ++it) {
        // check if there are groups with names "time_channels_#" and select the
        // one with the highest number
        if (it->first.size() > 14 &&
            it->first.substr(0, 14) == "time_channels_") {
          size_t n = boost::lexical_cast<size_t>(it->first.substr(14));
          if (n > time_channels_number) {
            time_channels_number = n;
          }
        }
      }
      if (time_channels_number > 0) // the numbers start with 1
      {
        m_file->openGroup("time_channels_" +
                              std::to_string(time_channels_number),
                          "IXtime_channels");
        entries = m_file->getEntries();
        for (string_map_t::const_iterator it = entries.begin();
             it != entries.end(); ++it) {
          if (it->first == "time_of_flight" || it->first == "event_time_bins") {
            loadTimeOfFlightData(*m_file, WS, it->first);
          }
        }
        m_file->closeGroup();
      }
      m_file->closeGroup(); // dae
      m_file->closeGroup(); // instrument
    }
  }

  // close top entry (or entry given in entry_name)
  m_file->closeGroup();
}

//-----------------------------------------------------------------------------
/**
* Load the time of flight data. file must have open the group containing
* "time_of_flight" data set.
* @param file :: The nexus file to read from.
* @param WS :: The event workspace collection to write to.
* @param binsName :: bins name
* @param start_wi :: First workspace index to process
* @param end_wi :: Last workspace index to process
*/
void LoadEventNexus::loadTimeOfFlightData(::NeXus::File &file,
                                          EventWorkspaceCollection_sptr WS,
                                          const std::string &binsName,
                                          size_t start_wi, size_t end_wi) {
  // first check if the data is already randomized
  std::map<std::string, std::string> entries;
  file.getEntries(entries);
  std::map<std::string, std::string>::const_iterator shift =
      entries.find("event_time_offset_shift");
  if (shift != entries.end()) {
    std::string random;
    file.readData("event_time_offset_shift", random);
    if (random == "random") {
      return;
    }
  }

  // if the data is not randomized randomize it uniformly within each bin
  file.openData(binsName);
  // time of flights of events
  std::vector<float> tof;
  file.getData(tof);
  // todo: try to find if tof can be reduced to just 3 numbers: start, end and
  // dt
  if (end_wi <= start_wi) {
    end_wi = WS->getNumberHistograms();
  }

  // random number generator
  boost::mt19937 rand_gen;

  // loop over spectra
  for (size_t wi = start_wi; wi < end_wi; ++wi) {
    EventList &event_list = WS->getSpectrum(wi);
    // sort the events
    event_list.sortTof();
    std::vector<TofEvent> &events = event_list.getEvents();
    if (events.empty())
      continue;
    size_t n = tof.size();
    // iterate over the events and time bins
    auto ev = events.begin();
    auto ev_end = events.end();
    for (size_t i = 1; i < n; ++i) {
      double right = double(tof[i]);
      // find the right boundary for the current event
      if (ev != ev_end && right < ev->tof()) {
        continue;
      }
      // count events which have the same right boundary
      size_t m = 0;
      while (ev != ev_end && ev->tof() < right) {
        ++ev;
        ++m; // count events in the i-th bin
      }

      if (m > 0) { // m events in this bin
        double left = double(tof[i - 1]);
        // spread the events uniformly inside the bin
        boost::uniform_real<> distribution(left, right);
        std::vector<double> random_numbers(m);
        for (double &random_number : random_numbers) {
          random_number = distribution(rand_gen);
        }
        std::sort(random_numbers.begin(), random_numbers.end());
        auto it = random_numbers.begin();
        for (auto ev1 = ev - m; ev1 != ev; ++ev1, ++it) {
          ev1->m_tof = *it;
        }
      }

    } // for i

    event_list.sortTof();
  } // for wi
  file.closeData();
}

/**
* Load information of the sample. It is valid only for ISIS it get the
* information from the group isis_vms_compat.
*
* If it does not find this group, it assumes that there is nothing to do.
* But, if the information is there, but not in the way it was expected, it
* will log the occurrence.
*
* @note: It does essentially the same thing of the
* method: LoadISISNexus2::loadSampleData
*
* @param file : handle to the nexus file
* @param WS : pointer to the workspace
*/
void LoadEventNexus::loadSampleDataISIScompatibility(
    ::NeXus::File &file, EventWorkspaceCollection &WS) {
  try {
    file.openGroup("isis_vms_compat", "IXvms");
  } catch (::NeXus::Exception &) {
    // No problem, it just means that this entry does not exist
    return;
  }

  // read the data
  try {
    std::vector<int32_t> spb;
    std::vector<float> rspb;
    file.readData("SPB", spb);
    file.readData("RSPB", rspb);

    WS.setGeometryFlag(spb[2]); // the flag is in the third value
    WS.setThickness(rspb[3]);
    WS.setHeight(rspb[4]);
    WS.setWidth(rspb[5]);
  } catch (::NeXus::Exception &ex) {
    // it means that the data was not as expected, report the problem
    std::stringstream s;
    s << "Wrong definition found in isis_vms_compat :> " << ex.what();
    file.closeGroup();
    throw std::runtime_error(s.str());
  }

  file.closeGroup();
}

/**
* Check the validity of the optional spectrum range/list provided and identify
*if partial data should be loaded.
*
* @param min :: The minimum spectrum number read from file
* @param max :: The maximum spectrum number read from file
*/

void LoadEventNexus::createSpectraList(int32_t min, int32_t max) {

  // check if range [SpectrumMin, SpectrumMax] was supplied
  if (m_specMin != EMPTY_INT() || m_specMax != EMPTY_INT()) {
    if (m_specMax == EMPTY_INT()) {
      m_specMax = max;
    }
    if (m_specMin == EMPTY_INT()) {
      m_specMin = min;
    }

    if (m_specMax > max) {
      throw std::invalid_argument("Inconsistent range property: SpectrumMax is "
                                  "larger than maximum spectrum found in "
                                  "file.");
    }

    // Sanity checks for min/max
    if (m_specMin > m_specMax) {
      throw std::invalid_argument("Inconsistent range property: SpectrumMin is "
                                  "larger than SpectrumMax.");
    }

    // Populate spec_list
    for (int32_t i = m_specMin; i <= m_specMax; i++)
      m_specList.push_back(i);
  } else {
    // Check if SpectrumList was supplied

    if (!m_specList.empty()) {
      // Check no negative/zero numbers have been passed
      auto itr = std::find_if(m_specList.begin(), m_specList.end(),
                              std::bind2nd(std::less<int32_t>(), 1));
      if (itr != m_specList.end()) {
        throw std::invalid_argument(
            "Negative/Zero SpectraList property encountered.");
      }

      // Check range and set m_specMax to maximum value in m_specList
      if ((m_specMax =
               *std::max_element(m_specList.begin(), m_specList.end())) >
          *std::max_element(m_specList.begin(), m_specList.end())) {
        throw std::invalid_argument("Inconsistent range property: SpectrumMax "
                                    "is larger than number of spectra.");
      }

      // Set m_specMin to minimum value in m_specList
      m_specMin = *std::min_element(m_specList.begin(), m_specList.end());
    }
  }

  if (!m_specList.empty()) {

    // Check that spectra supplied by user do not correspond to monitors
    auto nmonitors = m_ws->getInstrument()->getMonitors().size();

    for (size_t i = 0; i < nmonitors; ++i) {
      if (std::find(m_specList.begin(), m_specList.end(), i + 1) !=
          m_specList.end()) {
        throw std::invalid_argument("Inconsistent range property: some of the "
                                    "selected spectra correspond to monitors.");
      }
    }
  }
}

/**
 * Makes sure that m_file is a valid and open NeXus::File object.
 * Throws if there is an exception opening the file.
 *
 * @param fname name of the nexus file to open
 */
void LoadEventNexus::safeOpenFile(const std::string fname) {
  try {
    m_file = new ::NeXus::File(m_filename, NXACC_READ);
  } catch (std::runtime_error &e) {
    throw std::runtime_error("Severe failure when trying to open NeXus file: " +
                             std::string(e.what()));
  }
  // make sure that by no means we could dereference NULL later on
  if (!m_file) {
    throw std::runtime_error("An unexpected failure happened, unable to "
                             "initialize file object when trying to open NeXus "
                             "file: " +
                             fname);
  }
}

} // namespace DataHandling
} // namespace Mantid
