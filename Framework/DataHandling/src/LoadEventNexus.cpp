// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadEventNexus.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/DefaultEventLoader.h"
#include "MantidDataHandling/EventWorkspaceCollection.h"
#include "MantidDataHandling/LoadEventNexusIndexSetup.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidDataHandling/ParallelEventLoader.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/DateAndTimeHelpers.h"
#include "MantidKernel/EnumeratedString.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidNexus/NeXusException.hpp"
#include "MantidNexus/NeXusFile.hpp"
#include "MantidNexus/NexusIOHelper.h"

#include <H5Cpp.h>
#include <boost/format.hpp>
#include <memory>

#include <regex>

using Mantid::Types::Core::DateAndTime;
using std::map;
using std::string;
using std::vector;

namespace Mantid::DataHandling {

DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadEventNexus)

using namespace Kernel;
using namespace DateAndTimeHelpers;
using namespace Geometry;
using namespace API;
using namespace DataObjects;
using Types::Core::DateAndTime;

namespace {
const std::vector<std::string> binningModeNames{"Default", "Linear", "Logarithmic"};
enum class BinningMode { DEFAULT, LINEAR, LOGARITHMIC, enum_count };
typedef Mantid::Kernel::EnumeratedString<BinningMode, &binningModeNames> BINMODE;

const std::string LOG_CHARGE_NAME("proton_charge");

namespace PropertyNames {
const std::string COMPRESS_TOL("CompressTolerance");
const std::string COMPRESS_MODE("CompressBinningMode");
const std::string BAD_PULSES_CUTOFF("FilterBadPulsesLowerCutoff");
} // namespace PropertyNames
} // namespace

/**
 * Based on the current group in the file, does the named sub-entry exist?
 * @param file : File handle. This is not modified, but cannot be const
 * @param name : sub entry name to look for
 * @return true only if it exists
 */
bool exists(::NeXus::File &file, const std::string &name) {
  const auto entries = file.getEntries();
  return exists(entries, name);
}

bool exists(const std::map<std::string, std::string> &entries, const std::string &name) {
  return entries.find(name) != entries.end();
}

//----------------------------------------------------------------------------------------------
/** Empty default constructor
 */
LoadEventNexus::LoadEventNexus()
    : filter_tof_min(0), filter_tof_max(0), m_specMin(0), m_specMax(0), longest_tof(0), shortest_tof(0), bad_tofs(0),
      discarded_events(0), compressEvents(false), m_instrument_loaded_correctly(false), loadlogs(false),
      event_id_is_spec(false) {
  compressTolerance = EMPTY_DBL();
}

//----------------------------------------------------------------------------------------------
/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadEventNexus::confidence(Kernel::NexusDescriptor &descriptor) const {

  int confidence = 0;
  const std::map<std::string, std::set<std::string>> &allEntries = descriptor.getAllEntries();
  if (allEntries.count("NXevent_data") == 1) {
    if (descriptor.isEntry("/entry", "NXentry") || descriptor.isEntry("/raw_data_1", "NXentry")) {
      confidence = 80;
    }
  }

  return confidence;
}

//----------------------------------------------------------------------------------------------
/** Initialisation method.
 */
void LoadEventNexus::init() {
  const std::vector<std::string> exts{".nxs.h5", ".nxs", "_event.nxs"};
  this->declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, exts),
                        "The name of the Event NeXus file to read, including its full or "
                        "relative path. "
                        "The file name is typically of the form INST_####_event.nxs (N.B. case "
                        "sensitive if running on Linux).");

  this->declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output),
                        "The name of the output EventWorkspace or WorkspaceGroup in which to "
                        "load the EventNexus file.");

  declareProperty(std::make_unique<PropertyWithValue<string>>("NXentryName", "", Direction::Input),
                  "Optional: Name of the NXentry to load if it's not the default.");

  declareProperty(std::make_unique<PropertyWithValue<double>>("FilterByTofMin", EMPTY_DBL(), Direction::Input),
                  "Optional: To exclude events that do not fall within a range "
                  "of times-of-flight. "
                  "This is the minimum accepted value in microseconds. Keep "
                  "blank to load all events.");

  declareProperty(std::make_unique<PropertyWithValue<double>>("FilterByTofMax", EMPTY_DBL(), Direction::Input),
                  "Optional: To exclude events that do not fall within a range "
                  "of times-of-flight. "
                  "This is the maximum accepted value in microseconds. Keep "
                  "blank to load all events.");

  declareProperty(std::make_unique<PropertyWithValue<double>>("FilterByTimeStart", EMPTY_DBL(), Direction::Input),
                  "Optional: To only include events after the provided start "
                  "time, in seconds (relative to the start of the run).");

  declareProperty(std::make_unique<PropertyWithValue<double>>("FilterByTimeStop", EMPTY_DBL(), Direction::Input),
                  "Optional: To only include events before the provided stop "
                  "time, in seconds (relative to the start of the run).");

  declareProperty(
      std::make_unique<PropertyWithValue<double>>(PropertyNames::BAD_PULSES_CUTOFF, EMPTY_DBL(), Direction::Input),
      "Optional: To filter bad pulses set the Lower Cutoff percentage to use.");

  std::string grp1 = "Filter Events";
  setPropertyGroup("FilterByTofMin", grp1);
  setPropertyGroup("FilterByTofMax", grp1);
  setPropertyGroup("FilterByTimeStart", grp1);
  setPropertyGroup("FilterByTimeStop", grp1);
  setPropertyGroup("FilterBadPulsesLowerCutoff", grp1);

  declareProperty(std::make_unique<ArrayProperty<string>>("BankName", Direction::Input),
                  "Optional: To only include events from one bank. Any bank "
                  "whose name does not match the given string will have no "
                  "events.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("SingleBankPixelsOnly", true, Direction::Input),
                  "Optional: Only applies if you specified a single bank to "
                  "load with BankName. "
                  "Only pixels in the specified bank will be created if true; "
                  "all of the instrument's pixels will be created otherwise.");
  setPropertySettings("SingleBankPixelsOnly", std::make_unique<VisibleWhenProperty>("BankName", IS_NOT_DEFAULT));

  std::string grp2 = "Loading a Single Bank";
  setPropertyGroup("BankName", grp2);
  setPropertyGroup("SingleBankPixelsOnly", grp2);

  declareProperty(std::make_unique<PropertyWithValue<bool>>("Precount", true, Direction::Input),
                  "Pre-count the number of events in each pixel before allocating memory "
                  "(optional, default True). "
                  "This can significantly reduce memory use and memory fragmentation; it "
                  "may also speed up loading.");

  declareProperty(
      std::make_unique<PropertyWithValue<double>>(PropertyNames::COMPRESS_TOL, EMPTY_DBL(), Direction::Input),
      "CompressEvents while loading (optional, default: off). "
      "This specified the tolerance to use (in microseconds) when compressing where positive is linear tolerance, "
      "negative is logorithmic tolerance, and zero indicates that time-of-flight must be identical to compress.");
  declareProperty(
      PropertyNames::COMPRESS_MODE, binningModeNames[size_t(BinningMode::DEFAULT)],
      std::make_shared<Mantid::Kernel::StringListValidator>(binningModeNames),
      "Optional. "
      "Binning behavior can be specified in the usual way through sign of binwidth and other properties ('Default'); "
      "or can be set to one of the allowed binning modes. "
      "This will override all other specification or default behavior.");

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

  std::string grp3 = "Reduce Memory Use";
  setPropertyGroup("Precount", grp3);
  setPropertyGroup(PropertyNames::COMPRESS_TOL, grp3);
  setPropertyGroup(PropertyNames::COMPRESS_MODE, grp3);
  setPropertyGroup("ChunkNumber", grp3);
  setPropertyGroup("TotalChunks", grp3);

  declareProperty(std::make_unique<PropertyWithValue<bool>>("LoadMonitors", false, Direction::Input),
                  "Load the monitors from the file (optional, default False).");

  std::vector<std::string> options{"", "Events", "Histogram"};
  declareProperty("MonitorsLoadOnly", "", std::make_shared<Kernel::StringListValidator>(options),
                  "If multiple repesentations exist, which one to load. "
                  "Default is to load the one that is present.");

  declareProperty(std::make_unique<PropertyWithValue<double>>("FilterMonByTofMin", EMPTY_DBL(), Direction::Input),
                  "Optional: To exclude events from monitors that do not fall "
                  "within a range of times-of-flight. "
                  "This is the minimum accepted value in microseconds.");

  declareProperty(std::make_unique<PropertyWithValue<double>>("FilterMonByTofMax", EMPTY_DBL(), Direction::Input),
                  "Optional: To exclude events from monitors that do not fall "
                  "within a range of times-of-flight. "
                  "This is the maximum accepted value in microseconds.");

  declareProperty(std::make_unique<PropertyWithValue<double>>("FilterMonByTimeStart", EMPTY_DBL(), Direction::Input),
                  "Optional: To only include events from monitors after the "
                  "provided start time, in seconds (relative to the start of "
                  "the run).");

  declareProperty(std::make_unique<PropertyWithValue<double>>("FilterMonByTimeStop", EMPTY_DBL(), Direction::Input),
                  "Optional: To only include events from monitors before the "
                  "provided stop time, in seconds (relative to the start of "
                  "the run).");

  setPropertySettings("MonitorsLoadOnly", std::make_unique<VisibleWhenProperty>("LoadMonitors", IS_EQUAL_TO, "1"));
  auto asEventsIsOn = [] {
    std::unique_ptr<IPropertySettings> prop = std::make_unique<VisibleWhenProperty>("LoadMonitors", IS_EQUAL_TO, "1");
    return prop;
  };
  setPropertySettings("FilterMonByTofMin", asEventsIsOn());
  setPropertySettings("FilterMonByTofMax", asEventsIsOn());
  setPropertySettings("FilterMonByTimeStart", asEventsIsOn());
  setPropertySettings("FilterMonByTimeStop", asEventsIsOn());

  std::string grp4 = "Monitors";
  setPropertyGroup("LoadMonitors", grp4);
  setPropertyGroup("MonitorsLoadOnly", grp4);
  setPropertyGroup("FilterMonByTofMin", grp4);
  setPropertyGroup("FilterMonByTofMax", grp4);
  setPropertyGroup("FilterMonByTimeStart", grp4);
  setPropertyGroup("FilterMonByTimeStop", grp4);

  declareProperty("SpectrumMin", EMPTY_INT(), mustBePositive, "The number of the first spectrum to read.");
  declareProperty("SpectrumMax", EMPTY_INT(), mustBePositive, "The number of the last spectrum to read.");
  declareProperty(std::make_unique<ArrayProperty<int32_t>>("SpectrumList"),
                  "A comma-separated list of individual spectra to read.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("MetaDataOnly", false, Direction::Input),
                  "If true, only the meta data and sample logs will be loaded.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("LoadLogs", true, Direction::Input),
                  "Load only the Sample/DAS logs from the file (default True).");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("LoadAllLogs", false, Direction::Input),
                  "Load all the logs from the nxs, without checking or processing them; if checked, LoadLogs will be "
                  "ignored; use with caution");

  std::vector<std::string> loadType{"Default"};

