#include <sstream>
#include <stdexcept>
#include <iostream>
#include <vector>
#include "MantidDataHandling/LoadEventPreNeXus.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileProperty.h"
#include "MantidKernel/System.h"
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
static const string PERIOD_PARAM("PeriodList");
static const string OUT_PARAM("OutputWorkspace");

/// Default number of items to read in from any of the files.
static const size_t DEFAULT_BLOCK_SIZE = 100000; // 100,000
/// All pixel ids with matching this mask are errors.
static const PixelType ERROR_PID = 0x80000000;
/// The maximum possible tof as native type
static const uint32_t MAX_TOF_UINT32 = std::numeric_limits<uint32_t>::max();

/// The difference in seconds between standard unix and gps epochs.
static const uint32_t EPOCH_DIFF = 631152000;
/// The EPOCH for GPS times.
static const ptime EPOCH(boost::gregorian::from_simple_string("1990-1-1"));
/// The number of nanoseconds in a second.
static const uint32_t NANO_TO_SEC = 1000000000;

/// Determine the size of the buffer for reading in binary files.
static size_t getBufferSize(const size_t num_items);

LoadEventPreNeXus::LoadEventPreNeXus() : Mantid::API::Algorithm()
{
  this->eventfile = NULL;
}

LoadEventPreNeXus::~LoadEventPreNeXus()
{
  if (this->eventfile != NULL)
    delete this->eventfile;
}

//-----------------------------------------------------------------------------
/** Initialize the algorithm */
void LoadEventPreNeXus::init()
{
  // reset the logger's name
  this->g_log.setName("LoadEventPreNeXus");

  // which files to use
  this->declareProperty(new FileProperty(EVENT_PARAM, "", FileProperty::Load, "event.dat"),
                        "A preNeXus neutron event file");
  this->declareProperty(new FileProperty(PULSEID_PARAM, "", FileProperty::OptionalLoad, "pulseid.dat"),
                        "A preNeXus pulseid file. Used only if specified.");
  this->declareProperty(new FileProperty(MAP_PARAM, "", FileProperty::OptionalLoad, ".dat"),
                        "TS mapping file converting detector id to pixel id. Used only if specified.");

  // which pixels to load
  this->declareProperty(new ArrayProperty<int>(PID_PARAM),
                        "A list of individual spectra to read. Only used if set.");
  // which states to load
  this->declareProperty(new ArrayProperty<int>(PERIOD_PARAM),
                        "A list of periods to read. Only used if set.");

  // the output workspace name
  this->declareProperty(new WorkspaceProperty<EventWorkspace>(OUT_PARAM,"",Direction::Output));

}

//-----------------------------------------------------------------------------
/** Execute the algorithm */
void LoadEventPreNeXus::exec()
{
  // what to load
  this->spectra_list = this->getProperty(PID_PARAM);
  this->period_list = this->getProperty(PERIOD_PARAM);

  // load the mapping file
  string mapping_filename = this->getPropertyValue(MAP_PARAM);
  this->loadPixelMap(mapping_filename);

  // open the pulseid file
  string pulseid_filename = this->getPropertyValue(PULSEID_PARAM);
  this->readPulseidFile(pulseid_filename);

  // open the event file
  string event_filename = this->getPropertyValue(EVENT_PARAM);
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

  //Process the events into pixels
  this->procEvents(localWorkspace);
  
  //Save output
  this->setProperty(OUT_PARAM, localWorkspace);
}

//-----------------------------------------------------------------------------
/** Turn a pixel id into a "corrected" pixelid and period.
 *
 */
