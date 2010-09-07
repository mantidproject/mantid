#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <functional>
#include <iostream>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <set>
#include <vector>
#include <boost/timer.hpp>
#include "MantidAPI/FileFinder.h"
#include "MantidDataHandling/LoadEventPreNeXus.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileValidator.h"
#include "MantidKernel/Glob.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/BinaryFile.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadEventPreNeXus)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using boost::posix_time::ptime;
using boost::posix_time::time_duration;
using DataObjects::EventList;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_sptr;
using DataObjects::TofEvent;
using std::cout;
using std::endl;
using std::ifstream;
using std::runtime_error;
using std::stringstream;
using std::string;
using std::vector;

// constants for locating the parameters to use in execution
static const string EVENT_PARAM("EventFilename");
static const string PULSEID_PARAM("PulseidFilename");
static const string MAP_PARAM("MappingFilename");
static const string PID_PARAM("SpectrumList");
static const string PAD_PIXELS_PARAM("PadEmptyPixels");
static const string PARALLEL_PARAM("UseParallelProcessing");
static const string BLOCK_SIZE_PARAM("LoadingBlockSize");
static const string OUT_PARAM("OutputWorkspace");

static const string PULSE_EXT("pulseid.dat");
static const string EVENT_EXT("event.dat");

/// Default number of items to read in from any of the files.
static const size_t DEFAULT_BLOCK_SIZE = 1000000; // 100,000
/// All pixel ids with matching this mask are errors.
static const PixelType ERROR_PID = 0x80000000;
/// The maximum possible tof as native type
static const uint32_t MAX_TOF_UINT32 = std::numeric_limits<uint32_t>::max();

/// The difference in seconds between standard unix and gps epochs.
static const uint32_t EPOCH_DIFF = 631152000;
/// The epoch for GPS times.
static const ptime GPS_EPOCH(boost::gregorian::date(1990, 1, 1));
/// The epoch for Unix times.
static const ptime UNIX_EPOCH(boost::gregorian::date(1970, 1, 1));
/// The number of nanoseconds in a second.
static const uint32_t NANO_TO_SEC = 1000000000;

/// Conversion factor between 100 nanoseconds and 1 microsecond.
static const double TOF_CONVERSION = .1;
/// Conversion factor between picoColumbs and microAmp*hours
static const double CURRENT_CONVERSION = 1.e-6 / 3600.;

LoadEventPreNeXus::LoadEventPreNeXus() : Mantid::API::Algorithm()
{
  this->eventfile = NULL;
  this->max_events = 0;
}

LoadEventPreNeXus::~LoadEventPreNeXus()
{
  if (this->eventfile)
    delete this->eventfile;
}

//-----------------------------------------------------------------------------
/** Initialize the algorithm */
void LoadEventPreNeXus::init()
{
  // reset the logger's name
  this->g_log.setName("DataHandling::LoadEventPreNeXus");

  // which files to use
  this->declareProperty(new FileProperty(EVENT_PARAM, "", FileProperty::Load, EVENT_EXT),
                        "A preNeXus neutron event file");
  this->declareProperty(new FileProperty(PULSEID_PARAM, "", FileProperty::OptionalLoad, PULSE_EXT),
                        "A preNeXus pulseid file. Used only if specified.");
  this->declareProperty(new FileProperty(MAP_PARAM, "", FileProperty::OptionalLoad, ".dat"),
                        "TS mapping file converting detector id to pixel id. Used only if specified.");

  // which pixels to load
  this->declareProperty(new ArrayProperty<int>(PID_PARAM),
                        "A list of individual spectra (pixel IDs) to read. Only used if set.");

  // Pad out empty pixels?
  this->declareProperty(new PropertyWithValue<bool>(PAD_PIXELS_PARAM, false, Direction::Input) );

#ifdef LOADEVENTPRENEXUS_ALLOW_PARALLEL
  //Parallel processing
  this->declareProperty(new PropertyWithValue<bool>(PARALLEL_PARAM, true, Direction::Input) );

  //Loading block size
  this->declareProperty(new PropertyWithValue<int>(BLOCK_SIZE_PARAM, 500000, Direction::Input) );
#endif

  // the output workspace name
  this->declareProperty(new WorkspaceProperty<IEventWorkspace>(OUT_PARAM,"",Direction::Output));

}

