#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <functional>
#include <iostream>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <set>
#include <vector>
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
//                        "Set to True to pad empty pixels, loaded from the instrument geometry file. Nothing is done if no geometry file was specified.");

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
{
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

  this->runLoadInstrument(event_filename, localWorkspace);

  // load the mapping file
  string mapping_filename = this->getPropertyValue(MAP_PARAM);
  if (mapping_filename.empty()) {
    mapping_filename = generateMappingfileName(localWorkspace);
    if (!mapping_filename.empty())
      this->g_log.information() << "Found mapping file \"" << mapping_filename << "\"" << std::endl;
  }
  this->loadPixelMap(mapping_filename);

  //Process the events into pixels
  this->procEvents(localWorkspace);

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

  double shortest_tof = static_cast<double>(MAX_TOF_UINT32) * TOF_CONVERSION;
  double longest_tof = 0.;

  //Initialize progress reporting.
  size_t event_buffer_size = Mantid::Kernel::DEFAULT_BLOCK_SIZE;
  Progress prog(this,0.0,1.0, this->num_events/event_buffer_size);

  //Allocate the buffer
  DasEvent * event_buffer = new DasEvent[event_buffer_size];

  //For slight speed up
  bool loadOnlySomeSpectra = (this->spectra_list.size() > 0);
  //Turn the spectra list into a map, for speed
  std::map<int, bool> spectraLoadMap;
  for (std::vector<int>::iterator it = spectra_list.begin(); it != spectra_list.end(); it++)
    spectraLoadMap[*it] = true;

  while (eventfile->getOffset() < this->num_events)
  {
    //Load a block into the buffer directly
    size_t current_event_buffer_size = eventfile->loadBlock(event_buffer, event_buffer_size);

    DasEvent temp;
    uint32_t period;
    size_t frame_index(0);

    size_t event_offset = eventfile->getOffset();

    // process the individual events
    for (size_t i = 0; i < current_event_buffer_size; i++) {
      temp = *(event_buffer + i);

      if ((temp.pid & ERROR_PID) == ERROR_PID) // marked as bad
      {
        this->num_error_events++;
        continue;
      }

      //Covert the pixel ID from DAS pixel to our pixel ID
      this->fixPixelId(temp.pid, period);

      //Now check if this pid we want to load.
      if (loadOnlySomeSpectra)
      {
        std::map<int, bool>::iterator it;
        it = spectraLoadMap.find(temp.pid);
        if (it == spectraLoadMap.end())
        {
          //Pixel ID was not found, so the event is being ignored.
          this->num_ignored_events++;
          continue;
        }
      }

      // work with the good guys
      frame_index = this->getFrameIndex(event_offset + i, frame_index);
      double tof = static_cast<double>(temp.tof) * TOF_CONVERSION;
      TofEvent event;
      event = TofEvent(tof, frame_index);


      //Find the overall max/min tof
      if (tof < shortest_tof)
        shortest_tof = tof;
      if (tof > longest_tof)
        longest_tof = tof;

      //The addEventQuickly method does not clear the cache, making things slightly faster.
      workspace->getEventListAtPixelID(temp.pid).addEventQuickly(event);

      // TODO work with period
      this->num_good_events++;
    }


    //Report progress
    prog.report();
  }

  delete [] event_buffer;


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

  // add the frame information to the event workspace
  for (size_t i = 0; i < this->num_pulses; i++)
    workspace->addTime(i, this->pulsetimes[i]);

  this->setProtonCharge(workspace);

  //finalize loading; this condenses the pixels into a 0-based, dense vector.
  workspace->doneLoadingData(1);

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
/** Parallel-version of the procedure to process the event file properly.
 * @param workspace EventWorkspace to write to.
 */
void LoadEventPreNeXus::procEventsParallel(DataObjects::EventWorkspace_sptr & workspace)
{
//  std::cout << "--- procEventsParallel starting ----\n";
//
//  // do the actual loading
//  this->num_error_events = 0;
//  this->num_good_events = 0;
//
//  size_t event_offset = 0;
//  size_t event_buffer_size = getBufferSize(this->num_events);
//
//  double shortest_tof = static_cast<double>(MAX_TOF_UINT32) * TOF_CONVERSION;
//  double longest_tof = 0.;
//
//  //Initialize progress reporting.
//  Progress prog(this,0.0,1.0, this->num_events/event_buffer_size);
//
//  //Try to parallelize the code - commented out because it actually makes the code slower!
//  size_t frame_index = 0;
//  for (event_offset = 0; event_offset  < this->num_events; event_offset += event_buffer_size)
//  {
//    //Variables are defined
//    DasEvent * event_buffer = new DasEvent[event_buffer_size];
//    DasEvent temp;
//    size_t current_event_buffer_size;
//    //Local incremented variables
//    size_t my_errors = 0;
//    size_t my_events = 0;
//
//    // adjust the buffer size
//    current_event_buffer_size = event_buffer_size;
//    if (event_offset + current_event_buffer_size > this->num_events)
//      current_event_buffer_size = this->num_events - event_offset;
//
//    // read in the data
//    std::cout << "--- Loading from File ----\n";
////    this->eventfile->read(reinterpret_cast<char *>(event_buffer),
////        current_event_buffer_size * sizeof(DasEvent));
//
//    //This is a map, with the DAS pixel ID as the index
//    typedef std::map<int, std::vector<DasTofType> > PixelMapType;
//    PixelMapType das_pixel_map;
//
//    std::cout << "--- Process (serially) the entire list of DAS events ----\n";
//    //--- Process (serially) the entire list of DAS events ----
//    for (size_t i=0; i < current_event_buffer_size; i++)
//    {
//      temp = *(event_buffer + i);
//      //Add this time of flight to this das pixelid entry.
//      das_pixel_map[temp.pid].push_back(temp.tof);
//    }
//
//    //# of unique DAS pixel IDs
//    PixelType num_unique_pixels = das_pixel_map.size();
//    std::cout << "# of unique pixels = " << num_unique_pixels << "\n";
//
//    //Will split in blocks of this size.
//    size_t num_cpus = 4;
//    //Size of block is 1/num_cpus, rounded up
//    PixelType block_size = (num_unique_pixels + (num_cpus-1)) / num_cpus;
//
//    std::cout << "--- Find out where the iterators need to be ----\n";
//    //--- Find out where the iterators need to be ----
//    PixelMapType::iterator it_counter;
//    PixelMapType::iterator * start_map_it = new PixelMapType::iterator[num_cpus+1];
//    start_map_it[0] = das_pixel_map.begin();
//    size_t counter = 0;
//    size_t block_counter = 0;
//    for (it_counter = das_pixel_map.begin(); it_counter != das_pixel_map.end(); it_counter++)
//    {
//      if ((counter % block_size) == 0)
//      {
//        //This is where you need to start iterating
//        start_map_it[block_counter] = it_counter;
//        //No need to iterate to the end
//        if (block_counter == num_cpus - 1)
//          break;
//        block_counter++;
//      }
//      counter++;
//    }
//    //The ending iterator
//    start_map_it[num_cpus] = das_pixel_map.end();
//
//    PARALLEL_FOR1(workspace)
//    for (int block_num=0; block_num < num_cpus; block_num++)
//    {
//      std::cout << "Starting iterating through block " << block_num << "\n";
//      //Make an iterator into the map
//      PixelMapType::iterator it;
//
//      //Iterate through this chunk of the map
//      for (it = start_map_it[block_num]; it != start_map_it[block_num+1]; it++)
//      {
//        //Get the pixel id of it
//        PixelType pid = it->first;
//        //std::cout << "pixelid is " << pid << "\n";
//        //This is the vector of tof values
//        std::vector<DasTofType> tof_vector = das_pixel_map[pid];
//        std::vector<DasTofType>::iterator it2;
//
//        for (it2 = tof_vector.begin(); it2 != tof_vector.end(); it2++)
//        {
//          //The time of flight
//          DasTofType tof = *it2;
//          //Make a TofEvent here
//          TofEvent event = TofEvent(tof, frame_index);
//          //TODO: Fix the PID here
//          //Add it to the list
//          workspace->getEventListAtPixelID(pid).addEventQuickly(event);
//        }
//      }
//    }
//
//    delete [] start_map_it;
//
////
////    // process the individual events
////    for (size_t i = 0; i < current_event_buffer_size; i++) {
////      temp = *(event_buffer + i);
////
////      if ((temp.pid & ERROR_PID) == ERROR_PID) // marked as bad
////      {
////        my_errors++;
////        continue;
////      }
////
////      // work with the good guys
////      frame_index = this->getFrameIndex(event_offset + i, frame_index);
////      double tof = static_cast<double>(temp.tof) * TOF_CONVERSION;
////      TofEvent event;
////      event = TofEvent(tof, frame_index);
////
////      //Find the overall max/min tof
////      if (tof < shortest_tof)
////        shortest_tof = tof;
////      if (tof > longest_tof)
////        longest_tof = tof;
////
////      //Covert the pixel ID from DAS pixel to our pixel ID
////      this->fixPixelId(temp.pid, period);
////
////      //Parallel: this critical block is almost certainly killing parallel efficiency.
////      //workspace->getEventListAtPixelID(temp.pid) += event;
////
////      //The addEventQuickly method does not clear the cache, making things slightly faster.
////      workspace->getEventListAtPixelID(temp.pid).addEventQuickly(event);
////
////            // TODO work with period
////            // TODO filter based on pixel ids
////      my_events++;
////    }
//
//    //Fold back the counters into the main one
//    //PARALLEL_CRITICAL(num_error_events)
//    {
//      this->num_error_events += my_errors;
//      this->num_good_events += my_events;
//    }
//
//    delete event_buffer;
//
//    //Report progress
//    prog.report();
//    //PARALLEL_END_INTERUPT_REGION
//  }
//  //PARALLEL_CHECK_INTERUPT_REGION
//
//  /*
//  //OK, you've done all the events; but if some pixels got no events, their
//  //  EventList wasn't initialized.
//  std::vector<PixelType>::iterator pix;
//  for (pix = this->pixelmap.begin(); pix < this->pixelmap.end(); pix++)
//  {
//    //Go through each pixel in the map
//    // and simply get the event list. It will be created if empty.
//    workspace->getEventListAtPixelID(*pix);
//  }
//   */
//
//  // add the frame information to the event workspace
//  for (size_t i = 0; i < this->num_pulses; i++)
//    workspace->addTime(i, this->pulsetimes[i]);
//
//  this->setProtonCharge(workspace);
//
//  std::cout << "About to finalize with doneLoadingData()\n";
//
//  //finalize loading; this condenses the pixels into a 0-based, dense vector.
//  workspace->doneLoadingData();
//
//  //std::cout << "Shortest tof " << shortest_tof << " longest was " << longest_tof << "\n";
//
//  //Now, create a default X-vector for histogramming, with just 2 bins.
//  Kernel::cow_ptr<MantidVec> axis;
//  MantidVec& xRef = axis.access();
//  xRef.resize(2);
//  xRef[0] = shortest_tof - 1; //Just to make sure the bins hold it all
//  xRef[1] = longest_tof + 1;
//  workspace->setAllX(axis);
//
//  stringstream msg;
//  msg << "Read " << this->num_good_events << " events + "
//      << this->num_error_events << " errors";
//  msg << ". Shortest tof: " << shortest_tof << " microsec; longest tof: " << longest_tof << " microsec.";
//  this->g_log.information(msg.str());

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
 * @param event_index The index of the event.
 * @param last_frame_index Last frame found. This parameter reduces the
 * search to be from the current point forward.
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