void LoadEventPreNeXus::fixPixelId(PixelType &pixel, uint32_t &period) const
{
  if (this->pixelmap.empty()) { // nothing to do here
    period = 0;
    return;
  }

  PixelType unmapped_pid = pixel % this->numpixel;
  period = (pixel - unmapped_pid) / this->numpixel;
  pixel = this->pixelmap[unmapped_pid];
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

  size_t event_offset = 0;
  size_t event_buffer_size = getBufferSize(this->num_events);

  double shortest_tof = static_cast<double>(MAX_TOF_UINT32) * .1;
  double longest_tof = 0.;

  //uint32_t period;
  //Initialize progress reporting.
  Progress prog(this,0.0,1.0, this->num_events/event_buffer_size);

  //Try to parallelize the code - commented out because it actually makes the code slower!
  //PARALLEL_FOR1(workspace)
  size_t frame_index = 0;
  for (event_offset = 0; event_offset  < this->num_events; event_offset += event_buffer_size)
  {
    //PARALLEL_START_INTERUPT_REGION
    //Variables are defined
    DasEvent * event_buffer = new DasEvent[event_buffer_size];
    DasEvent temp;
    TofEvent event;
    uint32_t period;
    size_t current_event_buffer_size;
    //Local incremented variables
    size_t my_errors = 0;
    size_t my_events = 0;

    // adjust the buffer size
    current_event_buffer_size = event_buffer_size;
    if (event_offset + current_event_buffer_size > this->num_events)
      current_event_buffer_size = this->num_events - event_offset;
    
    // read in the data
    //PARALLEL_CRITICAL(eventfile_read)
    this->eventfile->read(reinterpret_cast<char *>(event_buffer),
        current_event_buffer_size * sizeof(DasEvent));

    // process the individual events
    for (size_t i = 0; i < current_event_buffer_size; i++) {
      temp = *(event_buffer + i);

      if ((temp.pid & ERROR_PID) == ERROR_PID) // marked as bad
      {
        my_errors++;
        continue;
      }

      // work with the good guys
      frame_index = this->getFrameIndex(event_offset + i, frame_index);
      double tof = static_cast<double>(temp.tof) * 0.1; // convert units of 100 ns to microsecond
      event = TofEvent(tof, frame_index);

      //Find the overall max/min tof
      if (tof < shortest_tof)
        shortest_tof = tof;
      if (tof > longest_tof)
        longest_tof = tof;

      //Covert the pixel ID from DAS pixel to our pixel ID
      this->fixPixelId(temp.pid, period);

      //Parallel: this critical block is almost certainly killing parallel efficiency.
      //workspace->getEventList(temp.pid) += event;

      //The addEventQuickly method does not clear the cache, making things slightly faster.
      workspace->getEventList(temp.pid).addEventQuickly(event);

            // TODO work with period
            // TODO filter based on pixel ids
      my_events++;
    }

    //Fold back the counters into the main one
    //PARALLEL_CRITICAL(num_error_events)
    {
      this->num_error_events += my_errors;
      this->num_good_events += my_events;
    }

    delete event_buffer;

    //Report progress
    prog.report();
    //PARALLEL_END_INTERUPT_REGION
  }
  //PARALLEL_CHECK_INTERUPT_REGION

  /*
  //OK, you've done all the events; but if some pixels got no events, their
  //  EventList wasn't initialized.
  std::vector<PixelType>::iterator pix;
  for (pix = this->pixelmap.begin(); pix < this->pixelmap.end(); pix++)
  {
    //Go through each pixel in the map
    // and simply get the event list. It will be created if empty.
    workspace->getEventList(*pix);
  }
   */

  // add the frame information to the event workspace
  for (size_t i = 0; i < this->num_pulses; i++)
    workspace->addTime(i, this->pulsetimes[i]);

  //finalize loading; this condenses the pixels into a 0-based, dense vector.
  workspace->doneLoadingData();

  //std::cout << "Shortest tof " << shortest_tof << " longest was " << longest_tof << "\n";

  //Now, create a default X-vector for histogramming, with just 2 bins.
  Kernel::cow_ptr<MantidVec> axis;
  MantidVec& xRef = axis.access();
  xRef.resize(2);
  xRef[0] = shortest_tof - 1; //Just to make sure the bins hold it all
  xRef[1] = longest_tof + 1;
  workspace->setAllX(axis);


  stringstream msg;
  msg << "Read " << this->num_good_events << " events + "
      << this->num_error_events << " errors";
  msg << ". Shortest tof: " << shortest_tof << " microsec; longest tof: " << longest_tof << " microsec.";
  this->g_log.information(msg.str());

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
  for ( ; next_frame_index < this->num_pulses; next_frame_index++)
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
/** Get the size of a file as a multiple of a particular data type
 * @tparam T type to load in the file
 * @param handle Handle to the file stream
 * */
template<typename T>
static size_t getFileSize(ifstream * handle)
{
  if (!handle) {
    throw runtime_error("Cannot find the size of a file from a null handle");
  }

  // get the size of the file in bytes and reset the handle back to the beginning
  handle->seekg(0, std::ios::end);
  size_t filesize = handle->tellg();
  handle->seekg(0, std::ios::beg);

  // check the file is a compatible size
  if (filesize % sizeof(T) != 0) {
    stringstream msg;
    msg << "File size is not compatible with data size ";
    msg << filesize << "%" << sizeof(T) << "=";
    msg << filesize % sizeof(T);
    throw runtime_error(msg.str());
  }

  return filesize / sizeof(T);
}


//-----------------------------------------------------------------------------
/** Get a buffer size for loading blocks of data.
 * @param num_items
 */
static size_t getBufferSize(const size_t num_items)
{
  if (num_items < DEFAULT_BLOCK_SIZE)
    return num_items;
  else 
    return DEFAULT_BLOCK_SIZE;
}

//-----------------------------------------------------------------------------
/** Load a pixel mapping file
 * @param filename Path to file.
 */
void LoadEventPreNeXus::loadPixelMap(const std::string &filename)
{
  this->pixelmap.clear();

  // check that there is a mapping file
  if (filename.empty()) {
    this->g_log.information("NOT using a mapping file");
    return;
  }

  // actually deal with the file
  this->g_log.debug("Using mapping file \"" + filename + "\"");
  ifstream * handle = new ifstream(filename.c_str(), std::ios::binary);

  size_t file_size = getFileSize<PixelType>(handle);
  //std::cout << "file is " << file_size << std::endl;
  size_t offset = 0;
  size_t buffer_size = getBufferSize(file_size);
  PixelType * buffer = new PixelType[buffer_size];

  size_t obj_size = sizeof(PixelType);
  while (offset < file_size) {
    // read a section and put it into the object
    handle->read(reinterpret_cast<char *>(buffer), buffer_size * obj_size);
    this->pixelmap.insert(this->pixelmap.end(), buffer, (buffer + buffer_size));
    offset += buffer_size;

    // make sure not to read past EOF
    if (offset + buffer_size > file_size)
    {
      buffer_size = file_size - offset;
    }
  }

  //Let's assume that the # of pixels in the instrument matches the mapping file length.
  this->numpixel = file_size;

  // cleanup
  delete buffer;
  handle->close();
  delete handle;
}

//-----------------------------------------------------------------------------
/** Open an event file
 * @param filename file to open.
 */
void LoadEventPreNeXus::openEventFile(const std::string &filename)
{
  this->eventfile = new ifstream(filename.c_str(), std::ios::binary);
  this->num_events = getFileSize<DasEvent>(this->eventfile);
  stringstream msg;
  msg << "Reading " <<  this->num_events << " event records";
  this->g_log.information(msg.str());
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
  return EPOCH + boost::posix_time::seconds(sec) + boost::posix_time::nanoseconds(nano);
#else
  return EPOCH + boost::posix_time::seconds(sec);
#endif
}

//-----------------------------------------------------------------------------
/** Read a pulse ID file
 * @param filename file to load.
 */
void LoadEventPreNeXus::readPulseidFile(const std::string &filename)
{
  // jump out early if there isn't a filename
  if (filename.empty()) {
    this->g_log.information("NOT using a pulseid file");
    this->num_pulses = 0;
    return;
  }

  // set up for reading
  this->g_log.debug("Using pulseid file \"" + filename + "\"");
  ifstream * handle = new ifstream(filename.c_str(), std::ios::binary);
  this->num_pulses = getFileSize<Pulse>(handle);
  size_t buffer_size = getBufferSize(this->num_pulses);
  size_t offset = 0;
  Pulse* buffer = new Pulse[buffer_size];
  size_t obj_size = sizeof(Pulse);

  // go through the file
  while (offset < this->num_pulses)
  {
    handle->read(reinterpret_cast<char *>(buffer), buffer_size * obj_size);
    for (size_t i = 0; i < buffer_size; i++)
    {
      this->pulsetimes.push_back(getTime(buffer[i].seconds, buffer[i].nanoseconds));
      this->event_indices.push_back(buffer[i].event_index);
    }
    offset += buffer_size;

    // make sure not to read past EOF
    if (offset + buffer_size > this->num_pulses)
      buffer_size = this->num_pulses - offset;
  }

  delete buffer;
  handle->close();
  delete handle;
}


} // namespace DataHandling
} // namespace Mantid

