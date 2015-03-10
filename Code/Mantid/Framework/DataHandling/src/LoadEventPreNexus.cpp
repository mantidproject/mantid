#include "MantidDataHandling/LoadEventPreNexus.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileValidator.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Glob.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/BinaryFile.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/MemoryManager.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <functional>
#include <iostream>
#include <set>
#include <vector>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <boost/timer.hpp>

namespace Mantid {
namespace DataHandling {

DECLARE_FILELOADER_ALGORITHM(LoadEventPreNexus);

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
static const string PARALLEL_PARAM("UseParallelProcessing");
static const string BLOCK_SIZE_PARAM("LoadingBlockSize");
static const string OUT_PARAM("OutputWorkspace");

static const string PULSE_EXT("pulseid.dat");
static const string EVENT_EXT("event.dat");

/// All pixel ids with matching this mask are errors.
static const PixelType ERROR_PID = 0x80000000;
/// The maximum possible tof as native type
static const uint32_t MAX_TOF_UINT32 = std::numeric_limits<uint32_t>::max();

/// Conversion factor between 100 nanoseconds and 1 microsecond.
static const double TOF_CONVERSION = .1;
/// Conversion factor between picoColumbs and microAmp*hours
static const double CURRENT_CONVERSION = 1.e-6 / 3600.;

LoadEventPreNexus::LoadEventPreNexus()
  : Mantid::API::IFileLoader<Kernel::FileDescriptor>(),
  prog(NULL), spectra_list(), pulsetimes(), event_indices(), proton_charge(),
  proton_charge_tot(0), pixel_to_wkspindex(), pixelmap(), detid_max(),
  eventfile(NULL), num_events(0), num_pulses(0), numpixel(0),
  num_good_events(0), num_error_events(0), num_ignored_events(0),
  first_event(0), max_events(0), using_mapping_file(false),
  loadOnlySomeSpectra(false), spectraLoadMap(), longest_tof(0),
  shortest_tof(0), parallelProcessing(false) {
}

LoadEventPreNexus::~LoadEventPreNexus() { delete this->eventfile; }

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadEventPreNexus::confidence(Kernel::FileDescriptor &descriptor) const {
  if (descriptor.extension().rfind("dat") == std::string::npos)
    return 0;

  // If this looks like a binary file where the exact file length is a multiple
  // of the DasEvent struct then we're probably okay.
  if (descriptor.isAscii())
    return 0;

  const size_t objSize = sizeof(DasEvent);
  auto &handle = descriptor.data();
  // get the size of the file in bytes and reset the handle back to the
  // beginning
  handle.seekg(0, std::ios::end);
  const size_t filesize = static_cast<size_t>(handle.tellg());
  handle.seekg(0, std::ios::beg);

  if (filesize % objSize == 0)
    return 60;
  else
    return 0;
}

//-----------------------------------------------------------------------------
/** Initialize the algorithm */
void LoadEventPreNexus::init() {
  // which files to use
  declareProperty(
      new FileProperty(EVENT_PARAM, "", FileProperty::Load, EVENT_EXT),
      "The name of the neutron event file to read, including its full or "
      "relative path. The file typically ends in neutron_event.dat (N.B. case "
      "sensitive if running on Linux).");
  declareProperty(new FileProperty(PULSEID_PARAM, "",
                                   FileProperty::OptionalLoad, PULSE_EXT),
                  "File containing the accelerator pulse information; the "
                  "filename will be found automatically if not specified.");
  declareProperty(
      new FileProperty(MAP_PARAM, "", FileProperty::OptionalLoad, ".dat"),
      "File containing the pixel mapping (DAS pixels to pixel IDs) file "
      "(typically INSTRUMENT_TS_YYYY_MM_DD.dat). The filename will be found "
      "automatically if not specified.");

  // which pixels to load
  declareProperty(new ArrayProperty<int64_t>(PID_PARAM),
                  "A list of individual spectra (pixel IDs) to read, specified "
                  "as e.g. 10:20. Only used if set.");

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
  setPropertySettings("TotalChunks",
                      new VisibleWhenProperty("ChunkNumber", IS_NOT_DEFAULT));

  std::vector<std::string> propOptions;
  propOptions.push_back("Auto");
  propOptions.push_back("Serial");
  propOptions.push_back("Parallel");
  declareProperty("UseParallelProcessing", "Auto",
                  boost::make_shared<StringListValidator>(propOptions),
                  "Use multiple cores for loading the data?\n"
                  "  Auto: Use serial loading for small data sets, parallel "
                  "for large data sets.\n"
                  "  Serial: Use a single core.\n"
                  "  Parallel: Use all available cores.");

  // the output workspace name
  declareProperty(
      new WorkspaceProperty<IEventWorkspace>(OUT_PARAM, "", Direction::Output),
      "The name of the workspace that will be created, filled with the read-in "
      "data and stored in the [[Analysis Data Service]].");
}

//-----------------------------------------------------------------------------
static string generatePulseidName(string eventfile) {
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
static string generateMappingfileName(EventWorkspace_sptr &wksp) { //
  // get the name of the mapping file as set in the parameter files
  std::vector<string> temp =
      wksp->getInstrument()->getStringParameter("TS_mapping_file");
  if (temp.empty())
    return "";
  string mapping = temp[0];
  // Try to get it from the working directory
  Poco::File localmap(mapping);
  if (localmap.exists())
    return mapping;

  // Try to get it from the data directories
  string dataversion = Mantid::API::FileFinder::Instance().getFullPath(mapping);
  if (!dataversion.empty())
    return dataversion;

  // get a list of all proposal directories
  string instrument = wksp->getInstrument()->getName();
  Poco::File base("/SNS/" + instrument + "/");
  // try short instrument name
  if (!base.exists()) {
    instrument =
        Kernel::ConfigService::Instance().getInstrument(instrument).shortName();
    base = Poco::File("/SNS/" + instrument + "/");
    if (!base.exists())
      return "";
  }
  vector<string> dirs; // poco won't let me reuse temp
  base.list(dirs);

  // check all of the proposals for the mapping file in the canonical place
  const string CAL("_CAL");
  const size_t CAL_LEN = CAL.length(); // cache to make life easier
  vector<string> files;
  for (size_t i = 0; i < dirs.size(); ++i) {
    if ((dirs[i].length() > CAL_LEN) &&
        (dirs[i].compare(dirs[i].length() - CAL.length(), CAL.length(), CAL) ==
         0)) {
      if (Poco::File(base.path() + "/" + dirs[i] + "/calibrations/" + mapping)
              .exists())
        files.push_back(base.path() + "/" + dirs[i] + "/calibrations/" +
                        mapping);
    }
  }

  if (files.empty())
    return "";
  else if (files.size() == 1)
    return files[0];
  else // just assume that the last one is the right one, this should never be
       // fired
    return *(files.rbegin());
}

namespace { // anonymous namespace
string getRunnumber(const string &filename) {
  // start by trimming the filename
  string runnumber(Poco::Path(filename).getBaseName());

  if (runnumber.find("neutron") >= string::npos)
    return "0";

  std::size_t left = runnumber.find("_");
  std::size_t right = runnumber.find("_", left + 1);

  return runnumber.substr(left + 1, right - left - 1);
}
}

//-----------------------------------------------------------------------------
/** Execute the algorithm */
void LoadEventPreNexus::exec() {
  // Check 'chunk' properties are valid, if set
  const int chunks = getProperty("TotalChunks");
  if (!isEmpty(chunks) && int(getProperty("ChunkNumber")) > chunks) {
    throw std::out_of_range("ChunkNumber cannot be larger than TotalChunks");
  }

  prog = new Progress(this, 0.0, 1.0, 100);

  // what spectra (pixel ID's) to load
  this->spectra_list = this->getProperty(PID_PARAM);

  // the event file is needed in case the pulseid fileanme is empty
  string event_filename = this->getPropertyValue(EVENT_PARAM);
  string pulseid_filename = this->getPropertyValue(PULSEID_PARAM);
  bool throwError = true;
  if (pulseid_filename.empty()) {
    pulseid_filename = generatePulseidName(event_filename);
    if (!pulseid_filename.empty()) {
      if (Poco::File(pulseid_filename).exists()) {
        this->g_log.information() << "Found pulseid file " << pulseid_filename
                                  << std::endl;
        throwError = false;
      } else {
        pulseid_filename = "";
      }
    }
  }

  prog->report("Loading Pulse ID file");
  this->readPulseidFile(pulseid_filename, throwError);

  this->openEventFile(event_filename);

  prog->report("Creating output workspace");
  // prep the output workspace
  EventWorkspace_sptr localWorkspace =
      EventWorkspace_sptr(new EventWorkspace());
  // Make sure to initialize.
  //   We can use dummy numbers for arguments, for event workspace it doesn't
  //   matter
  localWorkspace->initialize(1, 1, 1);

  // Set the units
  localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  localWorkspace->setYUnit("Counts");
  // TODO localWorkspace->setTitle(title);

  // Add the run_start property
  // Use the first pulse as the run_start time.
  if (this->num_pulses > 0) {
    // add the start of the run as a ISO8601 date/time string. The start = the
    // first pulse.
    // (this is used in LoadInstrument to find the right instrument file to
    // use).
    localWorkspace->mutableRun().addProperty(
        "run_start", pulsetimes[0].toISO8601String(), true);
  }

  // determine the run number and add it to the run object
  localWorkspace->mutableRun().addProperty("run_number",
                                           getRunnumber(event_filename));

  // Get the instrument!
  prog->report("Loading Instrument");
  this->runLoadInstrument(event_filename, localWorkspace);

  // load the mapping file
  prog->report("Loading Mapping File");
  string mapping_filename = this->getPropertyValue(MAP_PARAM);
  if (mapping_filename.empty()) {
    mapping_filename = generateMappingfileName(localWorkspace);
    if (!mapping_filename.empty())
      this->g_log.information() << "Found mapping file \"" << mapping_filename
                                << "\"" << std::endl;
  }
  this->loadPixelMap(mapping_filename);

  // Process the events into pixels
  this->procEvents(localWorkspace);

  // Save output
  this->setProperty<IEventWorkspace_sptr>(OUT_PARAM, localWorkspace);

  // Cleanup
  delete prog;
}

//-----------------------------------------------------------------------------
/** Load the instrument geometry File
 *  @param eventfilename :: Used to pick the instrument.
 *  @param localWorkspace :: MatrixWorkspace in which to put the instrument
 * geometry
 */
void LoadEventPreNexus::runLoadInstrument(const std::string &eventfilename,
                                          MatrixWorkspace_sptr localWorkspace) {
  // determine the instrument parameter file
  string instrument = Poco::Path(eventfilename).getFileName();
  size_t pos = instrument.rfind("_");   // get rid of 'event.dat'
  pos = instrument.rfind("_", pos - 1); // get rid of 'neutron'
  pos = instrument.rfind("_", pos - 1); // get rid of the run number
  instrument = instrument.substr(0, pos);

  // do the actual work
  IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  loadInst->setPropertyValue("InstrumentName", instrument);
  loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", localWorkspace);
  loadInst->setProperty("RewriteSpectraMap", false);
  loadInst->executeAsChildAlg();

  // Populate the instrument parameters in this workspace - this works around a
  // bug
  localWorkspace->populateInstrumentParameters();
}

//-----------------------------------------------------------------------------
/** Turn a pixel id into a "corrected" pixelid and period.
 *
 */
inline void LoadEventPreNexus::fixPixelId(PixelType &pixel,
                                          uint32_t &period) const {
  if (!this->using_mapping_file) { // nothing to do here
    period = 0;
    return;
  }

  PixelType unmapped_pid = pixel % this->numpixel;
  period = (pixel - unmapped_pid) / this->numpixel;
  pixel = this->pixelmap[unmapped_pid];
}

//-----------------------------------------------------------------------------
/** Process the event file properly.
 * @param workspace :: EventWorkspace to write to.
 */
void
LoadEventPreNexus::procEvents(DataObjects::EventWorkspace_sptr &workspace) {
  this->num_error_events = 0;
  this->num_good_events = 0;
  this->num_ignored_events = 0;

  // Default values in the case of no parallel
  size_t loadBlockSize = Mantid::Kernel::DEFAULT_BLOCK_SIZE * 2;

  shortest_tof = static_cast<double>(MAX_TOF_UINT32) * TOF_CONVERSION;
  longest_tof = 0.;

  // Initialize progress reporting.
  size_t numBlocks = (max_events + loadBlockSize - 1) / loadBlockSize;

  // We want to pad out empty pixels.
  detid2det_map detector_map;
  workspace->getInstrument()->getDetectors(detector_map);

  // -------------- Determine processing mode
  std::string procMode = getProperty("UseParallelProcessing");
  if (procMode == "Serial")
    parallelProcessing = false;
  else if (procMode == "Parallel")
    parallelProcessing = true;
  else {
    // Automatic determination. Loading serially (for me) is about 3 million
    // events per second,
    // (which is sped up by ~ x 3 with parallel processing, say 10 million per
    // second, e.g. 7 million events more per seconds).
    // compared to a setup time/merging time of about 10 seconds per million
    // detectors.
    double setUpTime = double(detector_map.size()) * 10e-6;
    parallelProcessing = ((double(max_events) / 7e6) > setUpTime);
    g_log.debug() << (parallelProcessing ? "Using" : "Not using")
                  << " parallel processing." << std::endl;
  }

  // determine maximum pixel id
  detid2det_map::iterator it;
  detid_max = 0; // seems like a safe lower bound
  for (it = detector_map.begin(); it != detector_map.end(); it++)
    if (it->first > detid_max)
      detid_max = it->first;

  // Pad all the pixels
  prog->report("Padding Pixels");
  this->pixel_to_wkspindex.reserve(
      detid_max + 1); // starting at zero up to and including detid_max
  // Set to zero
  this->pixel_to_wkspindex.assign(detid_max + 1, 0);
  size_t workspaceIndex = 0;
  for (it = detector_map.begin(); it != detector_map.end(); it++) {
    if (!it->second->isMonitor()) {
      this->pixel_to_wkspindex[it->first] = workspaceIndex;
      EventList &spec = workspace->getOrAddEventList(workspaceIndex);
      spec.addDetectorID(it->first);
      // Start the spectrum number at 1
      spec.setSpectrumNo(specid_t(workspaceIndex + 1));
      workspaceIndex += 1;
    }
  }

  // For slight speed up
  loadOnlySomeSpectra = (this->spectra_list.size() > 0);

  // Turn the spectra list into a map, for speed of access
  for (std::vector<int64_t>::iterator it = spectra_list.begin();
       it != spectra_list.end(); it++)
    spectraLoadMap[*it] = true;

  CPUTimer tim;

  // --------------- Create the partial workspaces
  // ------------------------------------------
  // Vector of partial workspaces, for parallel processing.
  std::vector<EventWorkspace_sptr> partWorkspaces;
  std::vector<DasEvent *> buffers;

  /// Pointer to the vector of events
  typedef std::vector<TofEvent> *EventVector_pt;
  /// Bare array of arrays of pointers to the EventVectors
  EventVector_pt **eventVectors;

  /// How many threads will we use?
  size_t numThreads = 1;
  if (parallelProcessing)
    numThreads = size_t(PARALLEL_GET_MAX_THREADS);

  partWorkspaces.resize(numThreads);
  buffers.resize(numThreads);
  eventVectors = new EventVector_pt *[numThreads];

  // cppcheck-suppress syntaxError
  PRAGMA_OMP( parallel for if (parallelProcessing) )
  for (int i = 0; i < int(numThreads); i++) {
    // This is the partial workspace we are about to create (if in parallel)
    EventWorkspace_sptr partWS;
    if (parallelProcessing) {
      prog->report("Creating Partial Workspace");
      // Create a partial workspace
      partWS = EventWorkspace_sptr(new EventWorkspace());
      // Make sure to initialize.
      partWS->initialize(1, 1, 1);
      // Copy all the spectra numbers and stuff (no actual events to copy
      // though).
      partWS->copyDataFrom(*workspace);
      // Push it in the array
      partWorkspaces[i] = partWS;
    } else
      partWS = workspace;

    // Allocate the buffers
    buffers[i] = new DasEvent[loadBlockSize];

    // For each partial workspace, make an array where index = detector ID and
    // value = pointer to the events vector
    eventVectors[i] = new EventVector_pt[detid_max + 1];
    EventVector_pt *theseEventVectors = eventVectors[i];
    for (detid_t j = 0; j < detid_max + 1; j++) {
      size_t wi = pixel_to_wkspindex[j];
      // Save a POINTER to the vector<tofEvent>
      theseEventVectors[j] = &partWS->getEventList(wi).getEvents();
    }
  }

  g_log.debug() << tim << " to create " << partWorkspaces.size()
                << " workspaces for parallel loading." << std::endl;

  prog->resetNumSteps(numBlocks, 0.1, 0.8);

  // ---------------------------------- LOAD THE DATA --------------------------
  PRAGMA_OMP( parallel for schedule(dynamic, 1) if (parallelProcessing) )
  for (int blockNum = 0; blockNum < int(numBlocks); blockNum++) {
    PARALLEL_START_INTERUPT_REGION

    // Find the workspace for this particular thread
    EventWorkspace_sptr ws;
    size_t threadNum = 0;
    if (parallelProcessing) {
      threadNum = PARALLEL_THREAD_NUMBER;
      ws = partWorkspaces[threadNum];
    } else
      ws = workspace;

    // Get the buffer (for this thread)
    DasEvent *event_buffer = buffers[threadNum];

    // Get the speeding-up array of vector<tofEvent> where index = detid.
    EventVector_pt *theseEventVectors = eventVectors[threadNum];

    // Where to start in the file?
    size_t fileOffset = first_event + (loadBlockSize * blockNum);
    // May need to reduce size of last (or only) block
    size_t current_event_buffer_size =
        (blockNum == int(numBlocks - 1))
            ? (max_events - (numBlocks - 1) * loadBlockSize)
            : loadBlockSize;

    // Load this chunk of event data (critical block)
    PARALLEL_CRITICAL(LoadEventPreNexus_fileAccess) {
      current_event_buffer_size = eventfile->loadBlockAt(
          event_buffer, fileOffset, current_event_buffer_size);
    }

    // This processes the events. Can be done in parallel!
    procEventsLinear(ws, theseEventVectors, event_buffer,
                     current_event_buffer_size, fileOffset);

    // Report progress
    prog->report("Load Event PreNeXus");

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  g_log.debug() << tim << " to load the data." << std::endl;

  // ---------------------------------- MERGE WORKSPACES BACK TOGETHER
  // --------------------------
  if (parallelProcessing) {
    PARALLEL_START_INTERUPT_REGION
    prog->resetNumSteps(workspace->getNumberHistograms(), 0.8, 0.95);

    size_t memoryCleared = 0;
    MemoryManager::Instance().releaseFreeMemory();

    // Merge all workspaces, index by index.
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int iwi = 0; iwi < int(workspace->getNumberHistograms()); iwi++) {
      size_t wi = size_t(iwi);

      // The output event list.
      EventList &el = workspace->getEventList(wi);
      el.clear(false);

      // How many events will it have?
      size_t numEvents = 0;
      for (size_t i = 0; i < numThreads; i++)
        numEvents += partWorkspaces[i]->getEventList(wi).getNumberEvents();
      // This will avoid too much copying.
      el.reserve(numEvents);

      // Now merge the event lists
      for (size_t i = 0; i < numThreads; i++) {
        EventList &partEl = partWorkspaces[i]->getEventList(wi);
        el += partEl.getEvents();
        // Free up memory as you go along.
        partEl.clear(false);
      }

      // With TCMalloc, release memory when you accumulate enough to make sense
      PARALLEL_CRITICAL(LoadEventPreNexus_trackMemory) {
        memoryCleared += numEvents;
        if (memoryCleared > 10000000) // ten million events = about 160 MB
        {
          MemoryManager::Instance().releaseFreeMemory();
          memoryCleared = 0;
        }
      }
      prog->report("Merging Workspaces");
    }
    // Final memory release
    MemoryManager::Instance().releaseFreeMemory();
    g_log.debug() << tim << " to merge workspaces together." << std::endl;
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Delete the buffers for each thread.
  for (size_t i = 0; i < numThreads; i++) {
    delete[] buffers[i];
    delete[] eventVectors[i];
  }
  delete[] eventVectors;
  // delete [] pulsetimes;

  prog->resetNumSteps(3, 0.94, 1.00);

  // finalize loading
  prog->report("Deleting Empty Lists");
  if (loadOnlySomeSpectra)
    workspace->deleteEmptyLists();

  prog->report("Setting proton charge");
  this->setProtonCharge(workspace);
  g_log.debug() << tim << " to set the proton charge log." << std::endl;

  // Make sure the MRU is cleared
  workspace->clearMRU();

  // Now, create a default X-vector for histogramming, with just 2 bins.
  Kernel::cow_ptr<MantidVec> axis;
  MantidVec &xRef = axis.access();
  xRef.resize(2);
  xRef[0] = shortest_tof - 1; // Just to make sure the bins hold it all
  xRef[1] = longest_tof + 1;
  workspace->setAllX(axis);
  this->pixel_to_wkspindex.clear();

  g_log.information() << "Read " << this->num_good_events << " events + "
                      << this->num_error_events << " errors"
                      << ". Shortest TOF: " << shortest_tof
                      << " microsec; longest TOF: " << longest_tof
                      << " microsec." << std::endl;
}

//-----------------------------------------------------------------------------
/** Linear-version of the procedure to process the event file properly.
 * @param workspace :: EventWorkspace to write to.
 * @param arrayOfVectors :: For speed up: this is an array, of size detid_max+1,
 * where the
 *        index is a pixel ID, and the value is a pointer to the
 * vector<tofEvent> in the given EventList.
 * @param event_buffer :: The buffer containing the DAS events
 * @param current_event_buffer_size :: The length of the given DAS buffer
 * @param fileOffset :: Value for an offset into the binary file
 */
void LoadEventPreNexus::procEventsLinear(
    DataObjects::EventWorkspace_sptr & /*workspace*/,
    std::vector<TofEvent> **arrayOfVectors, DasEvent *event_buffer,
    size_t current_event_buffer_size, size_t fileOffset) {
  // Starting pulse time
  DateAndTime pulsetime;
  int64_t pulse_i = 0;
  int64_t numPulses = static_cast<int64_t>(num_pulses);
  if (event_indices.size() < num_pulses) {
    g_log.warning()
        << "Event_indices vector is smaller than the pulsetimes array.\n";
    numPulses = static_cast<int64_t>(event_indices.size());
  }

  size_t local_num_error_events = 0;
  size_t local_num_ignored_events = 0;
  size_t local_num_good_events = 0;
  double local_shortest_tof =
      static_cast<double>(MAX_TOF_UINT32) * TOF_CONVERSION;
  double local_longest_tof = 0.;

  // process the individual events
  for (size_t i = 0; i < current_event_buffer_size; i++) {
    DasEvent &temp = *(event_buffer + i);
    PixelType pid = temp.pid;

    if ((pid & ERROR_PID) == ERROR_PID) // marked as bad
    {
      local_num_error_events++;
      continue;
    }

    // Covert the pixel ID from DAS pixel to our pixel ID
    if (this->using_mapping_file) {
      PixelType unmapped_pid = pid % this->numpixel;
      pid = this->pixelmap[unmapped_pid];
    }

    // Avoid segfaults for wrong pixel IDs
    if (pid > static_cast<PixelType>(detid_max)) {
      local_num_error_events++;
      continue;
    }

    // Now check if this pid we want to load.
    if (loadOnlySomeSpectra) {
      std::map<int64_t, bool>::iterator it;
      it = spectraLoadMap.find(pid);
      if (it == spectraLoadMap.end()) {
        // Pixel ID was not found, so the event is being ignored.
        local_num_ignored_events++;
        continue;
      }
    }

    // work with the good guys

    // Find the pulse time for this event index
    if (pulse_i < numPulses - 1) {
      // This is the total offset into the file
      size_t total_i = i + fileOffset;
      // Go through event_index until you find where the index increases to
      // encompass the current index. Your pulse = the one before.
      while (!((total_i >= event_indices[pulse_i]) &&
               (total_i < event_indices[pulse_i + 1]))) {
        pulse_i++;
        if (pulse_i >= (numPulses - 1))
          break;
      }

      // if (pulsetimes[pulse_i] != pulsetime)    std::cout << pulse_i << " at "
      // << pulsetimes[pulse_i] << "\n";

      // Save the pulse time at this index for creating those events
      pulsetime = pulsetimes[pulse_i];
    }

    double tof = static_cast<double>(temp.tof) * TOF_CONVERSION;
    TofEvent event(tof, pulsetime);

    // Find the overall max/min tof
    if (tof < local_shortest_tof)
      local_shortest_tof = tof;
    if (tof > local_longest_tof)
      local_longest_tof = tof;

    // The addEventQuickly method does not clear the cache, making things
    // slightly faster.
    // workspace->getEventList(this->pixel_to_wkspindex[pid]).addEventQuickly(event);

    // This is equivalent to
    // workspace->getEventList(this->pixel_to_wkspindex[pid]).addEventQuickly(event);
    // But should be faster as a bunch of these calls were cached.
    arrayOfVectors[pid]->push_back(event);

    // TODO work with period
    local_num_good_events++;

  } // for each event

  PARALLEL_CRITICAL(LoadEventPreNexus_global_statistics) {
    this->num_good_events += local_num_good_events;
    this->num_ignored_events += local_num_ignored_events;
    this->num_error_events += local_num_error_events;
    if (local_shortest_tof < shortest_tof)
      shortest_tof = local_shortest_tof;
    if (local_longest_tof > longest_tof)
      longest_tof = local_longest_tof;
  }
}

//-----------------------------------------------------------------------------
/// Comparator for sorting dasevent lists
bool intermediatePixelIDComp(IntermediateEvent x, IntermediateEvent y) {
  return (x.pid < y.pid);
}

//-----------------------------------------------------------------------------
/**
 * Add a sample environment log for the proton chage (charge of the pulse in
 *picoCoulombs)
 * and set the scalar value (total proton charge, microAmps*hours, on the
 *sample)
 *
 * @param workspace :: Event workspace to set the proton charge on
 */
void LoadEventPreNexus::setProtonCharge(
    DataObjects::EventWorkspace_sptr &workspace) {
  if (this->proton_charge.empty()) // nothing to do
    return;

  Run &run = workspace->mutableRun();

  // Add the proton charge entries.
  TimeSeriesProperty<double> *log =
      new TimeSeriesProperty<double>("proton_charge");
  log->setUnits("picoCoulombs");

  // Add the time and associated charge to the log
  log->addValues(this->pulsetimes, this->proton_charge);

  /// TODO set the units for the log
  run.addLogData(log);
  double integ = run.integrateProtonCharge();
  // run.setProtonCharge(this->proton_charge_tot); //This is now redundant
  this->g_log.information() << "Total proton charge of " << integ
                            << " microAmp*hours found by integrating.\n";
}

//-----------------------------------------------------------------------------
/** Load a pixel mapping file
 * @param filename :: Path to file.
 */
void LoadEventPreNexus::loadPixelMap(const std::string &filename) {
  this->using_mapping_file = false;
  this->pixelmap.clear();

  // check that there is a mapping file
  if (filename.empty()) {
    this->g_log.information("NOT using a mapping file");
    return;
  }

  // actually deal with the file
  this->g_log.debug("Using mapping file \"" + filename + "\"");

  // Open the file; will throw if there is any problem
  BinaryFile<PixelType> pixelmapFile(filename);
  PixelType max_pid = static_cast<PixelType>(pixelmapFile.getNumElements());
  // Load all the data
  pixelmapFile.loadAllInto(this->pixelmap);

  // Check for funky file
  if (std::find_if(pixelmap.begin(), pixelmap.end(),
                   std::bind2nd(std::greater<PixelType>(), max_pid)) !=
      pixelmap.end()) {
    this->g_log.warning("Pixel id in mapping file was out of bounds. Loading "
                        "without mapping file");
    this->numpixel = 0;
    this->pixelmap.clear();
    this->using_mapping_file = false;
    return;
  }

  // If we got here, the mapping file was loaded correctly and we'll use it
  this->using_mapping_file = true;
  // Let's assume that the # of pixels in the instrument matches the mapping
  // file length.
  this->numpixel = static_cast<uint32_t>(pixelmapFile.getNumElements());
}

//-----------------------------------------------------------------------------
/** Open an event file
 * @param filename :: file to open.
 */
void LoadEventPreNexus::openEventFile(const std::string &filename) {
  // Open the file
  eventfile = new BinaryFile<DasEvent>(filename);
  num_events = eventfile->getNumElements();
  g_log.debug() << "File contains " << num_events << " event records.\n";

  // Check if we are only loading part of the event file
  const int chunk = getProperty("ChunkNumber");
  if (isEmpty(chunk)) // We are loading the whole file
  {
    first_event = 0;
    max_events = num_events;
  } else // We are loading part - work out the event number range
  {
    const int totalChunks = getProperty("TotalChunks");
    max_events = num_events / totalChunks;
    first_event = (chunk - 1) * max_events;
    // Need to add any remainder to the final chunk
    if (chunk == totalChunks)
      max_events += num_events % totalChunks;
  }

  g_log.information() << "Reading " << max_events << " event records\n";
}

//-----------------------------------------------------------------------------
/** Read a pulse ID file
 * @param filename :: file to load.
 * @param throwError :: Flag to trigger error throwing instead of just logging
 */
void LoadEventPreNexus::readPulseidFile(const std::string &filename,
                                        const bool throwError) {
  this->proton_charge_tot = 0.;
  this->num_pulses = 0;

  // jump out early if there isn't a filename
  if (filename.empty()) {
    this->g_log.information("NOT using a pulseid file");
    return;
  }

  std::vector<Pulse> *pulses;

  // set up for reading
  // Open the file; will throw if there is any problem
  try {
    BinaryFile<Pulse> pulseFile(filename);

    // Get the # of pulse
    this->num_pulses = pulseFile.getNumElements();
    this->g_log.information() << "Using pulseid file \"" << filename
                              << "\", with " << num_pulses << " pulses.\n";

    // Load all the data
    pulses = pulseFile.loadAll();
  } catch (runtime_error &e) {
    if (throwError) {
      throw;
    } else {
      this->g_log.information()
          << "Encountered error in pulseidfile (ignoring file): " << e.what()
          << "\n";
      return;
    }
  }

  double temp;

  if (num_pulses > 0) {
    this->pulsetimes.reserve(num_pulses);
    for (size_t i = 0; i < num_pulses; i++) {
      Pulse &it = (*pulses)[i];
      this->pulsetimes.push_back(
          DateAndTime((int64_t)it.seconds, (int64_t)it.nanoseconds));
      this->event_indices.push_back(it.event_index);

      temp = it.pCurrent;
      this->proton_charge.push_back(temp);
      if (temp < 0.)
        this->g_log.warning("Individual proton charge < 0 being ignored");
      else
        this->proton_charge_tot += temp;
    }
  }

  this->proton_charge_tot = this->proton_charge_tot * CURRENT_CONVERSION;

  // Clear the vector
  delete pulses;
}

} // namespace DataHandling
} // namespace Mantid
