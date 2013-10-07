/*WIKI* 

The LoadEventNeXus algorithm loads data from an EventNexus file into an [[EventWorkspace]]. The default histogram bin boundaries consist of a single bin able to hold all events (in all pixels), and will have their [[units]] set to time-of-flight. Since it is an [[EventWorkspace]], it can be rebinned to finer bins with no loss of data.

Sample logs, such as motor positions or e.g. temperature vs time, are also loaded using the [[LoadNexusLogs]] child algorithm.

=== Optional properties ===

If desired, you can filter out the events at the time of loading, by specifying minimum and maximum time-of-flight values. This can speed up loading and reduce memory requirements if you are only interested in a narrow range of the times-of-flight of your data. 

You may also filter out events by providing the start and stop times, in seconds, relative to the first pulse (the start of the run).

If you wish to load only a single bank, you may enter its name and no events from other banks will be loaded.

The Precount option will count the number of events in each pixel before allocating the memory for each event list. Without this option, because of the way vectors grow and are re-allocated, it is possible for up to 2x too much memory to be allocated for a given event list, meaning that your EventWorkspace may occupy nearly twice as much memory as needed. The pre-counting step takes some time but that is normally compensated by the speed-up in avoid re-allocating, so the net result is smaller memory footprint and approximately the same loading time.

==== Veto Pulses ====

Veto pulses can be filtered out in a separate step using [[FilterByLogValue]]:

 FilterByLogValue(InputWorkspace="ws", OutputWorkspace="ws", LogName="veto_pulse_time", PulseFilter="1")


*WIKI*/


//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadEventNexus.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/shared_array.hpp>

#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ThreadSchedulerMutexes.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidKernel/Timer.h"

using std::endl;
using std::map;
using std::string;
using std::vector;

using namespace ::NeXus;

namespace Mantid
{
namespace DataHandling
{

DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadEventNexus);

using namespace Kernel;
using namespace Geometry;
using namespace API;
using namespace DataObjects;

//===============================================================================================
//===============================================================================================
/** Constructor. Loads the pulse times from the bank entry of the file
 *
 * @param file :: nexus file open in the right bank entry
 */
BankPulseTimes::BankPulseTimes(::NeXus::File & file)
{
  file.openData("event_time_zero");
  // Read the offset (time zero)
  file.getAttr("offset", startTime);
  DateAndTime start(startTime);
  // Load the seconds offsets
  std::vector<double> seconds;
  file.getData(seconds);
  file.closeData();
  // Now create the pulseTimes
  numPulses = seconds.size();
  if (numPulses == 0)
    throw std::runtime_error("event_time_zero field has no data!");
  pulseTimes = new DateAndTime[numPulses];
  for (size_t i=0; i<numPulses; i++)
    pulseTimes[i] = start + seconds[i];
}

/** Constructor. Build from a vector of date and times.
 * Handles a zero-sized vector */
BankPulseTimes::BankPulseTimes(std::vector<Kernel::DateAndTime> & times)
{
  numPulses = times.size();
  pulseTimes = NULL;
  if (numPulses == 0)
    return;
  pulseTimes = new DateAndTime[numPulses];
  for (size_t i=0; i<numPulses; i++)
    pulseTimes[i] = times[i];
}


/** Destructor */
BankPulseTimes::~BankPulseTimes()
{
  delete [] this->pulseTimes;
}

/** Comparison. Is this bank's pulse times array the same as another one.
 *
 * @param otherNumPulse :: number of pulses in the OTHER bank event_time_zero.
 * @param otherStartTime :: "offset" attribute of the OTHER bank event_time_zero.
 * @return true if the pulse times are the same and so don't need to be reloaded.
 */
bool BankPulseTimes::equals(size_t otherNumPulse, std::string otherStartTime)
{
  return ((this->startTime == otherStartTime) && (this->numPulses == otherNumPulse));
}

//===============================================================================================
//===============================================================================================
/** This task does the disk IO from loading the NXS file,
 * and so will be on a disk IO mutex */
class ProcessBankData : public Task
{
public:
  /**
   *
   * @param alg :: LoadEventNexus
   * @param entry_name :: name of the bank
   * @param prog :: Progress reporter
   * @param scheduler :: ThreadScheduler running this task
   * @param event_id :: array with event IDs
   * @param event_time_of_flight :: array with event TOFS
   * @param numEvents :: how many events in the arrays
   * @param startAt :: index of the first event from event_index
   * @param event_index_ptr :: ptr to a vector of event index (length of # of pulses)
   * @param thisBankPulseTimes :: ptr to the pulse times for this particular bank.
   * @param have_weight :: flag for handling simulated files
   * @param event_weight :: array with weights for events
   * @return
   */
  ProcessBankData(LoadEventNexus * alg, std::string entry_name,
                  Progress * prog, ThreadScheduler * scheduler,
                  boost::shared_array<uint32_t> event_id,
                  boost::shared_array<float> event_time_of_flight,
                  size_t numEvents, size_t startAt,
                  boost::shared_ptr<std::vector<uint64_t> > event_index,
                  BankPulseTimes * thisBankPulseTimes,
                  bool have_weight, boost::shared_array<float> event_weight,
                  detid_t min_event_id, detid_t max_event_id)
  : Task(),
    alg(alg), entry_name(entry_name), pixelID_to_wi_vector(alg->pixelID_to_wi_vector), pixelID_to_wi_offset(alg->pixelID_to_wi_offset),
    prog(prog), scheduler(scheduler),
    event_id(event_id), event_time_of_flight(event_time_of_flight), numEvents(numEvents), startAt(startAt),
    event_index(event_index),
    thisBankPulseTimes(thisBankPulseTimes), have_weight(have_weight),
    event_weight(event_weight), m_min_id(min_event_id), m_max_id(max_event_id)
  {
    // Cost is approximately proportional to the number of events to process.
    m_cost = static_cast<double>(numEvents);
  }

  //----------------------------------------------------
  // Run the data processing
  void run()
  {
    //Local tof limits
    double my_shortest_tof = static_cast<double>(std::numeric_limits<uint32_t>::max()) * 0.1;
    double my_longest_tof = 0.;
    // A count of "bad" TOFs that were too high
    size_t badTofs = 0;
    size_t my_discarded_events(0);

    prog->report(entry_name + ": precount");

    // ---- Pre-counting events per pixel ID ----
    if (alg->precount)
    {
      std::vector<size_t> counts(m_max_id-m_min_id+1, 0);
      for (size_t i=0; i < numEvents; i++)
      {
        detid_t thisId = detid_t(event_id[i]);
        if (thisId >= m_min_id && thisId <= m_max_id)
          counts[thisId-m_min_id]++;
      }

      // Now we pre-allocate (reserve) the vectors of events in each pixel counted
      for (detid_t pixID = m_min_id; pixID <= m_max_id; pixID++)
      {
        if (counts[pixID-m_min_id] > 0)
        {
          //Find the the workspace index corresponding to that pixel ID
          size_t wi = pixelID_to_wi_vector[pixID+pixelID_to_wi_offset];
          // Allocate it
          alg->WS->getEventList(wi).reserve( counts[pixID-m_min_id] );
          if (alg->getCancel()) break; // User cancellation
        }
      }
    }

    // Check for cancelled algorithm
    if (alg->getCancel())
    {
      return;
    }

    //Default pulse time (if none are found)
    Mantid::Kernel::DateAndTime pulsetime;
    Mantid::Kernel::DateAndTime lastpulsetime(0);

    bool pulsetimesincreasing = true;

    // Index into the pulse array
    int pulse_i = 0;

    // And there are this many pulses
    int numPulses = static_cast<int>(thisBankPulseTimes->numPulses);
    if (numPulses > static_cast<int>(event_index->size()))
    {
      alg->getLogger().warning() << "Entry " << entry_name << "'s event_index vector is smaller than the event_time_zero field. This is inconsistent, so we cannot find pulse times for this entry.\n";
      //This'll make the code skip looking for any pulse times.
      pulse_i = numPulses + 1;
    }

    prog->report(entry_name + ": filling events");

    // The workspace
    EventWorkspace_sptr WS = alg->WS;

    // Will we need to compress?
    bool compress = (alg->compressTolerance >= 0);

    // Which detector IDs were touched? - only matters if compress is on
    std::vector<bool> usedDetIds;
    if (compress) usedDetIds.assign(m_max_id-m_min_id+1, false);

    //Go through all events in the list
    for (std::size_t i = 0; i < numEvents; i++)
    {
      //------ Find the pulse time for this event index ---------
      if (pulse_i < numPulses-1)
      {
        bool breakOut = false;
        //Go through event_index until you find where the index increases to encompass the current index. Your pulse = the one before.
        while ( (i+startAt < event_index->operator[](pulse_i))
                  || (i+startAt >= event_index->operator[](pulse_i+1)) )
        {
          pulse_i++;
          // Check once every new pulse if you need to cancel (checking on every event might slow things down more)
          if (alg->getCancel()) breakOut = true;
          if (pulse_i >= (numPulses-1))
            break;
        }

        //Save the pulse time at this index for creating those events
        pulsetime = thisBankPulseTimes->pulseTimes[pulse_i];

        // Determine if pulse times continue to increase
        if (pulsetime < lastpulsetime)
          pulsetimesincreasing = false;
        else
          lastpulsetime = pulsetime;

        // Flag to break out of the event loop without using goto
        if (breakOut)
          break;
      }

      // We cached a pointer to the vector<tofEvent> -> so retrieve it and add the event
      detid_t detId = event_id[i];
      if (detId >= m_min_id && detId <= m_max_id)
      {
        //Create the tofevent
        double tof = static_cast<double>( event_time_of_flight[i] );
        if ((tof >= alg->filter_tof_min) && (tof <= alg->filter_tof_max))
        {
          // Handle simulated data if present
          if (have_weight)
          {
            double weight = static_cast<double>(event_weight[i]);
            double errorSq = weight * weight;
            std::vector<Mantid::DataObjects::WeightedEvent> *eventVector = alg->weightedEventVectors[detId];
            // NULL eventVector indicates a bad spectrum lookup
            if(eventVector)
            {
#if !(defined(__INTEL_COMPILER)) && !(defined(__clang__))
              // This avoids a copy constructor call but is only available with GCC (requires variadic templates)
              eventVector->emplace_back( tof, pulsetime, weight, errorSq );
#else
              eventVector->push_back( WeightedEvent(tof, pulsetime, weight, errorSq) );
#endif
            }
            else
            {
              ++my_discarded_events;
            }
          }
          else
          {
            // We have cached the vector of events for this detector ID
            std::vector<Mantid::DataObjects::TofEvent> *eventVector = alg->eventVectors[detId];
            // NULL eventVector indicates a bad spectrum lookup
            if(eventVector)
            {
#if !(defined(__INTEL_COMPILER)) && !(defined(__clang__))
              // This avoids a copy constructor call but is only available with GCC (requires variadic templates)
              eventVector->emplace_back( tof, pulsetime );
#else
              eventVector->push_back( TofEvent(tof, pulsetime) );
#endif
            }
            else
            {
              ++my_discarded_events;
            }
          }

          //Local tof limits
          if (tof < my_shortest_tof) { my_shortest_tof = tof;}
          // Skip any events that are the cause of bad DAS data (e.g. a negative number in uint32 -> 2.4 billion * 100 nanosec = 2.4e8 microsec)
          if (tof < 2e8)
          {
            if (tof > my_longest_tof) { my_longest_tof = tof;}
          }
          else
            badTofs++;

          // Track all the touched wi (only necessary when compressing events, for thread safety)
          if (compress) usedDetIds[detId-m_min_id] = true;
        } // valid time-of-flight

      } // valid detector IDs
    } //(for each event)

    //------------ Compress Events (or set sort order) ------------------
    // Do it on all the detector IDs we touched
    if (compress)
    {
      for (detid_t pixID = m_min_id; pixID <= m_max_id; pixID++)
      {
        if (usedDetIds[pixID-m_min_id])
        {
          //Find the the workspace index corresponding to that pixel ID
          size_t wi = pixelID_to_wi_vector[pixID+pixelID_to_wi_offset];
          EventList * el = WS->getEventListPtr(wi);
          if (compress)
            el->compressEvents(alg->compressTolerance, el);
          else
          {
            if (pulsetimesincreasing)
              el->setSortOrder(DataObjects::PULSETIME_SORT);
            else
              el->setSortOrder(DataObjects::UNSORTED);
          }
        }
      }
    }
    prog->report(entry_name + ": filled events");

    alg->getLogger().debug() << entry_name << (pulsetimesincreasing ? " had " : " DID NOT have ") <<
        "monotonically increasing pulse times" << std::endl;

    //Join back up the tof limits to the global ones
    PARALLEL_CRITICAL(tof_limits)
    {
      //This is not thread safe, so only one thread at a time runs this.
      if (my_shortest_tof < alg->shortest_tof) { alg->shortest_tof = my_shortest_tof;}
      if (my_longest_tof > alg->longest_tof ) { alg->longest_tof  = my_longest_tof;}
      alg->bad_tofs += badTofs;
      alg->discarded_events += my_discarded_events;
    }


    // For Linux with tcmalloc, make sure memory goes back;
    // but don't call if more than 15% of memory is still available, since that slows down the loading.
    MemoryManager::Instance().releaseFreeMemoryIfAbove(0.85);

#ifndef _WIN32
    alg->getLogger().debug() << "Time to process " << entry_name << " " << m_timer << "\n";
#endif
  }


private:
  /// Algorithm being run
  LoadEventNexus * alg;
  /// NXS path to bank
  std::string entry_name;
  /// Vector where (index = pixel ID+pixelID_to_wi_offset), value = workspace index)
  const std::vector<size_t> & pixelID_to_wi_vector;
  /// Offset in the pixelID_to_wi_vector to use.
  detid_t pixelID_to_wi_offset;
  /// Progress reporting
  Progress * prog;
  /// ThreadScheduler running this task
  ThreadScheduler * scheduler;
  /// event pixel ID array
  boost::shared_array<uint32_t> event_id;
  /// event TOF array
  boost::shared_array<float> event_time_of_flight;
  /// # of events in arrays
  size_t numEvents;
  /// index of the first event from event_index
  size_t startAt;
  /// vector of event index (length of # of pulses)
  boost::shared_ptr<std::vector<uint64_t> > event_index;
  /// Pulse times for this bank
  BankPulseTimes * thisBankPulseTimes;
  /// Flag for simulated data
  bool have_weight;
  /// event weights array
  boost::shared_array<float> event_weight;
  /// Minimum pixel id
  detid_t m_min_id;
  /// Maximum pixel id
  detid_t m_max_id;
  /// timer for performance
  Mantid::Kernel::Timer m_timer;
};




//===============================================================================================
//===============================================================================================
/** This task does the disk IO from loading the NXS file,
 * and so will be on a disk IO mutex */
class LoadBankFromDiskTask : public Task
{

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
   */
  LoadBankFromDiskTask(LoadEventNexus * alg, const std::string& entry_name, const std::string & entry_type,
                       const std::size_t numEvents, const bool oldNeXusFileNames,
                       Progress * prog, Mutex * ioMutex, ThreadScheduler * scheduler)
  : Task(),
    alg(alg), entry_name(entry_name), entry_type(entry_type),
    pixelID_to_wi_vector(alg->pixelID_to_wi_vector), pixelID_to_wi_offset(alg->pixelID_to_wi_offset),
    prog(prog), scheduler(scheduler), thisBankPulseTimes(NULL), m_loadError(false),
    m_oldNexusFileNames(oldNeXusFileNames), m_loadStart(), m_loadSize(), m_event_id(NULL),
    m_event_time_of_flight(NULL), m_have_weight(false), m_event_weight(NULL)
  {
    setMutex(ioMutex);
    m_cost = static_cast<double>(numEvents);
    m_min_id = std::numeric_limits<uint32_t>::max();
    m_max_id = 0;
  }