#ifndef _WIN32
  loadType.emplace_back("Multiprocess (experimental)");
#endif // _WIN32

  auto loadTypeValidator = std::make_shared<StringListValidator>(loadType);
  declareProperty("LoadType", "Default", loadTypeValidator,
                  "Set type of loader. 2 options {Default, Multiproceess},"
                  "'Multiprocess' should work faster for big files and it is "
                  "experimental, available only in Linux");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("LoadNexusInstrumentXML", true, Direction::Input),
                  "Reads the embedded Instrument XML from the NeXus file "
                  "(optional, default True). ");

  declareProperty("NumberOfBins", 500, mustBePositive,
                  "The number of bins intially defined. Use Rebin to change "
                  "the binning later.  If there is no data loaded, or you "
                  "select meta data only you will only get 1 bin.");

  // Flexible log loading
  declareProperty(std::make_unique<PropertyWithValue<std::vector<std::string>>>("AllowList", std::vector<std::string>(),
                                                                                Direction::Input),
                  "If specified, only these logs will be loaded from the file (each "
                  "separated by a space).");
  declareProperty(std::make_unique<PropertyWithValue<std::vector<std::string>>>("BlockList", std::vector<std::string>(),
                                                                                Direction::Input),
                  "If specified, these logs will NOT be loaded from the file (each "
                  "separated by a space).");
}