//-----------------------------------------------------------------------------
static string generatePulseidName(string eventfile)
{
  size_t start;
  string ending;

  // normal ending
  ending = "neutron_event.dat";
  start = eventfile.find(ending);
  if (start != string::npos)
    return eventfile.replace(start, ending.size(), "pulseid.dat");

  // split up event files - yes this is copy and pasted code
  ending = "neutron0_event.dat";
  start = eventfile.find(ending);
  if (start != string::npos)
    return eventfile.replace(start, ending.size(), "pulseid0.dat");

  ending = "neutron1_event.dat";
  start = eventfile.find(ending);
  if (start != string::npos)
    return eventfile.replace(start, ending.size(), "pulseid1.dat");

  return "";
}

//-----------------------------------------------------------------------------
static string generateMappingfileName(EventWorkspace_sptr &wksp)
{//
  // get the name of the mapping file as set in the parameter files
  std::vector<string> temp = wksp->getInstrument()->getStringParameter("TS_mapping_file");
  if (temp.empty())
    return "";
  string mapping = temp[0];

  // Try to get it from the data directories
  string dataversion = Mantid::API::FileFinder::Instance().getFullPath(mapping);
  if (!dataversion.empty())
    return dataversion;

  // get a list of all proposal directories
  string instrument = wksp->getInstrument()->getName();
  Poco::File base("/SNS/" + instrument + "/");
  if (!base.exists())
    return "";
  vector<string> dirs; // poco won't let me reuse temp
  base.list(dirs);

  // check all of the proposals for the mapping file in the canonical place
  const string CAL("_CAL");
  vector<string> files;
  for (size_t i = 0; i < dirs.size(); ++i) {
    if (dirs[i].compare(dirs[i].length() - CAL.length(), CAL.length(), CAL) == 0) {
      if (Poco::File(base.path() + "/" + dirs[i] + "/calibrations/" + mapping).exists())
        files.push_back(base.path() + "/" + dirs[i] + "/calibrations/" + mapping);
    }
  }

  if (files.empty())
    return "";
  else if (files.size() == 1)
    return files[0];
  else // just assume that the last one is the right one, this should never be fired
    return *(files.rbegin());
}

//-----------------------------------------------------------------------------
/** Execute the algorithm */
void LoadEventPreNeXus::exec()
{
  // what spectra (pixel ID's) to load
  this->spectra_list = this->getProperty(PID_PARAM);

  // the event file is needed in case the pulseid fileanme is empty
  string event_filename = this->getPropertyValue(EVENT_PARAM);
  string pulseid_filename = this->getPropertyValue(PULSEID_PARAM);
  if (pulseid_filename.empty())
  {
    pulseid_filename = generatePulseidName(event_filename);
    if (!pulseid_filename.empty())
    {
      if (Poco::File(pulseid_filename).exists())
        this->g_log.information() << "Found pulseid file " << pulseid_filename << std::endl;
      else
        pulseid_filename = "";

    }
  }

  this->readPulseidFile(pulseid_filename);

  this->openEventFile(event_filename);

  // prep the output workspace
  EventWorkspace_sptr localWorkspace = EventWorkspace_sptr(new EventWorkspace());
  //Make sure to initialize.
  //   We can use dummy numbers for arguments, for event workspace it doesn't matter
  localWorkspace->initialize(1,1,1);

  // Set the units
  localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  localWorkspace->setYUnit("Counts");
  // TODO localWorkspace->setTitle(title);

  //Get the instrument!
  this->runLoadInstrument(event_filename, localWorkspace);

  // load the mapping file
  string mapping_filename = this->getPropertyValue(MAP_PARAM);
  if (mapping_filename.empty()) {
    mapping_filename = generateMappingfileName(localWorkspace);
    if (!mapping_filename.empty())
      this->g_log.information() << "Found mapping file \"" << mapping_filename << "\"" << std::endl;
  }
  this->loadPixelMap(mapping_filename);

#ifdef LOADEVENTPRENEXUS_ALLOW_PARALLEL
  clock_t start = clock();
#endif

  //Process the events into pixels
  this->procEvents(localWorkspace);

#ifdef LOADEVENTPRENEXUS_ALLOW_PARALLEL
  clock_t stop = clock();
  double elapsed = double(stop - start)/CLOCKS_PER_SEC;
  std::cout << "procEvents (Parallel = " << this->parallelProcessing << ") took " << elapsed << " seconds.\n";
#endif

  //Save output
  this->setProperty<IEventWorkspace_sptr>(OUT_PARAM, localWorkspace);
}



