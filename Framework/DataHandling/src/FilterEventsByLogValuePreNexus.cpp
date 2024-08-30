// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/FilterEventsByLogValuePreNexus.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BinaryFile.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/FileValidator.h"
#include "MantidKernel/Glob.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VisibleWhenProperty.h"

#if BOOST_VERSION < 107100
#include <boost/timer.hpp>
#else
#include <boost/timer/timer.hpp>
#endif

#include <Poco/File.h>
#include <Poco/Path.h>

#include <algorithm>
#include <functional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

// #define DBOUT

namespace Mantid::DataHandling {

DECLARE_FILELOADER_ALGORITHM(FilterEventsByLogValuePreNexus)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace Geometry;
using DataObjects::EventList;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_sptr;
using std::ifstream;
using std::runtime_error;
using std::string;
using std::stringstream;
using std::vector;
using Types::Core::DateAndTime;
using Types::Event::TofEvent;

//----------------------------------------------------------------------------------------------
// constants for locating the parameters to use in execution
//----------------------------------------------------------------------------------------------
static const string EVENT_PARAM("EventFilename");
static const string PULSEID_PARAM("PulseidFilename");
static const string MAP_PARAM("MappingFilename");
static const string PID_PARAM("SpectrumList");
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

static const string EVENT_EXTS[] = {"_neutron_event.dat",  "_neutron0_event.dat", "_neutron1_event.dat",
                                    "_neutron2_event.dat", "_neutron3_event.dat", "_live_neutron_event.dat"};
static const string PULSE_EXTS[] = {"_pulseid.dat",  "_pulseid0.dat", "_pulseid1.dat",
                                    "_pulseid2.dat", "_pulseid3.dat", "_live_pulseid.dat"};
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

  std::size_t left = runnumber.find('_');
  std::size_t right = runnumber.find('_', left + 1);

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
  std::vector<string> temp = wksp->getInstrument()->getStringParameter("TS_mapping_file");
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
  for (auto &dir : dirs) {
    if ((dir.length() > CAL_LEN) && (dir.compare(dir.length() - CAL.length(), CAL.length(), CAL) == 0)) {
      std::string path = std::string(base.path()).append("/").append(dir).append("/calibrations/").append(mapping);
      if (Poco::File(path).exists()) {
        files.emplace_back(path);
      }
    }
  }