std::map<std::string, std::string> LoadEventNexus::validateInputs() {
  std::map<std::string, std::string> result;

  if (!isDefault(PropertyNames::BAD_PULSES_CUTOFF)) {
    const double cutoff = getProperty(PropertyNames::BAD_PULSES_CUTOFF);
    if (cutoff < 0 || cutoff > 100)
      result[PropertyNames::BAD_PULSES_CUTOFF] = "Must be empty or between 0 and 100";
  }

  return result;
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

  try {
    while (true) {
      const auto entry = m_file->getNextEntry();
      if (entry.second == "NXentry") {
        if ((entry.first == "entry") || (entry.first == "raw_data_1")) {
          m_top_entry_name = entry.first;
          break;
        }
      } else if (entry == ::NeXus::EOD_ENTRY) {
        g_log.error() << "Unable to determine name of top level NXentry - assuming "
                         "\"entry\".\n";
        m_top_entry_name = "entry";
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
    if ((!ConfigService::Instance().hasProperty("loadeventnexus.keeppausedevents")) &&
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
void LoadEventNexus::filterDuringPause<EventWorkspaceCollection_sptr>(EventWorkspaceCollection_sptr workspace) {
  // We provide a function pointer to the filter method of the object
  using std::placeholders::_1;
  auto func = std::bind(&LoadEventNexus::filterDuringPause<MatrixWorkspace_sptr>, this, _1);
  workspace->applyFilterInPlace(func);
}

//-----------------------------------------------------------------------------
/**
 * Filter the events by pulse time - no in place version so have to return workspace
 *
 */
template <typename T>
T LoadEventNexus::filterEventsByTime(T workspace, Mantid::Types::Core::DateAndTime &startTime,
                                     Mantid::Types::Core::DateAndTime &stopTime) {
  auto filterByTime = createChildAlgorithm("FilterByTime");
  g_log.information("Filtering events by time...");
  filterByTime->setProperty("InputWorkspace", workspace);
  // sample log already filtered by time so use absolute times to be safe
  filterByTime->setProperty("AbsoluteStartTime", startTime.toISO8601String());
  filterByTime->setProperty("AbsoluteStopTime", stopTime.toISO8601String());
  filterByTime->execute();
  return filterByTime->getProperty("OutputWorkspace");
}

template <>
EventWorkspaceCollection_sptr
LoadEventNexus::filterEventsByTime<EventWorkspaceCollection_sptr>(EventWorkspaceCollection_sptr workspace,
                                                                  Mantid::Types::Core::DateAndTime &startTime,
                                                                  Mantid::Types::Core::DateAndTime &stopTime) {
  // We provide a function pointer to the filter method of the object
  using std::placeholders::_1;
  auto func = std::bind(&LoadEventNexus::filterEventsByTime<EventWorkspace_sptr>, this, _1, startTime, stopTime);
  workspace->applyFilter(func);
  return workspace;
}

//------------------------------------------------------------------------------------------------
/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 */
void LoadEventNexus::execLoader() {
  // Retrieve the filename from the properties
  m_filename = getPropertyValue("Filename");

  compressEvents = !isDefault(PropertyNames::COMPRESS_TOL);
  compressTolerance = getProperty(PropertyNames::COMPRESS_TOL);
  if (compressEvents) {
    BINMODE mode = getPropertyValue(PropertyNames::COMPRESS_MODE);
    if (mode == BinningMode::LINEAR)
      compressTolerance = std::fabs(compressTolerance);
    else if (mode == BinningMode::LOGARITHMIC)
      compressTolerance = -1. * std::fabs(compressTolerance);
  }

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
  m_ws = std::make_shared<EventWorkspaceCollection>(); // Algorithm currently
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

  // close the file since LoadNexusMonitors will take care of its own file
  // handle
  m_file->close();

  // Load the monitors with child algorithm 'LoadNexusMonitors'
  if (load_monitors) {
    prog.report("Loading monitors");
    this->runLoadMonitors();
  }
}

std::pair<DateAndTime, DateAndTime> firstLastPulseTimes(::NeXus::File &file, Kernel::Logger &logger) {
  file.openData("event_time_zero");
  DateAndTime offset;
  // According to the Nexus standard, if the offset is not present, it implies
  // the offset is and absolute timestamp, which is relative to the start of
  // Unix epoch (https://manual.nexusformat.org/classes/base_classes/NXlog.html)
  if (!file.hasAttr("offset")) {
    offset = DateAndTime("1970-01-01T00:00:00Z");
    logger.warning("In firstLastPulseTimes: no ISO8601 offset attribute "
                   "provided for event_time_zero, using UNIX epoch instead");
  } else {
    std::string isooffset; // ISO8601 offset
    file.getAttr("offset", isooffset);
    offset = DateAndTime(isooffset);
  }
  std::string units; // time units
  if (file.hasAttr("units"))
    file.getAttr("units", units);
  // Read in the pulse times
  auto pulse_times = Mantid::NeXus::NeXusIOHelper::readNexusVector<double>(file, "event_time_zero");
  // Remember to close the entry
  file.closeData();
  if (pulse_times.empty()) {
    throw std::invalid_argument("Cannot find run start; event_time_zero contains no pulse times");
  }
  // Convert to seconds
  auto conv = Kernel::Units::timeConversionValue(units, "s");
  return std::make_pair(DateAndTime(pulse_times.front() * conv, 0.0) + offset.totalNanoseconds(),
                        DateAndTime(pulse_times.back() * conv, 0.0) + offset.totalNanoseconds());
} // namespace DataHandling

/**
 * Get the number of events in the currently opened group.
 *
 * @param file The handle to the nexus file opened to the group to look at.
 * @param hasTotalCounts Whether to try looking at the total_counts field.
 * This variable will be changed if the field is not there.
 * @param oldNeXusFileNames Whether to try using old names. This variable will
 * be changed if it is determined that old names are being used.
 * @param prefix current entry name prefix (e.g. /entry)
 * @param descriptor input containing metadata information
 * @return The number of events.
 */
std::size_t numEvents(::NeXus::File &file, bool &hasTotalCounts, bool &oldNeXusFileNames, const std::string &prefix,
                      const NexusDescriptor &descriptor) {
  // try getting the value of total_counts
  if (hasTotalCounts) {
    hasTotalCounts = false;
    if (descriptor.isEntry(prefix + "/total_counts")) {
      try {
        file.openData("total_counts");
        auto info = file.getInfo();
        file.closeData();
        if (info.type == NXnumtype::UINT64) {
          uint64_t eventCount;
          file.readData("total_counts", eventCount);
          hasTotalCounts = true;
          return eventCount;
        }
      } catch (::NeXus::Exception &) {
      }
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

/** Load the log from the nexus file
 *
 * @param nexusfilename :: The name of the nexus file being loaded
 * @param localWorkspace :: Templated workspace in which to put the instrument
 *geometry
 * @param alg :: Handle of the algorithm
 * @param returnpulsetimes :: flag to return shared pointer for
 *BankPulseTimes, otherwise NULL.
 * @param nPeriods : Number of periods (write to)
 * @param periodLog : Period logs DateAndTime to int map.
 *
 * @return Pulse times given in the DAS logs
 */
template <typename T>
std::shared_ptr<BankPulseTimes>
LoadEventNexus::runLoadNexusLogs(const std::string &nexusfilename, T localWorkspace, API::Algorithm &alg,
                                 bool returnpulsetimes, int &nPeriods,
                                 std::unique_ptr<const TimeSeriesProperty<int>> &periodLog) {
  // --------------------- Load DAS Logs -----------------
  // The pulse times will be empty if not specified in the DAS logs.
  // BankPulseTimes * out = NULL;
  std::shared_ptr<BankPulseTimes> out;
  auto loadLogs = alg.createChildAlgorithm("LoadNexusLogs");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    alg.getLogger().information() << "Loading logs from NeXus file..."
                                  << "\n";
    loadLogs->setPropertyValue("Filename", nexusfilename);
    loadLogs->setProperty<API::MatrixWorkspace_sptr>("Workspace", localWorkspace);
    try {
      loadLogs->setPropertyValue("NXentryName", alg.getPropertyValue("NXentryName"));
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
      const auto *temp = run.getProperty("period_log");
      // Check for corrupted period logs
      std::string status = "";
      std::unique_ptr<TimeSeriesProperty<int>> tempPeriodLog(dynamic_cast<TimeSeriesProperty<int> *>(temp->clone()));
      nPeriods = checkForCorruptedPeriods(std::move(tempPeriodLog), periodLog, nPeriods, nexusfilename, status);
      if (!status.empty())
        alg.getLogger().warning(status);
    }

    // If successful, we can try to load the pulse times
    std::vector<Types::Core::DateAndTime> temp;
    if (localWorkspace->run().hasProperty("proton_charge")) {
      const auto *log =
          dynamic_cast<Kernel::TimeSeriesProperty<double> *>(localWorkspace->mutableRun().getProperty("proton_charge"));
      if (log)
        temp = log->timesAsVector();
    }
    if (returnpulsetimes)
      out = std::make_shared<BankPulseTimes>(temp);

    // Use the first pulse as the run_start time.
    if (!temp.empty()) {
      if (temp[0] < Types::Core::DateAndTime("1991-01-01T00:00:00"))
        alg.getLogger().warning() << "Found entries in the proton_charge "
                                     "sample log with invalid pulse time!\n";

      Types::Core::DateAndTime run_start = localWorkspace->getFirstPulseTime();
      // add the start of the run as a ISO8601 date/time string. The start =
      // first non-zero time.
      // (this is used in LoadInstrument to find the right instrument file to
      // use).
      localWorkspace->mutableRun().addProperty("run_start", run_start.toISO8601String(), true);
    } else if (run.hasProperty("start_time")) {
      localWorkspace->mutableRun().addProperty("run_start", run.getProperty("start_time")->value(), true);
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
  } catch (const InvalidLogPeriods &) {
    // Rethrow so LoadEventNexus fails.
    // If we don't, Mantid will crash.
    throw;
  } catch (...) {
    alg.getLogger().error() << "Error while loading Logs from SNS Nexus. Some "
                               "sample logs may be missing."
                            << "\n";
    return out;
  }
  return out;
}

/** Load the log from the nexus file
 *
 * @param nexusfilename :: The name of the nexus file being loaded
 * @param localWorkspace :: Templated workspace in which to put the instrument
 *geometry
 * @param alg :: Handle of the algorithm
 * @param returnpulsetimes :: flag to return shared pointer for
 *BankPulseTimes, otherwise NULL.
 * @param nPeriods : Number of periods (write to)
 * @param periodLog : Period logs DateAndTime to int map.
 * @param allow_list: list of properties that will be loaded
 * @param block_list: list of properties that will be excluded from loading
 *
 * @return Pulse times given in the DAS logs
 */
template <typename T>
std::shared_ptr<BankPulseTimes> LoadEventNexus::runLoadNexusLogs(
    const std::string &nexusfilename, T localWorkspace, API::Algorithm &alg, bool returnpulsetimes, int &nPeriods,
    std::unique_ptr<const TimeSeriesProperty<int>> &periodLog, const std::vector<std::string> &allow_list,
    const std::vector<std::string> &block_list) {
  // --------------------- Load DAS Logs -----------------
  // The pulse times will be empty if not specified in the DAS logs.
  // BankPulseTimes * out = NULL;
  std::shared_ptr<BankPulseTimes> out;
  auto loadLogs = alg.createChildAlgorithm("LoadNexusLogs");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    alg.getLogger().information() << "Loading logs from NeXus file..."
                                  << "\n";
    loadLogs->setPropertyValue("Filename", nexusfilename);
    loadLogs->setProperty<API::MatrixWorkspace_sptr>("Workspace", localWorkspace);
    loadLogs->setProperty<std::vector<std::string>>("AllowList", allow_list);
    loadLogs->setProperty<std::vector<std::string>>("BlockList", block_list);

    try {
      loadLogs->setPropertyValue("NXentryName", alg.getPropertyValue("NXentryName"));
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
      std::string status = "";
      // Check for corrupted period logs
      std::unique_ptr<TimeSeriesProperty<int>> tempPeriodLog(dynamic_cast<TimeSeriesProperty<int> *>(temp->clone()));
      nPeriods = checkForCorruptedPeriods(std::move(tempPeriodLog), periodLog, nPeriods, nexusfilename, status);
      if (!status.empty())
        alg.getLogger().warning(status);
    }

    // If successful, we can try to load the pulse times
    std::vector<Types::Core::DateAndTime> temp;
    if (localWorkspace->run().hasProperty("proton_charge")) {
      auto *log =
          dynamic_cast<Kernel::TimeSeriesProperty<double> *>(localWorkspace->mutableRun().getProperty("proton_charge"));
      if (log)
        temp = log->timesAsVector();
    }
    if (returnpulsetimes)
      out = std::make_shared<BankPulseTimes>(temp);

    // Use the first pulse as the run_start time.
    if (!temp.empty()) {
      if (temp[0] < Types::Core::DateAndTime("1991-01-01T00:00:00"))
        alg.getLogger().warning() << "Found entries in the proton_charge "
                                     "sample log with invalid pulse time!\n";

      Types::Core::DateAndTime run_start = localWorkspace->getFirstPulseTime();
      // add the start of the run as a ISO8601 date/time string. The start =
      // first non-zero time.
      // (this is used in LoadInstrument to find the right instrument file to
      // use).
      localWorkspace->mutableRun().addProperty("run_start", run_start.toISO8601String(), true);
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
  } catch (const InvalidLogPeriods &) {
    // Rethrow so LoadEventNexus fails.
    // If we don't, Mantid will crash.
    throw;
  } catch (...) {
    alg.getLogger().error() << "Error while loading Logs from SNS Nexus. Some "
                               "sample logs may be missing."
                            << "\n";
    return out;
  }
  return out;
}

/** Check for corrupted period logs
 * If data is historical (1 periods, period is labelled 0) then change period
 * labels to 1 If number of periods does not match expected number of periods
 * then throw an error
 * @param tempPeriodLog :: a temporary local copy of period logs, which will
 * change
 * @param periodLog :: unique pointer which will point to period logs once they
 * have been changed
 * @param nPeriods :: the value in the nperiods log of the run. Number of
 * expected periods
 * @param nexusfilename :: the filename of the run to load
 * @param status :: will contain any status message. Empty if no problems.
 * @return Number of periods with data
 */
int LoadEventNexus::checkForCorruptedPeriods(std::unique_ptr<TimeSeriesProperty<int>> tempPeriodLog,
                                             std::unique_ptr<const TimeSeriesProperty<int>> &periodLog,
                                             const int &nPeriods, const std::string &nexusfilename,
                                             std::string &status) {
  const auto valuesAsVector = tempPeriodLog->valuesAsVector();
  const auto nPeriodsInLog = *std::max_element(valuesAsVector.begin(), valuesAsVector.end());
  int numberOfValidPeriods = nPeriodsInLog;
  status = "";

  // Check for historic files
  if (nPeriodsInLog == 0 && nPeriods == 1) {
    // "modernize" the local copy here by making period_log
    // a vector of 1s
    const std::vector<int> newValues(tempPeriodLog->realSize(), 1);
    const auto times = tempPeriodLog->timesAsVector();
    periodLog.reset(new const TimeSeriesProperty<int>("period_log", times, newValues));
    numberOfValidPeriods = 1;
  } else if (nPeriodsInLog < nPeriods) {
    status = boost::str(
        boost::format(
            "The number of periods specified in the file (%1%) is greater than the maximum period in the data (%2%).") %
        nPeriods % nPeriodsInLog);
  } else if (nPeriodsInLog > nPeriods) {
    // Sanity check here that period_log only contains period numbers up to
    // nperiods. These values can be different due to instrument noise, and
    // cause undescriptive crashes if not caught.
    // We throw here to make it clear
    // that the file is corrupted and must be manually assessed.
    const auto msg = "File " + nexusfilename +
                     " has been corrupted. The log framelog/period_log/value "
                     "contains " +
                     std::to_string(nPeriodsInLog) + " periods, but periods/number contains " +
                     std::to_string(nPeriods) + ". This file should be manually inspected and corrected.";
    throw InvalidLogPeriods(msg);
  } else {
    // periodLog should point to a copy of the period logs
    periodLog = std::make_unique<const TimeSeriesProperty<int>>(*tempPeriodLog);
    tempPeriodLog.reset();
  }
  return numberOfValidPeriods;
}

/** Load the instrument from the nexus file
 *
 * @param nexusfilename :: The name of the nexus file being loaded
 * @param localWorkspace :: EventWorkspaceCollection in which to put the
 *instrument
 *geometry
 * @param alg :: Handle of the algorithm
 * @param returnpulsetimes :: flag to return shared pointer for
 *BankPulseTimes, otherwise NULL.
 * @param nPeriods : Number of periods (write to)
 * @param periodLog : Period logs DateAndTime to int map.
 *
 * @return Pulse times given in the DAS logs
 */
template <>
std::shared_ptr<BankPulseTimes> LoadEventNexus::runLoadNexusLogs<EventWorkspaceCollection_sptr>(
    const std::string &nexusfilename, EventWorkspaceCollection_sptr localWorkspace, API::Algorithm &alg,
    bool returnpulsetimes, int &nPeriods, std::unique_ptr<const TimeSeriesProperty<int>> &periodLog) {
  auto ws = localWorkspace->getSingleHeldWorkspace();
  auto ret = runLoadNexusLogs<MatrixWorkspace_sptr>(nexusfilename, ws, alg, returnpulsetimes, nPeriods, periodLog);
  return ret;
}

/** Load the instrument from the nexus file
 *
 * @param nexusfilename :: The name of the nexus file being loaded
 * @param localWorkspace :: EventWorkspaceCollection in which to put the
 *instrument
 *geometry
 * @param alg :: Handle of the algorithm
 * @param returnpulsetimes :: flag to return shared pointer for
 *BankPulseTimes, otherwise NULL.
 * @param nPeriods : Number of periods (write to)
 * @param periodLog : Period logs DateAndTime to int map.
 * @param allow_list: log entry that will be loaded
 * @param block_list: log entry that will be excluded
 *
 * @return Pulse times given in the DAS logs
 */
template <>
std::shared_ptr<BankPulseTimes> LoadEventNexus::runLoadNexusLogs<EventWorkspaceCollection_sptr>(
    const std::string &nexusfilename, EventWorkspaceCollection_sptr localWorkspace, API::Algorithm &alg,
    bool returnpulsetimes, int &nPeriods, std::unique_ptr<const TimeSeriesProperty<int>> &periodLog,
    const std::vector<std::string> &allow_list, const std::vector<std::string> &block_list) {
  auto ws = localWorkspace->getSingleHeldWorkspace();
  auto ret = runLoadNexusLogs<MatrixWorkspace_sptr>(nexusfilename, ws, alg, returnpulsetimes, nPeriods, periodLog,
                                                    allow_list, block_list);
  return ret;
}

enum class LoadEventNexus::LoaderType { MULTIPROCESS, DEFAULT };

//-----------------------------------------------------------------------------
/**
 * Load events from the file.
 * @param prog :: A pointer to the progress reporting object
 * @param monitors :: If true the events from the monitors are loaded and not
 *the main banks
 *
 * This also loads the instrument, but only if it has not been set in the
 *workspace
 * being used as input (m_ws data member). Same applies to the logs.
 */
void LoadEventNexus::loadEvents(API::Progress *const prog, const bool monitors) {
  bool metaDataOnly = getProperty("MetaDataOnly");

  // Get the time filters
  setTimeFilters(monitors);

  // Get the log filter if provided
  std::vector<std::string> allow_list = getProperty("AllowList"); // if not empty, only these logs will be loaded
  std::vector<std::string> block_list = getProperty("BlockList"); // if not empty, these logs won't be loaded

  // The run_start will be loaded from the pulse times.
  DateAndTime run_start(0, 0);
  bool takeTimesFromEvents = false;
  // Initialize the counter of bad TOFs
  bad_tofs = 0;
  int nPeriods = 1;
  auto periodLog = std::make_unique<const TimeSeriesProperty<int>>("period_log");

  bool loadAllLogs = getProperty("LoadAllLogs"); // load all logs, not just DAS logs.

  if (loadlogs) {
    if (!loadAllLogs) {
      prog->doReport("Loading DAS logs");

      if (allow_list.empty() && block_list.empty()) {
        m_allBanksPulseTimes =
            runLoadNexusLogs<EventWorkspaceCollection_sptr>(m_filename, m_ws, *this, true, nPeriods, periodLog);
      } else {
        m_allBanksPulseTimes = runLoadNexusLogs<EventWorkspaceCollection_sptr>(m_filename, m_ws, *this, true, nPeriods,
                                                                               periodLog, allow_list, block_list);
      }

      try {
        run_start = m_ws->getFirstPulseTime();
      } catch (Kernel::Exception::NotFoundError &) {
        /*
          This is added to (a) support legacy behaviour of continuing to take
          times from the proto_charge log, but (b) allowing a fall back of
          getting run start and end from actual pulse times within the
          NXevent_data group. Note that the latter is better Nexus compliant.
        */
        takeTimesFromEvents = true;
      }
    } else {
      prog->doReport("Loading all logs");
      try {
        // Open NeXus file
        ::NeXus::File nxHandle(m_filename, NXACC_READ);
        LoadHelper::addNexusFieldsToWsRun(nxHandle, m_ws->mutableRun(), "", true);
      } catch (const ::NeXus::Exception &e) {
        g_log.debug() << "Failed to open nexus file \"" << m_filename << "\" in read mode: " << e.what() << "\n";
      }
    }
  } else {
    g_log.information() << "Skipping the loading of sample logs!\n"
                        << "Reading the start time directly from /" << m_top_entry_name << "/start_time\n";
    // start_time is read and set
    m_file->openPath("/");
    m_file->openGroup(m_top_entry_name, "NXentry");
    std::string tmp;
    m_file->readData("start_time", tmp);
    m_file->closeGroup();
    run_start = createFromSanitizedISO8601(tmp);
    m_ws->mutableRun().addProperty("run_start", run_start.toISO8601String(), true);
  }
  // set more properties on the workspace
  const std::shared_ptr<NexusDescriptor> descriptor = getFileInfo();

  try {
    // this is a static method that is why it is passing the
    // file object and the file path

    loadEntryMetadata<EventWorkspaceCollection_sptr>(m_filename, m_ws, m_top_entry_name, *descriptor);
  } catch (std::runtime_error &e) {
    // Missing metadata is not a fatal error. Log and go on with your life
    g_log.error() << "Error loading metadata: " << e.what() << '\n';
  }

  m_ws->setNPeriods(static_cast<size_t>(nPeriods),
                    periodLog); // This is how many workspaces we are going to make.

  // Make sure you have a non-NULL m_allBanksPulseTimes
  if (m_allBanksPulseTimes == nullptr) {
    std::vector<DateAndTime> temp;
    m_allBanksPulseTimes = std::make_shared<BankPulseTimes>(temp);
  }

  if (!m_ws->getInstrument() || !m_instrument_loaded_correctly) {
    // Load the instrument (if not loaded before)
    prog->report("Loading instrument");
    // Note that closing an re-opening the file is needed here for loading
    // instruments directly from the nexus file containing the event data.
    // This may not be needed in the future if both LoadEventNexus and
    // LoadInstrument are made to use the same Nexus/HDF5 library
    m_file->close();
    m_instrument_loaded_correctly = loadInstrument(m_filename, m_ws, m_top_entry_name, this, descriptor.get());

    if (!m_instrument_loaded_correctly)
      throw std::runtime_error("Instrument was not initialized correctly! "
                               "Loading cannot continue.");
    // reopen file
    safeOpenFile(m_filename);
  }

  // top level file information
  m_file->openPath("/");
  // Start with the base entry
  m_file->openGroup(m_top_entry_name, "NXentry");

  // Now we want to go through all the bankN_event entries
  vector<string> bankNames;
  vector<std::size_t> bankNumEvents;
  std::string classType = monitors ? "NXmonitor" : "NXevent_data";
  bool oldNeXusFileNames(false);
  bool haveWeights = false;
  auto firstPulseT = DateAndTime::maximum();

  const std::map<std::string, std::set<std::string>> &allEntries = descriptor->getAllEntries();

  auto itClassEntries = allEntries.find(classType);

  if (itClassEntries != allEntries.end()) {

    const std::set<std::string> &classEntries = itClassEntries->second;
    const std::regex classRegex("(/" + m_top_entry_name + "/)([^/]*)");
    std::smatch groups;

    for (const std::string &classEntry : classEntries) {

      if (std::regex_match(classEntry, groups, classRegex)) {
        const std::string entry_name(groups[2].str());

        // skip entries with junk data
        if (entry_name == "bank_error_events" || entry_name == "bank_unmapped_events")
          continue;

        m_file->openGroup(entry_name, classType);

        // get the number of events
        const std::string prefix = "/" + m_top_entry_name + "/" + entry_name;
        bool hasTotalCounts = true;
        std::size_t num = numEvents(*m_file, hasTotalCounts, oldNeXusFileNames, prefix, *descriptor);
        bankNames.emplace_back(entry_name);
        bankNumEvents.emplace_back(num);

        if (takeTimesFromEvents && num > 0) {
          /* If we are here, we are loading logs, but have failed to establish
           * the run_start from the proton_charge log. We are going to get this
           * from our event_time_zero instead
           */
          auto localFirstLast = firstLastPulseTimes(*m_file, this->g_log);
          firstPulseT = std::min(firstPulseT, localFirstLast.first);
        }

        // Look for weights in simulated file
        const std::string absoluteEventWeightName = prefix + "/event_weight";
        haveWeights = descriptor->isEntry(absoluteEventWeightName);
        m_file->closeGroup();
      }
    }
  }

  if (takeTimesFromEvents)
    run_start = firstPulseT;

  loadSampleDataISIScompatibility(*m_file, *m_ws);

  // Close the 'top entry' group (raw_data_1 for NexusProcessed, etc.)
  m_file->closeGroup();

  // Delete the output workspace name if it existed
  std::string outName = getPropertyValue("OutputWorkspace");
  if (AnalysisDataService::Instance().doesExist(outName))
    AnalysisDataService::Instance().remove(outName);

  // --------------------------- Time filtering
  // ------------------------------------
  double filter_time_start_sec, filter_time_stop_sec;
  filter_time_start_sec = getProperty("FilterByTimeStart");
  filter_time_stop_sec = getProperty("FilterByTimeStop");

  // Default to ALL pulse times
  filter_time_start = Types::Core::DateAndTime::minimum();
  filter_time_stop = Types::Core::DateAndTime::maximum();

  if (m_allBanksPulseTimes->numberOfPulses() > 0) {
    // If not specified, use the limits of doubles. Otherwise, convert from
    // seconds to absolute PulseTime
    if (filter_time_start_sec != EMPTY_DBL()) {
      filter_time_start = run_start + filter_time_start_sec;
      m_is_time_filtered = true;
    }

    if (filter_time_stop_sec != EMPTY_DBL()) {
      filter_time_stop = run_start + filter_time_stop_sec;
      m_is_time_filtered = true;
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

  // setup filter bad pulses
  filter_bad_pulses = !isDefault(PropertyNames::BAD_PULSES_CUTOFF);

  if (filter_bad_pulses) {
    double min_pcharge, max_pcharge;
    std::tie(min_pcharge, max_pcharge, std::ignore) =
        m_ws->run().getBadPulseRange(LOG_CHARGE_NAME, getProperty(PropertyNames::BAD_PULSES_CUTOFF));

    const auto *pcharge_log =
        dynamic_cast<Kernel::TimeSeriesProperty<double> *>(m_ws->run().getLogData(LOG_CHARGE_NAME));
    bad_pulses_timeroi = std::make_shared<TimeROI>(
        pcharge_log->makeFilterByValue(min_pcharge, max_pcharge, false, TimeInterval(0, 1), 0., true));
  }

  if (metaDataOnly) {
    // Now, create a default X-vector for histogramming, with just 2 bins.
    auto axis = HistogramData::BinEdges{1, static_cast<double>(std::numeric_limits<uint32_t>::max()) * 0.1 - 1};
    // Set the binning axis using this.
    m_ws->setAllX(axis);

    createSpectraMapping(m_filename, monitors, std::vector<std::string>());
    return;
  }

  // --------- Loading only one bank ? ----------------------------------
  std::vector<std::string> someBanks = getProperty("BankName");
  const bool SingleBankPixelsOnly = getProperty("SingleBankPixelsOnly");
  if ((!someBanks.empty()) && (!monitors)) {
    std::vector<std::string> eventedBanks;
    eventedBanks.reserve(someBanks.size());
    std::transform(someBanks.cbegin(), someBanks.cend(), std::back_inserter(eventedBanks),
                   [](const auto &bank) { return bank + "_events"; });

    // check that all of the requested banks are in the file
    const auto invalidBank =
        std::find_if(eventedBanks.cbegin(), eventedBanks.cend(), [&bankNames](const auto &someBank) {
          return std::none_of(bankNames.cbegin(), bankNames.cend(),
                              [&someBank](const auto &name) { return name == someBank; });
        });
    if (invalidBank != eventedBanks.cend()) {
      throw std::invalid_argument("No entry named '" + *invalidBank + "' was found in the .NXS file.");
    }

    // change the number of banks to load
    bankNames.assign(eventedBanks.cbegin(), eventedBanks.cend());

    // TODO this equally weights the banks
    bankNumEvents.assign(someBanks.size(), 1);

    if (!SingleBankPixelsOnly)
      someBanks.clear(); // Marker to load all pixels
  }

  prog->report("Initializing all pixels");

  // Remove unused banks if parameter is set
  if (m_ws->getInstrument()->hasParameter("remove-unused-banks")) {
    std::vector<double> instrumentUnused = m_ws->getInstrument()->getNumberParameter("remove-unused-banks", true);
    if (!instrumentUnused.empty()) {
      const auto unused = static_cast<int>(instrumentUnused.front());
      if (unused == 1)
        deleteBanks(m_ws, bankNames);
    }
  }
  //----------------- Pad Empty Pixels -------------------------------
  createSpectraMapping(m_filename, monitors, someBanks);

  // Set all (empty) event lists as sorted by pulse time. That way, calling
  // SortEvents will not try to sort these empty lists.
  for (size_t i = 0; i < m_ws->getNumberHistograms(); i++)
    m_ws->getSpectrum(i).setSortOrder(DataObjects::PULSETIME_SORT);

  // Count the limits to time of flight
  shortest_tof = static_cast<double>(std::numeric_limits<uint32_t>::max()) * 0.1;
  longest_tof = 0.;

  bool loaded{false};
  auto loaderType = defineLoaderType(haveWeights, oldNeXusFileNames, classType);
  if (loaderType == LoaderType::MULTIPROCESS) {
    auto ws = m_ws->getSingleHeldWorkspace();
    m_file->close();

    struct ExceptionOutput {
      static void out(decltype(g_log) &log, const std::exception &except, int level = 0) {
        log.warning() << std::string(level, ' ') << "exception: " << except.what() << '\n';
        try {
          std::rethrow_if_nested(except);
        } catch (const std::exception &e) {
          ExceptionOutput::out(log, e, level + 1);
        } catch (...) {
        }
      }
    };

    try {
      ParallelEventLoader::loadMultiProcess(*ws, m_filename, m_top_entry_name, bankNames, event_id_is_spec,
                                            getProperty("Precount"));
      g_log.information() << "Used Multiprocess ParallelEventLoader.\n";
      loaded = true;
      shortest_tof = 0.0;
      longest_tof = 1e10;
    } catch (const std::exception &e) {
      ExceptionOutput::out(g_log, e);
      g_log.warning() << "\nMultiprocess event loader failed, falling back "
                         "to default loader.\n";
    }

    safeOpenFile(m_filename);
  }
  if (!loaded) {
    loaderType = LoaderType::DEFAULT; // to be used later
    bool precount = getProperty("Precount");
    int chunk = getProperty("ChunkNumber");
    int totalChunks = getProperty("TotalChunks");
    const auto startTime = std::chrono::high_resolution_clock::now();
    DefaultEventLoader::load(this, *m_ws, haveWeights, event_id_is_spec, bankNames, periodLog->valuesAsVector(),
                             classType, bankNumEvents, oldNeXusFileNames, precount, chunk, totalChunks);
    addTimer("loadEvents", startTime, std::chrono::high_resolution_clock::now());
  }

  // Info reporting
  const std::size_t eventsLoaded = m_ws->getNumberEvents();
  g_log.information() << "Read " << eventsLoaded << " events"
                      << ". Shortest TOF: " << shortest_tof << " microsec; longest TOF: " << longest_tof
                      << " microsec.\n";

  if (shortest_tof < 0)
    g_log.warning() << "The shortest TOF was negative! At least 1 event has an "
                       "invalid time-of-flight.\n";
  if (bad_tofs > 0)
    g_log.warning() << "Found " << bad_tofs
                    << " events with TOF > 2e8. This "
                       "may indicate errors in the raw "
                       "TOF data.\n";

  // Use T0 offset from TOPAZ Parameter file if it exists
  if (m_ws->getInstrument()->hasParameter("T0")) {
    std::vector<double> instrumentT0 = m_ws->getInstrument()->getNumberParameter("T0", true);
    if (!instrumentT0.empty()) {
      const double mT0 = instrumentT0.front();
      if (mT0 != 0.0) {
        auto numHistograms = static_cast<int64_t>(m_ws->getNumberHistograms());
        PARALLEL_FOR_IF(Kernel::threadSafe(*m_ws))
        for (int64_t i = 0; i < numHistograms; ++i) {
          PARALLEL_START_INTERRUPT_REGION
          // Do the offsetting
          m_ws->getSpectrum(i).addTof(mT0);
          PARALLEL_END_INTERRUPT_REGION
        }
        PARALLEL_CHECK_INTERRUPT_REGION
        // set T0 in the run parameters
        API::Run &run = m_ws->mutableRun();
        run.addProperty<double>("T0", mT0, true);
      }
    }
  }
  // Now, create a default X-vector for histogramming, with just 2 bins.
  if (eventsLoaded > 0) {
    int nBins = getProperty("NumberOfBins");
    auto binEdgesVec = std::vector<double>(nBins + 1);
    binEdgesVec[0] = shortest_tof;        // left edge is inclusive
    binEdgesVec[nBins] = longest_tof + 1; // right edge is exclusive
    double binStep = (binEdgesVec[nBins] - binEdgesVec[0]) / nBins;
    for (int binIndex = 1; binIndex < nBins; binIndex++) {
      binEdgesVec[binIndex] = binEdgesVec[0] + (binStep * binIndex);
    }
    m_ws->setAllX(HistogramData::BinEdges{binEdgesVec});
  } else
    m_ws->setAllX(HistogramData::BinEdges{0.0, 1.0});

  // if there is time_of_flight load it
  adjustTimeOfFlightISISLegacy(*m_file, m_ws, m_top_entry_name, classType, descriptor.get());

  if (m_is_time_filtered) {
    if (loaderType == LoaderType::MULTIPROCESS) {
      // Now filter out the run and events, using the DateAndTime type.
      // This will sort both by pulse time
      filterEventsByTime(m_ws, filter_time_start, filter_time_stop);
    } else {
      // events were filtered during read
      // filter the logs the same way FilterByTime does
      TimeROI timeroi(filter_time_start, filter_time_stop);
      if (filter_bad_pulses)
        timeroi.update_intersection(*bad_pulses_timeroi);
      m_ws->mutableRun().setTimeROI(timeroi);
      m_ws->mutableRun().removeDataOutsideTimeROI();
    }
  } else if (filter_bad_pulses) {
    m_ws->mutableRun().setTimeROI(*bad_pulses_timeroi);
    m_ws->mutableRun().removeDataOutsideTimeROI();
  }
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
bool LoadEventNexus::runLoadIDFFromNexus<EventWorkspaceCollection_sptr>(const std::string &nexusfilename,
                                                                        EventWorkspaceCollection_sptr localWorkspace,
                                                                        const std::string &top_entry_name,
                                                                        Algorithm *alg) {
  auto ws = localWorkspace->getSingleHeldWorkspace();
  auto hasLoaded = runLoadIDFFromNexus<MatrixWorkspace_sptr>(nexusfilename, ws, top_entry_name, alg);
  localWorkspace->setInstrument(ws->getInstrument());
  return hasLoaded;
}

/** method used to return instrument name for some old ISIS files where it is
 * not written properly within the instrument
 * @param hFile :: A reference to the NeXus file opened at the root entry
 */
std::string LoadEventNexus::readInstrumentFromISIS_VMSCompat(::NeXus::File &hFile) {
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
 *  @param descriptor :: input containing metadata information
 *  @return true if successful
 */
template <>
bool LoadEventNexus::runLoadInstrument<EventWorkspaceCollection_sptr>(const std::string &nexusfilename,
                                                                      EventWorkspaceCollection_sptr localWorkspace,
                                                                      const std::string &top_entry_name, Algorithm *alg,
                                                                      const Kernel::NexusDescriptor *descriptor) {
  auto ws = localWorkspace->getSingleHeldWorkspace();
  auto hasLoaded = runLoadInstrument<MatrixWorkspace_sptr>(nexusfilename, ws, top_entry_name, alg, descriptor);
  localWorkspace->setInstrument(ws->getInstrument());
  return hasLoaded;
}

//-----------------------------------------------------------------------------
/**
 * Deletes banks for a workspace given the bank names.
 * @param workspace :: The workspace to contain the spectra mapping
 * @param bankNames :: Bank names that are in Nexus file
 */
void LoadEventNexus::deleteBanks(const EventWorkspaceCollection_sptr &workspace,
                                 const std::vector<std::string> &bankNames) {
  Instrument_sptr inst = std::const_pointer_cast<Instrument>(workspace->getInstrument()->baseInstrument());
  // Build a list of Rectangular Detectors
  std::vector<std::shared_ptr<RectangularDetector>> detList;
  for (int i = 0; i < inst->nelements(); i++) {
    std::shared_ptr<RectangularDetector> det;
    std::shared_ptr<ICompAssembly> assem;
    std::shared_ptr<ICompAssembly> assem2;

    det = std::dynamic_pointer_cast<RectangularDetector>((*inst)[i]);
    if (det) {
      detList.emplace_back(det);
    } else {
      // Also, look in the first sub-level for RectangularDetectors (e.g.
      // PG3). We are not doing a full recursive search since that will be
      // very long for lots of pixels.
      assem = std::dynamic_pointer_cast<ICompAssembly>((*inst)[i]);
      if (assem) {
        for (int j = 0; j < assem->nelements(); j++) {
          det = std::dynamic_pointer_cast<RectangularDetector>((*assem)[j]);
          if (det) {
            detList.emplace_back(det);

          } else {
            // Also, look in the second sub-level for RectangularDetectors
            // (e.g. PG3). We are not doing a full recursive search since that
            // will be very long for lots of pixels.
            assem2 = std::dynamic_pointer_cast<ICompAssembly>((*assem)[j]);
            if (assem2) {
              for (int k = 0; k < assem2->nelements(); k++) {
                det = std::dynamic_pointer_cast<RectangularDetector>((*assem2)[k]);
                if (det) {
                  detList.emplace_back(det);
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
    for (const auto &bankName : bankNames) {
      size_t pos = bankName.find("_events");
      if (det_name == bankName.substr(0, pos))
        keep = true;
      if (keep)
        break;
    }
    if (!keep) {
      std::shared_ptr<const IComponent> parent = inst->getComponentByName(det_name);
      std::vector<Geometry::IComponent_const_sptr> children;
      std::shared_ptr<const Geometry::ICompAssembly> asmb =
          std::dynamic_pointer_cast<const Geometry::ICompAssembly>(parent);
      asmb->getChildren(children, false);
      for (auto &col : children) {
        std::shared_ptr<const Geometry::ICompAssembly> asmb2 =
            std::dynamic_pointer_cast<const Geometry::ICompAssembly>(col);
        std::vector<Geometry::IComponent_const_sptr> grandchildren;
        asmb2->getChildren(grandchildren, false);

        for (auto &row : grandchildren) {
          auto *d = dynamic_cast<Detector *>(const_cast<IComponent *>(row.get()));
          if (d)
            inst->removeDetector(d);
        }
      }
      auto *comp = dynamic_cast<IComponent *>(det.get());
      inst->remove(comp);
    }
  }
}
//-----------------------------------------------------------------------------
/**
 * Create the required spectra mapping. If the file contains an
 * isis_vms_compat block then the mapping is read from there, otherwise a 1:1
 * map with the instrument is created (along with the associated spectra axis)
 * @param nxsfile :: The name of a nexus file to load the mapping from
 * @param monitorsOnly :: Load only the monitors is true
 * @param bankNames :: An optional bank name for loading specified banks
 */
void LoadEventNexus::createSpectraMapping(const std::string &nxsfile, const bool monitorsOnly,
                                          const std::vector<std::string> &bankNames) {
  LoadEventNexusIndexSetup indexSetup(m_ws->getSingleHeldWorkspace(), getProperty("SpectrumMin"),
                                      getProperty("SpectrumMax"), getProperty("SpectrumList"));
  if (!monitorsOnly && !bankNames.empty()) {
    if (!isDefault("SpectrumMin") || !isDefault("SpectrumMax") || !isDefault("SpectrumList"))
      g_log.warning() << "Spectrum min/max/list selection ignored when "
                         "`SingleBankPixelsOnly` is enabled\n";
    m_ws->setIndexInfo(indexSetup.makeIndexInfo(bankNames));
    g_log.debug() << "Populated spectra map for select banks\n";
  } else if (auto mapping = loadISISVMSSpectraMapping(m_top_entry_name)) {
    if (monitorsOnly) {
      g_log.debug() << "Loading only monitor spectra from " << nxsfile << "\n";
    } else {
      g_log.debug() << "Loading only detector spectra from " << nxsfile << "\n";
    }
    m_ws->setIndexInfo(indexSetup.makeIndexInfo(*mapping, monitorsOnly));
  } else {
    g_log.debug() << "No custom spectra mapping found, continuing with default "
                     "1:1 mapping of spectrum:detectorID\n";
    m_ws->setIndexInfo(indexSetup.makeIndexInfo());
    g_log.debug() << "Populated 1:1 spectra map for the whole instrument \n";
  }
  std::tie(m_specMin, m_specMax) = indexSetup.eventIDLimits();
}

//-----------------------------------------------------------------------------
/**
 * Load the Monitors from the NeXus file into a workspace. The original
 * workspace name is used and appended with _monitors.
 *
 */
void LoadEventNexus::runLoadMonitors() {
  std::string mon_wsname = this->getProperty("OutputWorkspace");
  mon_wsname.append("_monitors");

  auto loadMonitors = createChildAlgorithm("LoadNexusMonitors");
  g_log.information("Loading monitors from NeXus file...");
  loadMonitors->setPropertyValue("Filename", m_filename);
  g_log.information() << "New workspace name for monitors: " << mon_wsname << '\n';
  loadMonitors->setPropertyValue("OutputWorkspace", mon_wsname);
  loadMonitors->setPropertyValue("LoadOnly", this->getProperty("MonitorsLoadOnly"));
  loadMonitors->setPropertyValue("NXentryName", this->getProperty("NXentryName"));
  loadMonitors->execute();
  Workspace_sptr monsOut = loadMonitors->getProperty("OutputWorkspace");
  // create the output workspace property on the fly
  this->declareProperty(
      std::make_unique<WorkspaceProperty<Workspace>>("MonitorWorkspace", mon_wsname, Direction::Output),
      "Monitors from the Event NeXus file");
  this->setProperty("MonitorWorkspace", monsOut);

  // The output will either be a group workspace or a matrix workspace
  MatrixWorkspace_sptr mons = std::dynamic_pointer_cast<MatrixWorkspace>(monsOut);
  if (mons) {
    // Set the internal monitor workspace pointer as well
    m_ws->setMonitorWorkspace(mons);

    filterDuringPause(mons);
  } else {
    WorkspaceGroup_sptr monsGrp = std::dynamic_pointer_cast<WorkspaceGroup>(monsOut);
    if (monsGrp) {
      // declare a property for each member of the group
      for (int i = 0; i < monsGrp->getNumberOfEntries(); i++) {
        std::stringstream ssWsName;
        ssWsName << mon_wsname << "_" << i + 1;
        std::stringstream ssPropName;
        ssPropName << "MonitorWorkspace"
                   << "_" << i + 1;
        this->declareProperty(
            std::make_unique<WorkspaceProperty<MatrixWorkspace>>(ssPropName.str(), ssWsName.str(), Direction::Output),
            "Monitors from the Event NeXus file");
        this->setProperty(ssPropName.str(), monsGrp->getItem(i));
      }
    }
  }
}

//
/**
 * Load a spectra mapping from the given file. This currently checks for the
 * existence of
 * an isis_vms_compat block in the file, if it exists it pulls out the spectra
 * mapping listed there
 * @param entry_name :: name of the NXentry to open.
 * @returns True if the mapping was loaded or false if the block does not
 * exist
 */
std::unique_ptr<std::pair<std::vector<int32_t>, std::vector<int32_t>>>
LoadEventNexus::loadISISVMSSpectraMapping(const std::string &entry_name) {
  const std::string vms_str = "/isis_vms_compat";
  try {
    g_log.debug() << "Attempting to load custom spectra mapping from '" << entry_name << vms_str << "'.\n";
    m_file->openPath("/" + entry_name + vms_str);
  } catch (::NeXus::Exception &) {
    return nullptr; // Doesn't exist
  }

  // The ISIS spectrum mapping is defined by 2 arrays in isis_vms_compat
  // block:
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
    os << "UDET/SPEC list size mismatch. UDET=" << udet.size() << ", SPEC=" << spec.size() << "\n";
    throw std::runtime_error(os.str());
  }
  // If mapping loaded the event ID is the spectrum number and not det ID
  this->event_id_is_spec = true;
  return std::make_unique<std::pair<std::vector<int32_t>, std::vector<int32_t>>>(std::move(spec), std::move(udet));
}

/**
 * Set the filters on TOF.
 * @param monitors :: If true check the monitor properties else use the
 * standard ones
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
    filter_tof_range = false;
  } else if ((filter_tof_min != EMPTY_DBL()) && (filter_tof_max != EMPTY_DBL())) {
    // Both specified. Keep these values
    filter_tof_range = true;
  } else {
    std::string msg("You must specify both min & max or neither TOF filters");
    if (monitors)
      msg = " for the monitors.";
    throw std::invalid_argument(msg);
  }
}

//-----------------------------------------------------------------------------

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
void LoadEventNexus::loadSampleDataISIScompatibility(::NeXus::File &file, EventWorkspaceCollection &WS) {
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
 * Makes sure that m_file is a valid and open NeXus::File object.
 * Throws if there is an exception opening the file.
 *
 * @param fname name of the nexus file to open
 */
void LoadEventNexus::safeOpenFile(const std::string &fname) {
  try {
    m_file = std::make_unique<::NeXus::File>(m_filename, NXACC_READ);
  } catch (std::runtime_error &e) {
    throw std::runtime_error("Severe failure when trying to open NeXus file: " + std::string(e.what()));
  }
  // make sure that by no means we could dereference NULL later on
  if (!m_file) {
    throw std::runtime_error("An unexpected failure happened, unable to "
                             "initialize file object when trying to open NeXus "
                             "file: " +
                             fname);
  }
}

/// The parallel loader currently has no support for a series of special
/// cases, as indicated by the return value of this method.
LoadEventNexus::LoaderType LoadEventNexus::defineLoaderType(const bool haveWeights, const bool oldNeXusFileNames,
                                                            const std::string &classType) const {
  auto propVal = getPropertyValue("LoadType");
  if (propVal == "Default")
    return LoaderType::DEFAULT;

  bool noParallelConstrictions = true;
  noParallelConstrictions &= !(m_ws->nPeriods() != 1);
  noParallelConstrictions &= !haveWeights;
  noParallelConstrictions &= !oldNeXusFileNames;
  noParallelConstrictions &= !(filter_tof_range);
  noParallelConstrictions &= !((filter_time_start != Types::Core::DateAndTime::minimum() ||
                                filter_time_stop != Types::Core::DateAndTime::maximum()));
  noParallelConstrictions &= !((!isDefault(PropertyNames::COMPRESS_TOL) || !isDefault("SpectrumMin") ||
                                !isDefault("SpectrumMax") || !isDefault("SpectrumList") || !isDefault("ChunkNumber")));
  noParallelConstrictions &= !(classType != "NXevent_data");

  if (!noParallelConstrictions)
    return LoaderType::DEFAULT;
  return LoaderType::MULTIPROCESS;
}
} // namespace Mantid::DataHandling