//-----------------------------------------------------------------------------
/** Load the instrument geometry File
 *  @param eventfilename Used to pick the instrument.
 *  @param localWorkspace MatrixWorkspace in which to put the instrument geometry
 */
void LoadEventPreNeXus::runLoadInstrument(const std::string &eventfilename, MatrixWorkspace_sptr localWorkspace)
{
  // determine the instrument parameter file
  string instrument = Poco::Path(eventfilename).getFileName();
  size_t pos = instrument.rfind("_"); // get rid of 'event.dat'
  pos = instrument.rfind("_", pos-1); // get rid of 'neutron'
  pos = instrument.rfind("_", pos-1); // get rid of the run number
  instrument = instrument.substr(0, pos);

  string filename = Mantid::Kernel::ConfigService::Instance().getInstrumentFilename(instrument);
  if (filename.empty())
    return;
  if (!Poco::File(filename).exists())
    return;

  // do the actual work
  IAlgorithm_sptr loadInst= createSubAlgorithm("LoadInstrument");

  // Now execute the sub-algorithm. Catch and log any error, but don't stop.
  bool executionSuccessful(true);
  try
  {
    loadInst->setPropertyValue("Filename", filename);
    loadInst->setProperty<MatrixWorkspace_sptr> ("Workspace", localWorkspace);
    loadInst->execute();

    // Populate the instrument parameters in this workspace - this works around a bug
    localWorkspace->populateInstrumentParameters();
  } catch (std::invalid_argument& e)
  {
    stringstream msg;
    msg << "Invalid argument to LoadInstrument sub-algorithm : " << e.what();
    g_log.information(msg.str());
    executionSuccessful = false;
  } catch (std::runtime_error& e)
  {
    g_log.information("Unable to successfully run LoadInstrument sub-algorithm");
    g_log.information(e.what());
    executionSuccessful = false;
  }

  // If loading instrument definition file fails
  if (!executionSuccessful)
  {
    g_log.error() << "Error loading Instrument definition file\n";
    //TODO: Load some other way???
//    g_log.information() << "Instrument definition file "
//      << fullPathIDF << " not found. Attempt to load information about \n"
//      << "the instrument from raw data file.\n";
//    runLoadInstrumentFromRaw(fileName,localWorkspace);
  }
  else
  {
    this->instrument_loaded_correctly = true;
//    m_monitordetectorList = loadInst->getProperty("MonitorList");
//    std::vector<int>::const_iterator itr;
//    for (itr = m_monitordetectorList.begin(); itr != m_monitordetectorList.end(); ++itr)
//    {
//      g_log.debug() << "Monitor detector id is " << (*itr) << std::endl;
//    }
  }
}

//-----------------------------------------------------------------------------
/** Turn a pixel id into a "corrected" pixelid and period.
 *
 */
void LoadEventPreNeXus::fixPixelId(PixelType &pixel, uint32_t &period) const
{
  if (!this->using_mapping_file) { // nothing to do here
    period = 0;
    return;
  }

  PixelType unmapped_pid = pixel % this->numpixel;
  period = (pixel - unmapped_pid) / this->numpixel;
  pixel = this->pixelmap[unmapped_pid];
}