  //---------------------------------------------------------------------------------------------------
  /** Load the pulse times, if needed. This sets
   * thisBankPulseTimes to the right pointer.
   * */
  void loadPulseTimes(::NeXus::File & file)
  {
    try
    {
      // First, get info about the event_time_zero field in this bank
      file.openData("event_time_zero");
    }
    catch (::NeXus::Exception&)
    {
      // Field not found error is most likely.
      // Use the "proton_charge" das logs.
      thisBankPulseTimes = alg->m_allBanksPulseTimes;
      return;
    }
    std::string thisStartTime = "";
    size_t thisNumPulses = 0;
    file.getAttr("offset", thisStartTime);
    if (file.getInfo().dims.size() > 0)
      thisNumPulses = file.getInfo().dims[0];
    file.closeData();

    // Now, we look through existing ones to see if it is already loaded
    thisBankPulseTimes = NULL;
    for (size_t i=0; i<alg->m_bankPulseTimes.size(); i++)
    {
      if (alg->m_bankPulseTimes[i]->equals(thisNumPulses, thisStartTime))
      {
        thisBankPulseTimes = alg->m_bankPulseTimes[i];
        return;
      }
    }

    // Not found? Need to load and add it
    thisBankPulseTimes = new BankPulseTimes(file);
    alg->m_bankPulseTimes.push_back(thisBankPulseTimes);
  }


