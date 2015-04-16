#include "MantidDataHandling/FilterEventsByLogValuePreNexus.h"
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
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileValidator.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Glob.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/BinaryFile.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

#include <algorithm>
#include <sstream>
#include "MantidAPI/MemoryManager.h"

// #define DBOUT

namespace Mantid {
namespace DataHandling {

DECLARE_FILELOADER_ALGORITHM(FilterEventsByLogValuePreNexus)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
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

//----------------------------------------------------------------------------------------------
// constants for locating the parameters to use in execution
//----------------------------------------------------------------------------------------------
static const string EVENT_PARAM("EventFilename");
static const string PULSEID_PARAM("PulseidFilename");
static const string MAP_PARAM("MappingFilename");
static const string PID_PARAM("SpectrumList");
static const string PARALLEL_PARAM("UseParallelProcessing");
static const string BLOCK_SIZE_PARAM("LoadingBlockSize");
static const string OUT_PARAM("OutputWorkspace");
/// All pixel ids with matching this mask are errors.
static const PixelType ERROR_PID = 0x80000000;
/// The maximum possible tof as native type
static const uint32_t MAX_TOF_UINT32 = std::numeric_limits<uint32_t>::max();
/// Conversion factor between 100 nanoseconds and 1 microsecond.
static const double TOF_CONVERSION = .1;
/// Conversion factor between picoColumbs and microAmp*hours
static const double CURRENT_CONVERSION = 1.e-6 / 3600.;
/// Veto mask as 0xFF000000000
static const uint64_t VETOFLAG(72057594037927935);

static const string EVENT_EXTS[] = {
    "_neutron_event.dat",  "_neutron0_event.dat", "_neutron1_event.dat",
    "_neutron2_event.dat", "_neutron3_event.dat", "_live_neutron_event.dat"};
static const string PULSE_EXTS[] = {"_pulseid.dat",  "_pulseid0.dat",
                                    "_pulseid1.dat", "_pulseid2.dat",
                                    "_pulseid3.dat", "_live_pulseid.dat"};
static const int NUM_EXT = 6;

//----------------------------------------------------------------------------------------------
// Functions to deal with file name and run information
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Get run number
  */
static string getRunnumber(const string &filename) {
  // start by trimming the filename
  string runnumber(Poco::Path(filename).getBaseName());

  if (runnumber.find("neutron") >= string::npos)
    return "0";

  std::size_t left = runnumber.find("_");
  std::size_t right = runnumber.find("_", left + 1);

  return runnumber.substr(left + 1, right - left - 1);
}

//----------------------------------------------------------------------------------------------
/** Generate pulse ID
  */
static string generatePulseidName(string eventfile) {
  // initialize vector of endings and put live at the beginning
  vector<string> eventExts(EVENT_EXTS, EVENT_EXTS + NUM_EXT);
  std::reverse(eventExts.begin(), eventExts.end());
  vector<string> pulseExts(PULSE_EXTS, PULSE_EXTS + NUM_EXT);
  std::reverse(pulseExts.begin(), pulseExts.end());

  // look for the correct ending
  for (std::size_t i = 0; i < eventExts.size(); ++i) {
    size_t start = eventfile.find(eventExts[i]);
    if (start != string::npos)
      return eventfile.replace(start, eventExts[i].size(), pulseExts[i]);
  }

  // give up and return nothing
  return "";
}

//----------------------------------------------------------------------------------------------
/** Generate mapping file name
  */
static string generateMappingfileName(EventWorkspace_sptr &wksp) {
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
    return "";
#if 0
      instrument = Kernel::ConfigService::Instance().getInstrument(instrument).shortName();
      base = Poco::File("/SNS/" + instrument + "/");
      if (!base.exists())
        return "";
#endif
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

//----------------------------------------------------------------------------------------------
// Member functions
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Constructor
*/
FilterEventsByLogValuePreNexus::FilterEventsByLogValuePreNexus()
    : Mantid::API::IFileLoader<Kernel::FileDescriptor>(), eventfile(NULL),
      m_maxNumEvents(0) {}

//----------------------------------------------------------------------------------------------
/** Desctructor
 */
FilterEventsByLogValuePreNexus::~FilterEventsByLogValuePreNexus() {
  delete this->eventfile;
}

//----------------------------------------------------------------------------------------------
/** Return the confidence with with this algorithm can load the file
 *  @param descriptor A descriptor for the file
 *  @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int FilterEventsByLogValuePreNexus::confidence(
    Kernel::FileDescriptor &descriptor) const {
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
    return 10;
  else
    return 0;
}

//----------------------------------------------------------------------------------------------
/**  Initialize the algorithm
  */
void FilterEventsByLogValuePreNexus::init() {
  // File files to use
  vector<string> eventExts(EVENT_EXTS, EVENT_EXTS + NUM_EXT);
  declareProperty(
      new FileProperty(EVENT_PARAM, "", FileProperty::Load, eventExts),
      "The name of the neutron event file to read, including its full or "
      "relative path. In most cases, the file typically ends in "
      "neutron_event.dat (N.B. case sensitive if running on Linux).");
  vector<string> pulseExts(PULSE_EXTS, PULSE_EXTS + NUM_EXT);
  declareProperty(new FileProperty(PULSEID_PARAM, "",
                                   FileProperty::OptionalLoad, pulseExts),
                  "File containing the accelerator pulse information; the "
                  "filename will be found automatically if not specified.");
  declareProperty(
      new FileProperty(MAP_PARAM, "", FileProperty::OptionalLoad, ".dat"),
      "File containing the pixel mapping (DAS pixels to pixel IDs) file "
      "(typically INSTRUMENT_TS_YYYY_MM_DD.dat). The filename will be found "
      "automatically if not specified.");

  // Pixels to load
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

  // Loading option
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

  // Optional output table workspace
  declareProperty(new WorkspaceProperty<ITableWorkspace>(
                      "EventLogTableWorkspace", "", PropertyMode::Optional),
                  "Optional output table workspace containing the event log "
                  "(pixel) information. ");

  //
  std::vector<std::string> vecfunmode;
  vecfunmode.push_back("LoadData");
  vecfunmode.push_back("Filter");
  vecfunmode.push_back("ExamineEventLog");
  declareProperty("FunctionMode", "LoadData",
                  boost::make_shared<StringListValidator>(vecfunmode),
                  "Function mode for different purpose. ");

  declareProperty("PixelIDtoExamine", EMPTY_INT(),
                  "Pixel ID for the events to be examined. ");

  declareProperty("NumberOfEventsToExamine", EMPTY_INT(),
                  "Number of events on the pixel ID to get examined. ");

  declareProperty(new ArrayProperty<int>("LogPixelIDs"),
                  "Pixel IDs for event log. Must have 2 (or more) entries. ");

  declareProperty(
      new ArrayProperty<std::string>("LogPIxelTags"),
      "Pixel ID tags for event log. Must have same items as 'LogPixelIDs'. ");

  declareProperty("AcceleratorFrequency", 60, "Freuqency of the accelerator at "
                                              "which the experiment runs. It "
                                              "can 20, 30 or 60.");

  declareProperty("CorrectTOFtoSample", false,
                  "Correct TOF to sample position. ");

  declareProperty("DBPixelID", EMPTY_INT(),
                  "ID of the pixel (detector) for debug output. ");

  return;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm
 * 1. check all the inputs
 * 2. create an EventWorkspace object
 * 3. process events
 * 4. set out output
 */
void FilterEventsByLogValuePreNexus::exec() {
  // Process inputs
  prog = new Progress(this, 0.0, 1.0, 100);
  processProperties();

  // Read input files
  prog->report("Loading Pulse ID file");
  readPulseidFile(m_pulseIDFileName, m_throwError);

  prog->report("Loading Event File");
  openEventFile(m_eventFileName);

  // Correct wrong event index in loaded eventindexes
  unmaskVetoEventIndexes();

  // Find out the frequency of the frequency
  int runfreq = findRunFrequency();
  if (m_freqHz != runfreq) {
    if (m_freqHz % runfreq == 0) {
      int frame = m_freqHz / runfreq;
      g_log.warning() << "Input frequency " << m_freqHz
                      << " is different from data. "
                      << "It is forced to use input frequency, while all "
                         "events' pulse time will be "
                      << "set to " << frame << "-th freme. "
                      << "\n";
    } else {
      throw std::runtime_error("Operation frequency is not self-consistent");
    }
  }
  istep = 60 / m_freqHz;

  // Create and set up output EventWorkspace
  localWorkspace = setupOutputEventWorkspace();
  if (m_functionMode == "Filter")
    localWorkspaceBA = setupOutputEventWorkspace();

  // Process the events into pixels
  if (m_functionMode == "Filter") {
    filterEvents();
  } else {
    procEvents(localWorkspace);
  }

  // set that the sort order on the event lists
  if (this->num_pulses > 0 && this->pulsetimesincreasing) {
    const int64_t numberOfSpectra = localWorkspace->getNumberHistograms();
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int64_t i = 0; i < numberOfSpectra; i++) {
      PARALLEL_START_INTERUPT_REGION
      localWorkspace->getEventListPtr(i)
          ->setSortOrder(DataObjects::PULSETIME_SORT);
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
  }

  // Save output
  setProperty<IEventWorkspace_sptr>(OUT_PARAM, localWorkspace);
  if (m_functionMode == "Filter") {
    declareProperty(new WorkspaceProperty<IEventWorkspace>(
                        "OutputFilteredWorkspace", "WS_A", Direction::Output),
                    "");
    setProperty<IEventWorkspace_sptr>("OutputFilteredWorkspace",
                                      localWorkspaceBA);
  }

  // Add fast frequency sample environment (events) data to workspace's log
  processEventLogs();

  // -1. Cleanup
  delete prog;

  return;
} // exec()

//----------------------------------------------------------------------------------------------
/** Process input properties
  */
void FilterEventsByLogValuePreNexus::processProperties() {
  // Process and check input properties
  // Check 'chunk' properties are valid, if set
  const int chunks = getProperty("TotalChunks");
  if (!isEmpty(chunks) && int(getProperty("ChunkNumber")) > chunks) {
    throw std::out_of_range("ChunkNumber cannot be larger than TotalChunks");
  }

  // What spectra (pixel ID's) to load
  this->spectra_list = this->getProperty(PID_PARAM);

  // The event file is needed in case the pulseid fileanme is empty
  m_eventFileName = this->getPropertyValue(EVENT_PARAM);

  // Pulse ID file
  m_pulseIDFileName = this->getPropertyValue(PULSEID_PARAM);
  m_throwError = true;

  if (m_pulseIDFileName.empty()) {
    // Pulse ID file is not given: generate by routine
    m_pulseIDFileName = generatePulseidName(m_eventFileName);
    if (!m_pulseIDFileName.empty()) {
      // Check existence of pulse ID file with generated name
      if (Poco::File(m_pulseIDFileName).exists()) {
        g_log.information() << "Found pulseid file " << m_pulseIDFileName
                            << "\n";
        m_throwError = false;
      } else {
        m_pulseIDFileName = "";
        g_log.warning("Generated pulse ID file name does not point to an "
                      "existing file. ");
      }
    } else {
      g_log.warning("Generated an empty pulse ID file. ");
    }
  }

  m_functionMode = getPropertyValue("FunctionMode");

  m_pixelid2exam = getProperty("PixelIDtoExamine");
  m_numevents2write = getProperty("NumberOfEventsToExamine");

  // Check whether option function mode is valid
  m_examEventLog = false;
  if (m_functionMode == "ExamineEventLog") {
    bool nogo = false;
    if (isEmpty(m_pixelid2exam)) {
      nogo = true;
    }

    if (nogo) {
      g_log.warning()
          << "In functional mode ExamineEventLog, pixel ID must be given!"
          << "\n";
      throw std::runtime_error("Incorrect input.");
    }

    m_examEventLog = true;
  } else if (m_functionMode == "Filter") {
    m_vecLogPixelID = getProperty("LogPixelIDs");
    m_vecLogPixelTag = getProperty("LogPIxelTags");

    if (m_vecLogPixelID.size() < 2) {
      throw std::runtime_error(
          "Input log pixel IDs must have more than 2 entries. ");
    } else if (m_vecLogPixelID.size() != m_vecLogPixelTag.size()) {
      throw std::runtime_error(
          "Input log pixel tags must have the same number of items as "
          "log pixe IDs. ");
    }
  }

  //---------------------------------------------------------------------------
  // Load partial spectra
  //---------------------------------------------------------------------------
  // For slight speed up
  loadOnlySomeSpectra = (this->spectra_list.size() > 0);

  // Turn the spectra list into a map, for speed of access
  for (std::vector<int64_t>::iterator it = spectra_list.begin();
       it != spectra_list.end(); it++)
    spectraLoadMap[*it] = true;

  //---------------------------------------------------------------------------
  // Other features
  //---------------------------------------------------------------------------
  // Accelerator frquency
  m_freqHz = getProperty("AcceleratorFrequency");
  if (m_freqHz != 20 && m_freqHz != 30 && m_freqHz != 60)
    throw runtime_error("Only 20, 30 and 60Hz are supported. ");

  int tempint = getProperty("DBPixelID");
  if (isEmpty(tempint))
    m_useDBOutput = false;
  else {
    m_useDBOutput = true;
    m_dbPixelID = static_cast<int64_t>(tempint);
  }

  m_corretctTOF = getProperty("CorrectTOFtoSample");

  return;
} // END of processProperties

//----------------------------------------------------------------------------------------------
/** Create, initialize and set up output EventWorkspace
  */
DataObjects::EventWorkspace_sptr
FilterEventsByLogValuePreNexus::setupOutputEventWorkspace() {
  // Create and initialize output EventWorkspace
  prog->report("Creating output workspace");

  EventWorkspace_sptr tempworkspace;

  tempworkspace = EventWorkspace_sptr(new EventWorkspace());
  // Make sure to initialize. We can use dummy numbers for arguments, for event
  // workspace it doesn't matter
  tempworkspace->initialize(1, 1, 1);
  // Set the units and title
  tempworkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  tempworkspace->setYUnit("Counts");
  tempworkspace->setTitle("Dummy Title");

  // Add some properties to output workspace, including
  //   the run_start property (Use the first pulse as the run_start time)
  if (this->num_pulses > 0) {
// add the start of the run as a ISO8601 date/time string. The start = the first
// pulse.
// (this is used in LoadInstrument to find the right instrument file to use).
#if 0
      DateAndTime pulse0 = pulsetimes[0];
      g_log.notice() << "Pulse time 0 = " <<  pulse0.totalNanoseconds() << "\n";
#endif
    tempworkspace->mutableRun().addProperty(
        "run_start", pulsetimes[0].toISO8601String(), true);
  }

  //   the run number and add it to the run object
  tempworkspace->mutableRun().addProperty("run_number",
                                          getRunnumber(m_eventFileName));

  // Add the instrument!
  prog->report("Loading Instrument");
  this->runLoadInstrument(m_eventFileName, tempworkspace);

  // Load the mapping file
  prog->report("Loading Mapping File");
  string mapping_filename = this->getPropertyValue(MAP_PARAM);
  if (mapping_filename.empty()) {
    // No mapping file given: genrate mapping file name by routine
    mapping_filename = generateMappingfileName(tempworkspace);
    if (!mapping_filename.empty())
      g_log.information() << "Found mapping file \"" << mapping_filename << "\""
                          << "\n";
    else
      g_log.warning("No mapping file is generated. ");
  }
  // if (!mapping_filename.empty())
  loadPixelMap(mapping_filename);

  return tempworkspace;
}

//----------------------------------------------------------------------------------------------
/** Process imbed logs (marked by bad pixel IDs)
  * (1) Add special event log to workspace log
  * (2) (Optionally) do statistic to each pixel
  * (3) (Optionally) write out information
  */
void FilterEventsByLogValuePreNexus::processEventLogs() {
  std::set<PixelType>::iterator pit;
  std::map<PixelType, size_t>::iterator mit;
  for (pit = this->wrongdetids.begin(); pit != this->wrongdetids.end(); ++pit) {
    // Convert Pixel ID to 'wrong detectors ID' map's index
    PixelType pid = *pit;
    mit = this->wrongdetidmap.find(pid);
    size_t mindex = mit->second;
    if (mindex > this->wrongdetid_pulsetimes.size()) {
      g_log.error() << "Wrong Index " << mindex << " for Pixel " << pid
                    << std::endl;
      throw std::invalid_argument("Wrong array index for pixel from map");
    } else {
      g_log.information() << "Processing imbed log marked by Pixel " << pid
                          << " with size = "
                          << this->wrongdetid_pulsetimes[mindex].size()
                          << std::endl;
    }

    // Generate the log name
    std::stringstream ssname;
    ssname << "Pixel" << pid;
    std::string logname = ssname.str();

    // Add this map entry to log
    addToWorkspaceLog(logname, mindex);

    // Do some statistic to this event log
    doStatToEventLog(mindex);

    g_log.information() << "Added Log " << logname
                        << " to output workspace. \n";

  } // ENDFOR pit

  // Output table workspace
  std::string evlog = getPropertyValue("EventLogTableWorkspace");
  if (!evlog.empty()) {
    // Initialize table workspace
    TableWorkspace_sptr evtablews = boost::make_shared<TableWorkspace>();
    evtablews->addColumn("int", "Pixel-ID");
    evtablews->addColumn("int", "NumberOfEvents");

    // Add information rows
    std::map<PixelType, size_t>::iterator git;
    for (git = this->wrongdetidmap.begin(); git != this->wrongdetidmap.end();
         ++git) {
      PixelType tmpid = git->first;
      size_t vindex = git->second;

      TableRow temprow = evtablews->appendRow();
      temprow << static_cast<int>(tmpid)
              << static_cast<int>(wrongdetid_pulsetimes[vindex].size());
    }

    // Set property
    setProperty("EventLogTableWorkspace",
                boost::dynamic_pointer_cast<ITableWorkspace>(evtablews));
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Add absolute time series to log
  * @param logtitle :: title of the log to be inserted to workspace
  * @param mindex ::  index of the the series in the wrong detectors map
  */
void FilterEventsByLogValuePreNexus::addToWorkspaceLog(std::string logtitle,
                                                       size_t mindex) {
  // Create TimeSeriesProperty
  TimeSeriesProperty<double> *property =
      new TimeSeriesProperty<double>(logtitle);

  // Add entries
  size_t nbins = this->wrongdetid_pulsetimes[mindex].size();
  for (size_t k = 0; k < nbins; k++) {
    double tof = this->wrongdetid_tofs[mindex][k];
    DateAndTime pulsetime = wrongdetid_pulsetimes[mindex][k];
    int64_t abstime_ns =
        pulsetime.totalNanoseconds() + static_cast<int64_t>(tof * 1000);
    DateAndTime abstime(abstime_ns);

    double value = tof;

    property->addValue(abstime, value);
  } // ENDFOR

  // Add property to workspace
  localWorkspace->mutableRun().addProperty(property, false);

  g_log.information() << "Size of Property " << property->name() << " = "
                      << property->size() << " vs Original Log Size = " << nbins
                      << "\n";

  return;
}

//----------------------------------------------------------------------------------------------
/** Perform statistics to event (wrong pixel ID) logs
  * @param mindex ::  index of the the series in the list
  */
void FilterEventsByLogValuePreNexus::doStatToEventLog(size_t mindex) {
  // Create a vector of event log time entries
  size_t nbins = this->wrongdetid_pulsetimes[mindex].size();
  if (nbins <= 2) {
    g_log.warning() << "Event log of map index " << mindex << " has " << nbins
                    << " entries.  There is no need to do statistic on it. "
                    << "\n";
  }

  std::vector<int64_t> vec_logtime(nbins);
  for (size_t i = 0; i < nbins; ++i) {
    DateAndTime ptime = wrongdetid_pulsetimes[mindex][i];
    int64_t templogtime =
        ptime.totalNanoseconds() +
        static_cast<int64_t>(wrongdetid_tofs[mindex][i] * 1000.);

    vec_logtime[i] = templogtime;
  } // ENDFOR

  // Sort
  std::sort(vec_logtime.begin(), vec_logtime.end());

  // Do statistic
  int64_t min_dt = vec_logtime[1] - vec_logtime[0];
  int64_t max_dt = min_dt;
  int64_t sum_dt = min_dt;
  int64_t numzeros = 0;
  for (size_t i = 2; i < nbins; ++i) {
    int64_t temp_dt = vec_logtime[i] - vec_logtime[i - 1];
    if (temp_dt == 0)
      ++numzeros;

    sum_dt += temp_dt;

    if (temp_dt < min_dt)
      min_dt = temp_dt;
    else if (temp_dt > max_dt)
      max_dt = temp_dt;
  }

  double avg_dt = static_cast<double>(sum_dt) / static_cast<double>(nbins - 1);

  g_log.information() << "Event log of map index " << mindex
                      << ": Avg(dt) = " << avg_dt * 1.0E-9
                      << ", Min(dt) = " << static_cast<double>(min_dt) * 1.0E-9
                      << ", Max(dt) = " << static_cast<double>(max_dt) * 1.0E-9
                      << "\n";
  g_log.information() << "Number of zero-interval eveng log = " << numzeros
                      << "\n";

  return;
}

//-----------------------------------------------------------------------------
/** Load the instrument geometry File
 *  @param eventfilename :: Used to pick the instrument.
 *  @param localWorkspace :: MatrixWorkspace in which to put the instrument
 * geometry
 */
void FilterEventsByLogValuePreNexus::runLoadInstrument(
    const std::string &eventfilename, MatrixWorkspace_sptr localWorkspace) {
  // start by getting just the filename
  string instrument = Poco::Path(eventfilename).getFileName();

  // initialize vector of endings and put live at the beginning
  vector<string> eventExts(EVENT_EXTS, EVENT_EXTS + NUM_EXT);
  std::reverse(eventExts.begin(), eventExts.end());

  for (size_t i = 0; i < eventExts.size(); ++i) {
    size_t pos = instrument.find(eventExts[i]);
    if (pos != string::npos) {
      instrument = instrument.substr(0, pos);
      break;
    }
  }

  // determine the instrument parameter file
  size_t pos = instrument.rfind("_"); // get rid of the run number
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

//----------------------------------------------------------------------------------------------
/** Process the event file properly.
 * @param workspace :: EventWorkspace to write to.
 */
void FilterEventsByLogValuePreNexus::procEvents(
    DataObjects::EventWorkspace_sptr &workspace) {
  // Initialize stat parameters
  this->num_error_events = 0;
  this->num_good_events = 0;
  this->num_ignored_events = 0;
  this->num_bad_events = 0;
  this->num_wrongdetid_events = 0;

  shortest_tof = static_cast<double>(MAX_TOF_UINT32) * TOF_CONVERSION;
  longest_tof = 0.;

  // -------------------------------------------------------------------
  // Set up instrument related parameters such as detector map and etc.
  // We want to pad out empty pixels.
  //--------------------------------------------------------------------
  detid2det_map detector_map;
  workspace->getInstrument()->getDetectors(detector_map);

  // Determine maximum pixel id
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
      // Add non-monitor detector ID
      this->pixel_to_wkspindex[it->first] = workspaceIndex;
      EventList &spec = workspace->getOrAddEventList(workspaceIndex);
      spec.addDetectorID(it->first);
      // Start the spectrum number at 1
      spec.setSpectrumNo(specid_t(workspaceIndex + 1));
      workspaceIndex += 1;
    }
  }

  // ----------------------------------------------------------------
  // Determine processing mode and file-loading parameters
  //------------------------------------------------------------------
  // Set up some default values in the case of no parallel
  size_t loadBlockSize = Mantid::Kernel::DEFAULT_BLOCK_SIZE * 2;
  size_t numBlocks = (m_maxNumEvents + loadBlockSize - 1) / loadBlockSize;

  std::string procMode = getProperty("UseParallelProcessing");
  if (procMode == "Serial") {
    parallelProcessing = false;
  } else if (procMode == "Parallel") {
    parallelProcessing = true;
  } else {
    // Automatic determination. Loading serially (for me) is about 3 million
    // events per second,
    // (which is sped up by ~ x 3 with parallel processing, say 10 million per
    // second, e.g. 7 million events more per seconds).
    // compared to a setup time/merging time of about 10 seconds per million
    // detectors.
    double setUpTime = double(detector_map.size()) * 10e-6;
    parallelProcessing = ((double(m_maxNumEvents) / 7e6) > setUpTime);
    g_log.information() << (parallelProcessing ? "Using" : "Not using")
                        << " parallel processing."
                        << "\n";
  }

  if (m_functionMode == "ExamineEventLog" && parallelProcessing) {
    parallelProcessing = false;
    g_log.notice("In function mode 'ExamineEventLog', processing mode is "
                 "forced to serial. ");
  }

#if 0
    //For slight speed up
    loadOnlySomeSpectra = (this->spectra_list.size() > 0);

    //Turn the spectra list into a map, for speed of access
    for (std::vector<int64_t>::iterator it = spectra_list.begin(); it != spectra_list.end(); it++)
      spectraLoadMap[*it] = true;
#endif

  CPUTimer tim;

  // -------------------------------------------------------------------
  // Create the partial workspaces
  //--------------------------------------------------------------------
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

  // Processing by number of threads
  g_log.information() << "Processing input event preNexus by " << numThreads
                      << " threads"
                      << " in " << numBlocks << " blocks. "
                      << "\n";

  // cppcheck-suppress syntaxError
    PRAGMA_OMP( parallel for schedule(dynamic, 1) if (parallelProcessing) )
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
    } // END FOR [Threads]

    g_log.information() << tim << " to create " << partWorkspaces.size()
                        << " workspaces for parallel loading."
                        << "\n";

    prog->resetNumSteps(numBlocks, 0.1, 0.8);

    // -------------------------------------------------------------------
    // LOAD THE DATA
    //--------------------------------------------------------------------
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
              ? (m_maxNumEvents - (numBlocks - 1) * loadBlockSize)
              : loadBlockSize;

      // Load this chunk of event data (critical block)
      PARALLEL_CRITICAL(FilterEventsByLogValuePreNexus_fileAccess) {
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

    g_log.information() << tim << " to load the data." << std::endl;

    // -------------------------------------------------------------------
    // MERGE WORKSPACES BACK TOGETHER
    //--------------------------------------------------------------------
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

        // With TCMalloc, release memory when you accumulate enough to make
        // sense
        PARALLEL_CRITICAL(FilterEventsByLogValuePreNexus_trackMemory) {
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

    // -------------------------------------------------------------------
    // Final message output
    //--------------------------------------------------------------------
    g_log.notice() << "Read " << num_good_events << " events + "
                   << num_error_events << " errors"
                   << ". Shortest TOF: " << shortest_tof
                   << " microsec; longest TOF: " << longest_tof << " microsec."
                   << "\n"
                   << "Bad Events = " << num_bad_events
                   << "  Events of Wrong Detector = " << num_wrongdetid_events
                   << "\n"
                   << "Number of Wrong Detector IDs = " << wrongdetids.size()
                   << "\n";

    std::set<PixelType>::iterator wit;
    for (wit = this->wrongdetids.begin(); wit != this->wrongdetids.end();
         ++wit) {
      g_log.notice() << "Wrong Detector ID : " << *wit << std::endl;
    }
    std::map<PixelType, size_t>::iterator git;
    for (git = this->wrongdetidmap.begin(); git != this->wrongdetidmap.end();
         ++git) {
      PixelType tmpid = git->first;
      size_t vindex = git->second;
      g_log.notice() << "Pixel " << tmpid << ":  Total number of events = "
                     << this->wrongdetid_pulsetimes[vindex].size() << std::endl;
    }

    return;
} // End of procEvents

//----------------------------------------------------------------------------------------------
/** Linear-version of the procedure to process the event file properly.
  * @param workspace :: EventWorkspace to write to.
  * @param arrayOfVectors :: For speed up: this is an array, of size
 * detid_max+1, where the
  *        index is a pixel ID, and the value is a pointer to the
 * vector<tofEvent> in the given EventList.
  * @param event_buffer :: The buffer containing the DAS events
  * @param current_event_buffer_size :: The length of the given DAS buffer
  * @param fileOffset :: Value for an offset into the binary file
  */
void FilterEventsByLogValuePreNexus::procEventsLinear(
    DataObjects::EventWorkspace_sptr & /*workspace*/,
    std::vector<TofEvent> **arrayOfVectors, DasEvent *event_buffer,
    size_t current_event_buffer_size, size_t fileOffset) {
  //----------------------------------------------------------------------------------
  // Set up parameters to process events from raw file
  //----------------------------------------------------------------------------------
  // Pulse ID and pulse time
  DateAndTime pulsetime;
  int64_t numPulses = static_cast<int64_t>(num_pulses);
  if (m_vecEventIndex.size() < num_pulses) {
    g_log.warning()
        << "Event_indices vector is smaller than the pulsetimes array.\n";
    numPulses = static_cast<int64_t>(m_vecEventIndex.size());
  }

  uint64_t maxeventid = m_vecEventIndex.back();
  g_log.debug() << "Maximum event index = " << maxeventid << " vs. "
                << m_maxNumEvents << "\n";
  maxeventid = m_maxNumEvents + 1;

  size_t numbadeventindex = 0;

  int numeventswritten = 0;

  // Declare local statistic parameters
  size_t local_num_error_events = 0;
  size_t local_num_bad_events = 0;
  size_t local_num_wrongdetid_events = 0;
  size_t local_num_ignored_events = 0;
  size_t local_num_good_events = 0;
  double local_shortest_tof =
      static_cast<double>(MAX_TOF_UINT32) * TOF_CONVERSION;
  double local_longest_tof = 0.;

  // Local data structure for loaded events
  std::map<PixelType, size_t> local_pidindexmap;
  std::vector<std::vector<Kernel::DateAndTime>> local_pulsetimes;
  std::vector<std::vector<double>> local_tofs;

  std::set<PixelType> local_wrongdetids;
  size_t numwrongpid = 0;

  //----------------------------------------------------------------------------------
  // process the individual events
  //----------------------------------------------------------------------------------
  int64_t i_pulse = 0;

  for (size_t ievent = 0; ievent < current_event_buffer_size; ++ievent) {
    bool iswrongdetid = false;

    // Load DasEvent
    DasEvent &tempevent = *(event_buffer + ievent);

    // DasEvetn's pixel ID
    PixelType pixelid = tempevent.pid;

    // Check Pixels IDs
    if ((pixelid & ERROR_PID) == ERROR_PID) {
      // Marked as bad
      local_num_error_events++;
      local_num_bad_events++;
      continue;
    } else {
      // Covert DAS Pixel ID to Mantid Pixel ID
      if (pixelid == 1073741843) {
        // downstream monitor pixel for SNAP
        pixelid = 1179648;
      } else if (this->using_mapping_file) {
        // Converted by pixel mapping file
        PixelType unmapped_pid = pixelid % this->numpixel;
        pixelid = this->pixelmap[unmapped_pid];
      }

      // Check special/wrong pixel IDs against max Detector ID
      if (pixelid > static_cast<PixelType>(detid_max)) {
        // Record the wrong/special ID
        iswrongdetid = true;

        ++local_num_error_events;
        ++local_num_wrongdetid_events;
        local_wrongdetids.insert(pixelid);
      }

      // Check if this pid we want to load.
      if (loadOnlySomeSpectra && !iswrongdetid) {
        std::map<int64_t, bool>::iterator it;
        it = spectraLoadMap.find(pixelid);
        if (it == spectraLoadMap.end()) {
          // Pixel ID was not found, so the event is being ignored.
          local_num_ignored_events++;
          continue;
        }
      }

      // Work with the events to be processed
      // Find the pulse time for this event index
      if (i_pulse < numPulses - istep) {
        // This is the total offset into the file
        size_t i_totaloffset = ievent + fileOffset;

        // Go through event_index until you find where the index increases to
        // encompass the current index.
        // Your pulse = the one before.
        uint64_t thiseventindex = m_vecEventIndex[i_pulse];
        uint64_t nexteventindex = m_vecEventIndex[i_pulse + istep];
        while (!((i_totaloffset >= thiseventindex) &&
                 (i_totaloffset < nexteventindex))) {
          i_pulse += istep;
          thiseventindex = m_vecEventIndex[i_pulse];
          if (i_pulse >= (numPulses - istep))
            break;
          nexteventindex = m_vecEventIndex[i_pulse + istep];
        }

        // Save the pulse time at this index for creating those events
        pulsetime = pulsetimes[i_pulse];
      } // Find pulse time

      double tof = static_cast<double>(tempevent.tof) * TOF_CONVERSION;

#if 0
        if (fileOffset == 0 && ievent < 100)
        {
          g_log.notice() << ievent << "\t" << i_pulse << "\t" << pulsetime << "\t"
                         << tof << "\t" << pixelid << "\n";
          // g_log.notice() << "Event " << ievent << "\t\t" << pixelid << "\t\t" << i_pulse << "\n";
        }
#endif

      // For function option "ExamineEventLog"
      if (m_examEventLog && pixelid == m_pixelid2exam &&
          numeventswritten < m_numevents2write) {
        int64_t totaltime =
            pulsetime.totalNanoseconds() + static_cast<int64_t>(tof * 1000);
        // Output: [EEL] for Examine Event Log
        g_log.notice() << "[EEL] " << numeventswritten << "\t\t" << totaltime
                       << "\t\t" << pixelid << "\t\t" << i_pulse << "\t\t"
                       << fileOffset << "\n";
        ++numeventswritten;
      }

      if (!iswrongdetid) {
        // Event on REAL detector
        // - Find the overall max/min tof
        if (tof < local_shortest_tof)
          local_shortest_tof = tof;
        if (tof > local_longest_tof)
          local_longest_tof = tof;

// The addEventQuickly method does not clear the cache, making things slightly
// faster.
// workspace->getEventList(this->pixel_to_wkspindex[pid]).addEventQuickly(event);

// - Add event to data structure
// (This is equivalent to
// workspace->getEventList(this->pixel_to_wkspindex[pid]).addEventQuickly(event))
// (But should be faster as a bunch of these calls were cached.)
#if defined(__GNUC__) && !(defined(__INTEL_COMPILER)) && !(defined(__clang__))
        // This avoids a copy constructor call but is only available with GCC
        // (requires variadic templates)
        arrayOfVectors[pixelid]->emplace_back(tof, pulsetime);
#else
        arrayOfVectors[pixelid]->push_back(TofEvent(tof, pulsetime));
#endif

        ++local_num_good_events;

#if 0
          if (fileOffset == 0 && numeventsprint < 10)
          {
            g_log.notice() << "[E10]" << "Pulse Time = " << pulsetime << ", TOF = " << tof
                           << ", Pixel ID = " << pixelid
                           << " Pulse index = " << i_pulse << ", FileOffset =" << fileOffset << "\n";
            ++ numeventsprint;
          }
#endif
      } else {
        // Special events/Wrong detector id
        // - get/add index of the entry in map
        std::map<PixelType, size_t>::iterator it;
        it = local_pidindexmap.find(pixelid);
        size_t theindex = 0;
        if (it == local_pidindexmap.end()) {
          // Initialize it!
          size_t newindex = local_pulsetimes.size();
          local_pidindexmap[pixelid] = newindex;

          std::vector<Kernel::DateAndTime> tempvectime;
          std::vector<double> temptofs;
          local_pulsetimes.push_back(tempvectime);
          local_tofs.push_back(temptofs);

          theindex = newindex;

          ++numwrongpid;
        } else {
          // existing
          theindex = it->second;
        }

        // Store pulse time and tof of this event
        local_pulsetimes[theindex].push_back(pulsetime);
        local_tofs[theindex].push_back(tof);
      } // END-IF-ELSE: On Event's Pixel's Nature

    } // ENDIF (event is masked error)

  } // ENDFOR each event

  g_log.debug() << "Number of wrong pixel ID = " << numwrongpid
                << " of single block. "
                << "\n";

  PARALLEL_CRITICAL(FilterEventsByLogValuePreNexus_global_statistics) {
    this->num_good_events += local_num_good_events;
    this->num_ignored_events += local_num_ignored_events;
    this->num_error_events += local_num_error_events;

    this->num_bad_events += local_num_bad_events;
    this->num_wrongdetid_events += local_num_wrongdetid_events;

    std::set<PixelType>::iterator it;
    for (it = local_wrongdetids.begin(); it != local_wrongdetids.end(); ++it) {
      PixelType tmpid = *it;
      this->wrongdetids.insert(*it);

      // Obtain the global map index for this wrong detector ID events entry in
      // local map
      size_t mindex = 0;
      std::map<PixelType, size_t>::iterator git =
          this->wrongdetidmap.find(tmpid);
      if (git == this->wrongdetidmap.end()) {
        // Create 'wrong detid' global map entry if not there
        size_t newindex = this->wrongdetid_pulsetimes.size();
        this->wrongdetidmap[tmpid] = newindex;

        std::vector<Kernel::DateAndTime> vec_pulsetimes;
        std::vector<double> vec_tofs;
        this->wrongdetid_pulsetimes.push_back(vec_pulsetimes);
        this->wrongdetid_tofs.push_back(vec_tofs);

        mindex = newindex;
      } else {
        mindex = git->second;
      }

      // Find local map index
      std::map<PixelType, size_t>::iterator lit = local_pidindexmap.find(tmpid);
      size_t localindex = lit->second;

      // Append local (thread) loaded events (pulse + tof) to global wrong detid
      // data structure
      for (size_t iv = 0; iv < local_pulsetimes[localindex].size(); iv++) {
        this->wrongdetid_pulsetimes[mindex].push_back(
            local_pulsetimes[localindex][iv]);
        this->wrongdetid_tofs[mindex].push_back(local_tofs[localindex][iv]);
      }
    }

    if (local_shortest_tof < shortest_tof)
      shortest_tof = local_shortest_tof;
    if (local_longest_tof > longest_tof)
      longest_tof = local_longest_tof;
  }

  if (numbadeventindex > 0) {
    g_log.notice() << "Single block: Encountered " << numbadeventindex
                   << " bad event indexes"
                   << "\n";
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Correct wrong event indexes
  */
void FilterEventsByLogValuePreNexus::unmaskVetoEventIndexes() {
  // Check pulse ID with events
  size_t numveto = 0;
  size_t numerror = 0;

    PRAGMA_OMP(parallel for schedule(dynamic, 1) )
    for (int i = 0; i < static_cast<int>(m_vecEventIndex.size()); ++i) {
      PARALLEL_START_INTERUPT_REGION

      uint64_t eventindex = m_vecEventIndex[i];
      if (eventindex > static_cast<uint64_t>(num_events)) {
        uint64_t realeventindex = eventindex & VETOFLAG;
        m_vecEventIndex[i] = realeventindex;
      }
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

#if 0
        // Examine whether it is a veto
        bool isveto = false;




        if (realeventindex <= num_events)
        {
          if (i == 0 || realeventindex >= m_vecEventIndex[i-1])
          {
            isveto = true;
          }
        }

        if (isveto)
        {
          // Is veto, use the unmasked event index
          m_vecEventIndex[i] = realeventindex;
          // ++ numveto;
          g_log.debug() << "[DB Output]" << "Event index " << eventindex
                        << " is corrected to " << realeventindex << "\n";
        }
        else
        {
          PARALLEL_CRITICAL(unmask_veto)
          {
            m_vecEventIndex[i] = m_vecEventIndex[i-1];
            ++ numerror;
            g_log.error() << "EventIndex " << eventindex << " of pulse (indexed as " << i << ") is wrong! "
                          << "Tried to convert them to " << realeventindex << " , still exceeding max event index "
                          << num_events << "\n";
          }
        }
      } // END
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
#endif

    // Check
    PRAGMA_OMP(parallel for schedule(dynamic, 1) )
    for (int i = 0; i < static_cast<int>(m_vecEventIndex.size()); ++i) {
      PARALLEL_START_INTERUPT_REGION

      uint64_t eventindex = m_vecEventIndex[i];
      if (eventindex > static_cast<uint64_t>(num_events)) {
        PARALLEL_CRITICAL(unmask_veto_check) {
          g_log.information() << "Check: Pulse " << i
                              << ": unphysical event index = " << eventindex
                              << "\n";
        }
      }

      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    g_log.notice() << "Number of veto pulses = " << numveto
                   << ", Number of error-event-index pulses = " << numerror
                   << "\n";

    return;
}

//----------------------------------------------------------------------------------------------
/** Process the event file properly.
 */
void FilterEventsByLogValuePreNexus::filterEvents() {
  // Initialize stat parameters
  shortest_tof = static_cast<double>(MAX_TOF_UINT32) * TOF_CONVERSION;
  longest_tof = 0.;

  // -------------------------------------------------------------------
  // Set up instrument related parameters such as detector map and etc.
  // We want to pad out empty pixels.
  //--------------------------------------------------------------------
  size_t detectorsize = padOutEmptyPixels(localWorkspace);
  setupPixelSpectrumMap(localWorkspace);
  setupPixelSpectrumMap(localWorkspaceBA);

  // ----------------------------------------------------------------
  // Determine processing mode and file-loading parameters
  //------------------------------------------------------------------
  // Set up some default values in the case of no parallel
  size_t loadBlockSize = Mantid::Kernel::DEFAULT_BLOCK_SIZE * 2;
  size_t numBlocks = (m_maxNumEvents + loadBlockSize - 1) / loadBlockSize;

  std::string procMode = getProperty("UseParallelProcessing");
  if (procMode == "Serial") {
    parallelProcessing = false;
  } else if (procMode == "Parallel") {
    parallelProcessing = true;
  } else {
    // Automatic determination. Loading serially (for me) is about 3 million
    // events per second,
    // (which is sped up by ~ x 3 with parallel processing, say 10 million per
    // second, e.g. 7 million events more per seconds).
    // compared to a setup time/merging time of about 10 seconds per million
    // detectors.
    double setUpTime = double(detectorsize) * 10e-6;
    parallelProcessing = ((double(m_maxNumEvents) / 7e6) > setUpTime);
    g_log.information() << (parallelProcessing ? "Using" : "Not using")
                        << " parallel processing."
                        << "\n";
  }

  CPUTimer tim;

  // FIXME - Only serial mode supported for filtering events
  g_log.warning()
      << "Only serial mode is supported at this moment for filtering. "
      << "\n";

  // -------------------------------------------------------------------
  // Create the partial workspaces
  //--------------------------------------------------------------------
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

  // Processing by number of threads
  g_log.information() << "Processing input event preNexus by " << numThreads
                      << " threads"
                      << " in " << numBlocks << " blocks. "
                      << "\n";

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
        partWS->copyDataFrom(*localWorkspace);
        // Push it in the array
        partWorkspaces[i] = partWS;
      } else
        partWS = localWorkspace;

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
    } // END FOR [Threads]

    g_log.information() << tim << " to create " << partWorkspaces.size()
                        << " workspaces for parallel loading."
                        << "\n";

    prog->resetNumSteps(numBlocks, 0.1, 0.8);

    // -------------------------------------------------------------------
    // LOAD THE DATA
    //--------------------------------------------------------------------
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
        ws = localWorkspace;

      // Get the buffer (for this thread)
      DasEvent *event_buffer = buffers[threadNum];

      // Get the speeding-up array of vector<tofEvent> where index = detid.
      EventVector_pt *theseEventVectors = eventVectors[threadNum];

      // Where to start in the file?
      size_t fileOffset = first_event + (loadBlockSize * blockNum);
      // May need to reduce size of last (or only) block
      size_t current_event_buffer_size =
          (blockNum == int(numBlocks - 1))
              ? (m_maxNumEvents - (numBlocks - 1) * loadBlockSize)
              : loadBlockSize;

      // Load this chunk of event data (critical block)
      PARALLEL_CRITICAL(FilterEventsByLogValuePreNexus_fileAccess) {
        current_event_buffer_size = eventfile->loadBlockAt(
            event_buffer, fileOffset, current_event_buffer_size);
      }

      // This processes the events. Can be done in parallel!
      filterEventsLinear(ws, theseEventVectors, event_buffer,
                         current_event_buffer_size, fileOffset);

      // Report progress
      prog->report("Load Event PreNeXus");

      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    g_log.information() << tim << " to load the data." << std::endl;

    // -------------------------------------------------------------------
    // MERGE WORKSPACES BACK TOGETHER
    //--------------------------------------------------------------------
    if (parallelProcessing) {
      PARALLEL_START_INTERUPT_REGION
      prog->resetNumSteps(localWorkspace->getNumberHistograms(), 0.8, 0.95);

      size_t memoryCleared = 0;
      MemoryManager::Instance().releaseFreeMemory();

      // Merge all workspaces, index by index.
      PARALLEL_FOR_NO_WSP_CHECK()
      for (int iwi = 0; iwi < int(localWorkspace->getNumberHistograms());
           iwi++) {
        size_t wi = size_t(iwi);

        // The output event list.
        EventList &el = localWorkspace->getEventList(wi);
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

        // With TCMalloc, release memory when you accumulate enough to make
        // sense
        PARALLEL_CRITICAL(FilterEventsByLogValuePreNexus_trackMemory) {
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
      localWorkspace->deleteEmptyLists();

    prog->report("Setting proton charge");
    this->setProtonCharge(localWorkspace);
    g_log.debug() << tim << " to set the proton charge log." << std::endl;

    // Make sure the MRU is cleared
    localWorkspace->clearMRU();

    // Now, create a default X-vector for histogramming, with just 2 bins.
    Kernel::cow_ptr<MantidVec> axis;
    MantidVec &xRef = axis.access();
    xRef.resize(2);
    xRef[0] = shortest_tof - 1; // Just to make sure the bins hold it all
    xRef[1] = longest_tof + 1;
    localWorkspace->setAllX(axis);
    this->pixel_to_wkspindex.clear();

    // -------------------------------------------------------------------
    // Final message output
    //--------------------------------------------------------------------
    g_log.notice() << "Read " << num_good_events << " events + "
                   << num_error_events << " errors"
                   << ". Shortest TOF: " << shortest_tof
                   << " microsec; longest TOF: " << longest_tof << " microsec."
                   << "\n";

    std::set<PixelType>::iterator wit;
    for (wit = this->wrongdetids.begin(); wit != this->wrongdetids.end();
         ++wit) {
      g_log.notice() << "Wrong Detector ID : " << *wit << std::endl;
    }
    std::map<PixelType, size_t>::iterator git;
    for (git = this->wrongdetidmap.begin(); git != this->wrongdetidmap.end();
         ++git) {
      PixelType tmpid = git->first;
      size_t vindex = git->second;
      g_log.notice() << "Pixel " << tmpid << ":  Total number of events = "
                     << this->wrongdetid_pulsetimes[vindex].size() << std::endl;
    }

    return;
} // End of filterEvents

//----------------------------------------------------------------------------------------------
/** Linear-version of the procedure to process the event file properly.
  * @param workspace :: EventWorkspace to write to.
  * @param arrayOfVectors :: For speed up: this is an array, of size
 * detid_max+1, where the
  *        index is a pixel ID, and the value is a pointer to the
 * vector<tofEvent> in the given EventList.
  * @param event_buffer :: The buffer containing the DAS events
  * @param current_event_buffer_size :: The length of the given DAS buffer
  * @param fileOffset :: Value for an offset into the binary file
  */
void FilterEventsByLogValuePreNexus::filterEventsLinear(
    DataObjects::EventWorkspace_sptr & /*workspace*/,
    std::vector<TofEvent> **arrayOfVectors, DasEvent *event_buffer,
    size_t current_event_buffer_size, size_t fileOffset) {
  //----------------------------------------------------------------------------------
  // Set up parameters to process events from raw file
  //----------------------------------------------------------------------------------
  // Pulse ID and pulse time
  DateAndTime pulsetime;
  int64_t numPulses = static_cast<int64_t>(num_pulses);
  if (m_vecEventIndex.size() < num_pulses) {
    g_log.warning()
        << "Event_indices vector is smaller than the pulsetimes array.\n";
    numPulses = static_cast<int64_t>(m_vecEventIndex.size());
  }

  uint64_t maxeventid = m_vecEventIndex.back();
  g_log.notice() << "Maximum event index = " << maxeventid << " vs. "
                 << m_maxNumEvents << "\n";
  maxeventid = m_maxNumEvents + 1;

  size_t numbadeventindex = 0;

  // Declare local statistic parameters
  size_t local_num_error_events = 0;
  size_t local_num_bad_events = 0;
  size_t local_num_ignored_events = 0;
  size_t local_num_good_events = 0;
  double local_shortest_tof =
      static_cast<double>(MAX_TOF_UINT32) * TOF_CONVERSION;
  double local_longest_tof = 0.;

#if 0
    // NOT CALLED AT ALL
    // Local data structure for loaded events
    std::map<PixelType, size_t> local_pidindexmap;
    std::vector<std::vector<Kernel::DateAndTime> > local_pulsetimes;
    std::vector<std::vector<double> > local_tofs;
#endif

  //----------------------------------------------------------------------------------
  // Find out the filter-status
  //----------------------------------------------------------------------------------
  int filterstatus = -1;
  DateAndTime logpulsetime;
  // double logtof;
  bool definedfilterstatus = false;
  if (fileOffset == 0) {
    // First file loading chunk
    filterstatus = -1;
    definedfilterstatus = false;
  } else {
    size_t firstindex = 1234567890;
    for (size_t i = 0; i <= current_event_buffer_size; ++i) {
      DasEvent &tempevent = *(event_buffer + i);
      PixelType pixelid = tempevent.pid;
      if (pixelid == m_vecLogPixelID[0]) {
        filterstatus = -1;
        definedfilterstatus = true;
        firstindex = i;
        break;
      } else if (pixelid == m_vecLogPixelID[1]) {
        filterstatus = 1;
        definedfilterstatus = true;
        firstindex = i;
        break;
      }

      // g_log.notice() << "[DB] " << "Offset = " << fileOffset << ", i = " << i
      // << ", Pid = " << pixelid << "\n";
    }

    if (!definedfilterstatus) {
      g_log.error() << "File offset " << fileOffset
                    << " unable to find a previoiusly defined log event. "
                    << "\n";
    } else {
      g_log.warning() << "File offset " << fileOffset
                      << " 1-st event log at index = " << firstindex
                      << ", status = " << filterstatus << "\n";
    }
  }

  Instrument_const_sptr instrument = localWorkspace->getInstrument();
  if (!instrument)
    throw std::runtime_error("Instrument is not setup in localWorkspace.");
  IComponent_const_sptr source =
      boost::dynamic_pointer_cast<const IComponent>(instrument->getSource());
  if (!source)
    throw std::runtime_error("Source is not set up in local workspace.");
  double l1 = instrument->getDistance(*source);
  g_log.notice() << "[DB] L1 = " << l1 << "\n";

  //----------------------------------------------------------------------------------
  // process the individual events
  //----------------------------------------------------------------------------------
  bool firstlogevent = true;
  int64_t i_pulse = 0;
  int64_t boundtime(0);
  int64_t boundindex(0);
  int64_t prevbtime(0);
  PixelType boundpixel(0);

  for (size_t ievent = 0; ievent < current_event_buffer_size; ++ievent) {
    bool iswrongdetid = false;
    bool islogevent = false;

    // Load DasEvent
    DasEvent &tempevent = *(event_buffer + ievent);

    // DasEvetn's pixel ID
    PixelType pixelid = tempevent.pid;

    // Check Pixels IDs
    if ((pixelid & ERROR_PID) == ERROR_PID) {
      // Marked as bad
      local_num_error_events++;
      local_num_bad_events++;
      continue;
    } else {
      // Covert DAS Pixel ID to Mantid Pixel ID
      if (pixelid == 1073741843) {
        // downstream monitor pixel for SNAP
        pixelid = 1179648;
      } else if (this->using_mapping_file) {
        // Converted by pixel mapping file
        PixelType unmapped_pid = pixelid % this->numpixel;
        pixelid = this->pixelmap[unmapped_pid];
      }

      // Check special/wrong pixel IDs against max Detector ID
      if (pixelid > static_cast<PixelType>(detid_max)) {
        // Record the wrong/special ID
        if (pixelid == m_vecLogPixelID[0]) {
          if (firstlogevent && definedfilterstatus) {
            if (filterstatus != -1)
              g_log.error()
                  << "Pre-defined filter status is wrong of fileoffset = "
                  << fileOffset << " at index = " << ievent << "\n";
            firstlogevent = false;
          }
          filterstatus = 1;
          islogevent = true;
          boundindex = ievent;
          boundpixel = m_vecLogPixelID[0];
        } else if (pixelid == m_vecLogPixelID[1]) {
          if (firstlogevent && definedfilterstatus) {
            if (filterstatus != 1)
              g_log.error()
                  << "pre-defined filter status is wrong of fileoffset = "
                  << fileOffset << " at index = " << ievent << "\n";
            firstlogevent = false;
          }
          filterstatus = -1;
          islogevent = true;
          boundindex = ievent;
          boundpixel = m_vecLogPixelID[1];
        } else
          iswrongdetid = true;
      }
#if 1
      int64_t i_totaloffsetX = ievent + fileOffset;
      bool dbprint = (i_totaloffsetX == 23551354 || i_totaloffsetX == -117704);
      if (dbprint) {
        g_log.notice() << "[Special] ievent = " << i_totaloffsetX
                       << ", Filter status = " << filterstatus
                       << ", Prev-boundary-pixel = " << boundpixel << "\n";
      }
#endif

      // Check if this pid we want to load.
      if (loadOnlySomeSpectra && !iswrongdetid && !islogevent) {
        std::map<int64_t, bool>::iterator it;
        it = spectraLoadMap.find(pixelid);
        if (it == spectraLoadMap.end()) {
          // Pixel ID was not found, so the event is being ignored.
          continue;
        }
      }

      // Work with the events to be processed
      // Find the pulse time for this event index
      if (i_pulse < numPulses - istep) {
        // This is the total offset into the file
        size_t i_totaloffset = ievent + fileOffset;

        // Go through event_index until you find where the index increases to
        // encompass the current index.
        // Your pulse = the one before.
        uint64_t thiseventindex = m_vecEventIndex[i_pulse];
        uint64_t nexteventindex = m_vecEventIndex[i_pulse + istep];
        while (!((i_totaloffset >= thiseventindex) &&
                 (i_totaloffset < nexteventindex))) {
          i_pulse += istep;
          if (i_pulse >= (numPulses - istep))
            break;

          thiseventindex = nexteventindex;
          nexteventindex = m_vecEventIndex[i_pulse + istep];
        }

        // Save the pulse time at this index for creating those events
        pulsetime = pulsetimes[i_pulse];
      } // Find pulse time

      double tof = static_cast<double>(tempevent.tof) * TOF_CONVERSION;

#ifdef DBOUT
      // Can be modifed for other output purpose
      if (static_cast<int64_t>(m_examEventLog) && pixelid == m_pixelid2exam &&
          numeventswritten < m_numevents2write) {
        g_log.notice() << "[E] Event " << ievent << "\t: Pulse ID = " << i_pulse
                       << ", Pulse Time = " << pulsetime << ", TOF = " << tof
                       << ", Pixel ID = " << pixelid << "\n";
        ++numeventswritten;
      }
#endif

      int64_t abstime(0);
      bool reversestatus(false);
      if (islogevent) {
        // Record the log boundary time
        prevbtime = boundtime;
        boundtime =
            pulsetime.totalNanoseconds() + static_cast<int64_t>(tof * 1000);
        logpulsetime = pulsetime;
        // logtof = tof;
      } else {
        double factor(1.0);
        if (m_corretctTOF) {
          // Calculate TOF correction value
          IComponent_const_sptr det =
              boost::dynamic_pointer_cast<const IComponent>(
                  instrument->getDetector(pixelid));
          if (!det)
            throw std::runtime_error("Unable to get access to detector ");
          double l2 = instrument->getDistance(*det);
          factor = (l1) / (l1 + l2);
        }

        // Examine whether to revert the filter
        if (m_corretctTOF)
          abstime = pulsetime.totalNanoseconds() +
                    static_cast<int64_t>(tof * factor * 1000);
        else
          abstime =
              pulsetime.totalNanoseconds() + static_cast<int64_t>(tof * 1000);
        if (abstime < boundtime) {
          // In case that the boundary time is bigger (DAS' mistake), seek
          // previous one
          reversestatus = true;
#if 1
          if (dbprint)
            g_log.warning() << "Event " << ievent + fileOffset
                            << " is behind an event log though it is earlier.  "
                            << "Diff = " << boundtime - abstime << " ns \n";
#endif
        } else
#if 1
            if (dbprint)
          g_log.notice() << "[Special] Event " << ievent + fileOffset
                         << " Revert status = " << reversestatus
                         << ", Filter-status = " << filterstatus << "\n";
#endif
      }

      int currstatus = filterstatus;
      if (dbprint)
        g_log.notice() << "[Special] A Event " << ievent + fileOffset
                       << " Revert status = " << reversestatus
                       << ", current-status = " << currstatus
                       << ", Filter-status = " << filterstatus << "\n";
      if (reversestatus)
        currstatus = -filterstatus;
      if (dbprint)
        g_log.notice() << "[Special] B Event " << ievent + fileOffset
                       << " Revert status = " << reversestatus
                       << ", current-status = " << currstatus
                       << ", Filter-status = " << filterstatus << "\n";
      if (!iswrongdetid && !islogevent && currstatus > 0) {
// Event on REAL detector
#if 1
        if (dbprint)
          g_log.notice() << "[Special] ievent = " << i_totaloffsetX
                         << ", Filter In "
                         << "\n";
#endif

        // Update summary variable: shortest and longest TOF
        if (tof < local_shortest_tof)
          local_shortest_tof = tof;
        if (tof > local_longest_tof)
          local_longest_tof = tof;

// Add event to vector of events
// (This is equivalent to
// workspace->getEventList(this->pixel_to_wkspindex[pid]).addEventQuickly(event))
// (But should be faster as a bunch of these calls were cached.)
#if defined(__GNUC__) && !(defined(__INTEL_COMPILER)) && !(defined(__clang__))
        // This avoids a copy constructor call but is only available with GCC
        // (requires variadic templates)
        arrayOfVectors[pixelid]->emplace_back(tof, pulsetime);
#else
        arrayOfVectors[pixelid]->push_back(TofEvent(tof, pulsetime));
#endif

        ++local_num_good_events;

#ifdef DBOUT
        if (fileOffset == 0 && numeventsprint < 10) {
          g_log.notice() << "[E10] Event " << ievent
                         << "\t: Pulse ID = " << i_pulse
                         << ", Pulse Time = " << pulsetime << ", TOF = " << tof
                         << ", Pixel ID = " << pixelid << "\n";

          ++numeventsprint;
        }
#endif
        if ((m_useDBOutput && pixelid == m_dbPixelID) || dbprint) {
          g_log.notice() << "[Event_DB11A] Index = " << ievent + fileOffset
                         << ", AbsTime = " << abstime
                         << ", Pulse time = " << pulsetime << ", TOF = " << tof
                         << ", Bound Index = " << boundindex
                         << ", Boundary time = " << boundtime
                         << ", Prev Boundary time = " << prevbtime
                         << ", Boundary Pixel = " << boundpixel
                         << ", Pixell ID = " << pixelid << "\n";
        }
      } else {
#if 1
        if (dbprint)
          g_log.notice() << "[Special] ievent = " << i_totaloffsetX
                         << ", Filter Out "
                         << "\n";
#endif

        if ((m_useDBOutput && pixelid == m_dbPixelID) || dbprint) {
          g_log.notice() << "[Event_DB11B] Index = " << ievent + fileOffset
                         << ", AbsTime = " << abstime
                         << ", Pulse time = " << pulsetime << ", TOF = " << tof
                         << ", Bound Index = " << boundindex
                         << ", Boundary time = " << boundtime
                         << ", Prev Boundary Time = " << prevbtime
                         << ", Boundary Pixel = " << boundpixel
                         << ", Pixell ID = " << pixelid << "\n";
        }
#ifdef DBOUT
        // Special events/Wrong detector id
        // - get/add index of the entry in map
        std::map<PixelType, size_t>::iterator it;
        it = local_pidindexmap.find(pixelid);
        size_t theindex = 0;
        if (it == local_pidindexmap.end()) {
          // Initialize it!
          size_t newindex = local_pulsetimes.size();
          local_pidindexmap[pixelid] = newindex;

          std::vector<Kernel::DateAndTime> tempvectime;
          std::vector<double> temptofs;
          local_pulsetimes.push_back(tempvectime);
          local_tofs.push_back(temptofs);

          theindex = newindex;

          ++numwrongpid;
        } else {
          // existing
          theindex = it->second;
        }

        // Store pulse time and tof of this event
        local_pulsetimes[theindex].push_back(pulsetime);
        local_tofs[theindex].push_back(tof);
#else
        // Ignore all operation
        ;
#endif
      } // END-IF-ELSE: On Event's Pixel's Nature

    } // ENDIF (event is masked error)

  } // ENDFOR each event

  PARALLEL_CRITICAL(FilterEventsByLogValuePreNexus_global_statistics) {
    this->num_good_events += local_num_good_events;
    this->num_ignored_events += local_num_ignored_events;
    this->num_error_events += local_num_error_events;

    this->num_bad_events += local_num_bad_events;

    if (local_shortest_tof < shortest_tof)
      shortest_tof = local_shortest_tof;
    if (local_longest_tof > longest_tof)
      longest_tof = local_longest_tof;
  }

  g_log.notice() << "Encountered " << numbadeventindex << " bad event indexes"
                 << "\n";

  return;
} // FilterEventsLinearly

//----------------------------------------------------------------------------------------------
/** Set up instrument related parameters such as detector map and etc for
 * 'eventws'
  * and create a pixel-spectrum map
  * We want to pad out empty pixels: monitor
  */
size_t FilterEventsByLogValuePreNexus::padOutEmptyPixels(
    DataObjects::EventWorkspace_sptr eventws) {
  // Obtain detector map
  detid2det_map detector_map;
  eventws->getInstrument()->getDetectors(detector_map);

  // Determine maximum pixel id
  detid2det_map::iterator it;
  detid_max = 0; // seems like a safe lower bound
  for (it = detector_map.begin(); it != detector_map.end(); it++)
    if (it->first > detid_max)
      detid_max = it->first;

  // Pad all the pixels
  prog->report("Padding Pixels of workspace");
  this->pixel_to_wkspindex.reserve(
      detid_max + 1); // starting at zero up to and including detid_max
  // Set to zero
  this->pixel_to_wkspindex.assign(detid_max + 1, 0);

  // Set up the map between workspace index and pixel ID
  size_t workspaceIndex = 0;
  for (it = detector_map.begin(); it != detector_map.end(); it++) {
    if (!it->second->isMonitor()) {
      // Add non-monitor detector ID
      this->pixel_to_wkspindex[it->first] = workspaceIndex;

      // EventList & spec = workspace->getOrAddEventList(workspaceIndex);
      // spec.addDetectorID(it->first);
      // Start the spectrum number at 1
      // spec.setSpectrumNo(specid_t(workspaceIndex+1));
      workspaceIndex += 1;
    }
  }

  return detector_map.size();
}

//----------------------------------------------------------------------------------------------
/** Set up instrument related parameters such as detector map and etc for
 * 'eventws' create a
  * pixel-spectrum map
  */
void FilterEventsByLogValuePreNexus::setupPixelSpectrumMap(
    DataObjects::EventWorkspace_sptr eventws) {
  // Obtain detector map
  detid2det_map detector_map;
  eventws->getInstrument()->getDetectors(detector_map);

  // Set up
  for (detid2det_map::iterator it = detector_map.begin();
       it != detector_map.end(); it++) {
    if (!it->second->isMonitor()) {
      // Add non-monitor detector ID
      size_t workspaceIndex = pixel_to_wkspindex[it->first];
      // this->pixel_to_wkspindex[it->first] = workspaceIndex;
      EventList &spec = eventws->getOrAddEventList(workspaceIndex);
      spec.addDetectorID(it->first);
      // Start the spectrum number at 1
      spec.setSpectrumNo(specid_t(workspaceIndex + 1));
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Use pulse index/ event index to find out the frequency of instrument running
  */
int FilterEventsByLogValuePreNexus::findRunFrequency() {
  g_log.debug() << "Size of pulse / event index  = " << m_vecEventIndex.size()
                << "\n";

  size_t shortestsame = 100;
  size_t checksize = 1200;
  if (m_vecEventIndex.size() < checksize)
    checksize = m_vecEventIndex.size();

  uint64_t prev_event_index = m_vecEventIndex[0];
  size_t istart = 0;
  for (size_t i = 1; i < checksize; ++i) {
    uint64_t curr_event_index = m_vecEventIndex[i];
    if (curr_event_index > m_maxNumEvents)
      break;
    if (curr_event_index != prev_event_index) {
      size_t duration = i - istart;
      if (duration < shortestsame) {
        g_log.notice() << "istart = " << istart
                       << " w/ value = " << m_vecEventIndex[istart]
                       << ", icurr = " << i
                       << " w/ value = " << m_vecEventIndex[i] << "\n";
        shortestsame = duration;
      }
      prev_event_index = curr_event_index;
      istart = i;
    }
  }

  int freq = 60 / static_cast<int>(shortestsame);

  g_log.notice() << "Shortest duration = " << shortestsame << " ---> "
                 << "Operation frequency = " << freq << "\n";

  return freq;
}

//----------------------------------------------------------------------------------------------
/**
 * Add a sample environment log for the proton chage (charge of the pulse in
 *picoCoulombs)
 * and set the scalar value (total proton charge, microAmps*hours, on the
 *sample)
 *
 * @param workspace :: Event workspace to set the proton charge on
 */
void FilterEventsByLogValuePreNexus::setProtonCharge(
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

  // TODO set the units for the log
  run.addLogData(log);
  double integ = run.integrateProtonCharge();
  // run.setProtonCharge(this->proton_charge_tot); //This is now redundant
  this->g_log.information() << "Total proton charge of " << integ
                            << " microAmp*hours found by integrating.\n";

  return;
}

//----------------------------------------------------------------------------------------------
/** Load a pixel mapping file
 * @param filename :: Path to file.
 */
void FilterEventsByLogValuePreNexus::loadPixelMap(const std::string &filename) {
  this->using_mapping_file = false;
  this->pixelmap.clear();

  // check that there is a mapping file
  if (filename.empty()) {
    g_log.information("Pixel mapping file name is empty. Pixel map is not "
                      "loaded and thus empty. ");
    return;
  }

  // actually deal with the file
  this->g_log.information("Using mapping file \"" + filename + "\"");

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

  return;
}

//-----------------------------------------------------------------------------
/** Open an event file
 * @param filename :: file to open.
 */
void
FilterEventsByLogValuePreNexus::openEventFile(const std::string &filename) {
  // Open the file
  eventfile = new BinaryFile<DasEvent>(filename);
  num_events = eventfile->getNumElements();
  g_log.debug() << "File contains " << num_events << " event records.\n";

  // Check if we are only loading part of the event file
  const int chunk = getProperty("ChunkNumber");
  if (isEmpty(chunk)) // We are loading the whole file
  {
    first_event = 0;
    m_maxNumEvents = num_events;
  } else // We are loading part - work out the event number range
  {
    const int totalChunks = getProperty("TotalChunks");
    m_maxNumEvents = num_events / totalChunks;
    first_event = (chunk - 1) * m_maxNumEvents;
    // Need to add any remainder to the final chunk
    if (chunk == totalChunks)
      m_maxNumEvents += num_events % totalChunks;
  }

  g_log.information() << "Reading " << m_maxNumEvents << " event records\n";
}

//-----------------------------------------------------------------------------
/** Read a pulse ID file
 * @param filename :: file to load.
 * @param throwError :: Flag to trigger error throwing instead of just logging
 */
void
FilterEventsByLogValuePreNexus::readPulseidFile(const std::string &filename,
                                                const bool throwError) {
  this->proton_charge_tot = 0.;
  this->num_pulses = 0;
  this->pulsetimesincreasing = true;

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
    DateAndTime lastPulseDateTime(0, 0);
    this->pulsetimes.reserve(num_pulses);
    for (size_t i = 0; i < num_pulses; i++) {
      Pulse &it = (*pulses)[i];
      DateAndTime pulseDateTime((int64_t)it.seconds, (int64_t)it.nanoseconds);
      this->pulsetimes.push_back(pulseDateTime);
      this->m_vecEventIndex.push_back(it.event_index);

      if (pulseDateTime < lastPulseDateTime)
        this->pulsetimesincreasing = false;
      else
        lastPulseDateTime = pulseDateTime;

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