//-----------------------------------------------------------------------------
/** Special function to reduce the number of loaded events.
 *
 */
void LoadEventPreNeXus::setMaxEventsToLoad(std::size_t max_events_to_load)
{
  this->max_events = max_events_to_load;
}


//-----------------------------------------------------------------------------
/** Process the event file properly.
 * @param workspace EventWorkspace to write to.
 */
void LoadEventPreNeXus::procEvents(DataObjects::EventWorkspace_sptr & workspace)
{
  // do the actual loading
  this->num_error_events = 0;
  this->num_good_events = 0;
  this->num_ignored_events = 0;

  //Default values in the case of no parallel
  parallelProcessing = false;
  loadBlockSize = Mantid::Kernel::DEFAULT_BLOCK_SIZE;

#ifdef LOADEVENTPRENEXUS_ALLOW_PARALLEL
  parallelProcessing = this->getProperty(PARALLEL_PARAM);

  //Get the block size
  int temp = this->getProperty(BLOCK_SIZE_PARAM);
  loadBlockSize = static_cast<size_t>( temp );
  if (loadBlockSize < Mantid::Kernel::MIN_BLOCK_SIZE)
    loadBlockSize = Mantid::Kernel::MIN_BLOCK_SIZE;
  if (loadBlockSize > Mantid::Kernel::MAX_BLOCK_SIZE)
    loadBlockSize = Mantid::Kernel::MAX_BLOCK_SIZE;
  g_log.information() << "Loading events with a block size of " << loadBlockSize << "\n.";
#endif

  shortest_tof = static_cast<double>(MAX_TOF_UINT32) * TOF_CONVERSION;
  longest_tof = 0.;

  //Initialize progress reporting.
  Progress prog(this,0.0,1.0, this->num_events/loadBlockSize);

  //Allocate the buffer
  DasEvent * event_buffer = new DasEvent[loadBlockSize];

  //For slight speed up
  loadOnlySomeSpectra = (this->spectra_list.size() > 0);

  //Turn the spectra list into a map, for speed of access
  for (std::vector<int>::iterator it = spectra_list.begin(); it != spectra_list.end(); it++)
    spectraLoadMap[*it] = true;

  //Allocate the intermediate buffer, if it will be used in parallel processing
  IntermediateEvent * intermediate_buffer;
  if (parallelProcessing)
    intermediate_buffer = new IntermediateEvent[loadBlockSize];


  while (eventfile->getOffset() < this->num_events)
  {
    //Offset into the file before loading this new block
    size_t fileOffset = eventfile->getOffset();

    //Load a block into the buffer directly, up to loadBlockSize elements
    size_t current_event_buffer_size = eventfile->loadBlock(event_buffer, loadBlockSize);

    if (parallelProcessing)
    {
      //First we need to pre-treat the event buffer
      makeIntermediateEventBuffer(event_buffer, current_event_buffer_size, intermediate_buffer, fileOffset);

      //and now process the intermediate buffer
      procEventsParallel(workspace, intermediate_buffer, current_event_buffer_size);
    }
    else
      procEventsLinear(workspace, event_buffer, current_event_buffer_size, fileOffset);

    //Report progress
    prog.report();
  }

  //Clean up the buffers
  delete [] event_buffer;
  if (parallelProcessing)
    delete [] intermediate_buffer;

  //--------- Pad Empty Pixels -----------
  if (this->getProperty(PAD_PIXELS_PARAM))
  {
    //We want to pad out empty pixels.
    if (!this->instrument_loaded_correctly)
    {
      g_log.warning() << "Warning! Cannot pad empty pixels, since the instrument geometry did not load correctly or was not specified. Sorry!\n";
    }
    else
    {
      std::map<int, Geometry::IDetector_sptr> detector_map = workspace->getInstrument()->getDetectors();
      std::map<int, Geometry::IDetector_sptr>::iterator it;
      for (it = detector_map.begin(); it != detector_map.end(); it++)
      {
        //Go through each pixel in the map, but forget monitors.
        if (!it->second->isMonitor())
        {
          // and simply get the event list. It will be created if it was not there already.
          workspace->getEventListAtPixelID(it->first); //it->first is detector ID #
        }
      }
    }
  }


//  if (parallelProcessing)
//  {
//    //TODO: Pad pixels
//    //Finalize for the parallel mode
//    workspace->doneAddingEventLists();
//  }
//  else
//  {
    //finalize loading; this condenses the pixels into a 0-based, dense vector.
    workspace->doneLoadingData(1);
//  }


  // add the frame information to the event workspace
  for (size_t i = 0; i < this->num_pulses; i++)
    workspace->addTime(i, this->pulsetimes[i]);

  this->setProtonCharge(workspace);


  //Make sure the MRU is cleared
  workspace->clearMRU();

  //Now, create a default X-vector for histogramming, with just 2 bins.
  Kernel::cow_ptr<MantidVec> axis;
  MantidVec& xRef = axis.access();
  xRef.resize(2);
  xRef[0] = shortest_tof - 1; //Just to make sure the bins hold it all
  xRef[1] = longest_tof + 1;
  workspace->setAllX(axis);

  g_log.information() << "Read " << this->num_good_events << " events + "
      << this->num_error_events << " errors"
      << ". Shortest TOF: " << shortest_tof << " microsec; longest TOF: "
      << longest_tof << " microsec." << std::endl;

}