  if (files.empty())
    return std::string();
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
    : Mantid::API::IFileLoader<Kernel::FileDescriptor>(), m_protonChargeTot(0), m_detid_max(0), m_eventFile(nullptr),
      m_numEvents(0), m_numPulses(0), m_numPixel(0), m_numGoodEvents(0), m_numErrorEvents(0), m_numBadEvents(0),
      m_numWrongdetidEvents(0), m_numIgnoredEvents(0), m_firstEvent(0), m_maxNumEvents(0), m_usingMappingFile(false),
      m_loadOnlySomeSpectra(false), m_longestTof(0.0), m_shortestTof(0.0), m_parallelProcessing(false),
      m_pulseTimesIncreasing(false), m_throwError(true), m_examEventLog(false), m_pixelid2exam(0), m_numevents2write(0),
      m_freqHz(0), m_istep(0), m_dbPixelID(0), m_useDBOutput(false), m_corretctTOF(false) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
FilterEventsByLogValuePreNexus::~FilterEventsByLogValuePreNexus() = default;

//----------------------------------------------------------------------------------------------
/** Return the confidence with with this algorithm can load the file
 *  @param descriptor A descriptor for the file
 *  @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int FilterEventsByLogValuePreNexus::confidence(Kernel::FileDescriptor &descriptor) const {
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
  const auto filesize = static_cast<size_t>(handle.tellg());
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
  declareProperty(std::make_unique<FileProperty>(EVENT_PARAM, "", FileProperty::Load, eventExts),
                  "The name of the neutron event file to read, including its full or "
                  "relative path. In most cases, the file typically ends in "
                  "neutron_event.dat (N.B. case sensitive if running on Linux).");
  vector<string> pulseExts(PULSE_EXTS, PULSE_EXTS + NUM_EXT);
  declareProperty(std::make_unique<FileProperty>(PULSEID_PARAM, "", FileProperty::OptionalLoad, pulseExts),
                  "File containing the accelerator pulse information; the "
                  "filename will be found automatically if not specified.");
  declareProperty(std::make_unique<FileProperty>(MAP_PARAM, "", FileProperty::OptionalLoad, ".dat"),
                  "File containing the pixel mapping (DAS pixels to pixel IDs) file "
                  "(typically INSTRUMENT_TS_YYYY_MM_DD.dat). The filename will be found "
                  "automatically if not specified.");

  // Pixels to load
  declareProperty(std::make_unique<ArrayProperty<int64_t>>(PID_PARAM),
                  "A list of individual spectra (pixel IDs) to read, specified "
                  "as e.g. 10:20. Only used if set.");

  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
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
  setPropertySettings("TotalChunks", std::make_unique<VisibleWhenProperty>("ChunkNumber", IS_NOT_DEFAULT));

  // Loading option
  std::vector<std::string> propOptions{"Auto", "Serial", "Parallel"};
  declareProperty("UseParallelProcessing", "Auto", std::make_shared<StringListValidator>(propOptions),
                  "Use multiple cores for loading the data?\n"
                  "  Auto: Use serial loading for small data sets, parallel "
                  "for large data sets.\n"
                  "  Serial: Use a single core.\n"
                  "  Parallel: Use all available cores.");

  // the output workspace name
  declareProperty(std::make_unique<WorkspaceProperty<IEventWorkspace>>(OUT_PARAM, "", Direction::Output),
                  "The name of the workspace that will be created, filled with the read-in "
                  "data and stored in the [[Analysis Data Service]].");

  // Optional output table workspace
  declareProperty(
      std::make_unique<WorkspaceProperty<ITableWorkspace>>("EventLogTableWorkspace", "", PropertyMode::Optional),
      "Optional output table workspace containing the event log "
      "(pixel) information. ");

  //
  std::vector<std::string> vecfunmode{"LoadData", "Filter", "ExamineEventLog"};
  declareProperty("FunctionMode", "LoadData", std::make_shared<StringListValidator>(vecfunmode),
                  "Function mode for different purpose. ");

  declareProperty("PixelIDtoExamine", EMPTY_INT(), "Pixel ID for the events to be examined. ");

  declareProperty("NumberOfEventsToExamine", EMPTY_INT(), "Number of events on the pixel ID to get examined. ");

  declareProperty(std::make_unique<ArrayProperty<int>>("LogPixelIDs"),
                  "Pixel IDs for event log. Must have 2 (or more) entries. ");

  declareProperty(std::make_unique<ArrayProperty<std::string>>("LogPIxelTags"),
                  "Pixel ID tags for event log. Must have same items as 'LogPixelIDs'. ");

  declareProperty("AcceleratorFrequency", 60,
                  "Freuqency of the accelerator at "
                  "which the experiment runs. It "
                  "can 20, 30 or 60.");

  declareProperty("CorrectTOFtoSample", false, "Correct TOF to sample position. ");

  declareProperty("DBPixelID", EMPTY_INT(), "ID of the pixel (detector) for debug output. ");
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
  m_progress = std::make_unique<Progress>(this, 0.0, 1.0, 100);
  processProperties();

  // Read input files
  m_progress->report("Loading Pulse ID file");
  readPulseidFile(m_pulseIDFileName, m_throwError);

  m_progress->report("Loading Event File");
  openEventFile(m_eventFileName);

  // Correct wrong event index in loaded eventindexes
  unmaskVetoEventIndexes();

  // Find out the frequency of the frequency
  int runfreq = findRunFrequency();
  if (m_freqHz != runfreq) {
    if (m_freqHz % runfreq == 0) {
      int frame = m_freqHz / runfreq;
      g_log.warning() << "Input frequency " << m_freqHz << " is different from data. "
                      << "It is forced to use input frequency, while all "
                         "events' pulse time will be "
                      << "set to " << frame << "-th freme. "
                      << "\n";
    } else {
      throw std::runtime_error("Operation frequency is not self-consistent");
    }
  }
  m_istep = 60 / m_freqHz;

  // Create and set up output EventWorkspace
  m_localWorkspace = setupOutputEventWorkspace();
  if (m_functionMode == "Filter")
    m_localWorkspaceBA = setupOutputEventWorkspace();

  // Process the events into pixels
  if (m_functionMode == "Filter") {
    filterEvents();
  } else {
    procEvents(m_localWorkspace);
  }

  // set that the sort order on the event lists
  if (this->m_numPulses > 0 && this->m_pulseTimesIncreasing) {
    const int64_t numberOfSpectra = m_localWorkspace->getNumberHistograms();
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int64_t i = 0; i < numberOfSpectra; i++) {
      PARALLEL_START_INTERRUPT_REGION
      m_localWorkspace->getSpectrum(i).setSortOrder(DataObjects::PULSETIME_SORT);
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION
  }

  // Save output
  setProperty<IEventWorkspace_sptr>(OUT_PARAM, m_localWorkspace);
  if (m_functionMode == "Filter") {
    declareProperty(
        std::make_unique<WorkspaceProperty<IEventWorkspace>>("OutputFilteredWorkspace", "WS_A", Direction::Output), "");
    setProperty<IEventWorkspace_sptr>("OutputFilteredWorkspace", m_localWorkspaceBA);
  }

  // Add fast frequency sample environment (events) data to workspace's log
  processEventLogs();

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
  this->m_spectraList = this->getProperty(PID_PARAM);

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
        g_log.information() << "Found pulseid file " << m_pulseIDFileName << "\n";
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
      g_log.warning() << "In functional mode ExamineEventLog, pixel ID must be given!"
                      << "\n";
      throw std::runtime_error("Incorrect input.");
    }

    m_examEventLog = true;
  } else if (m_functionMode == "Filter") {
    m_vecLogPixelID = getProperty("LogPixelIDs");
    m_vecLogPixelTag = getProperty("LogPIxelTags");

    if (m_vecLogPixelID.size() < 2) {
      throw std::runtime_error("Input log pixel IDs must have more than 2 entries. ");
    } else if (m_vecLogPixelID.size() != m_vecLogPixelTag.size()) {
      throw std::runtime_error("Input log pixel tags must have the same number of items as "
                               "log pixe IDs. ");
    }
  }

  //---------------------------------------------------------------------------
  // Load partial spectra
  //---------------------------------------------------------------------------
  // For slight speed up
  m_loadOnlySomeSpectra = (!this->m_spectraList.empty());

  // Turn the spectra list into a map, for speed of access
  for (auto spectra : m_spectraList)
    spectraLoadMap[spectra] = true;

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
} // END of processProperties

//----------------------------------------------------------------------------------------------
/** Create, initialize and set up output EventWorkspace
 */
DataObjects::EventWorkspace_sptr FilterEventsByLogValuePreNexus::setupOutputEventWorkspace() {
  // Create and initialize output EventWorkspace
  m_progress->report("Creating output workspace");

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
  if (this->m_numPulses > 0) {
// add the start of the run as a ISO8601 date/time string. The start = the first
// pulse.
// (this is used in LoadInstrument to find the right instrument file to use).
#if 0
      DateAndTime pulse0 = pulsetimes[0];
      g_log.notice() << "Pulse time 0 = " <<  pulse0.totalNanoseconds() << "\n";
#endif
    tempworkspace->mutableRun().addProperty("run_start", pulsetimes[0].toISO8601String(), true);
  }

  //   the run number and add it to the run object
  tempworkspace->mutableRun().addProperty("run_number", getRunnumber(m_eventFileName));

  // Add the instrument!
  m_progress->report("Loading Instrument");
  this->runLoadInstrument(m_eventFileName, tempworkspace);

  // Load the mapping file
  m_progress->report("Loading Mapping File");
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

  // Create workspace of correct size
  // Number of non-monitors in instrument
  size_t nSpec = tempworkspace->getInstrument()->getDetectorIDs(true).size();
  if (!this->m_spectraList.empty())
    nSpec = this->m_spectraList.size();
  auto ws = createWorkspace<EventWorkspace>(nSpec, 2, 1);
  WorkspaceFactory::Instance().initializeFromParent(*tempworkspace, *ws, true);

  return ws;
}

//----------------------------------------------------------------------------------------------
/** Process imbed logs (marked by bad pixel IDs)
 * (1) Add special event log to workspace log
 * (2) (Optionally) do statistic to each pixel
 * (3) (Optionally) write out information
 */
void FilterEventsByLogValuePreNexus::processEventLogs() {
  std::map<PixelType, size_t>::iterator mit;
  for (const auto pid : this->wrongdetids) {
    // Convert Pixel ID to 'wrong detectors ID' map's index
    mit = this->wrongdetidmap.find(pid);
    size_t mindex = mit->second;
    if (mindex > this->wrongdetid_pulsetimes.size()) {
      g_log.error() << "Wrong Index " << mindex << " for Pixel " << pid << '\n';
      throw std::invalid_argument("Wrong array index for pixel from map");
    } else {
      g_log.information() << "Processing imbed log marked by Pixel " << pid
                          << " with size = " << this->wrongdetid_pulsetimes[mindex].size() << '\n';
    }

    // Generate the log name
    std::stringstream ssname;
    ssname << "Pixel" << pid;
    std::string logname = ssname.str();

    // Add this map entry to log
    addToWorkspaceLog(logname, mindex);

    // Do some statistic to this event log
    doStatToEventLog(mindex);

    g_log.information() << "Added Log " << logname << " to output workspace. \n";

  } // ENDFOR pit

  // Output table workspace
  std::string evlog = getPropertyValue("EventLogTableWorkspace");
  if (!evlog.empty()) {
    // Initialize table workspace
    TableWorkspace_sptr evtablews = std::make_shared<TableWorkspace>();
    evtablews->addColumn("int", "Pixel-ID");
    evtablews->addColumn("int", "NumberOfEvents");

    // Add information rows
    std::map<PixelType, size_t>::iterator git;
    for (git = this->wrongdetidmap.begin(); git != this->wrongdetidmap.end(); ++git) {
      PixelType tmpid = git->first;
      size_t vindex = git->second;

      TableRow temprow = evtablews->appendRow();
      temprow << static_cast<int>(tmpid) << static_cast<int>(wrongdetid_pulsetimes[vindex].size());
    }

    // Set property
    setProperty("EventLogTableWorkspace", std::dynamic_pointer_cast<ITableWorkspace>(evtablews));
  }
}

//----------------------------------------------------------------------------------------------
/** Add absolute time series to log
 * @param logtitle :: title of the log to be inserted to workspace
 * @param mindex ::  index of the series in the wrong detectors map
 */
void FilterEventsByLogValuePreNexus::addToWorkspaceLog(const std::string &logtitle, size_t mindex) {
  // Create TimeSeriesProperty
  auto property = new TimeSeriesProperty<double>(logtitle);

  // Add entries
  size_t nbins = this->wrongdetid_pulsetimes[mindex].size();
  for (size_t k = 0; k < nbins; k++) {
    double tof = this->wrongdetid_tofs[mindex][k];
    DateAndTime pulsetime = wrongdetid_pulsetimes[mindex][k];
    int64_t abstime_ns = pulsetime.totalNanoseconds() + static_cast<int64_t>(tof * 1000);
    DateAndTime abstime(abstime_ns);

    double value = tof;

    property->addValue(abstime, value);
  } // ENDFOR

  g_log.information() << "Size of Property " << property->name() << " = " << property->size()
                      << " vs Original Log Size = " << nbins << "\n";

  // Add property to workspace
  m_localWorkspace->mutableRun().addProperty(std::move(property), false);
}

//----------------------------------------------------------------------------------------------
/** Perform statistics to event (wrong pixel ID) logs
 * @param mindex ::  index of the series in the list
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
    int64_t templogtime = ptime.totalNanoseconds() + static_cast<int64_t>(wrongdetid_tofs[mindex][i] * 1000.);

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

  if (nbins - 1) {
    double avg_dt = static_cast<double>(sum_dt) / static_cast<double>(nbins - 1);
    g_log.information() << "Event log of map index " << mindex << ": Avg(dt) = " << avg_dt * 1.0E-9
                        << ", Min(dt) = " << static_cast<double>(min_dt) * 1.0E-9
                        << ", Max(dt) = " << static_cast<double>(max_dt) * 1.0E-9 << "\n";
  } else {
    g_log.information() << "Event log of map index " << mindex << ": Avg(dt) = " << static_cast<double>(sum_dt) * 1.0E-9
                        << ", Min(dt) = " << static_cast<double>(min_dt) * 1.0E-9
                        << ", Max(dt) = " << static_cast<double>(max_dt) * 1.0E-9 << "\n";
  }

  g_log.information() << "Number of zero-interval eveng log = " << numzeros << "\n";
}

//-----------------------------------------------------------------------------
/** Load the instrument geometry File
 *  @param eventfilename :: Used to pick the instrument.
 *  @param localWorkspace :: MatrixWorkspace in which to put the instrument
 *  geometry
 */
void FilterEventsByLogValuePreNexus::runLoadInstrument(const std::string &eventfilename,
                                                       const MatrixWorkspace_sptr &localWorkspace) {
  // start by getting just the filename
  string instrument = Poco::Path(eventfilename).getFileName();

  // initialize vector of endings and put live at the beginning
  vector<string> eventExts(EVENT_EXTS, EVENT_EXTS + NUM_EXT);
  std::reverse(eventExts.begin(), eventExts.end());

  for (auto &eventExt : eventExts) {
    size_t pos = instrument.find(eventExt);
    if (pos != string::npos) {
      instrument = instrument.substr(0, pos);
      break;
    }
  }

  // determine the instrument parameter file
  size_t pos = instrument.rfind('_'); // get rid of the run number
  instrument = instrument.substr(0, pos);

  // do the actual work
  auto loadInst = createChildAlgorithm("LoadInstrument");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  loadInst->setPropertyValue("InstrumentName", instrument);
  loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", localWorkspace);
  loadInst->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(false));
  loadInst->executeAsChildAlg();

