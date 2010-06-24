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

namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadEventPreNeXus)

using namespace Kernel;
using namespace API;
using namespace Geometry;
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
static const string PID_MIN_PARAM("Spectrum Min");
static const string PID_MAX_PARAM("Spectrum Max");
static const string PID_PARAM("Spectrum List");
static const string PERIOD_PARAM("Period List");
static const string OUT_PARAM("OutputWorkspace");

/// Default number of items to read in from any of the files.
static const size_t DEFAULT_BLOCK_SIZE = 100000; // 100,000
static const PixelType ERROR_PID = 0x80000000;

/// Determine the size of the buffer for reading in binary files.
static size_t get_buffer_size(const size_t num_items);

LoadEventPreNeXus::LoadEventPreNeXus() : Mantid::API::Algorithm()
{
  this->eventfile = NULL;
  this->pulsefile = NULL;
}

LoadEventPreNeXus::~LoadEventPreNeXus()
{
  if (this->eventfile != NULL)
    delete this->eventfile;
  if (this->pulsefile != NULL)
    delete this->pulsefile;
}

//-----------------------------------------------------------------------------
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
  BoundedValidator<int> *mustBePositive = new BoundedValidator<int> ();
  mustBePositive->setLower(1);
  this->declareProperty(PID_MIN_PARAM, 1, mustBePositive,
      "The index number of the first spectrum to read. Only used if the spectrum max is set.");
  this->declareProperty(PID_MAX_PARAM, Mantid::EMPTY_INT(), mustBePositive->clone(),
      "The index number of the last spectrum to read. Used only if specifed");
  this->declareProperty(new ArrayProperty<int>(PID_PARAM),
      "A comma separated list of individual spectra to read. Only used if set.");

  // which states to load
  this->declareProperty(new ArrayProperty<int>(PERIOD_PARAM),
      "A comma separated list of periods to read. Only used if set.");

  // the output workspace name
  this->declareProperty(new WorkspaceProperty<EventWorkspace>(OUT_PARAM,"",Direction::Output));

}

//-----------------------------------------------------------------------------
void LoadEventPreNeXus::exec()
{
  // what to load
  int spectra_min = this->getProperty(PID_MIN_PARAM);
  int spectra_max = this->getProperty(PID_MAX_PARAM);
  if (spectra_max != Mantid::EMPTY_INT())
  {
    // TODO something useful here
  }
  else
  {
    this->spectra_list = this->getProperty(PID_PARAM);
  }
  this->period_list = this->getProperty(PERIOD_PARAM);

  // load the mapping file
  string mapping_filename = this->getPropertyValue(MAP_PARAM);
  this->load_pixel_map(mapping_filename);

  // open the pulseid file
  string pulseid_filename = this->getPropertyValue(PULSEID_PARAM);
  this->open_pulseid_file(pulseid_filename);
  size_t pulse_buffer_size = get_buffer_size(this->num_pulses);

  // open the event file
  string event_filename = this->getPropertyValue(EVENT_PARAM);
  this->open_event_file(event_filename);

  // prep the output workspace
  EventWorkspace_sptr localWorkspace = EventWorkspace_sptr(new EventWorkspace());
  //Make sure to initialize.
  //   We can use dummy numbers for arguments, for event workspace it doesn't matter
  localWorkspace->initialize(1,1,1);
  
  //Process the events into pixels
  this->proc_events(localWorkspace);
  
  //Save output
  this->setProperty(OUT_PARAM, localWorkspace);
}

//-----------------------------------------------------------------------------
void LoadEventPreNeXus::fix_pixel_id(PixelType &pixel, uint32_t &period) const
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
void LoadEventPreNeXus::proc_events(EventWorkspace_sptr & workspace)
{
  // do the actual loading
  this->num_error_events = 0;
  this->num_good_events = 0;

  size_t event_offset = 0;
  size_t event_buffer_size = get_buffer_size(this->num_events);
  DasEvent * event_buffer = new DasEvent[event_buffer_size];

  DasEvent temp;
  TofEvent event;
  uint32_t period;
  while (event_offset  < this->num_events)
  {
    if (event_offset >= this->num_events) {
      break;
    }

    // adjust the buffer size
    if (event_offset + event_buffer_size > this->num_events)
      event_buffer_size = this->num_events - event_offset;
    
    // read in the data
    this->eventfile->read(reinterpret_cast<char *>(event_buffer),
			  event_buffer_size * sizeof(DasEvent));

    // process the individual events
    for (size_t i = 0; i < event_buffer_size; i++) {
      temp = *(event_buffer + i);

      if ((temp.pid & ERROR_PID) == ERROR_PID) // marked as bad
      {
        this->num_error_events += 1;
        continue;
      }

      // work with the good guys
      event = TofEvent(static_cast<double>(temp.tof)/.1, 0); // convert to microsecond

      //TODO: Fix this function; doesn't work because numpixel is not set
      //this->fix_pixel_id(temp.pid, period);

      workspace->getEventList(temp.pid) += event; // TODO work with period
                                                  // TODO filter based on pixel ids
      this->num_good_events += 1;
    }

    // adjust the record of the location in the file
    event_offset += event_buffer_size;
  }

  //OK, you've done all the events; but if some pixels got no events, their
  //  EventList wasn't initialized.
  std::vector<PixelType>::iterator pix;
  for (pix = this->pixelmap.begin(); pix < this->pixelmap.end(); pix++)
  {
    //Go through each pixel in the map
    // and simply get the event list. It will be created if empty.
    workspace->getEventList(*pix);
  }

  //finalize loading; this condenses the pixels into a 0-based, dense vector.
  workspace->doneLoadingData();

  delete event_buffer;
  stringstream msg;
  msg << "Read " << this->num_good_events << " events + "
      << this->num_error_events << " errors";
  this->g_log.information(msg.str());
}

//-----------------------------------------------------------------------------
template<typename T>
static size_t get_file_size(ifstream * handle)
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
static size_t get_buffer_size(const size_t num_items)
{
  if (num_items < DEFAULT_BLOCK_SIZE)
    return num_items;
  else 
    return DEFAULT_BLOCK_SIZE;
}

//-----------------------------------------------------------------------------
void LoadEventPreNeXus::load_pixel_map(const string &filename)
{
  this->pixelmap.clear();

  // check that there is a mapping file
  if (filename.empty()) {
    this->g_log.information("NOT using a mapping file");
    return;
  }

  // actually deal with the file
  this->g_log.information("Using mapping file \"" + filename + "\""); // TODO change to debug
  ifstream * handle = new ifstream(filename.c_str(), std::ios::binary);

  size_t file_size = get_file_size<PixelType>(handle);
  size_t offset = 0;
  size_t buffer_size = get_buffer_size(file_size);
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

  // cleanup
  delete buffer;
  handle->close();
  delete handle;
}

//-----------------------------------------------------------------------------
void LoadEventPreNeXus::open_event_file(const string &filename)
{
  this->eventfile = new ifstream(filename.c_str(), std::ios::binary);
  this->num_events = get_file_size<DasEvent>(this->eventfile);
  stringstream msg;
  msg << "Reading " <<  this->num_events << " event records";
  this->g_log.information(msg.str());
}

//-----------------------------------------------------------------------------
void LoadEventPreNeXus::open_pulseid_file(const string &filename)
{
  if (filename.empty()) {
    this->g_log.information("NOT using a pulseid file");
    this->pulsefile = NULL;
    this->num_pulses = 0;
    return;
  }

  // TODO something
}


} // namespace DataHandling
} // namespace Mantid