//-----------------------------------------------------------------------------
/** Linear-version of the procedure to process the event file properly.
 * @param workspace EventWorkspace to write to.
 */
void LoadEventPreNeXus::procEventsLinear(DataObjects::EventWorkspace_sptr & workspace, DasEvent * event_buffer,
    size_t current_event_buffer_size, size_t fileOffset)
{

  DasEvent temp;
  uint32_t period;
  size_t frame_index(0);

  // process the individual events
  for (size_t i = 0; i < current_event_buffer_size; i++)
  {
    temp = *(event_buffer + i);
    PixelType pid = temp.pid;

    if ((pid & ERROR_PID) == ERROR_PID) // marked as bad
    {
      this->num_error_events++;
      continue;
    }

    //Covert the pixel ID from DAS pixel to our pixel ID
    this->fixPixelId(pid, period);

    //Now check if this pid we want to load.
    if (loadOnlySomeSpectra)
    {
      std::map<int, bool>::iterator it;
      it = spectraLoadMap.find(pid);
      if (it == spectraLoadMap.end())
      {
        //Pixel ID was not found, so the event is being ignored.
        this->num_ignored_events++;
        continue;
      }
    }

    // work with the good guys
    frame_index = this->getFrameIndex(fileOffset + i, frame_index);
    double tof = static_cast<double>(temp.tof) * TOF_CONVERSION;
    TofEvent event;
    event = TofEvent(tof, frame_index);


    //Find the overall max/min tof
    if (tof < shortest_tof)
      shortest_tof = tof;
    if (tof > longest_tof)
      longest_tof = tof;

    //The addEventQuickly method does not clear the cache, making things slightly faster.
    workspace->getEventListAtPixelID(pid).addEventQuickly(event);

    // TODO work with period
    this->num_good_events++;

  }//for each event
}

//-----------------------------------------------------------------------------
/// Comparator for sorting dasevent lists
bool intermediatePixelIDComp(IntermediateEvent x, IntermediateEvent y)
{
  return (x.pid < y.pid);
}


//-----------------------------------------------------------------------------
/** Take the DASevent buffer and convert to the intermediate event structure for procEventsParallel.
 * The intermediate structure holds the frame index (which would be lost otherwise)
 *
 * @param event_buffer: the array of DasEvent loaded from disk
 * @param current_event_buffer_size: how much of the buffer was loaded (reference: will be modified
 *        if some of the events are in error)
 * @param intermediate_buffer: pointer to an array of IntermediateEvent's that will be filled up to
 *        current_event_buffer_size. Must have been allocated before!
 */