  //---------------------------------------------------------------------------------------------------
  /** Load the event_index field
   (a list of size of # of pulses giving the index in the event list for that pulse)

   * @param file :: File handle for the NeXus file
   * @param event_index :: ref to the vector
   */
  void loadEventIndex(::NeXus::File & file, std::vector<uint64_t> & event_index)
  {
    // Get the event_index (a list of size of # of pulses giving the index in the event list for that pulse)
    file.openData("event_index");
    //Must be uint64
    if (file.getInfo().type == ::NeXus::UINT64)
      file.getData(event_index);
    else
    {
     alg->getLogger().warning() << "Entry " << entry_name << "'s event_index field is not UINT64! It will be skipped.\n";
     m_loadError = true;
    }
    file.closeData();

    // Look for the sign that the bank is empty
    if (event_index.size()==1)
    {
      if (event_index[0] == 0)
      {
        //One entry, only zero. This means NO events in this bank.
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
   * @param event_index ::  (a list of size of # of pulses giving the index in the event list for that pulse)
   */
  void prepareEventId(::NeXus::File & file, size_t & start_event, size_t & stop_event, std::vector<uint64_t> & event_index)
  {
    // Get the list of pixel ID's
    if (m_oldNexusFileNames)
      file.openData("event_pixel_id");
    else
      file.openData("event_id");

    // By default, use all available indices
    start_event = 0;
    ::NeXus::Info id_info = file.getInfo();
    // dims[0] can be negative in ISIS meaning 2^32 + dims[0]. Take that into account
    int64_t dim0 = recalculateDataSize(id_info.dims[0]);
    stop_event = static_cast<size_t>(dim0);

    //Handle the time filtering by changing the start/end offsets.
    for (size_t i=0; i < thisBankPulseTimes->numPulses; i++)
    {
      if (thisBankPulseTimes->pulseTimes[i] >= alg->filter_time_start)
      {
        start_event = event_index[i];
        break; // stop looking
      }
    }

    if (start_event > static_cast<size_t>(dim0))
    {
      // If the frame indexes are bad then we can't construct the times of the events properly and filtering by time
      // will not work on this data
      alg->getLogger().warning() 
        << this->entry_name << "'s field 'event_index' seems to be invalid (start_index > than the number of events in the bank)."
        << "All events will appear in the same frame and filtering by time will not be possible on this data.\n";
      start_event = 0;
      stop_event =  static_cast<size_t>(dim0);
    }
    else
    {
      for (size_t i=0; i < thisBankPulseTimes->numPulses; i++)
      {
        if (thisBankPulseTimes->pulseTimes[i] > alg->filter_time_stop)
        {
          stop_event = event_index[i];
          break;
        }
      }
    }
    // We are loading part - work out the event number range
    if (alg->chunk != EMPTY_INT())
    {
      start_event = (alg->chunk - alg->firstChunkForBank) * alg->eventsPerChunk;
      // Don't change stop_event for the final chunk
      if ( start_event + alg->eventsPerChunk < stop_event ) stop_event = start_event + alg->eventsPerChunk;
    }

    // Make sure it is within range
    if (stop_event > static_cast<size_t>(dim0))
      stop_event = dim0;

    alg->getLogger().debug() << entry_name << ": start_event " << start_event << " stop_event "<< stop_event << std::endl;
  }


  //---------------------------------------------------------------------------------------------------
  /** Load the event_id field, which has been open */
  void loadEventId(::NeXus::File & file)
  {
    // This is the data size
    ::NeXus::Info id_info = file.getInfo();
    int64_t dim0 = recalculateDataSize(id_info.dims[0]);

    // Now we allocate the required arrays
    m_event_id = new uint32_t[m_loadSize[0]];

    // Check that the required space is there in the file.
    if (dim0 < m_loadSize[0]+m_loadStart[0])
    {
      alg->getLogger().warning() << "Entry " << entry_name << "'s event_id field is too small (" << dim0
                      << ") to load the desired data size (" << m_loadSize[0]+m_loadStart[0] << ").\n";
      m_loadError = true;
    }

    if (alg->getCancel()) m_loadError = true; //To allow cancelling the algorithm

    if (!m_loadError)
    {
      //Must be uint32
      if (id_info.type == ::NeXus::UINT32)
        file.getSlab(m_event_id, m_loadStart, m_loadSize);
      else
      {
        alg->getLogger().warning() << "Entry " << entry_name << "'s event_id field is not UINT32! It will be skipped.\n";
        m_loadError = true;
      }
      file.closeData();

      // determine the range of pixel ids
      uint32_t temp;
      for (auto i = 0; i < m_loadSize[0]; ++i)
      {
        temp = m_event_id[i];
        if (temp < m_min_id) m_min_id = temp;
        if (temp > m_max_id) m_max_id = temp;
      }
    }
  }

  //---------------------------------------------------------------------------------------------------
  /** Open and load the times-of-flight data */
  void loadTof(::NeXus::File & file)
  {
    // Allocate the array
    float* temp = new float[m_loadSize[0]];
    delete [] m_event_time_of_flight;
    m_event_time_of_flight = temp;

    // Get the list of event_time_of_flight's
    if (!m_oldNexusFileNames)
      file.openData("event_time_offset");
    else
      file.openData("event_time_of_flight");

    // Check that the required space is there in the file.
    ::NeXus::Info tof_info = file.getInfo();
    int64_t tof_dim0 = recalculateDataSize(tof_info.dims[0]);
    if (tof_dim0 < m_loadSize[0]+m_loadStart[0])
    {
      alg->getLogger().warning() << "Entry " << entry_name << "'s event_time_offset field is too small to load the desired data.\n";
      m_loadError = true;
    }

    //Check that the type is what it is supposed to be
    if (tof_info.type == ::NeXus::FLOAT32)
      file.getSlab(m_event_time_of_flight, m_loadStart, m_loadSize);
    else
    {
      alg->getLogger().warning() << "Entry " << entry_name << "'s event_time_offset field is not FLOAT32! It will be skipped.\n";
      m_loadError = true;
    }

    if (!m_loadError)
    {
      std::string units;
      file.getAttr("units", units);
      if (units != "microsecond")
      {
        alg->getLogger().warning() << "Entry " << entry_name << "'s event_time_offset field's units are not microsecond. It will be skipped.\n";
        m_loadError = true;
      }
      file.closeData();
    } //no error
  }

  void loadEventWeights(::NeXus::File &file)
  {
    try
    {
      // First, get info about the event_weight field in this bank
      file.openData("event_weight");
    }
    catch (::NeXus::Exception&)
    {
      // Field not found error is most likely.
      m_have_weight = false;
      return;
    }
    // OK, we've got them
    m_have_weight = true;

    // Allocate the array
    float* temp = new float[m_loadSize[0]];
    delete [] m_event_weight;
    m_event_weight = temp;

    ::NeXus::Info weight_info = file.getInfo();
    int64_t weight_dim0 = recalculateDataSize(weight_info.dims[0]);
    if (weight_dim0 < m_loadSize[0]+m_loadStart[0])
    {
      alg->getLogger().warning() << "Entry " << entry_name << "'s event_weight field is too small to load the desired data.\n";
      m_loadError = true;
    }

    // Check that the type is what it is supposed to be
    if (weight_info.type == ::NeXus::FLOAT32)
      file.getSlab(m_event_weight, m_loadStart, m_loadSize);
    else
    {
      alg->getLogger().warning() << "Entry " << entry_name << "'s event_weight field is not FLOAT32! It will be skipped.\n";
      m_loadError = true;
    }

    if (!m_loadError)
    {
      file.closeData();
    }
  }

  //---------------------------------------------------------------------------------------------------
  void run()
  {
    //The vectors we will be filling
    std::vector<uint64_t> * event_index_ptr = new std::vector<uint64_t>();
    std::vector<uint64_t> & event_index = *event_index_ptr;

    // These give the limits in each file as to which events we actually load (when filtering by time).
    m_loadStart.resize(1, 0);
    m_loadSize.resize(1, 0);

    // Data arrays
    m_event_id = NULL;
    m_event_time_of_flight = NULL;
    m_event_weight = NULL;

    m_loadError = false;
    m_have_weight = alg->m_haveWeights;

    prog->report(entry_name + ": load from disk");

    // Open the file
    ::NeXus::File file(alg->m_filename);
    try
    {
      // Navigate into the file
      file.openGroup(alg->m_top_entry_name, "NXentry");
      //Open the bankN_event group
      file.openGroup(entry_name, entry_type);

      // Load the event_index field.
      this->loadEventIndex(file, event_index);

      if (!m_loadError)
      {
        // Load and validate the pulse times
        this->loadPulseTimes(file);

        // The event_index should be the same length as the pulse times from DAS logs.
        if (event_index.size() != thisBankPulseTimes->numPulses)
          alg->getLogger().warning() << "Bank " << entry_name << " has a mismatch between the number of event_index entries and the number of pulse times in event_time_zero.\n";

        // Open and validate event_id field.
        size_t start_event = 0;
        size_t stop_event = 0;
        this->prepareEventId(file, start_event, stop_event, event_index);

        // These are the arguments to getSlab()
        m_loadStart[0] = static_cast<int>(start_event);
        m_loadSize[0] = static_cast<int>(stop_event - start_event);

        if ((m_loadSize[0] > 0) && (m_loadStart[0]>=0) )
        {
          // Load pixel IDs
          this->loadEventId(file);
          if (alg->getCancel()) m_loadError = true; //To allow cancelling the algorithm

          // And TOF.
          if (!m_loadError)
          {
            this->loadTof(file);
            if (m_have_weight)
            {
              this->loadEventWeights(file);
            }
          }
        } // Size is at least 1
        else
        {
          // Found a size that was 0 or less; stop processing
          m_loadError=true;
        }

      } //no error

    } // try block
    catch (std::exception & e)
    {
      alg->getLogger().error() << "Error while loading bank " << entry_name << ":" << std::endl;
      alg->getLogger().error() << e.what() << std::endl;
      m_loadError = true;
    }
    catch (...)
    {
      alg->getLogger().error() << "Unspecified error while loading bank " << entry_name << std::endl;
      m_loadError = true;
    }

    //Close up the file even if errors occured.
    file.closeGroup();
    file.close();

    //Abort if anything failed
    if (m_loadError)
    {
      prog->reportIncrement(4, entry_name + ": skipping");
      delete [] m_event_id;
      delete [] m_event_time_of_flight;
      if (m_have_weight)
      {
        delete [] m_event_weight;
      }
      delete event_index_ptr;
      return;
    }

    // No error? Launch a new task to process that data.
    size_t numEvents = m_loadSize[0];
    size_t startAt = m_loadStart[0];

    // convert things to shared_arrays
    boost::shared_array<uint32_t> event_id_shrd(m_event_id);
    boost::shared_array<float> event_time_of_flight_shrd(m_event_time_of_flight);
    boost::shared_array<float> event_weight_shrd(m_event_weight);
    boost::shared_ptr<std::vector<uint64_t> > event_index_shrd(event_index_ptr);

    // fixup the maximum pixel id
    if (m_max_id > static_cast<uint32_t>(alg->eventid_max)) m_max_id = static_cast<uint32_t>(alg->eventid_max);

    // schedule the job to generate the event lists
    auto mid_id = m_max_id;
    if (alg->splitProcessing)
      mid_id = (m_max_id + m_min_id) / 2;

    ProcessBankData * newTask1 = new ProcessBankData(alg, entry_name, prog,scheduler,
        event_id_shrd, event_time_of_flight_shrd, numEvents, startAt, event_index_shrd,
        thisBankPulseTimes, m_have_weight, event_weight_shrd,
        m_min_id, mid_id);
    scheduler->push(newTask1);
    if (alg->splitProcessing)
    {
      ProcessBankData * newTask2 = new ProcessBankData(alg, entry_name, prog,scheduler,
                                           event_id_shrd, event_time_of_flight_shrd, numEvents, startAt, event_index_shrd,
                                           thisBankPulseTimes, m_have_weight, event_weight_shrd,
                                           (mid_id+1), m_max_id);
      scheduler->push(newTask2);
    }
  }

  //---------------------------------------------------------------------------------------------------
  /**
  * Interpret the value describing the number of events. If the number is positive return it unchanged.
  * If the value is negative (can happen at ISIS) add 2^32 to it.
  * @param size :: The size of events value.
  */
  int64_t recalculateDataSize(const int64_t& size)
  {
    if (size < 0)
    {
      const int64_t shift = int64_t(1) << 32;
      return shift + size;
    }
    return size;
  }

private:
  /// Algorithm being run
  LoadEventNexus * alg;
  /// NXS path to bank
  std::string entry_name;
  /// NXS type
  std::string entry_type;
  /// Vector where (index = pixel ID+pixelID_to_wi_offset), value = workspace index)
  const std::vector<size_t> & pixelID_to_wi_vector;
  /// Offset in the pixelID_to_wi_vector to use.
  detid_t pixelID_to_wi_offset;
  /// Progress reporting
  Progress * prog;
  /// ThreadScheduler running this task
  ThreadScheduler * scheduler;
  /// Object with the pulse times for this bank
  BankPulseTimes * thisBankPulseTimes;
  /// Did we get an error in loading
  bool m_loadError;
  /// Old names in the file?
  bool m_oldNexusFileNames;
  /// Index to load start at in the file
  std::vector<int> m_loadStart;
  /// How much to load in the file
  std::vector<int> m_loadSize;
  /// Event pixel ID data
  uint32_t * m_event_id;
  /// Minimum pixel ID in this data
  uint32_t m_min_id;
  /// Maximum pixel ID in this data
  uint32_t m_max_id;
  /// TOF data
  float * m_event_time_of_flight;
  /// Flag for simulated data
  bool m_have_weight;
  /// Event weights
  float * m_event_weight;
};







//===============================================================================================
//===============================================================================================

/// Empty default constructor
LoadEventNexus::LoadEventNexus() : IFileLoader<Kernel::NexusDescriptor>(),
    discarded_events(0), event_id_is_spec(false), m_allBanksPulseTimes(NULL)
{
}

/** Destructor */
LoadEventNexus::~LoadEventNexus()
{
  for (size_t i=0; i<m_bankPulseTimes.size(); i++)
    delete m_bankPulseTimes[i];
  delete m_allBanksPulseTimes;
}

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not be used
 */
int LoadEventNexus::confidence(Kernel::NexusDescriptor & descriptor) const
{
  int confidence(0);
  if(descriptor.classTypeExists("NXevent_data"))
  {
    if(descriptor.pathOfTypeExists("/entry", "NXentry") || descriptor.pathOfTypeExists("/raw_data_1", "NXentry"))
    {
      confidence = 80;
    }
  }
  return confidence;
}

/// Sets documentation strings for this algorithm
void LoadEventNexus::initDocs()
{
  this->setWikiSummary("Loads Event NeXus files (produced by the SNS) and stores it in an [[EventWorkspace]]. Optionally, you can filter out events falling outside a range of times-of-flight and/or a time interval. ");
  this->setOptionalMessage("Loads Event NeXus files (produced by the SNS) and stores it in an EventWorkspace. Optionally, you can filter out events falling outside a range of times-of-flight and/or a time interval.");
}

/// Initialisation method.
void LoadEventNexus::init()
{
  std::vector<std::string> exts;
  exts.push_back("_event.nxs");
  exts.push_back(".nxs.h5");
  exts.push_back(".nxs");
  this->declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
      "The name of the Event NeXus file to read, including its full or relative path. "
      "The file name is typically of the form INST_####_event.nxs (N.B. case sensitive if running on Linux)." );

  this->declareProperty(
    new WorkspaceProperty<IEventWorkspace>("OutputWorkspace", "", Direction::Output),
    "The name of the output EventWorkspace in which to load the EventNexus file." );

  declareProperty(
      new PropertyWithValue<double>("FilterByTofMin", EMPTY_DBL(), Direction::Input),
    "Optional: To exclude events that do not fall within a range of times-of-flight. "\
    "This is the minimum accepted value in microseconds. Keep blank to load all events." );

  declareProperty(
      new PropertyWithValue<double>("FilterByTofMax", EMPTY_DBL(), Direction::Input),
    "Optional: To exclude events that do not fall within a range of times-of-flight. "\
    "This is the maximum accepted value in microseconds. Keep blank to load all events." );

  declareProperty(
      new PropertyWithValue<double>("FilterByTimeStart", EMPTY_DBL(), Direction::Input),
    "Optional: To only include events after the provided start time, in seconds (relative to the start of the run).");

  declareProperty(
      new PropertyWithValue<double>("FilterByTimeStop", EMPTY_DBL(), Direction::Input),
    "Optional: To only include events before the provided stop time, in seconds (relative to the start of the run).");

  std::string grp1 = "Filter Events";
  setPropertyGroup("FilterByTofMin", grp1);
  setPropertyGroup("FilterByTofMax", grp1);
  setPropertyGroup("FilterByTimeStart", grp1);
  setPropertyGroup("FilterByTimeStop", grp1);

  declareProperty(
      new PropertyWithValue<string>("NXentryName", "", Direction::Input),
    "Optional: Name of the NXentry to load if it's not the default.");

  declareProperty(
      new PropertyWithValue<string>("BankName", "", Direction::Input),
    "Optional: To only include events from one bank. Any bank whose name does not match the given string will have no events.");

  declareProperty(
      new PropertyWithValue<bool>("SingleBankPixelsOnly", true, Direction::Input),
    "Optional: Only applies if you specified a single bank to load with BankName. "
    "Only pixels in the specified bank will be created if true; all of the instrument's pixels will be created otherwise.");
  setPropertySettings("SingleBankPixelsOnly", new VisibleWhenProperty("BankName", IS_NOT_DEFAULT) );

  std::string grp2 = "Loading a Single Bank";
  setPropertyGroup("BankName", grp2);
  setPropertyGroup("SingleBankPixelsOnly", grp2);

  declareProperty(
      new PropertyWithValue<bool>("Precount", false, Direction::Input),
      "Pre-count the number of events in each pixel before allocating memory (optional, default False). "
      "This can significantly reduce memory use and memory fragmentation; it may also speed up loading.");

  declareProperty(
      new PropertyWithValue<double>("CompressTolerance", -1.0, Direction::Input),
      "Run CompressEvents while loading (optional, leave blank or negative to not do). "
      "This specified the tolerance to use (in microseconds) when compressing.");
  
  auto mustBePositive = boost::make_shared<BoundedValidator<int> >();
  mustBePositive->setLower(1);
  declareProperty("ChunkNumber", EMPTY_INT(), mustBePositive,
      "If loading the file by sections ('chunks'), this is the section number of this execution of the algorithm.");
  declareProperty("TotalChunks", EMPTY_INT(), mustBePositive,
      "If loading the file by sections ('chunks'), this is the total number of sections.");
  // TotalChunks is only meaningful if ChunkNumber is set
  // Would be nice to be able to restrict ChunkNumber to be <= TotalChunks at validation
  setPropertySettings("TotalChunks", new VisibleWhenProperty("ChunkNumber", IS_NOT_DEFAULT));

  std::string grp3 = "Reduce Memory Use";
  setPropertyGroup("Precount", grp3);
  setPropertyGroup("CompressTolerance", grp3);
  setPropertyGroup("ChunkNumber", grp3);
  setPropertyGroup("TotalChunks", grp3);

  declareProperty(
      new PropertyWithValue<bool>("LoadMonitors", false, Direction::Input),
      "Load the monitors from the file (optional, default False).");

  declareProperty(new PropertyWithValue<bool>("MonitorsAsEvents", false, Direction::Input),
      "If present, load the monitors as events. '''WARNING:''' WILL SIGNIFICANTLY INCREASE MEMORY USAGE (optional, default False). ");

  declareProperty(
      new PropertyWithValue<double>("FilterMonByTofMin", EMPTY_DBL(), Direction::Input),
    "Optional: To exclude events from monitors that do not fall within a range of times-of-flight. "\
    "This is the minimum accepted value in microseconds." );

  declareProperty(
      new PropertyWithValue<double>("FilterMonByTofMax", EMPTY_DBL(), Direction::Input),
    "Optional: To exclude events from monitors that do not fall within a range of times-of-flight. "\
    "This is the maximum accepted value in microseconds." );

  declareProperty(
      new PropertyWithValue<double>("FilterMonByTimeStart", EMPTY_DBL(), Direction::Input),
    "Optional: To only include events from monitors after the provided start time, in seconds (relative to the start of the run).");

  declareProperty(
      new PropertyWithValue<double>("FilterMonByTimeStop", EMPTY_DBL(), Direction::Input),
    "Optional: To only include events from monitors before the provided stop time, in seconds (relative to the start of the run).");

  setPropertySettings("MonitorsAsEvents", new VisibleWhenProperty("LoadMonitors", IS_EQUAL_TO, "1") );
  IPropertySettings *asEventsIsOn = new VisibleWhenProperty("MonitorsAsEvents", IS_EQUAL_TO, "1");
  setPropertySettings("FilterMonByTofMin", asEventsIsOn);
  setPropertySettings("FilterMonByTofMax", asEventsIsOn->clone());
  setPropertySettings("FilterMonByTimeStart", asEventsIsOn->clone());
  setPropertySettings("FilterMonByTimeStop", asEventsIsOn->clone());

  std::string grp4 = "Monitors";
  setPropertyGroup("LoadMonitors", grp4);
  setPropertyGroup("MonitorsAsEvents", grp4);
  setPropertyGroup("FilterMonByTofMin", grp4);
  setPropertyGroup("FilterMonByTofMax", grp4);
  setPropertyGroup("FilterMonByTimeStart", grp4);
  setPropertyGroup("FilterMonByTimeStop", grp4);

  declareProperty(
      new PropertyWithValue<bool>("MetaDataOnly", false, Direction::Input),
      "If true, only the meta data and sample logs will be loaded.");

  declareProperty(
      new PropertyWithValue<bool>("LoadLogs", true, Direction::Input),
      "Load the Sample/DAS logs from the file (default True).");
}


/// set the name of the top level NXentry m_top_entry_name
void LoadEventNexus::setTopEntryName()
{
  std::string nxentryProperty = getProperty("NXentryName");
  if (nxentryProperty.size()>0)
  {
    m_top_entry_name = nxentryProperty;
    return;
  }
  typedef std::map<std::string,std::string> string_map_t; 
  try
  {
    string_map_t::const_iterator it;
    ::NeXus::File file = ::NeXus::File(m_filename);
    string_map_t entries = file.getEntries();

    // Choose the first entry as the default
    m_top_entry_name = entries.begin()->first;

    for (it = entries.begin(); it != entries.end(); ++it)
    {
      if ( ((it->first == "entry") || (it->first == "raw_data_1")) && (it->second == "NXentry") )
      {
        m_top_entry_name = it->first;
        break;
      }
    }
  }
  catch(const std::exception&)
  {
    g_log.error() << "Unable to determine name of top level NXentry - assuming \"entry\"." << std::endl;
    m_top_entry_name = "entry";
  }
}

//------------------------------------------------------------------------------------------------
/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 */
void LoadEventNexus::exec()
{
  // Retrieve the filename from the properties
  m_filename = getPropertyValue("Filename");

  precount = getProperty("Precount");
  compressTolerance = getProperty("CompressTolerance");

  loadlogs = getProperty("LoadLogs");

  // Check to see if the monitors need to be loaded later
  bool load_monitors = this->getProperty("LoadMonitors");
  setTopEntryName();

  //Initialize progress reporting.
  int reports = 3;
  if (load_monitors)
    reports++;
  Progress prog(this,0.0,0.3,  reports);
  
  // Load the detector events
  WS = createEmptyEventWorkspace(); // Algorithm currently relies on an object-level workspace ptr
  loadEvents(&prog, false); // Do not load monitor blocks

  if ( discarded_events > 0 )
  {
    g_log.information() << discarded_events
                        << " events were encountered coming from pixels which are not in the Instrument Definition File."
                           "These events were discarded.\n";
  }

  //add filename
  WS->mutableRun().addProperty("Filename",m_filename);
  //Save output
  this->setProperty<IEventWorkspace_sptr>("OutputWorkspace", WS);
  // Load the monitors
  if (load_monitors)
  {
    prog.report("Loading monitors");
    const bool eventMonitors = getProperty("MonitorsAsEvents");
    if( eventMonitors && this->hasEventMonitors() )
    {
      WS = createEmptyEventWorkspace(); // Algorithm currently relies on an object-level workspace ptr
      //add filename
      WS->mutableRun().addProperty("Filename",m_filename);
      // Perform the load
      loadEvents(&prog, true);
      std::string mon_wsname = this->getProperty("OutputWorkspace");
      mon_wsname.append("_monitors");
      this->declareProperty(new WorkspaceProperty<IEventWorkspace>
                            ("MonitorWorkspace", mon_wsname, Direction::Output), "Monitors from the Event NeXus file");
      this->setProperty<IEventWorkspace_sptr>("MonitorWorkspace", WS);      
    }
    else
    {
      this->runLoadMonitors();
    }
  }

  // Some memory feels like it sticks around (on Linux). Free it.
  MemoryManager::Instance().releaseFreeMemory();

  return;
}



//-----------------------------------------------------------------------------
/** Generate a look-up table where the index = the pixel ID of an event
 * and the value = a pointer to the EventList in the workspace
 * @param vectors :: the array to create the map on
 */
template <class T>
void LoadEventNexus::makeMapToEventLists(std::vector<T> & vectors)
{
  if( this->event_id_is_spec )
  {
    // Find max spectrum no
    Axis *ax1 = WS->getAxis(1);
    specid_t maxSpecNo = -std::numeric_limits<specid_t>::max(); // So that any number will be greater than this
    for (size_t i=0; i < ax1->length(); i++)
    {
      specid_t spec = ax1->spectraNo(i);
      if (spec > maxSpecNo) maxSpecNo = spec;
    }

    // These are used by the bank loader to figure out where to put the events
    // The index of eventVectors is a spectrum number so it is simply resized to the maximum
    // possible spectrum number
    eventid_max = maxSpecNo;
    vectors.resize(maxSpecNo+1, NULL);
    for(size_t i = 0; i < WS->getNumberHistograms(); ++i)
    {
      const ISpectrum * spec = WS->getSpectrum(i);
      if(spec)
      {
        getEventsFrom(WS->getEventList(i), vectors[spec->getSpectrumNo()]);
      }
    }
  }
  else
  {
    // To avoid going out of range in the vector, this is the MAX index that can go into it
    eventid_max = static_cast<int32_t>(pixelID_to_wi_vector.size()) + pixelID_to_wi_offset;
    
    // Make an array where index = pixel ID
    // Set the value to NULL by default
    vectors.resize(eventid_max+1, NULL);

    for (size_t j=size_t(pixelID_to_wi_offset); j<pixelID_to_wi_vector.size(); j++)
    {
      size_t wi = pixelID_to_wi_vector[j];
      // Save a POINTER to the vector
      if ( wi < WS->getNumberHistograms() )
      {
        getEventsFrom(WS->getEventList(wi), vectors[j-pixelID_to_wi_offset]);
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
std::size_t numEvents(::NeXus::File &file, bool &hasTotalCounts, bool &oldNeXusFileNames)
{
  // try getting the value of total_counts
  if (hasTotalCounts)
  {
    try
    {
      uint64_t numEvents;
      file.readData("total_counts", numEvents);
      return numEvents;
    }
    catch (::NeXus::Exception& )
    {
      hasTotalCounts=false; // carry on with the field not existing
    }
  }

  // just get the length of the event pixel ids
  try
  {
    if (oldNeXusFileNames)
      file.openData("event_pixel_id");
    else
      file.openData("event_id");
  }
  catch (::NeXus::Exception& )
  {
    // Older files (before Nov 5, 2010) used this field.
    try
    {
      file.openData("event_pixel_id");
      oldNeXusFileNames = true;
    }
    catch(::NeXus::Exception&)
    {
      // Some groups have neither indicating there are not events here
      return 0;
    }
  }

  size_t numEvents = static_cast<std::size_t>(file.getInfo().dims[0]);
  file.closeData();
  return numEvents;
}

//-----------------------------------------------------------------------------
/**
 * Load events from the file
 * @param prog :: A pointer to the progress reporting object
 * @param monitors :: If true the events from the monitors are loaded and not the main banks
 */
void LoadEventNexus::loadEvents(API::Progress * const prog, const bool monitors)
{
  bool metaDataOnly = getProperty("MetaDataOnly");

  // Get the time filters
  setTimeFilters(monitors);

  // The run_start will be loaded from the pulse times.
  DateAndTime run_start(0,0);
  // Initialize the counter of bad TOFs
  bad_tofs = 0;

  if (loadlogs)
  {
    prog->doReport("Loading DAS logs");
    m_allBanksPulseTimes = runLoadNexusLogs(m_filename, WS, this);
    run_start = WS->getFirstPulseTime();
  }
  else
  {
    g_log.information() << "Skipping the loading of sample logs!" << endl;
  }

  // Make sure you have a non-NULL m_allBanksPulseTimes
  if (m_allBanksPulseTimes == NULL)
  {
    std::vector<DateAndTime> temp;
    m_allBanksPulseTimes = new BankPulseTimes(temp);
  }


  //Load the instrument
  prog->report("Loading instrument");
  instrument_loaded_correctly = loadInstrument(m_filename, WS, m_top_entry_name, this);

  if (!this->instrument_loaded_correctly)
      throw std::runtime_error("Instrument was not initialized correctly! Loading cannot continue.");


  // top level file information
  ::NeXus::File file(m_filename);

  //Start with the base entry
  file.openGroup(m_top_entry_name, "NXentry");

  //Now we want to go through all the bankN_event entries
  vector<string> bankNames;
  vector<std::size_t> bankNumEvents;
  size_t total_events = 0;
  map<string, string> entries = file.getEntries();
  map<string,string>::const_iterator it = entries.begin();
  std::string classType = monitors ? "NXmonitor" : "NXevent_data";
  ::NeXus::Info info;
  bool oldNeXusFileNames(false);
  bool hasTotalCounts(true);
  m_haveWeights = false;
  for (; it != entries.end(); ++it)
  {
    std::string entry_name(it->first);
    std::string entry_class(it->second);
    if ( entry_class == classType )
    {
      // open the group
      file.openGroup(entry_name, classType);

      // get the number of events
      std::size_t num = numEvents(file, hasTotalCounts, oldNeXusFileNames);
      if (num == 0)
      {
        file.closeGroup();
        continue;
      }
      bankNames.push_back( entry_name );
      bankNumEvents.push_back(num);
      total_events += num;

      // Look for weights in simulated file
      try
      {
        file.openData("event_weight");
        m_haveWeights = true;
        file.closeData();
      }
      catch (::NeXus::Exception &)
      {
        // Swallow exception since flag is already false;
      }

      file.closeGroup();
    }
  }

  //Close up the file
  file.closeGroup();
  file.close();

  // Delete the output workspace name if it existed
  std::string outName = getPropertyValue("OutputWorkspace");
  if (AnalysisDataService::Instance().doesExist(outName))
    AnalysisDataService::Instance().remove( outName );

  // set more properties on the workspace
  try
  {
    loadEntryMetadata(m_filename, WS, m_top_entry_name);
  }
  catch (std::runtime_error & e)
  {
    // Missing metadata is not a fatal error. Log and go on with your life
    g_log.error() << "Error loading metadata: " << e.what() << std::endl;
  }

  // --------------------------- Time filtering ------------------------------------
  double filter_time_start_sec, filter_time_stop_sec;
  filter_time_start_sec = getProperty("FilterByTimeStart");
  filter_time_stop_sec = getProperty("FilterByTimeStop");
  chunk = getProperty("ChunkNumber");
  totalChunks = getProperty("TotalChunks");

  //Default to ALL pulse times
  bool is_time_filtered = false;
  filter_time_start = Kernel::DateAndTime::minimum();
  filter_time_stop = Kernel::DateAndTime::maximum();

  if (m_allBanksPulseTimes->numPulses > 0)
  {
    //If not specified, use the limits of doubles. Otherwise, convert from seconds to absolute PulseTime
    if (filter_time_start_sec != EMPTY_DBL())
    {
      filter_time_start = run_start + filter_time_start_sec;
      is_time_filtered = true;
    }

    if (filter_time_stop_sec != EMPTY_DBL())
    {
      filter_time_stop = run_start + filter_time_stop_sec;
      is_time_filtered = true;
    }

    //Silly values?
    if (filter_time_stop < filter_time_start)
    {
      std::string msg = "Your ";
      if(monitors) msg += "monitor ";
      msg += "filter for time's Stop value is smaller than the Start value.";
      throw std::invalid_argument(msg);
    }
  }

  if (is_time_filtered)
  {
    //Now filter out the run, using the DateAndTime type.
    WS->mutableRun().filterByTime(filter_time_start, filter_time_stop);
  }

  if(metaDataOnly) {
    //Now, create a default X-vector for histogramming, with just 2 bins.
    Kernel::cow_ptr<MantidVec> axis;
    MantidVec& xRef = axis.access();
    xRef.resize(2);
    xRef[0] = static_cast<double>(std::numeric_limits<uint32_t>::max()) * 0.1 - 1; //Just to make sure the bins hold it all
    xRef[1] = 1;
    //Set the binning axis using this.
    WS->setAllX(axis);
    return;
  }

  // --------- Loading only one bank ? ----------------------------------
  std::string onebank = getProperty("BankName");
  bool doOneBank = (onebank != "");
  bool SingleBankPixelsOnly = getProperty("SingleBankPixelsOnly");
  if (doOneBank && !monitors)
  {
    bool foundIt = false;
    for (std::vector<string>::iterator it=bankNames.begin(); it!= bankNames.end(); ++it)
    {
      if (*it == ( onebank + "_events") )
      {
        foundIt = true;
        break;
      }
    }
    if (!foundIt)
    {
      throw std::invalid_argument("No entry named '" + onebank + "_events'" + " was found in the .NXS file.\n");
    }
    bankNames.clear();
    bankNames.push_back( onebank + "_events" );
    bankNumEvents.clear();
    bankNumEvents.push_back(1);
    if( !SingleBankPixelsOnly ) onebank = ""; // Marker to load all pixels 
  }
  else
  {
    onebank = "";
  }

  prog->report("Initializing all pixels");
  // Remove used banks if parameter is set
  if (WS->getInstrument()->hasParameter("remove-unused-banks")) deleteBanks(WS, bankNames);
  //----------------- Pad Empty Pixels -------------------------------
  // Create the required spectra mapping so that the workspace knows what to pad to
  createSpectraMapping(m_filename, monitors, onebank);

  //This map will be used to find the workspace index
  if( this->event_id_is_spec )
    WS->getSpectrumToWorkspaceIndexVector(pixelID_to_wi_vector, pixelID_to_wi_offset);
  else
    WS->getDetectorIDToWorkspaceIndexVector(pixelID_to_wi_vector, pixelID_to_wi_offset, true);

  // Cache a map for speed.
  if (!m_haveWeights)
  {
    this->makeMapToEventLists<EventVector_pt>(eventVectors);
  }
  else
  {
    // Convert to weighted events
    for (size_t i=0; i < WS->getNumberHistograms(); i++)
    {
      WS->getEventList(i).switchTo(API::WEIGHTED);
    }
    this->makeMapToEventLists<WeightedEventVector_pt>(weightedEventVectors);
  }

  // Set all (empty) event lists as sorted by pulse time. That way, calling SortEvents will not try to sort these empty lists.
  for (size_t i=0; i < WS->getNumberHistograms(); i++)
    WS->getEventList(i).setSortOrder(DataObjects::PULSETIME_SORT);

  //Count the limits to time of flight
  shortest_tof = static_cast<double>(std::numeric_limits<uint32_t>::max()) * 0.1;
  longest_tof = 0.;

  // Make the thread pool
  ThreadScheduler * scheduler = new ThreadSchedulerMutexes();
  ThreadPool pool(scheduler);
  Mutex * diskIOMutex = new Mutex();
  size_t bank0 = 0;
  size_t bankn = bankNames.size();

  if (chunk != EMPTY_INT()) // We are loading part - work out the bank number range
  {
    eventsPerChunk = total_events / totalChunks;
    // Sort banks by size
    size_t tmp;
    string stmp;
    for (size_t i = 0; i < bankn; i++)
       for (size_t j = 0; j < bankn - 1; j++)
          if (bankNumEvents[j] < bankNumEvents[j + 1])
          {
             tmp = bankNumEvents[j];
             bankNumEvents[j] = bankNumEvents[j + 1];
             bankNumEvents[j + 1] = tmp;
             stmp = bankNames[j];
             bankNames[j] = bankNames[j + 1];
             bankNames[j + 1] = stmp;
          }
    int bigBanks = 0;
    for (size_t i = 0; i < bankn; i++) if (bankNumEvents[i] > eventsPerChunk)bigBanks++;
    // Each chunk is part of bank or multiple whole banks
    // 0.5 for last chunk of a bank with multiple chunks
    // 0.1 for multiple whole banks not completely filled
    eventsPerChunk += static_cast<size_t>((static_cast<double>(bigBanks) / static_cast<double>(totalChunks) * 
                                          0.5 + 0.05) * static_cast<double>(eventsPerChunk));
    double partialChunk = 0.;
    firstChunkForBank = 1;
    for (int chunki = 1; chunki <=chunk; chunki++)
    {
      if (partialChunk > 1.)
      {
        partialChunk = 0.;
        firstChunkForBank = chunki;
        bank0 = bankn;
      }
      if (bankNumEvents[bank0] > 1)
      {
        partialChunk += static_cast<double>(eventsPerChunk)/static_cast<double>(bankNumEvents[bank0]);
      }
      if (chunki < totalChunks) bankn = bank0 + 1;
      else bankn = bankNames.size();
      if (chunki == firstChunkForBank && partialChunk > 1.0) bankn += static_cast<size_t>(partialChunk) - 1;
      if (bankn > bankNames.size()) bankn = bankNames.size();
    }
    for (size_t i=bank0; i < bankn; i++)
    {
      size_t start_event = (chunk - firstChunkForBank) * eventsPerChunk;
      size_t stop_event = bankNumEvents[i];
      // Don't change stop_event for the final chunk
      if ( start_event + eventsPerChunk < stop_event ) stop_event = start_event + eventsPerChunk;
      bankNumEvents[i] = stop_event - start_event;
    }
  }

  // split banks up if the number of cores is more than twice the number of banks
  splitProcessing = bool(bankNames.size() * 2 < ThreadPool::getNumPhysicalCores());

  // set up progress bar for the rest of the (multi-threaded) process
  size_t numProg = bankNames.size() * (1 + 3); // 1 = disktask, 3 = proc task
  if (splitProcessing) numProg += bankNames.size() * 3; // 3 = second proc task
  Progress * prog2 = new Progress(this,0.3,1.0, numProg);

  for (size_t i=bank0; i < bankn; i++)
  {
    // We make tasks for loading
    if (bankNumEvents[i] > 0)
      pool.schedule( new LoadBankFromDiskTask(this, bankNames[i], classType, bankNumEvents[i], oldNeXusFileNames,
                                              prog2, diskIOMutex, scheduler) );
  }
  // Start and end all threads
  pool.joinAll();
  delete diskIOMutex;
  delete prog2;


  //Info reporting
  const std::size_t eventsLoaded = WS->getNumberEvents();
  g_log.information() << "Read " << eventsLoaded << " events"
      << ". Shortest TOF: " << shortest_tof << " microsec; longest TOF: "
      << longest_tof << " microsec." << std::endl;

  if (shortest_tof < 0)
    g_log.warning() << "The shortest TOF was negative! At least 1 event has an invalid time-of-flight." << std::endl;
  if (bad_tofs > 0)
    g_log.warning() << "Found " << bad_tofs << " events with TOF > 2e8. This may indicate errors in the raw TOF data." << std::endl;

  //Now, create a default X-vector for histogramming, with just 2 bins.
  Kernel::cow_ptr<MantidVec> axis;
  MantidVec& xRef = axis.access();
  xRef.resize(2,0.0);
  if ( eventsLoaded > 0)
  {
    xRef[0] = shortest_tof - 1; //Just to make sure the bins hold it all
    xRef[1] = longest_tof + 1;
  }
  //Set the binning axis using this.
  WS->setAllX(axis);

  // if there is time_of_flight load it
  loadTimeOfFlight(m_filename, WS, m_top_entry_name,classType);
}

//-----------------------------------------------------------------------------
/**
 * Create a blank event workspace
 * @returns A shared pointer to a new empty EventWorkspace object
 */
EventWorkspace_sptr LoadEventNexus::createEmptyEventWorkspace()
{
  // Create the output workspace
  EventWorkspace_sptr eventWS(new EventWorkspace());
  //Make sure to initialize.
  //   We can use dummy numbers for arguments, for event workspace it doesn't matter
  eventWS->initialize(1,1,1);

  // Set the units
  eventWS->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  eventWS->setYUnit("Counts");

  return eventWS;
}


//-----------------------------------------------------------------------------
/** Load the run number and other meta data from the given bank */
void LoadEventNexus::loadEntryMetadata(const std::string &nexusfilename, Mantid::API::MatrixWorkspace_sptr WS,
    const std::string &entry_name)
{
  // Open the file
  ::NeXus::File file(nexusfilename);
  file.openGroup(entry_name, "NXentry");

  // get the title
  file.openData("title");
  if (file.getInfo().type == ::NeXus::CHAR) {
    string title = file.getStrData();
    if (!title.empty())
      WS->setTitle(title);
  }
  file.closeData();

  // get the notes
  try {
    file.openData("notes");
    if (file.getInfo().type == ::NeXus::CHAR) {
      string notes = file.getStrData();
      if (!notes.empty())
        WS->mutableRun().addProperty("file_notes", notes);
    }
    file.closeData();
  } catch (::NeXus::Exception &) {
    // let it drop on floor
  }

  // Get the run number
  file.openData("run_number");
  string run("");
  if (file.getInfo().type == ::NeXus::CHAR) {
    run = file.getStrData();
  }
  if (!run.empty()) {
    WS->mutableRun().addProperty("run_number", run);
  }
  file.closeData();

  // get the duration
  file.openData("duration");
  std::vector<double> duration;
  file.getDataCoerce(duration);
  if (duration.size() == 1)
  {
    // get the units
    std::vector<AttrInfo> infos = file.getAttrInfos();
    std::string units("");
    for (std::vector<AttrInfo>::const_iterator it = infos.begin(); it != infos.end(); ++it)
    {
      if (it->name.compare("units") == 0)
      {
        units = file.getStrAttr(*it);
        break;
      }
    }

    // set the property
    WS->mutableRun().addProperty("duration", duration[0], units);
  }
  file.closeData();

  // close the file
  file.close();
}

//-----------------------------------------------------------------------------
/** Load the instrument from the nexus file or if not found from the IDF file
 *  specified by the info in the Nexus file
 *
 *  @param nexusfilename :: The Nexus file name
 *  @param localWorkspace :: MatrixWorkspace in which to put the instrument geometry
 *  @param top_entry_name :: entry name at the top of the Nexus file
 *  @param alg :: Handle of the algorithm 
 *  @return true if successful
 */
bool LoadEventNexus::loadInstrument(const std::string &nexusfilename, MatrixWorkspace_sptr localWorkspace,
    const std::string & top_entry_name, Algorithm * alg) 
{

   // Get the instrument group in the Nexus file
   ::NeXus::File nxfile(nexusfilename);

   bool foundInstrument = runLoadIDFFromNexus( nexusfilename, localWorkspace, alg);

   if(!foundInstrument) foundInstrument = runLoadInstrument( nexusfilename, localWorkspace, top_entry_name, alg );

   return foundInstrument;
}

//-----------------------------------------------------------------------------
/** Load the instrument from the nexus file
 *
 *  @param nxfile :: C++ interface to Nexus file with instrumentr group opened
 *  @param localWorkspace :: MatrixWorkspace in which to put the instrument geometry
 *  @return true if successful
 */
bool LoadEventNexus::runLoadIDFFromNexus(const std::string &nexusfilename, API::MatrixWorkspace_sptr localWorkspace, Algorithm * alg)
{
  // Code to be added here. In meantime, fail to find instrument

  IAlgorithm_sptr loadInst= alg->createChildAlgorithm("LoadIDFFromNexus",-1,-1,false);

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try
  {
    loadInst->setPropertyValue("Filename", nexusfilename);
    loadInst->setProperty<MatrixWorkspace_sptr> ("Workspace", localWorkspace);
    loadInst->setPropertyValue("InstrumentParentPath","/raw_data_1");
    loadInst->execute();
  }
  catch( std::invalid_argument&)
  {
    alg->getLogger().error("Invalid argument to LoadIDFFromNexus Child Algorithm ");
  }
  catch (std::runtime_error&)
  {
    alg->getLogger().information("No IDF found in "+nexusfilename+" at raw_data_1/Instrument");
  }

  if ( !loadInst->isExecuted() ) alg->getLogger().information("No IDF loaded from Nexus file.");   
  return loadInst->isExecuted();
}

//-----------------------------------------------------------------------------
/** Load the instrument defination file specified by info in the NXS file.
 *
 *  @param nexusfilename :: Used to pick the instrument.
 *  @param localWorkspace :: MatrixWorkspace in which to put the instrument geometry
 *  @param top_entry_name :: entry name at the top of the NXS file
 *  @param alg :: Handle of the algorithm 
 *  @return true if successful
 */
bool LoadEventNexus::runLoadInstrument(const std::string &nexusfilename, MatrixWorkspace_sptr localWorkspace,
    const std::string & top_entry_name, Algorithm * alg)
{
  string instrument = "";

  // Get the instrument name
  ::NeXus::File nxfile(nexusfilename);
  //Start with the base entry
  nxfile.openGroup(top_entry_name, "NXentry");
  // Open the instrument
  nxfile.openGroup("instrument", "NXinstrument");
  try
  {
  nxfile.openData("name");
  instrument = nxfile.getStrData();
  alg->getLogger().debug() << "Instrument name read from NeXus file is " << instrument << std::endl;
  }
  catch ( ::NeXus::Exception &)
  {
    // Get the instrument name from the file instead
    size_t n = nexusfilename.rfind('/');
    if (n != std::string::npos)
    {
      std::string temp = nexusfilename.substr(n+1, nexusfilename.size()-n-1);
      n = temp.find('_');
      if (n != std::string::npos && n > 0)
      {
        instrument = temp.substr(0, n);
      }
    }
  }
  if (instrument.compare("POWGEN3") == 0) // hack for powgen b/c of bad long name
          instrument = "POWGEN";
  if (instrument.compare("NOM") == 0) // hack for nomad
          instrument = "NOMAD";

  if (instrument.empty())
    throw std::runtime_error("Could not find the instrument name in the NXS file or using the filename. Cannot load instrument!");

  // Now let's close the file as we don't need it anymore to load the instrument.
  nxfile.close();

  // do the actual work
  IAlgorithm_sptr loadInst= alg->createChildAlgorithm("LoadInstrument");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  bool executionSuccessful(true);
  try
  {
    loadInst->setPropertyValue("InstrumentName", instrument);
    loadInst->setProperty<MatrixWorkspace_sptr> ("Workspace", localWorkspace);
    loadInst->setProperty("RewriteSpectraMap", false);
    loadInst->execute();

    // Populate the instrument parameters in this workspace - this works around a bug
    localWorkspace->populateInstrumentParameters();
  } catch (std::invalid_argument& e)
  {
    alg->getLogger().information() << "Invalid argument to LoadInstrument Child Algorithm : " << e.what() << std::endl;
    executionSuccessful = false;
  } catch (std::runtime_error& e)
  {
    alg->getLogger().information("Unable to successfully run LoadInstrument Child Algorithm");
    alg->getLogger().information(e.what());
    executionSuccessful = false;
  }

  // If loading instrument definition file fails
  if (!executionSuccessful)
  {
    alg->getLogger().error() << "Error loading Instrument definition file\n";
    return false;
  }

  // Ticket #2049: Cleanup all loadinstrument members to a single instance
  // If requested update the instrument to positions in the data file
  const Geometry::ParameterMap & pmap = localWorkspace->instrumentParameters();
  if( !pmap.contains(localWorkspace->getInstrument()->getComponentID(),"det-pos-source") ) 
    return executionSuccessful;

  boost::shared_ptr<Geometry::Parameter> updateDets = pmap.get(localWorkspace->getInstrument()->getComponentID(),"det-pos-source");
  std::string value = updateDets->value<std::string>();
  if(value.substr(0,8)  == "datafile" )
  {
    IAlgorithm_sptr updateInst = alg->createChildAlgorithm("UpdateInstrumentFromFile");
    updateInst->setProperty<MatrixWorkspace_sptr>("Workspace", localWorkspace);
    updateInst->setPropertyValue("Filename", nexusfilename);
    if(value  == "datafile-ignore-phi" )
    {
      updateInst->setProperty("IgnorePhi", true);
      alg->getLogger().information("Detector positions in IDF updated with positions in the data file except for the phi values");
    }
    else 
    {
      alg->getLogger().information("Detector positions in IDF updated with positions in the data file");
    }
    // We want this to throw if it fails to warn the user that the information is not correct.
    updateInst->execute();
  }

  return executionSuccessful;
}


//-----------------------------------------------------------------------------
/** Load the sample logs from the NXS file
 *
 *  @param nexusfilename :: Used to pick the instrument.
 *  @param localWorkspace :: MatrixWorkspace in which to put the logs
 *  @param alg :: Handle of an algorithm for logging access
 *  @return the BankPulseTimes object created, NULL if it failed.
 */
BankPulseTimes * LoadEventNexus::runLoadNexusLogs(const std::string &nexusfilename, API::MatrixWorkspace_sptr localWorkspace,
    Algorithm * alg)
{
  // --------------------- Load DAS Logs -----------------
  //The pulse times will be empty if not specified in the DAS logs.
  BankPulseTimes * out = NULL;
  IAlgorithm_sptr loadLogs = alg->createChildAlgorithm("LoadNexusLogs");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try
  {
    alg->getLogger().information() << "Loading logs from NeXus file..." << endl;
    loadLogs->setPropertyValue("Filename", nexusfilename);
    loadLogs->setProperty<MatrixWorkspace_sptr> ("Workspace", localWorkspace);
    loadLogs->execute();

    //If successful, we can try to load the pulse times
    Kernel::TimeSeriesProperty<double> * log = dynamic_cast<Kernel::TimeSeriesProperty<double> *>( localWorkspace->mutableRun().getProperty("proton_charge") );
    std::vector<Kernel::DateAndTime> temp = log->timesAsVector();
    out = new BankPulseTimes(temp);

    // Use the first pulse as the run_start time.
    if (!temp.empty())
    {
      if (temp[0] < Kernel::DateAndTime("1991-01-01T00:00:00"))
        alg->getLogger().warning() << "Found entries in the proton_charge sample log with invalid pulse time!\n";

      Kernel::DateAndTime run_start = localWorkspace->getFirstPulseTime();
      // add the start of the run as a ISO8601 date/time string. The start = first non-zero time.
      // (this is used in LoadInstrument to find the right instrument file to use).
      localWorkspace->mutableRun().addProperty("run_start", run_start.toISO8601String(), true );
    }
    else
    {
      alg->getLogger().warning() << "Empty proton_charge sample log. You will not be able to filter by time.\n";
    }
    /// Attempt to make a gonoimeter from the logs
    try
    {
      Goniometer gm;
      gm.makeUniversalGoniometer();
      localWorkspace->mutableRun().setGoniometer(gm, true);
    }
    catch(std::runtime_error &)
    {
    }
  }
  catch (...)
  {
    alg->getLogger().error() << "Error while loading Logs from SNS Nexus. Some sample logs may be missing." << std::endl;
    return out;
  }
  return out;
}
//-----------------------------------------------------------------------------
/**
 * Create the required spectra mapping. If the file contains an isis_vms_compat block then
 * the mapping is read from there, otherwise a 1:1 map with the instrument is created (along
 * with the associated spectra axis)
 * @param workspace :: The workspace to contain the spectra mapping
 * @param bankNames :: Bank names that are in Nexus file
 */
void LoadEventNexus::deleteBanks(API::MatrixWorkspace_sptr workspace, std::vector<std::string> bankNames)
{
    Instrument_sptr inst = boost::const_pointer_cast<Instrument>(workspace->getInstrument()->baseInstrument());
    //Build a list of Rectangular Detectors
    std::vector<boost::shared_ptr<RectangularDetector> > detList;
    for (int i=0; i < inst->nelements(); i++)
    {
      boost::shared_ptr<RectangularDetector> det;
      boost::shared_ptr<ICompAssembly> assem;
      boost::shared_ptr<ICompAssembly> assem2;

      det = boost::dynamic_pointer_cast<RectangularDetector>( (*inst)[i] );
      if (det)
      {
        detList.push_back(det);
      }
      else
      {
        //Also, look in the first sub-level for RectangularDetectors (e.g. PG3).
        // We are not doing a full recursive search since that will be very long for lots of pixels.
        assem = boost::dynamic_pointer_cast<ICompAssembly>( (*inst)[i] );
        if (assem)
        {
          for (int j=0; j < assem->nelements(); j++)
          {
            det = boost::dynamic_pointer_cast<RectangularDetector>( (*assem)[j] );
            if (det)
            {
              detList.push_back(det);

            }
            else
            {
              //Also, look in the second sub-level for RectangularDetectors (e.g. PG3).
              // We are not doing a full recursive search since that will be very long for lots of pixels.
              assem2 = boost::dynamic_pointer_cast<ICompAssembly>( (*assem)[j] );
              if (assem2)
              {
                for (int k=0; k < assem2->nelements(); k++)
                {
                  det = boost::dynamic_pointer_cast<RectangularDetector>( (*assem2)[k] );
                  if (det)
                  {
                    detList.push_back(det);
                  }
                }
              }
            }
          }
        }
      }
    }
    if (detList.size() == 0) return;
    for (int i = 0; i<static_cast<int>(detList.size()); i++)
    {
        bool keep = false;
        boost::shared_ptr<RectangularDetector> det = detList[i];
        std::string det_name = det->getName();
        for (int j = 0; j<static_cast<int>(bankNames.size()); j++)
        {
            size_t pos = bankNames[j].find("_events");
            if(det_name.compare(bankNames[j].substr(0,pos)) == 0) keep = true;
            if(keep) break;
        }
        if (!keep)
        {
            boost::shared_ptr<const IComponent> parent = inst->getComponentByName(det_name);
            std::vector<Geometry::IComponent_const_sptr> children;
            boost::shared_ptr<const Geometry::ICompAssembly> asmb = boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(parent);
            asmb->getChildren(children, false);
            for (int col = 0; col<static_cast<int>(children.size()); col++)
            {
                boost::shared_ptr<const Geometry::ICompAssembly> asmb2 = boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(children[col]);
                std::vector<Geometry::IComponent_const_sptr> grandchildren;
                asmb2->getChildren(grandchildren,false);

                for (int row = 0; row<static_cast<int>(grandchildren.size()); row++)
                {
                    Detector* d = dynamic_cast<Detector*>(const_cast<IComponent*>(grandchildren[row].get()));
                    inst->removeDetector(d);
                }
            }
            IComponent* comp = dynamic_cast<IComponent*>(detList[i].get());
            inst->remove(comp);
        }
    }
      return;
}
//-----------------------------------------------------------------------------
/**
 * Create the required spectra mapping. If the file contains an isis_vms_compat block then
 * the mapping is read from there, otherwise a 1:1 map with the instrument is created (along
 * with the associated spectra axis)
 * @param nxsfile :: The name of a nexus file to load the mapping from
 * @param monitorsOnly :: Load only the monitors is true
 * @param bankName :: An optional bank name for loading a single bank
 */
void LoadEventNexus::createSpectraMapping(const std::string &nxsfile, 
    const bool monitorsOnly, const std::string & bankName)
{
  bool spectramap = false;
  if( !monitorsOnly && !bankName.empty() )
  {
    // Only build the map for the single bank
    std::vector<IDetector_const_sptr> dets;
    WS->getInstrument()->getDetectorsInBank(dets, bankName);
    if (!dets.empty())
    {
      WS->resizeTo(dets.size());
      // Make an event list for each.
      for(size_t wi=0; wi < dets.size(); wi++)
      {
        const detid_t detID = dets[wi]->getID();
        WS->getSpectrum(wi)->setDetectorID(detID);
      }
      spectramap = true;
      g_log.debug() << "Populated spectra map for single bank " << bankName << "\n";
    }
    else
      throw std::runtime_error("Could not find the bank named " + bankName +
          " as a component assembly in the instrument tree; or it did not contain any detectors."
          " Try unchecking SingleBankPixelsOnly.");
  }
  else
  {
    spectramap = loadSpectraMapping(nxsfile, monitorsOnly, m_top_entry_name);
    // Did we load one? If so then the event ID is the spectrum number and not det ID
    if( spectramap ) this->event_id_is_spec = true;
  }

  if( !spectramap )
  {
    g_log.debug() << "No custom spectra mapping found, continuing with default 1:1 mapping of spectrum:detectorID\n";
    // The default 1:1 will suffice but exclude the monitors as they are always in a separate workspace
    WS->padSpectra();
    g_log.debug() << "Populated 1:1 spectra map for the whole instrument \n";
  }
}

//-----------------------------------------------------------------------------
/**
 * Returns whether the file contains monitors with events in them
 * @returns True if the file contains monitors with event data, false otherwise
 */
bool LoadEventNexus::hasEventMonitors()
{
  bool result(false);
  // Determine whether to load histograms or events
  try
  {
    ::NeXus::File file(m_filename);
    file.openPath(m_top_entry_name);
    //Start with the base entry
    typedef std::map<std::string,std::string> string_map_t; 
    //Now we want to go through and find the monitors
    string_map_t entries = file.getEntries();
    for( string_map_t::const_iterator it = entries.begin(); it != entries.end(); ++it)
    {
      if( it->second == "NXmonitor" )
      {
        file.openGroup(it->first, it->second);
        break;
      }
    }
    file.openData("event_id");
    result = true;
    file.close();
  }
  catch(::NeXus::Exception &)
  {
    result = false;
  }
  return result;
}

//-----------------------------------------------------------------------------
/**
 * Load the Monitors from the NeXus file into a workspace. The original
 * workspace name is used and appended with _monitors.
 */
void LoadEventNexus::runLoadMonitors()
{
  std::string mon_wsname = this->getProperty("OutputWorkspace");
  mon_wsname.append("_monitors");

  IAlgorithm_sptr loadMonitors = this->createChildAlgorithm("LoadNexusMonitors");
  try
  {
    this->g_log.information() << "Loading monitors from NeXus file..."
        << std::endl;
    loadMonitors->setPropertyValue("Filename", m_filename);
    this->g_log.information() << "New workspace name for monitors: "
        << mon_wsname << std::endl;
    loadMonitors->setPropertyValue("OutputWorkspace", mon_wsname);
    loadMonitors->execute();
    MatrixWorkspace_sptr mons = loadMonitors->getProperty("OutputWorkspace");
    this->declareProperty(new WorkspaceProperty<>("MonitorWorkspace",
        mon_wsname, Direction::Output), "Monitors from the Event NeXus file");
    this->setProperty("MonitorWorkspace", mons);
  }
  catch (...)
  {
    this->g_log.error() << "Error while loading the monitors from the file. "
        << "File may contain no monitors." << std::endl;
  }
}

//
/**
 * Load a spectra mapping from the given file. This currently checks for the existence of
 * an isis_vms_compat block in the file, if it exists it pulls out the spectra mapping listed there
 * @param filename :: A filename
 * @param monitorsOnly :: If true then only the monitor spectra are loaded
 * @param entry_name :: name of the NXentry to open.
 * @returns True if the mapping was loaded or false if the block does not exist
 */
bool LoadEventNexus::loadSpectraMapping(const std::string& filename, const bool monitorsOnly, const std::string& entry_name )
{
  ::NeXus::File file(filename);
  try
  {
    g_log.debug() << "Attempting to load custom spectra mapping from '" << entry_name << "/isis_vms_compat'.\n";
    file.openPath(entry_name + "/isis_vms_compat");
  }
  catch(::NeXus::Exception&)
  {
    return false; // Doesn't exist
  }
  // UDET
  file.openData("UDET");
  std::vector<int32_t> udet;
  file.getData(udet);
  file.closeData();
  // SPEC
  file.openData("SPEC");
  std::vector<int32_t> spec;
  file.getData(spec);
  file.closeData();
  // Close 
  file.closeGroup();
  file.close();

  // The spec array will contain a spectrum number for each udet but the spectrum number
  // may be the same for more that one detector
  const size_t ndets(udet.size());
  if( ndets != spec.size() )
  {
    std::ostringstream os;
    os << "UDET/SPEC list size mismatch. UDET=" << udet.size() << ", SPEC=" << spec.size() << "\n";
    throw std::runtime_error(os.str());
  }
  // Monitor filtering/selection
  const std::vector<detid_t> monitors = WS->getInstrument()->getMonitors();
  const size_t nmons(monitors.size());
  if( monitorsOnly )
  {
    g_log.debug() << "Loading only monitor spectra from " << filename << "\n";
    // Find the det_ids in the udet array. 
    WS->resizeTo(nmons);
    for( size_t i = 0; i < nmons; ++i )
    {
      // Find the index in the udet array
      const detid_t & id = monitors[i];
      std::vector<int32_t>::const_iterator it = std::find(udet.begin(), udet.end(), id);
      if( it != udet.end() )
      {
        auto spectrum = WS->getSpectrum(i);
        const specid_t & specNo = spec[it - udet.begin()];
        spectrum->setSpectrumNo(specNo);
        spectrum->setDetectorID(id);
      }
    }
  }
  else
  {
    g_log.debug() << "Loading only detector spectra from " << filename << "\n";
    SpectrumDetectorMapping mapping(spec,udet);
    WS->resizeTo(mapping.getMapping().size()-nmons);
    WS->updateSpectraUsing(mapping);
  }
  return true;
}

/**
 * Set the filters on TOF.
 * @param monitors :: If true check the monitor properties else use the standard ones
 */
void LoadEventNexus::setTimeFilters(const bool monitors)
{
  //Get the limits to the filter
  std::string prefix("Filter");
  if(monitors) prefix += "Mon";

  filter_tof_min = getProperty(prefix + "ByTofMin");
  filter_tof_max = getProperty(prefix + "ByTofMax");
  if ( (filter_tof_min == EMPTY_DBL()) ||  (filter_tof_max == EMPTY_DBL()))
  {
    //Nothing specified. Include everything
    filter_tof_min = -1e20;
    filter_tof_max = +1e20;
  }
  else if ( (filter_tof_min != EMPTY_DBL()) ||  (filter_tof_max != EMPTY_DBL()))
  {
    //Both specified. Keep these values
  }
  else
  {
    std::string msg("You must specify both min & max or neither TOF filters");
    if(monitors) msg =  " for the monitors.";
    throw std::invalid_argument(msg);
  }

}

//-----------------------------------------------------------------------------
//               ISIS event corrections
//-----------------------------------------------------------------------------
/**
 * Check if time_of_flight can be found in the file and load it 
 * @param nexusfilename :: The name of the ISIS nexus event file.
 * @param WS :: The event workspace which events will be modified.
 * @param entry_name :: An NXentry tag in the file
 * @param classType :: The type of the events: either detector or monitor
 */
void LoadEventNexus::loadTimeOfFlight(const std::string &nexusfilename, DataObjects::EventWorkspace_sptr WS,
  const std::string &entry_name, const std::string &classType)
{
  bool done = false;
  // Open the file
  ::NeXus::File file(nexusfilename);
  file.openGroup(entry_name, "NXentry");
  
  typedef std::map<std::string,std::string> string_map_t; 
  string_map_t entries = file.getEntries();

  if (entries.find("detector_1_events") == entries.end())
  {// not an ISIS file
    return;
  }

  // try if monitors have their own bins
  if (classType == "NXmonitor")
  {
    std::vector<std::string> bankNames;
    for (string_map_t::const_iterator it = entries.begin(); it != entries.end(); ++it)
    {
      std::string entry_name(it->first);
      std::string entry_class(it->second);
      if ( entry_class == classType )
      {
        bankNames.push_back( entry_name );
      }
    }
    for(size_t i = 0; i < bankNames.size(); ++i)
    {
      const std::string& mon = bankNames[i];
      file.openGroup(mon,classType);
      entries = file.getEntries();
      string_map_t::const_iterator bins = entries.find("event_time_bins");
      if (bins == entries.end())
      {
        //bins = entries.find("time_of_flight"); // I think time_of_flight doesn't work here
        //if (bins == entries.end())
        //{
          done = false;
          file.closeGroup();
          break; // done == false => use bins from the detectors
        //}
      }
      done = true;
      loadTimeOfFlightData(file,WS,bins->first,i,i+1);
      file.closeGroup();
    }
  }

  if (!done)
  {
    // first check detector_1_events
    file.openGroup("detector_1_events", "NXevent_data");
    entries = file.getEntries();
    for(string_map_t::const_iterator it = entries.begin();it != entries.end(); ++it)
    {
      if (it->first == "time_of_flight" || it->first == "event_time_bins")
      {
        loadTimeOfFlightData(file,WS,it->first);
        done = true;
      }
    }
    file.closeGroup(); // detector_1_events

    if (!done) // if time_of_flight was not found try instrument/dae/time_channels_#
    {
      file.openGroup("instrument","NXinstrument");
      file.openGroup("dae","IXdae");
      entries = file.getEntries();
      size_t time_channels_number = 0;
      for(string_map_t::const_iterator it = entries.begin();it != entries.end(); ++it)
      {
        // check if there are groups with names "time_channels_#" and select the one with the highest number
        if (it->first.size() > 14 && it->first.substr(0,14) == "time_channels_")
        {
          size_t n = boost::lexical_cast<size_t>(it->first.substr(14));
          if (n > time_channels_number) 
          {
            time_channels_number = n;
          }
        }
      }
      if (time_channels_number > 0) // the numbers start with 1
      {
        file.openGroup("time_channels_" + boost::lexical_cast<std::string>(time_channels_number),"IXtime_channels");
        entries = file.getEntries();
        for(string_map_t::const_iterator it = entries.begin();it != entries.end(); ++it)
        {
          if (it->first == "time_of_flight" || it->first == "event_time_bins")
          {
            loadTimeOfFlightData(file,WS,it->first);
          }
        }
        file.closeGroup();
      }
      file.closeGroup(); // dae
      file.closeGroup(); // instrument
    }
  }

  file.close();
}

//-----------------------------------------------------------------------------
/** 
 * Load the time of flight data. file must have open the group containing "time_of_flight" data set.
 * @param file :: The nexus file to read from.
 * @param WS :: The event workspace to write to.
 * @param binsName :: bins name
 * @param start_wi :: First workspace index to process
 * @param end_wi :: Last workspace index to process
 */
void LoadEventNexus::loadTimeOfFlightData(::NeXus::File& file, DataObjects::EventWorkspace_sptr WS,
  const std::string& binsName,size_t start_wi, size_t end_wi)
{
  // first check if the data is already randomized
  std::map<std::string, std::string> entries;
  file.getEntries(entries);
  std::map<std::string, std::string>::const_iterator shift = entries.find("event_time_offset_shift");
  if (shift != entries.end())
  {
    std::string random;
    file.readData("event_time_offset_shift",random);
    if (random == "random")
    {
      return;
    }
  }

  // if the data is not randomized randomize it uniformly within each bin
  file.openData(binsName);
  // time of flights of events
  std::vector<float> tof;
  file.getData(tof);
  // todo: try to find if tof can be reduced to just 3 numbers: start, end and dt
  if (end_wi <= start_wi)
  {
    end_wi = WS->getNumberHistograms();
  }

  // random number generator
  boost::mt19937 rand_gen;

  // loop over spectra
  for(size_t wi = start_wi; wi < end_wi; ++wi)
  {
    EventList& event_list = WS->getEventList(wi);
    // sort the events
    event_list.sortTof();
    std::vector<TofEvent>& events = event_list.getEvents();
    if (events.empty()) continue;
    size_t n = tof.size();
    // iterate over the events and time bins
    std::vector<TofEvent>::iterator ev = events.begin();
    std::vector<TofEvent>::iterator ev_end = events.end();
    for(size_t i = 1; i < n; ++i)
    {
      double right = double(tof[i]);
      // find the right boundary for the current event
      if(ev != ev_end && right < ev->tof() )
      {
        continue;
      }
      // count events which have the same right boundary
      size_t m = 0;
      while(ev != ev_end && ev->tof() < right)
      {
        ++ev;
        ++m;  // count events in the i-th bin
      }
      
      if (m > 0)
      {// m events in this bin
        double left = double(tof[i-1]);
        // spread the events uniformly inside the bin
        boost::uniform_real<> distribution(left,right);
        std::vector<double> random_numbers(m);
        for(std::vector<double>::iterator it = random_numbers.begin(); it != random_numbers.end(); ++it)
        {
          *it = distribution(rand_gen);
        }
        std::sort(random_numbers.begin(),random_numbers.end());
        std::vector<double>::iterator it = random_numbers.begin();
        for(std::vector<TofEvent>::iterator ev1 = ev - m; ev1 != ev; ++ev1,++it)
        {
          ev1->m_tof = *it;
        }
      }
      
    } // for i

    event_list.sortTof();
  } // for wi
  file.closeData();
}

} // namespace DataHandling
} // namespace Mantid