  // Populate the instrument parameters in this workspace - this works around a
  // bug
  localWorkspace->populateInstrumentParameters();
}

//----------------------------------------------------------------------------------------------
/** Process the event file properly.
 * @param workspace :: EventWorkspace to write to.
 */
void FilterEventsByLogValuePreNexus::procEvents(DataObjects::EventWorkspace_sptr &workspace) {
  // Initialize stat parameters
  this->m_numErrorEvents = 0;
  this->m_numGoodEvents = 0;
  this->m_numIgnoredEvents = 0;
  this->m_numBadEvents = 0;
  this->m_numWrongdetidEvents = 0;

  m_shortestTof = static_cast<double>(MAX_TOF_UINT32) * TOF_CONVERSION;
  m_longestTof = 0.;

  // -------------------------------------------------------------------
  // Set up instrument related parameters such as detector map and etc.
  // We want to pad out empty pixels.
  //--------------------------------------------------------------------
  const auto &detectorInfo = workspace->detectorInfo();
  const auto &detIDs = detectorInfo.detectorIDs();

  // Determine maximum pixel id
  const auto it = std::max_element(detIDs.cbegin(), detIDs.cend());
  m_detid_max = it == detIDs.cend() ? 0 : *it; // in case detIDs is empty

  // Pad all the pixels
  m_progress->report("Padding Pixels");
  this->m_pixelToWkspindex.reserve(m_detid_max + 1); // starting at zero up to and including m_detid_max
  // Set to zero
  this->m_pixelToWkspindex.assign(m_detid_max + 1, 0);
  size_t workspaceIndex = 0;
  specnum_t spectrumNumber = 1;
  for (size_t i = 0; i < detectorInfo.size(); ++i) {
    if (!detectorInfo.isMonitor(i)) {
      if (!m_loadOnlySomeSpectra || (spectraLoadMap.find(detIDs[i]) != spectraLoadMap.end())) {
        // Add non-monitor detector ID
        this->m_pixelToWkspindex[detIDs[i]] = workspaceIndex;
        EventList &spec = workspace->getSpectrum(workspaceIndex);
        spec.addDetectorID(detIDs[i]);
        // Start the spectrum number at 1
        spec.setSpectrumNo(spectrumNumber);
        workspaceIndex += 1;
        ++workspaceIndex;
      } else {
        this->m_pixelToWkspindex[detIDs[i]] = -1;
      }
      ++spectrumNumber;
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
    m_parallelProcessing = false;
  } else if (procMode == "Parallel") {
    m_parallelProcessing = true;
  } else {
    // Automatic determination. Loading serially (for me) is about 3 million
    // events per second,
    // (which is sped up by ~ x 3 with parallel processing, say 10 million per
    // second, e.g. 7 million events more per seconds).
    // compared to a setup time/merging time of about 10 seconds per million
    // detectors.
    double setUpTime = double(detectorInfo.size()) * 10e-6;
    m_parallelProcessing = ((double(m_maxNumEvents) / 7e6) > setUpTime);
    g_log.information() << (m_parallelProcessing ? "Using" : "Not using") << " parallel processing."
                        << "\n";
  }

  if (m_functionMode == "ExamineEventLog" && m_parallelProcessing) {
    m_parallelProcessing = false;
    g_log.notice("In function mode 'ExamineEventLog', processing mode is "
                 "forced to serial. ");
  }

#if 0
    //For slight speed up
    m_loadOnlySomeSpectra = (this->m_spectraList.size() > 0);

    //Turn the spectra list into a map, for speed of access
    for (std::vector<int64_t>::iterator it = m_spectraList.begin(); it != m_spectraList.end(); it++)
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
  using EventVector_pt = std::vector<TofEvent> *;
  /// Bare array of arrays of pointers to the EventVectors
  EventVector_pt **eventVectors;

  /// How many threads will we use?
  size_t numThreads = 1;
  if (m_parallelProcessing)
    numThreads = size_t(PARALLEL_GET_MAX_THREADS);

  partWorkspaces.resize(numThreads);
  buffers.resize(numThreads);
  eventVectors = new EventVector_pt *[numThreads];

  // Processing by number of threads
  g_log.information() << "Processing input event preNexus by " << numThreads << " threads"
                      << " in " << numBlocks << " blocks. "
                      << "\n";

    PRAGMA_OMP( parallel for schedule(dynamic, 1) if (m_parallelProcessing) )
    for (int i = 0; i < int(numThreads); i++) {
      // This is the partial workspace we are about to create (if in parallel)
      EventWorkspace_sptr partWS;

      if (m_parallelProcessing) {
        m_progress->report("Creating Partial Workspace");
        // Create a partial workspace, copy all the spectra numbers and stuff
        // (no actual events to copy though).
        partWS = workspace->clone();
        // Push it in the array
        partWorkspaces[i] = partWS;
      } else
        partWS = workspace;

      // Allocate the buffers
      buffers[i] = new DasEvent[loadBlockSize];

      // For each partial workspace, make an array where index = detector ID and
      // value = pointer to the events vector
      eventVectors[i] = new EventVector_pt[m_detid_max + 1];
      EventVector_pt *theseEventVectors = eventVectors[i];
      for (detid_t j = 0; j < m_detid_max + 1; ++j) {
        size_t wi = m_pixelToWkspindex[j];
        // Save a POINTER to the vector<tofEvent>
        theseEventVectors[j] = &partWS->getSpectrum(wi).getEvents();
      }
    } // END FOR [Threads]

    g_log.information() << tim << " to create " << partWorkspaces.size() << " workspaces for parallel loading."
                        << "\n";

    m_progress->resetNumSteps(numBlocks, 0.1, 0.8);

    // -------------------------------------------------------------------
    // LOAD THE DATA
    //--------------------------------------------------------------------
    PRAGMA_OMP( parallel for schedule(dynamic, 1) if (m_parallelProcessing) )
    for (int blockNum = 0; blockNum < int(numBlocks); blockNum++) {
      PARALLEL_START_INTERRUPT_REGION

      // Find the workspace for this particular thread
      EventWorkspace_sptr ws;
      size_t threadNum = 0;
      if (m_parallelProcessing) {
        threadNum = PARALLEL_THREAD_NUMBER;
        ws = partWorkspaces[threadNum];
      } else
        ws = workspace;

      // Get the buffer (for this thread)
      DasEvent *event_buffer = buffers[threadNum];

      // Get the speeding-up array of vector<tofEvent> where index = detid.
      EventVector_pt *theseEventVectors = eventVectors[threadNum];

      // Where to start in the file?
      size_t fileOffset = m_firstEvent + (loadBlockSize * blockNum);
      // May need to reduce size of last (or only) block
      size_t current_event_buffer_size =
          (blockNum == int(numBlocks - 1)) ? (m_maxNumEvents - (numBlocks - 1) * loadBlockSize) : loadBlockSize;

      // Load this chunk of event data (critical block)
      PARALLEL_CRITICAL(FilterEventsByLogValuePreNexus_fileAccess) {
        current_event_buffer_size = m_eventFile->loadBlockAt(event_buffer, fileOffset, current_event_buffer_size);
      }

      // This processes the events. Can be done in parallel!
      procEventsLinear(ws, theseEventVectors, event_buffer, current_event_buffer_size, fileOffset);

      // Report progress
      m_progress->report("Load Event PreNeXus");

      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION

    g_log.information() << tim << " to load the data.\n";

    // -------------------------------------------------------------------
    // MERGE WORKSPACES BACK TOGETHER
    //--------------------------------------------------------------------
    if (m_parallelProcessing) {
      PARALLEL_START_INTERRUPT_REGION
      m_progress->resetNumSteps(workspace->getNumberHistograms(), 0.8, 0.95);

      // Merge all workspaces, index by index.
      PARALLEL_FOR_NO_WSP_CHECK()
      for (int iwi = 0; iwi < int(workspace->getNumberHistograms()); iwi++) {
        auto wi = size_t(iwi);

        // The output event list.
        EventList &el = workspace->getSpectrum(wi);
        el.clear(false);

        // How many events will it have?
        size_t numEvents = 0;
        for (size_t i = 0; i < numThreads; i++)
          numEvents += partWorkspaces[i]->getSpectrum(wi).getNumberEvents();
        // This will avoid too much copying.
        el.reserve(numEvents);

        // Now merge the event lists
        for (size_t i = 0; i < numThreads; i++) {
          EventList &partEl = partWorkspaces[i]->getSpectrum(wi);
          el += partEl.getEvents();
          // Free up memory as you go along.
          partEl.clear(false);
        }
        m_progress->report("Merging Workspaces");
      }

      g_log.debug() << tim << " to merge workspaces together.\n";
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION

    // Delete the buffers for each thread.
    for (size_t i = 0; i < numThreads; i++) {
      delete[] buffers[i];
      delete[] eventVectors[i];
    }
    delete[] eventVectors;

    m_progress->resetNumSteps(3, 0.94, 1.00);

    // finalize loading
    m_progress->report("Setting proton charge");
    this->setProtonCharge(workspace);
    g_log.debug() << tim << " to set the proton charge log.\n";

    // Make sure the MRU is cleared
    workspace->clearMRU();

    // Now, create a default X-vector for histogramming, with just 2 bins.
    auto axis = HistogramData::BinEdges{m_shortestTof - 1, m_longestTof + 1};
    workspace->setAllX(axis);
    this->m_pixelToWkspindex.clear();

    // -------------------------------------------------------------------
    // Final message output
    //--------------------------------------------------------------------
    g_log.notice() << "Read " << m_numGoodEvents << " events + " << m_numErrorEvents << " errors"
                   << ". Shortest TOF: " << m_shortestTof << " microsec; longest TOF: " << m_longestTof << " microsec."
                   << "\n"
                   << "Bad Events = " << m_numBadEvents << "  Events of Wrong Detector = " << m_numWrongdetidEvents
                   << "\n"
                   << "Number of Wrong Detector IDs = " << wrongdetids.size() << "\n";

    for (const auto pid : this->wrongdetids) {
      g_log.notice() << "Wrong Detector ID : " << pid << '\n';
    }
    for (const auto &detidPair : wrongdetidmap) {
      PixelType tmpid = detidPair.first;
      size_t vindex = detidPair.second;
      g_log.notice() << "Pixel " << tmpid
                     << ":  Total number of events = " << this->wrongdetid_pulsetimes[vindex].size() << '\n';
    }
} // End of procEvents

//----------------------------------------------------------------------------------------------
/** Linear-version of the procedure to process the event file properly.
 * @param workspace :: EventWorkspace to write to.
 * @param arrayOfVectors :: For speed up: this is an array, of size
 * m_detid_max+1, where the
 *        index is a pixel ID, and the value is a pointer to the
 * vector<tofEvent> in the given EventList.
 * @param event_buffer :: The buffer containing the DAS events
 * @param current_event_buffer_size :: The length of the given DAS buffer
 * @param fileOffset :: Value for an offset into the binary file
 */
void FilterEventsByLogValuePreNexus::procEventsLinear(DataObjects::EventWorkspace_sptr & /*workspace*/,
                                                      std::vector<TofEvent> **arrayOfVectors, DasEvent *event_buffer,
                                                      size_t current_event_buffer_size, size_t fileOffset) {
  //----------------------------------------------------------------------------------
  // Set up parameters to process events from raw file
  //----------------------------------------------------------------------------------
  // Pulse ID and pulse time
  DateAndTime pulsetime;
  auto numPulses = static_cast<int64_t>(m_numPulses);
  if (m_vecEventIndex.size() < m_numPulses) {
    g_log.warning() << "Event_indices vector is smaller than the pulsetimes array.\n";
    numPulses = static_cast<int64_t>(m_vecEventIndex.size());
  }

  uint64_t maxeventid = m_vecEventIndex.back();
  g_log.debug() << "Maximum event index = " << maxeventid << " vs. " << m_maxNumEvents << "\n";
  maxeventid = m_maxNumEvents + 1;

  int numeventswritten = 0;

  // Declare local statistic parameters
  size_t local_numErrorEvents = 0;
  size_t local_numBadEvents = 0;
  size_t local_numIgnoredEvents = 0;
  size_t local_numWrongdetidEvents = 0;
  size_t local_numGoodEvents = 0;
  double local_m_shortestTof = static_cast<double>(MAX_TOF_UINT32) * TOF_CONVERSION;
  double local_m_longestTof = 0.;

  // Local data structure for loaded events
  std::map<PixelType, size_t> local_pidindexmap;
  std::vector<std::vector<Types::Core::DateAndTime>> local_pulsetimes;
  std::vector<std::vector<double>> local_tofs;

  std::set<PixelType> local_wrongdetids;
  size_t numwrongpid = 0;

  //----------------------------------------------------------------------------------
  // process the individual events
  //----------------------------------------------------------------------------------
  int64_t i_pulse = 0;

  for (size_t ievent = 0; ievent < current_event_buffer_size; ++ievent) {
    // Load DasEvent
    DasEvent &tempevent = *(event_buffer + ievent);

    // DasEvetn's pixel ID
    PixelType pixelid = tempevent.pid;

    // Check Pixels IDs
    if ((pixelid & ERROR_PID) == ERROR_PID) {
      // Marked as bad
      local_numErrorEvents++;
      local_numBadEvents++;
      continue;
    } else {
      // Covert DAS Pixel ID to Mantid Pixel ID
      if (pixelid == 1073741843) {
        // downstream monitor pixel for SNAP
        pixelid = 1179648;
      } else if (this->m_usingMappingFile) {
        // Converted by pixel mapping file
        PixelType unmapped_pid = pixelid % this->m_numPixel;
        pixelid = this->m_pixelmap[unmapped_pid];
      }

      bool iswrongdetid = false;
      // Check special/wrong pixel IDs against max Detector ID
      if (pixelid > static_cast<PixelType>(m_detid_max)) {
        // Record the wrong/special ID
        iswrongdetid = true;

        ++local_numErrorEvents;
        ++local_numWrongdetidEvents;
        local_wrongdetids.insert(pixelid);
      }

      // Check if this pid we want to load.
      if (m_loadOnlySomeSpectra && !iswrongdetid) {
        std::map<int64_t, bool>::iterator it;
        it = spectraLoadMap.find(pixelid);
        if (it == spectraLoadMap.end()) {
          // Pixel ID was not found, so the event is being ignored.
          local_numIgnoredEvents++;
          continue;
        }
      }

      // Work with the events to be processed
      // Find the pulse time for this event index
      if (i_pulse < numPulses - m_istep) {
        // This is the total offset into the file
        size_t i_totaloffset = ievent + fileOffset;

        // Go through event_index until you find where the index increases to
        // encompass the current index.
        // Your pulse = the one before.
        uint64_t thiseventindex = m_vecEventIndex[i_pulse];
        uint64_t nexteventindex = m_vecEventIndex[i_pulse + m_istep];
        while (!((i_totaloffset >= thiseventindex) && (i_totaloffset < nexteventindex))) {
          i_pulse += m_istep;
          thiseventindex = m_vecEventIndex[i_pulse];
          if (i_pulse >= (numPulses - m_istep))
            break;
          nexteventindex = m_vecEventIndex[i_pulse + m_istep];
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
      if (m_examEventLog && pixelid == m_pixelid2exam && numeventswritten < m_numevents2write) {
        int64_t totaltime = pulsetime.totalNanoseconds() + static_cast<int64_t>(tof * 1000);
        // Output: [EEL] for Examine Event Log
        g_log.notice() << "[EEL] " << numeventswritten << "\t\t" << totaltime << "\t\t" << pixelid << "\t\t" << i_pulse
                       << "\t\t" << fileOffset << "\n";
        ++numeventswritten;
      }

      if (!iswrongdetid) {
        // Event on REAL detector
        // - Find the overall max/min tof
        if (tof < local_m_shortestTof)
          local_m_shortestTof = tof;
        if (tof > local_m_longestTof)
          local_m_longestTof = tof;

        // The addEventQuickly method does not clear the cache, making things
        // slightly
        // faster.
        // workspace->getSpectrum(this->m_pixelToWkspindex[pid]).addEventQuickly(event);

        // - Add event to data structure
        // (This is equivalent to
        // workspace->getSpectrum(this->m_pixelToWkspindex[pid]).addEventQuickly(event))
        // (But should be faster as a bunch of these calls were cached.)
        arrayOfVectors[pixelid]->emplace_back(tof, pulsetime);
        ++local_numGoodEvents;
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

          std::vector<Types::Core::DateAndTime> tempvectime;
          std::vector<double> temptofs;
          local_pulsetimes.emplace_back(tempvectime);
          local_tofs.emplace_back(temptofs);

          theindex = newindex;

          ++numwrongpid;
        } else {
          // existing
          theindex = it->second;
        }

        // Store pulse time and tof of this event
        local_pulsetimes[theindex].emplace_back(pulsetime);
        local_tofs[theindex].emplace_back(tof);
      } // END-IF-ELSE: On Event's Pixel's Nature

    } // ENDIF (event is masked error)

  } // ENDFOR each event

  g_log.debug() << "Number of wrong pixel ID = " << numwrongpid << " of single block. "
                << "\n";

  PARALLEL_CRITICAL(FilterEventsByLogValuePreNexus_global_statistics) {
    this->m_numGoodEvents += local_numGoodEvents;
    this->m_numIgnoredEvents += local_numIgnoredEvents;
    this->m_numErrorEvents += local_numErrorEvents;

    this->m_numBadEvents += local_numBadEvents;
    this->m_numWrongdetidEvents += local_numWrongdetidEvents;

    for (auto tmpid : local_wrongdetids) {
      this->wrongdetids.insert(tmpid);

      // Obtain the global map index for this wrong detector ID events entry in
      // local map
      size_t mindex = 0;
      auto git = this->wrongdetidmap.find(tmpid);
      if (git == this->wrongdetidmap.end()) {
        // Create 'wrong detid' global map entry if not there
        size_t newindex = this->wrongdetid_pulsetimes.size();
        this->wrongdetidmap[tmpid] = newindex;

        std::vector<Types::Core::DateAndTime> vec_pulsetimes;
        std::vector<double> vec_tofs;
        this->wrongdetid_pulsetimes.emplace_back(vec_pulsetimes);
        this->wrongdetid_tofs.emplace_back(vec_tofs);

        mindex = newindex;
      } else {
        mindex = git->second;
      }

      // Find local map index
      auto lit = local_pidindexmap.find(tmpid);
      size_t localindex = lit->second;

      // Append local (thread) loaded events (pulse + tof) to global wrong detid
      // data structure
      for (size_t iv = 0; iv < local_pulsetimes[localindex].size(); iv++) {
        this->wrongdetid_pulsetimes[mindex].emplace_back(local_pulsetimes[localindex][iv]);
        this->wrongdetid_tofs[mindex].emplace_back(local_tofs[localindex][iv]);
      }
    }

    if (local_m_shortestTof < m_shortestTof)
      m_shortestTof = local_m_shortestTof;
    if (local_m_longestTof > m_longestTof)
      m_longestTof = local_m_longestTof;
  }
}

//----------------------------------------------------------------------------------------------
/** Correct wrong event indexes
 */
void FilterEventsByLogValuePreNexus::unmaskVetoEventIndexes() {
  // Check pulse ID with events
  size_t numveto = 0;
  size_t numerror = 0;

    PRAGMA_OMP(parallel for schedule(dynamic, 1) )
    for (int i = 0; i < static_cast<int>(m_vecEventIndex.size()); // NOLINT
         ++i) {
      PARALLEL_START_INTERRUPT_REGION

      uint64_t eventindex = m_vecEventIndex[i];
      if (eventindex > static_cast<uint64_t>(m_numEvents)) {
        uint64_t realeventindex = eventindex & VETOFLAG;
        m_vecEventIndex[i] = realeventindex;
      }
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION

#if 0
        // Examine whether it is a veto
        bool isveto = false;




        if (realeventindex <= m_numEvents)
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
                          << m_numEvents << "\n";
          }
        }
      } // END
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION
#endif

    // Check
    PRAGMA_OMP(parallel for schedule(dynamic, 1) )
    for (int i = 0; i < static_cast<int>(m_vecEventIndex.size()); ++i) {
      PARALLEL_START_INTERRUPT_REGION

      uint64_t eventindex = m_vecEventIndex[i];
      if (eventindex > static_cast<uint64_t>(m_numEvents)) {
        PARALLEL_CRITICAL(unmask_veto_check) {
          g_log.information() << "Check: Pulse " << i << ": unphysical event index = " << eventindex << "\n";
        }
      }

      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION

    g_log.notice() << "Number of veto pulses = " << numveto << ", Number of error-event-index pulses = " << numerror
                   << "\n";
}

//----------------------------------------------------------------------------------------------
/** Process the event file properly.
 */
void FilterEventsByLogValuePreNexus::filterEvents() {
  // Initialize stat parameters
  m_shortestTof = static_cast<double>(MAX_TOF_UINT32) * TOF_CONVERSION;
  m_longestTof = 0.;

  // -------------------------------------------------------------------
  // Set up instrument related parameters such as detector map and etc.
  // We want to pad out empty pixels.
  //--------------------------------------------------------------------
  size_t detectorsize = padOutEmptyPixels(m_localWorkspace);
  setupPixelSpectrumMap(m_localWorkspace);
  setupPixelSpectrumMap(m_localWorkspaceBA);

  // ----------------------------------------------------------------
  // Determine processing mode and file-loading parameters
  //------------------------------------------------------------------
  // Set up some default values in the case of no parallel
  size_t loadBlockSize = Mantid::Kernel::DEFAULT_BLOCK_SIZE * 2;
  size_t numBlocks = (m_maxNumEvents + loadBlockSize - 1) / loadBlockSize;

  std::string procMode = getProperty("UseParallelProcessing");
  if (procMode == "Serial") {
    m_parallelProcessing = false;
  } else if (procMode == "Parallel") {
    m_parallelProcessing = true;
  } else {
    // Automatic determination. Loading serially (for me) is about 3 million
    // events per second,
    // (which is sped up by ~ x 3 with parallel processing, say 10 million per
    // second, e.g. 7 million events more per seconds).
    // compared to a setup time/merging time of about 10 seconds per million
    // detectors.
    double setUpTime = double(detectorsize) * 10e-6;
    m_parallelProcessing = ((double(m_maxNumEvents) / 7e6) > setUpTime);
    g_log.information() << (m_parallelProcessing ? "Using" : "Not using") << " parallel processing."
                        << "\n";
  }

  CPUTimer tim;

  // FIXME - Only serial mode supported for filtering events
  g_log.warning() << "Only serial mode is supported at this moment for filtering. "
                  << "\n";

  // -------------------------------------------------------------------
  // Create the partial workspaces
  //--------------------------------------------------------------------
  // Vector of partial workspaces, for parallel processing.
  std::vector<EventWorkspace_sptr> partWorkspaces;
  std::vector<DasEvent *> buffers;

  /// Pointer to the vector of events
  using EventVector_pt = std::vector<TofEvent> *;
  /// Bare array of arrays of pointers to the EventVectors
  EventVector_pt **eventVectors;

  /// How many threads will we use?
  size_t numThreads = 1;
  if (m_parallelProcessing)
    numThreads = size_t(PARALLEL_GET_MAX_THREADS);

  partWorkspaces.resize(numThreads);
  buffers.resize(numThreads);
  eventVectors = new EventVector_pt *[numThreads];

  // Processing by number of threads
  g_log.information() << "Processing input event preNexus by " << numThreads << " threads"
                      << " in " << numBlocks << " blocks. "
                      << "\n";

    PRAGMA_OMP( parallel for if (m_parallelProcessing) )
    for (int i = 0; i < int(numThreads); i++) {
      // This is the partial workspace we are about to create (if in parallel)
      EventWorkspace_sptr partWS;

      if (m_parallelProcessing) {
        m_progress->report("Creating Partial Workspace");
        // Create a partial workspace, copy all the spectra numbers and stuff
        // (no actual events to copy though).
        partWS = m_localWorkspace->clone();
        // Push it in the array
        partWorkspaces[i] = partWS;
      } else
        partWS = m_localWorkspace;

      // Allocate the buffers
      buffers[i] = new DasEvent[loadBlockSize];

      // For each partial workspace, make an array where index = detector ID and
      // value = pointer to the events vector
      eventVectors[i] = new EventVector_pt[m_detid_max + 1];
      EventVector_pt *theseEventVectors = eventVectors[i];
      for (detid_t j = 0; j < m_detid_max + 1; ++j) {
        size_t wi = m_pixelToWkspindex[j];
        // Save a POINTER to the vector<tofEvent>
        if (wi != static_cast<size_t>(-1))
          theseEventVectors[j] = &partWS->getSpectrum(wi).getEvents();
        else
          theseEventVectors[j] = nullptr;
      }
    } // END FOR [Threads]

    g_log.information() << tim << " to create " << partWorkspaces.size() << " workspaces for parallel loading."
                        << "\n";

    m_progress->resetNumSteps(numBlocks, 0.1, 0.8);

    // -------------------------------------------------------------------
    // LOAD THE DATA
    //--------------------------------------------------------------------
    PRAGMA_OMP( parallel for schedule(dynamic, 1) if (m_parallelProcessing) )
    for (int blockNum = 0; blockNum < int(numBlocks); blockNum++) {
      PARALLEL_START_INTERRUPT_REGION

      // Find the workspace for this particular thread
      EventWorkspace_sptr ws;
      size_t threadNum = 0;
      if (m_parallelProcessing) {
        threadNum = PARALLEL_THREAD_NUMBER;
        ws = partWorkspaces[threadNum];
      } else
        ws = m_localWorkspace;

      // Get the buffer (for this thread)
      DasEvent *event_buffer = buffers[threadNum];

      // Get the speeding-up array of vector<tofEvent> where index = detid.
      EventVector_pt *theseEventVectors = eventVectors[threadNum];

      // Where to start in the file?
      size_t fileOffset = m_firstEvent + (loadBlockSize * blockNum);
      // May need to reduce size of last (or only) block
      size_t current_event_buffer_size =
          (blockNum == int(numBlocks - 1)) ? (m_maxNumEvents - (numBlocks - 1) * loadBlockSize) : loadBlockSize;

      // Load this chunk of event data (critical block)
      PARALLEL_CRITICAL(FilterEventsByLogValuePreNexus_fileAccess) {
        current_event_buffer_size = m_eventFile->loadBlockAt(event_buffer, fileOffset, current_event_buffer_size);
      }

      // This processes the events. Can be done in parallel!
      filterEventsLinear(ws, theseEventVectors, event_buffer, current_event_buffer_size, fileOffset);

      // Report progress
      m_progress->report("Load Event PreNeXus");

      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION

    g_log.information() << tim << " to load the data.\n";

    // -------------------------------------------------------------------
    // MERGE WORKSPACES BACK TOGETHER
    //--------------------------------------------------------------------
    if (m_parallelProcessing) {
      PARALLEL_START_INTERRUPT_REGION
      m_progress->resetNumSteps(m_localWorkspace->getNumberHistograms(), 0.8, 0.95);

      // Merge all workspaces, index by index.
      PARALLEL_FOR_NO_WSP_CHECK()
      for (int iwi = 0; iwi < int(m_localWorkspace->getNumberHistograms()); iwi++) {
        auto wi = size_t(iwi);

        // The output event list.
        EventList &el = m_localWorkspace->getSpectrum(wi);
        el.clear(false);

        // How many events will it have?
        size_t numEvents = 0;
        for (size_t i = 0; i < numThreads; i++)
          numEvents += partWorkspaces[i]->getSpectrum(wi).getNumberEvents();
        // This will avoid too much copying.
        el.reserve(numEvents);

        // Now merge the event lists
        for (size_t i = 0; i < numThreads; i++) {
          EventList &partEl = partWorkspaces[i]->getSpectrum(wi);
          el += partEl.getEvents();
          // Free up memory as you go along.
          partEl.clear(false);
        }
        m_progress->report("Merging Workspaces");
      }

      g_log.debug() << tim << " to merge workspaces together.\n";
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION

    // Delete the buffers for each thread.
    for (size_t i = 0; i < numThreads; i++) {
      delete[] buffers[i];
      delete[] eventVectors[i];
    }
    delete[] eventVectors;

    m_progress->resetNumSteps(3, 0.94, 1.00);

    // finalize loading
    m_progress->report("Setting proton charge");
    this->setProtonCharge(m_localWorkspace);
    g_log.debug() << tim << " to set the proton charge log.\n";

    // Make sure the MRU is cleared
    m_localWorkspace->clearMRU();

    // Now, create a default X-vector for histogramming, with just 2 bins.
    auto axis = HistogramData::BinEdges{m_shortestTof - 1, m_longestTof + 1};
    m_localWorkspace->setAllX(axis);
    this->m_pixelToWkspindex.clear();

    // -------------------------------------------------------------------
    // Final message output
    //--------------------------------------------------------------------
    g_log.notice() << "Read " << m_numGoodEvents << " events + " << m_numErrorEvents << " errors"
                   << ". Shortest TOF: " << m_shortestTof << " microsec; longest TOF: " << m_longestTof << " microsec."
                   << "\n";

    for (const auto wrongdetid : this->wrongdetids) {
      g_log.notice() << "Wrong Detector ID : " << wrongdetid << '\n';
    }
    for (const auto &detidPair : this->wrongdetidmap) {
      PixelType tmpid = detidPair.first;
      size_t vindex = detidPair.second;
      g_log.notice() << "Pixel " << tmpid
                     << ":  Total number of events = " << this->wrongdetid_pulsetimes[vindex].size() << '\n';
    }
} // End of filterEvents

//----------------------------------------------------------------------------------------------
/** Linear-version of the procedure to process the event file properly.
 * @param workspace :: EventWorkspace to write to.
 * @param arrayOfVectors :: For speed up: this is an array, of size
 * m_detid_max+1, where the
 *        index is a pixel ID, and the value is a pointer to the
 * vector<tofEvent> in the given EventList.
 * @param event_buffer :: The buffer containing the DAS events
 * @param current_event_buffer_size :: The length of the given DAS buffer
 * @param fileOffset :: Value for an offset into the binary file
 */
void FilterEventsByLogValuePreNexus::filterEventsLinear(DataObjects::EventWorkspace_sptr & /*workspace*/,
                                                        std::vector<TofEvent> **arrayOfVectors, DasEvent *event_buffer,
                                                        size_t current_event_buffer_size, size_t fileOffset) {
  //----------------------------------------------------------------------------------
  // Set up parameters to process events from raw file
  //----------------------------------------------------------------------------------
  // Pulse ID and pulse time
  DateAndTime pulsetime;
  auto numPulses = static_cast<int64_t>(m_numPulses);
  if (m_vecEventIndex.size() < m_numPulses) {
    g_log.warning() << "Event_indices vector is smaller than the pulsetimes array.\n";
    numPulses = static_cast<int64_t>(m_vecEventIndex.size());
  }

  uint64_t maxeventid = m_vecEventIndex.back();
  g_log.notice() << "Maximum event index = " << maxeventid << " vs. " << m_maxNumEvents << "\n";
  maxeventid = m_maxNumEvents + 1;

  // Declare local statistic parameters
  size_t local_numErrorEvents = 0;
  size_t local_numBadEvents = 0;
  size_t local_numIgnoredEvents = 0;
  size_t local_numGoodEvents = 0;
  double local_m_shortestTof = static_cast<double>(MAX_TOF_UINT32) * TOF_CONVERSION;
  double local_m_longestTof = 0.;

#if 0
    // NOT CALLED AT ALL
    // Local data structure for loaded events
    std::map<PixelType, size_t> local_pidindexmap;
    std::vector<std::vector<Types::Core::DateAndTime> > local_pulsetimes;
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
      g_log.error() << "File offset " << fileOffset << " unable to find a previoiusly defined log event. "
                    << "\n";
    } else {
      g_log.warning() << "File offset " << fileOffset << " 1-st event log at index = " << firstindex
                      << ", status = " << filterstatus << "\n";
    }
  }

  Instrument_const_sptr instrument = m_localWorkspace->getInstrument();
  if (!instrument)
    throw std::runtime_error("Instrument is not setup in m_localWorkspace.");
  IComponent_const_sptr source = std::dynamic_pointer_cast<const IComponent>(instrument->getSource());
  if (!source)
    throw std::runtime_error("Source is not set up in local workspace.");

  //----------------------------------------------------------------------------------
  // process the individual events
  //----------------------------------------------------------------------------------
  bool firstlogevent = true;
  int64_t i_pulse = 0;
  int64_t boundtime(0);
  int64_t boundindex(0);
  int64_t prevbtime(0);
  PixelType boundpixel(0);

  const auto &detectorInfo = m_localWorkspace->detectorInfo();
  const auto &detIds = detectorInfo.detectorIDs();

  double l1 = detectorInfo.l1();
  g_log.notice() << "[DB] L1 = " << l1 << "\n";

  for (size_t ievent = 0; ievent < current_event_buffer_size; ++ievent) {

    // Load DasEvent
    DasEvent &tempevent = *(event_buffer + ievent);

    // DasEvetn's pixel ID
    PixelType pixelid = tempevent.pid;

    // Check Pixels IDs
    if ((pixelid & ERROR_PID) == ERROR_PID) {
      // Marked as bad
      local_numErrorEvents++;
      local_numBadEvents++;
      continue;
    } else {
      bool islogevent = false;
      bool iswrongdetid = false;

      // Covert DAS Pixel ID to Mantid Pixel ID
      if (pixelid == 1073741843) {
        // downstream monitor pixel for SNAP
        pixelid = 1179648;
      } else if (this->m_usingMappingFile) {
        // Converted by pixel mapping file
        PixelType unmapped_pid = pixelid % this->m_numPixel;
        pixelid = this->m_pixelmap[unmapped_pid];
      }

      // Check special/wrong pixel IDs against max Detector ID
      if (pixelid > static_cast<PixelType>(m_detid_max)) {
        // Record the wrong/special ID
        if (pixelid == m_vecLogPixelID[0]) {
          if (firstlogevent && definedfilterstatus) {
            if (filterstatus != -1)
              g_log.error() << "Pre-defined filter status is wrong of fileoffset = " << fileOffset
                            << " at index = " << ievent << "\n";
            firstlogevent = false;
          }
          filterstatus = 1;
          islogevent = true;
          boundindex = ievent;
          boundpixel = m_vecLogPixelID[0];
        } else if (pixelid == m_vecLogPixelID[1]) {
          if (firstlogevent && definedfilterstatus) {
            if (filterstatus != 1)
              g_log.error() << "pre-defined filter status is wrong of fileoffset = " << fileOffset
                            << " at index = " << ievent << "\n";
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
        g_log.notice() << "[Special] ievent = " << i_totaloffsetX << ", Filter status = " << filterstatus
                       << ", Prev-boundary-pixel = " << boundpixel << "\n";
      }
#endif

      // Check if this pid we want to load.
      if (m_loadOnlySomeSpectra && !iswrongdetid && !islogevent) {
        std::map<int64_t, bool>::iterator it;
        it = spectraLoadMap.find(pixelid);
        if (it == spectraLoadMap.end()) {
          // Pixel ID was not found, so the event is being ignored.
          continue;
        }
      }

      // Work with the events to be processed
      // Find the pulse time for this event index
      if (i_pulse < numPulses - m_istep) {
        // This is the total offset into the file
        size_t i_totaloffset = ievent + fileOffset;

        // Go through event_index until you find where the index increases to
        // encompass the current index.
        // Your pulse = the one before.
        uint64_t thiseventindex = m_vecEventIndex[i_pulse];
        uint64_t nexteventindex = m_vecEventIndex[i_pulse + m_istep];
        while (!((i_totaloffset >= thiseventindex) && (i_totaloffset < nexteventindex))) {
          i_pulse += m_istep;
          if (i_pulse >= (numPulses - m_istep))
            break;

          thiseventindex = nexteventindex;
          nexteventindex = m_vecEventIndex[i_pulse + m_istep];
        }

        // Save the pulse time at this index for creating those events
        pulsetime = pulsetimes[i_pulse];
      } // Find pulse time

      double tof = static_cast<double>(tempevent.tof) * TOF_CONVERSION;

#ifdef DBOUT
      // Can be modifed for other output purpose
      if (static_cast<int64_t>(m_examEventLog) && pixelid == m_pixelid2exam && numeventswritten < m_numevents2write) {
        g_log.notice() << "[E] Event " << ievent << "\t: Pulse ID = " << i_pulse << ", Pulse Time = " << pulsetime
                       << ", TOF = " << tof << ", Pixel ID = " << pixelid << "\n";
        ++numeventswritten;
      }
#endif

      int64_t abstime(0);
      bool reversestatus(false);
      if (islogevent) {
        // Record the log boundary time
        prevbtime = boundtime;
        boundtime = pulsetime.totalNanoseconds() + static_cast<int64_t>(tof * 1000);
        logpulsetime = pulsetime;
        // logtof = tof;
      } else {
        double factor(1.0);
        if (m_corretctTOF) {
          if (std::find(detIds.begin(), detIds.end(), pixelid) == detIds.end())
            throw std::runtime_error("Unable to get access to detector ");

          // Calculate TOF correction value
          double l2 = detectorInfo.l2(pixelid);
          factor = (l1) / (l1 + l2);
        }

        // Examine whether to revert the filter
        if (m_corretctTOF)
          abstime = pulsetime.totalNanoseconds() + static_cast<int64_t>(tof * factor * 1000);
        else
          abstime = pulsetime.totalNanoseconds() + static_cast<int64_t>(tof * 1000);
        if (abstime < boundtime) {
          // In case that the boundary time is bigger (DAS' mistake), seek
          // previous one
          reversestatus = true;
#if 1
          if (dbprint)
            g_log.warning() << "Event " << ievent + fileOffset << " is behind an event log though it is earlier.  "
                            << "Diff = " << boundtime - abstime << " ns \n";
#endif
        } else
#if 1
            if (dbprint)
          g_log.notice() << "[Special] Event " << ievent + fileOffset << " Revert status = " << reversestatus
                         << ", Filter-status = " << filterstatus << "\n";
#endif
      }

      int currstatus = filterstatus;
      if (dbprint)
        g_log.notice() << "[Special] A Event " << ievent + fileOffset << " Revert status = " << reversestatus
                       << ", current-status = " << currstatus << ", Filter-status = " << filterstatus << "\n";
      if (reversestatus)
        currstatus = -filterstatus;
      if (dbprint)
        g_log.notice() << "[Special] B Event " << ievent + fileOffset << " Revert status = " << reversestatus
                       << ", current-status = " << currstatus << ", Filter-status = " << filterstatus << "\n";
      if (!iswrongdetid && !islogevent && currstatus > 0) {
// Event on REAL detector
#if 1
        if (dbprint)
          g_log.notice() << "[Special] ievent = " << i_totaloffsetX << ", Filter In "
                         << "\n";
#endif

        // Update summary variable: shortest and longest TOF
        if (tof < local_m_shortestTof)
          local_m_shortestTof = tof;
        if (tof > local_m_longestTof)
          local_m_longestTof = tof;

        // Add event to vector of events
        // (This is equivalent to
        // workspace->getSpectrum(this->m_pixelToWkspindex[pid]).addEventQuickly(event))
        // (But should be faster as a bunch of these calls were cached.)
        arrayOfVectors[pixelid]->emplace_back(tof, pulsetime);
        ++local_numGoodEvents;

#ifdef DBOUT
        if (fileOffset == 0 && numeventsprint < 10) {
          g_log.notice() << "[E10] Event " << ievent << "\t: Pulse ID = " << i_pulse << ", Pulse Time = " << pulsetime
                         << ", TOF = " << tof << ", Pixel ID = " << pixelid << "\n";

          ++numeventsprint;
        }
#endif
        if ((m_useDBOutput && pixelid == m_dbPixelID) || dbprint) {
          g_log.notice() << "[Event_DB11A] Index = " << ievent + fileOffset << ", AbsTime = " << abstime
                         << ", Pulse time = " << pulsetime << ", TOF = " << tof << ", Bound Index = " << boundindex
                         << ", Boundary time = " << boundtime << ", Prev Boundary time = " << prevbtime
                         << ", Boundary Pixel = " << boundpixel << ", Pixell ID = " << pixelid << "\n";
        }
      } else {
#if 1
        if (dbprint)
          g_log.notice() << "[Special] ievent = " << i_totaloffsetX << ", Filter Out "
                         << "\n";
#endif

        if ((m_useDBOutput && pixelid == m_dbPixelID) || dbprint) {
          g_log.notice() << "[Event_DB11B] Index = " << ievent + fileOffset << ", AbsTime = " << abstime
                         << ", Pulse time = " << pulsetime << ", TOF = " << tof << ", Bound Index = " << boundindex
                         << ", Boundary time = " << boundtime << ", Prev Boundary Time = " << prevbtime
                         << ", Boundary Pixel = " << boundpixel << ", Pixell ID = " << pixelid << "\n";
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

          std::vector<Types::Core::DateAndTime> tempvectime;
          std::vector<double> temptofs;
          local_pulsetimes.emplace_back(tempvectime);
          local_tofs.emplace_back(temptofs);

          theindex = newindex;

          ++numwrongpid;
        } else {
          // existing
          theindex = it->second;
        }

        // Store pulse time and tof of this event
        local_pulsetimes[theindex].emplace_back(pulsetime);
        local_tofs[theindex].emplace_back(tof);
#else
        // Ignore all operation
        ;
#endif
      } // END-IF-ELSE: On Event's Pixel's Nature

    } // ENDIF (event is masked error)

  } // ENDFOR each event

  PARALLEL_CRITICAL(FilterEventsByLogValuePreNexus_global_statistics) {
    this->m_numGoodEvents += local_numGoodEvents;
    this->m_numIgnoredEvents += local_numIgnoredEvents;
    this->m_numErrorEvents += local_numErrorEvents;

    this->m_numBadEvents += local_numBadEvents;

    if (local_m_shortestTof < m_shortestTof)
      m_shortestTof = local_m_shortestTof;
    if (local_m_longestTof > m_longestTof)
      m_longestTof = local_m_longestTof;
  }

} // FilterEventsLinearly

//----------------------------------------------------------------------------------------------
/** Set up instrument related parameters such as detector map and etc for
 * 'eventws'
 * and create a pixel-spectrum map
 * We want to pad out empty pixels: monitor
 */
size_t FilterEventsByLogValuePreNexus::padOutEmptyPixels(const DataObjects::EventWorkspace_sptr &eventws) {
  const auto &detectorInfo = eventws->detectorInfo();
  const auto &detIDs = detectorInfo.detectorIDs();

  // Determine maximum pixel id
  const auto it = std::max_element(detIDs.cbegin(), detIDs.cend());
  m_detid_max = it == detIDs.cend() ? 0 : *it; // in case detIDs is empty

  // Pad all the pixels
  m_progress->report("Padding Pixels of workspace");
  this->m_pixelToWkspindex.reserve(m_detid_max + 1); // starting at zero up to and including m_detid_max
  // Set to zero
  this->m_pixelToWkspindex.assign(m_detid_max + 1, 0);

  // Set up the map between workspace index and pixel ID
  size_t workspaceIndex = 0;
  for (size_t i = 0; i < detectorInfo.size(); ++i) {
    if (!detectorInfo.isMonitor(i)) {
      if (!m_loadOnlySomeSpectra || (spectraLoadMap.find(detIDs[i]) != spectraLoadMap.end())) {
        this->m_pixelToWkspindex[detIDs[i]] = workspaceIndex;
        ++workspaceIndex;
      } else {
        this->m_pixelToWkspindex[detIDs[i]] = -1;
      }
    }
  }

  return detectorInfo.size();
}

//----------------------------------------------------------------------------------------------
/** Set up instrument related parameters such as detector map and etc for
 * 'eventws' create a
 * pixel-spectrum map
 */
void FilterEventsByLogValuePreNexus::setupPixelSpectrumMap(const DataObjects::EventWorkspace_sptr &eventws) {
  const auto &detectorInfo = eventws->detectorInfo();
  const auto &detIDs = detectorInfo.detectorIDs();

  // Set up
  specnum_t spectrumNumber = 1;
  for (size_t i = 0; i < detectorInfo.size(); ++i) {
    if (!detectorInfo.isMonitor(i)) {
      if (!m_loadOnlySomeSpectra || (spectraLoadMap.find(detIDs[i]) != spectraLoadMap.end())) {
        // Add non-monitor detector ID
        size_t workspaceIndex = m_pixelToWkspindex[detIDs[i]];
        EventList &spec = eventws->getSpectrum(workspaceIndex);
        spec.addDetectorID(detIDs[i]);
        // Start the spectrum number at 1
        spec.setSpectrumNo(spectrumNumber);
      }
      ++spectrumNumber;
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Use pulse index/ event index to find out the frequency of instrument running
 */
int FilterEventsByLogValuePreNexus::findRunFrequency() {
  g_log.debug() << "Size of pulse / event index  = " << m_vecEventIndex.size() << "\n";

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
        g_log.notice() << "istart = " << istart << " w/ value = " << m_vecEventIndex[istart] << ", icurr = " << i
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
void FilterEventsByLogValuePreNexus::setProtonCharge(DataObjects::EventWorkspace_sptr &workspace) {
  if (this->m_protonCharge.empty()) // nothing to do
    return;

  Run &run = workspace->mutableRun();

  // Add the proton charge entries.
  auto *log = new TimeSeriesProperty<double>("proton_charge");
  log->setUnits("picoCoulombs");

  // Add the time and associated charge to the log
  log->addValues(this->pulsetimes, this->m_protonCharge);

  // TODO set the units for the log
  run.addLogData(std::move(log));
  // Force re-integration
  run.integrateProtonCharge();
  double integ = run.getProtonCharge();
  this->g_log.information() << "Total proton charge of " << integ << " microAmp*hours found by integrating.\n";
}

//----------------------------------------------------------------------------------------------
/** Load a pixel mapping file
 * @param filename :: Path to file.
 */
void FilterEventsByLogValuePreNexus::loadPixelMap(const std::string &filename) {
  this->m_usingMappingFile = false;

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
  auto max_pid = static_cast<PixelType>(pixelmapFile.getNumElements());
  // Load all the data
  this->m_pixelmap = pixelmapFile.loadAllIntoVector();

  // Check for funky file
  using std::placeholders::_1;
  if (std::find_if(m_pixelmap.begin(), m_pixelmap.end(), std::bind(std::greater<PixelType>(), _1, max_pid)) !=
      m_pixelmap.end()) {
    this->g_log.warning("Pixel id in mapping file was out of bounds. Loading "
                        "without mapping file");
    this->m_numPixel = 0;
    this->m_pixelmap.clear();
    this->m_usingMappingFile = false;
    return;
  }

  // If we got here, the mapping file was loaded correctly and we'll use it
  this->m_usingMappingFile = true;
  // Let's assume that the # of pixels in the instrument matches the mapping
  // file length.
  this->m_numPixel = static_cast<uint32_t>(pixelmapFile.getNumElements());
}

//-----------------------------------------------------------------------------
/** Open an event file
 * @param filename :: file to open.
 */
void FilterEventsByLogValuePreNexus::openEventFile(const std::string &filename) {
  // Open the file
  m_eventFile = std::make_unique<BinaryFile<DasEvent>>(filename);
  m_numEvents = m_eventFile->getNumElements();
  g_log.debug() << "File contains " << m_numEvents << " event records.\n";

  // Check if we are only loading part of the event file
  const int chunk = getProperty("ChunkNumber");
  if (isEmpty(chunk)) // We are loading the whole file
  {
    m_firstEvent = 0;
    m_maxNumEvents = m_numEvents;
  } else // We are loading part - work out the event number range
  {
    const int totalChunks = getProperty("TotalChunks");
    m_maxNumEvents = m_numEvents / totalChunks;
    m_firstEvent = (chunk - 1) * m_maxNumEvents;
    // Need to add any remainder to the final chunk
    if (chunk == totalChunks)
      m_maxNumEvents += m_numEvents % totalChunks;
  }

  g_log.information() << "Reading " << m_maxNumEvents << " event records\n";
}

//-----------------------------------------------------------------------------
/** Read a pulse ID file
 * @param filename :: file to load.
 * @param throwError :: Flag to trigger error throwing instead of just logging
 */
void FilterEventsByLogValuePreNexus::readPulseidFile(const std::string &filename, const bool throwError) {
  this->m_protonChargeTot = 0.;
  this->m_numPulses = 0;
  this->m_pulseTimesIncreasing = true;

  // jump out early if there isn't a filename
  if (filename.empty()) {
    this->g_log.information("NOT using a pulseid file");
    return;
  }

  std::vector<Pulse> pulses;

  // set up for reading
  // Open the file; will throw if there is any problem
  try {
    BinaryFile<Pulse> pulseFile(filename);

    // Get the # of pulse
    this->m_numPulses = pulseFile.getNumElements();
    this->g_log.information() << "Using pulseid file \"" << filename << "\", with " << m_numPulses << " pulses.\n";

    // Load all the data
    pulses = pulseFile.loadAll();
  } catch (runtime_error &e) {
    if (throwError) {
      throw;
    } else {
      this->g_log.information() << "Encountered error in pulseidfile (ignoring file): " << e.what() << "\n";
      return;
    }
  }

  if (m_numPulses > 0) {
    DateAndTime lastPulseDateTime(0, 0);
    this->pulsetimes.reserve(m_numPulses);
    for (const auto &pulse : pulses) {
      DateAndTime pulseDateTime(static_cast<int64_t>(pulse.seconds), static_cast<int64_t>(pulse.nanoseconds));
      this->pulsetimes.emplace_back(pulseDateTime);
      this->m_vecEventIndex.emplace_back(pulse.event_index);

      if (pulseDateTime < lastPulseDateTime)
        this->m_pulseTimesIncreasing = false;
      else
        lastPulseDateTime = pulseDateTime;

      double temp = pulse.pCurrent;
      this->m_protonCharge.emplace_back(temp);
      if (temp < 0.)
        this->g_log.warning("Individual proton charge < 0 being ignored");
      else
        this->m_protonChargeTot += temp;
    }
  }

  this->m_protonChargeTot = this->m_protonChargeTot * CURRENT_CONVERSION;
}

} // namespace Mantid::DataHandling