void LoadEventPreNeXus::makeIntermediateEventBuffer(DasEvent * event_buffer, size_t& current_event_buffer_size,
    IntermediateEvent * intermediate_buffer, size_t fileOffset)
{
  DasEvent dasTemp;
  size_t frame_index = 0;
  size_t i;
  uint32_t period;

  //Offset into the file
  size_t total_event_offset = 0;

  size_t bad_events = 0;

  for (i = 0; i < current_event_buffer_size; i++)
  {
    //This is the DASevent
    dasTemp = *(event_buffer + i);
    //And this is where it was in the file
    total_event_offset = fileOffset + i;

    PixelType pid = dasTemp.pid;
    if ((pid & ERROR_PID) == ERROR_PID)
    {
      // Marked as bad
      bad_events++;
    }
    else
    {
      // NOT marked as bad

      //Covert the pixel ID from DAS pixel to our pixel ID
      this->fixPixelId(pid, period);

      //Now find the frame index
      if (frame_index < this->num_pulses-1)
      {
        //We are not at the end of the pulse ID list.
        //Is the current event offset now pushing us higher than the next frame?
        if (total_event_offset >= this->event_indices[frame_index+1])
        {
          // search for the frame that gives the right pulse id
          for ( ; frame_index < this->num_pulses-1; frame_index++)
          {
            //The next up pulse's offset is higher than us; so this must be right
            if (this->event_indices[frame_index+1] > total_event_offset)
              break;
          }

          //Check for the very last pulse
          if ((frame_index == this->num_pulses-2) &&
              (total_event_offset >= event_indices[num_pulses-1]))
            frame_index = num_pulses-1;
        }
      }

      //Copy the das event stuff and add the frame index
      intermediate_buffer[i].pid = pid;
      intermediate_buffer[i].tof = dasTemp.tof;
      intermediate_buffer[i].period = period;
      intermediate_buffer[i].frame_index = frame_index;

      //And this is a good event
      this->num_good_events ++;
    } //(all good pixels)
  }

  //Increment the bad events
  this->num_error_events += bad_events;
  //And fix the buffer size if any bad events were found
  current_event_buffer_size -= bad_events;
}

//-----------------------------------------------------------------------------
/** Parallel-version of the procedure to process the event file properly.
 * @param workspace EventWorkspace to write to.
 */
void LoadEventPreNeXus::procEventsParallel(DataObjects::EventWorkspace_sptr & workspace, IntermediateEvent * intermediate_buffer, size_t current_event_buffer_size)
{
  IntermediateEvent temp;

  //Start by sorting events by DAS pixel IDs
  std::sort( intermediate_buffer, intermediate_buffer + current_event_buffer_size, intermediatePixelIDComp );

  //This vector contains a list of pairs:
  //  pair->first = pid
  //  pair->second = index in event_buffer where pid _ENDS_
  typedef std::pair<PixelType, size_t> IndexPair;
  std::vector< IndexPair > indices;

  //Start at the lowest possible pixel ID
  PixelType lastpid = std::numeric_limits<PixelType>::min();
  size_t i;
  for (i = 0; i < current_event_buffer_size; i++)
  {
    PixelType pid = intermediate_buffer[i].pid;
    if (pid > lastpid)
    {
      //Switching to a new set of events
      //Put in the vector where that change was
      indices.push_back( IndexPair(lastpid, i) );
      lastpid = pid;
    }
  }
  //And save the last one
  indices.push_back( IndexPair(lastpid, i) );
  int num_indices = indices.size();

//  // Now we go and create all the workspace indices
//  int old_max = workspace->getNumberHistograms();
//  int maxPid = static_cast<int>(  indices.back().first  );
//  EventList newEL;
//  workspace->getOrAddEventList(maxPid).addDetectorID(i);
//  for (int i=old_max; i < maxPid ; i++)
//  {
//    workspace->getOrAddEventList(i).addDetectorID(i);
//  }


  // To prepare the workspace, we need to create all the needed event lists FIRST!
  //  (but not in parallel)
  for (int i_indices=1; i_indices < num_indices; i_indices++)
  {
    PixelType pid = indices[i_indices].first;
    //This call creates the event list as needed.
    //  And then allocates and event list of the size of # of events in here.
    workspace->getEventListAtPixelID(pid).allocateMoreEvents(indices[i_indices].second - indices[i_indices-1].second);
  }



  //Start the OpenMP parallel run. We are not checking that the WS is thread-safe,
  //  because it returns False at this point.

  //The first entry in the indices list is useless, so we start at 1
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int i_indices=1; i_indices < num_indices; i_indices++)
  {
    PARALLEL_START_INTERUPT_REGION

    IndexPair myIndex = indices[i_indices];
    PixelType pid = myIndex.first;
    size_t end = myIndex.second;
    size_t start = indices[i_indices-1].second;

    //Now check if this pid we want to load.
    if (loadOnlySomeSpectra)
    {
      std::map<int, bool>::iterator it;
      it = spectraLoadMap.find(pid);
      if (it == spectraLoadMap.end())
      {
        //Pixel ID was not found, so the event is being ignored.
        this->num_ignored_events++;
        continue;
      }
    }

    //Ok, now we can go through the N events and add them all
    for (size_t j = start; j < end; j++)
    {
      //This is the intermediate event at this point
      temp = intermediate_buffer[j];

      //Convert the TOF
      double tof = static_cast<double>(temp.tof) * TOF_CONVERSION;
      //Create an event with that calculated tof and the previously found frame index
      TofEvent event(tof, temp.frame_index);

      //The addEventQuickly method does not clear the cache, making things slightly faster.
      workspace->getEventListAtPixelID(pid).addEventQuickly(event);
      //workspace->getEventList(pid).addEventQuickly(event);

      //Find the overall max/min tof
      if (tof < shortest_tof)
        shortest_tof = tof;
      if (tof > longest_tof)
        longest_tof = tof;
    }

    PARALLEL_END_INTERUPT_REGION
  } //Looping through the index pairs
  PARALLEL_CHECK_INTERUPT_REGION

}



//-----------------------------------------------------------------------------
/**
 * Add a sample environment log for the proton chage and set the scalar 
 * value on the sample.
 * @param workspace Event workspace to set the proton charge on
 */
void LoadEventPreNeXus::setProtonCharge(DataObjects::EventWorkspace_sptr & workspace)
{
  if (this->proton_charge.empty()) // nothing to do
    return;
  Run& run = workspace->mutableRun();
  run.setProtonCharge(this->proton_charge_tot);

  TimeSeriesProperty<double>* log = new TimeSeriesProperty<double>("ProtonCharge");
  size_t num = this->proton_charge.size();
  for (size_t i = 0; i < num; i++)
    log->addValue(this->pulsetimes[i], this->proton_charge[i]);
  /// TODO set the units for the log
  run.addLogData(log);
}

//-----------------------------------------------------------------------------
/**
 * Determine the frame index from the event index.
 * @param event_index The index of the event in the event data
 * @param last_frame_index Last frame found. This parameter reduces the
 *        search to be from the current point forward.
 */
size_t LoadEventPreNeXus::getFrameIndex(const std::size_t event_index, const std::size_t last_frame_index)
{
  // if at the end of the file return the last frame_index
  if (last_frame_index + 1 >= this->num_pulses)
    return last_frame_index;

  // search for the next frame that increases the event index
  size_t next_frame_index = last_frame_index + 1;
  for ( ; next_frame_index < this->num_pulses - 1; next_frame_index++)
  {
    if (this->event_indices[next_frame_index] > this->event_indices[last_frame_index])
      break;
  }
  // determine the frame index
  if (this->event_indices[next_frame_index] <= event_index)
    return next_frame_index;
  else
    return last_frame_index;
}


//-----------------------------------------------------------------------------
/** Load a pixel mapping file
 * @param filename Path to file.
 */
void LoadEventPreNeXus::loadPixelMap(const std::string &filename)
{
  this->using_mapping_file = false;
  this->pixelmap.clear();

  // check that there is a mapping file
  if (filename.empty()) {
    this->g_log.information("NOT using a mapping file");
    return;
  }

  // actually deal with the file
  this->g_log.debug("Using mapping file \"" + filename + "\"");

  //Open the file; will throw if there is any problem
  BinaryFile<PixelType> pixelmapFile(filename);
  PixelType max_pid = static_cast<PixelType>(pixelmapFile.getNumElements());
  //Load all the data
  pixelmapFile.loadAllInto( this->pixelmap );

  //Check for funky file
  if (std::find_if(pixelmap.begin(), pixelmap.end(), std::bind2nd(std::greater<PixelType>(), max_pid))
          != pixelmap.end())
  {
    this->g_log.warning("Pixel id in mapping file was out of bounds. Loading without mapping file");
    this->numpixel = 0;
    this->pixelmap.clear();
    this->using_mapping_file = false;
    return;
  }

  //If we got here, the mapping file was loaded correctly and we'll use it
  this->using_mapping_file = true;
  //Let's assume that the # of pixels in the instrument matches the mapping file length.
  this->numpixel = pixelmapFile.getNumElements();
}

//-----------------------------------------------------------------------------
/** Open an event file
 * @param filename file to open.
 */
void LoadEventPreNeXus::openEventFile(const std::string &filename)
{
  //Open the file
  this->eventfile = new BinaryFile<DasEvent>(filename);
  this->num_events = eventfile->getNumElements();
  //Limit the # of events to load?
  if (this->max_events > 0)
    this->num_events = this->max_events;
  this->g_log.information()<< "Reading " <<  this->num_events << " event records\n";

}

//-----------------------------------------------------------------------------
/** Get time with nanosecond resolution.
 * @param sec seconds
 * @param nano nanoseconds
 */
static ptime getTime(uint32_t sec, uint32_t nano)
{
  // push the nanoseconds to be less than one second

  if (nano / NANO_TO_SEC > 0)
  {
    sec = nano / NANO_TO_SEC;
    nano = nano % NANO_TO_SEC;
  }

#ifdef BOOST_DATE_TIME_HAS_NANOSECONDS
  return GPS_EPOCH + boost::posix_time::seconds(sec) + boost::posix_time::nanoseconds(nano);
#else
  //Use microseconds if you can't use nano.
  return GPS_EPOCH + boost::posix_time::seconds(sec) + boost::posix_time::microseconds(nano / 1000);
#endif
}

//-----------------------------------------------------------------------------
/** Read a pulse ID file
 * @param filename file to load.
 */
void LoadEventPreNeXus::readPulseidFile(const std::string &filename)
{
  this->proton_charge_tot = 0.;
  this->num_pulses = 0;

  // jump out early if there isn't a filename
  if (filename.empty()) {
    this->g_log.information("NOT using a pulseid file");
    return;
  }

  // set up for reading
  this->g_log.debug("Using pulseid file \"" + filename + "\"");

  //Open the file; will throw if there is any problem
  BinaryFile<Pulse> pulseFile(filename);
  //Get the # of pulse
  this->num_pulses = pulseFile.getNumElements();

  //Load all the data
  std::vector<Pulse> * pulses;
  pulses = pulseFile.loadAll();

  double temp;
  for (std::vector<Pulse>::iterator it = pulses->begin(); it != pulses->end(); it++)
  {
    this->pulsetimes.push_back(getTime((*it).seconds, (*it).nanoseconds));
    this->event_indices.push_back((*it).event_index);
    temp = (*it).pCurrent; // * CURRENT_CONVERSION;
    this->proton_charge.push_back(temp);
    if (temp < 0.)
      this->g_log.warning("Individiual proton charge < 0 being ignored");
    else
      this->proton_charge_tot += temp;
  }

  this->proton_charge_tot = this->proton_charge_tot * CURRENT_CONVERSION;
  this->g_log.information() << "Total proton charge of " << this->proton_charge_tot << " microAmp*hours.\n";

  //Clear the vector
  delete pulses;

}

} // namespace DataHandling
} // namespace Mantid
