// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadNexusMonitors2.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ISISRunLogs.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTimeHelpers.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidNexus/NexusIOHelper.h"

#include <Poco/File.h>
#include <Poco/Path.h>
#include <boost/lexical_cast.hpp>

#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

using namespace Mantid::Kernel::DateAndTimeHelpers;
using Mantid::API::WorkspaceGroup;
using Mantid::API::WorkspaceGroup_sptr;
using Mantid::DataHandling::LoadNexusMonitorsAlg::MonitorInfo;
using Mantid::DataObjects::EventWorkspace;
using Mantid::DataObjects::EventWorkspace_sptr;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::Histogram;
using Mantid::Kernel::Direction;
using Mantid::Kernel::PropertyWithValue;

namespace Mantid::DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadNexusMonitors2)

namespace {
void loadSampleDataISIScompatibilityInfo(::NeXus::File &file, Mantid::API::MatrixWorkspace_sptr const &WS) {
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

    WS->mutableSample().setGeometryFlag(spb[2]); // the flag is in the third value
    WS->mutableSample().setThickness(rspb[3]);
    WS->mutableSample().setHeight(rspb[4]);
    WS->mutableSample().setWidth(rspb[5]);
  } catch (::NeXus::Exception &ex) {
    // it means that the data was not as expected, report the problem
    std::stringstream s;
    s << "Wrong definition found in isis_vms_compat :> " << ex.what();
    file.closeGroup();
    throw std::runtime_error(s.str());
  }

  file.closeGroup();
}

const std::string LOAD_EVENTS("Events");
const std::string LOAD_HISTO("Histogram");

namespace PropertyNames {
const std::string LOGS_ALLOW("AllowList");
const std::string LOGS_BLOCK("BlockList");
} // namespace PropertyNames

// collection of static methods to inspect monitors to determine type
bool keyExists(std::string const &key, std::map<std::string, std::string> const &entries) {
  return entries.find(key) != entries.cend();
}

// returns true if all of the entries necessary for a histogram exist monitor in
// the currently open group
bool isHistoMonitor(::NeXus::File &monitorFileHandle) {
  const auto fields = monitorFileHandle.getEntries();
  return keyExists("data", fields) && keyExists("time_of_flight", fields);
}

std::size_t sizeOfUnopenedSDS(::NeXus::File &file, const std::string &fieldName) {
  file.openData(fieldName);
  auto size = static_cast<std::size_t>(file.getInfo().dims[0]);
  file.closeData();
  return size;
}

bool eventIdNotEmptyIfExists(::NeXus::File &file, std::map<std::string, std::string> const &fields) {
  if (keyExists("event_id", fields))
    return sizeOfUnopenedSDS(file, "event_id") > 1;
  else
    return true;
}

// returns true if all of the entries necessary for an event monitor exist in
// the currently open group
bool isEventMonitor(::NeXus::File &file) {
  const auto fields = file.getEntries();
  return keyExists("event_index", fields) && keyExists("event_time_offset", fields) &&
         keyExists("event_time_zero", fields) && eventIdNotEmptyIfExists(file, fields);
}
} // anonymous namespace

//------------------------------------------------------------------------------
/// Initialization method.
void LoadNexusMonitors2::init() {
  const std::vector<std::string> exts{".nxs.h5", ".nxs"};
  declareProperty(std::make_unique<API::FileProperty>("Filename", "", API::FileProperty::Load, exts),
                  "The name (including its full or relative path) of the NeXus file to "
                  "attempt to load. The file extension must either be .nxs, .NXS, or .nxs.h5");

  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::Workspace>>("OutputWorkspace", "", Kernel::Direction::Output),
      "The name of the output workspace in which to load the NeXus monitors.");

  declareProperty(std::make_unique<Kernel::PropertyWithValue<std::string>>("NXentryName", "", Kernel::Direction::Input),
                  "Optional: Name of the NXentry to load if it's not the default.");

  std::vector<std::string> options{"", LOAD_EVENTS, LOAD_HISTO};
  declareProperty("LoadOnly", "", std::make_shared<Kernel::StringListValidator>(options),
                  "If multiple repesentations exist, which one to load. "
                  "Default is to load the one that is present, and Histogram "
                  "if both are present.");

  declareProperty(std::make_unique<PropertyWithValue<std::vector<std::string>>>(
                      PropertyNames::LOGS_ALLOW, std::vector<std::string>(), Direction::Input),
                  "If specified, only these logs will be loaded from the file (each "
                  "separated by a comma).");
  declareProperty(std::make_unique<PropertyWithValue<std::vector<std::string>>>(
                      PropertyNames::LOGS_BLOCK, std::vector<std::string>(), Direction::Input),
                  "If specified, these logs will NOT be loaded from the file (each "
                  "separated by a comma).");
}

//------------------------------------------------------------------------------
/**
 * Executes the algorithm. Reading in the file and creating and populating
 * the output workspace
 */
void LoadNexusMonitors2::exec() {
  // Retrieve the filename from the properties
  m_filename = this->getPropertyValue("Filename");

  API::Progress prog1(this, 0.0, 0.2, 2);

  if (!canOpenAsNeXus(m_filename)) {
    throw std::runtime_error("Failed to recognize this file as a NeXus file, cannot continue.");
  }

  m_top_entry_name = this->getPropertyValue("NXentryName");
  // must be done here before the NeXus::File, HDF5 files can't have 2
  // simultaneous handlers
  Kernel::NexusHDF5Descriptor descriptor(m_filename);

  // top level file information
  ::NeXus::File file(m_filename);

  // open the correct entry
  using string_map_t = std::map<std::string, std::string>;
  string_map_t entries = file.getEntries();

  if (m_top_entry_name.empty()) {
    const auto it = std::find_if(entries.cbegin(), entries.cend(), [](const auto &entry) {
      return ((entry.first == "entry" || entry.first == "raw_data_1") && entry.second == "NXentry");
    });
    if (it != entries.cend()) {
      file.openGroup(it->first, it->second);
      m_top_entry_name = it->first;
    }
  } else {
    if (!keyExists(m_top_entry_name, entries)) {
      throw std::invalid_argument(m_filename + " does not contain an entry named " + m_top_entry_name);
    }
    file.openGroup(m_top_entry_name, "NXentry"); // Open as will need to be
    // open for subsequent operations
  }
  prog1.report();

  size_t numPeriods = 0;
  m_monitor_count = getMonitorInfo(file, numPeriods);
  // Fix the detector numbers if the defaults above are not correct
  // fixUDets(detector_numbers, file, spectra_numbers, m_monitor_count);
  // a temporary place to put the spectra/detector numbers
  // this gets the ids from the "isis_vms_compat" group
  fixUDets(file);

  if (numPeriods > 1) {
    m_multiPeriodCounts.resize(m_monitor_count);
    m_multiPeriodBinEdges.resize(m_monitor_count);
  }

  // Nothing to do
  if (0 == m_monitor_count) {
    // previous version just used to return, but that
    // threw an error when the OutputWorkspace property was not set.
    // and the error message was confusing.
    // This has changed to throw a specific error.
    throw std::invalid_argument(m_filename + " does not contain any monitors");
  }

  // only used if using event monitors
  // EventWorkspace_sptr eventWS;
  // Create the output workspace
  std::vector<bool> loadMonitorFlags;
  bool useEventMon = createOutputWorkspace(loadMonitorFlags);

  API::Progress prog3(this, 0.6, 1.0, m_monitor_count);

  // cache path to entry for later
  const std::string entryPath = file.getPath();

  // TODO-NEXT: load event monitor if it is required to do so
  //            load histogram monitor if it is required to do so
  // Require a tuple: monitorNames[i], loadAsEvent[i], loadAsHistogram[i]
  for (std::size_t ws_index = 0; ws_index < m_monitor_count; ++ws_index) {
    // already know spectrum and detector numbers
    g_log.debug() << "monIndex = " << m_monitorInfo[ws_index].detNum << '\n';
    g_log.debug() << "spectrumNo = " << m_monitorInfo[ws_index].specNum << '\n';
    m_workspace->getSpectrum(ws_index).setSpectrumNo(m_monitorInfo[ws_index].specNum);
    m_workspace->getSpectrum(ws_index).setDetectorID(m_monitorInfo[ws_index].detNum);

    // Don't actually read all of the monitors
    g_log.information() << "Loading " << m_monitorInfo[ws_index].name;
    if (loadMonitorFlags[ws_index]) {
      g_log.information() << "\n";
      file.openGroup(m_monitorInfo[ws_index].name, "NXmonitor");
      if (useEventMon) {
        // load as an event monitor
        readEventMonitorEntry(file, ws_index);
      } else {
        // load as a histogram monitor
        readHistoMonitorEntry(file, ws_index, numPeriods);
      }
      file.closeGroup(); // NXmonitor
    } else {
      g_log.information() << " is skipped.\n";
    }

    prog3.report();
  }

  if (useEventMon) // set the x-range to be the range for min/max events
  {
    EventWorkspace_sptr eventWS = std::dynamic_pointer_cast<EventWorkspace>(m_workspace);
    double xmin, xmax;
    eventWS->getEventXMinMax(xmin, xmax);

    if (xmin > xmax) {
      xmin = 0;
      xmax = 1;
      if (eventWS->getNumberEvents() == 0) {
        g_log.warning("No events loading. Resetting time-of-flight range 0 to 1");
      } else {
        g_log.warning("time-of-flight range of events are unusual. Resetting time-of-flight range 0 to 1");
      }
    } else {
      // move out by one just like LoadEventNexus
      xmin = xmin - 1;
      xmax = xmax + 1;
    }

    auto axis = HistogramData::BinEdges{xmin, xmax};
    eventWS->setAllX(axis); // Set the binning axis using this.

    // a certain generation of ISIS files modify the time-of-flight
    const std::string currentPath = file.getPath();
    adjustTimeOfFlightISISLegacy(file, eventWS, m_top_entry_name, "NXmonitor");
    file.openPath(currentPath); // reset to where it was earlier
  }

  // Check for and ISIS compat block to get the detector IDs for the loaded
  // spectrum numbers
  // @todo: Find out if there is a better (i.e. more generic) way to do this
  try {
    g_log.debug() << "Load Sample data isis\n";
    loadSampleDataISIScompatibilityInfo(file, m_workspace);
  } catch (::NeXus::Exception &) {
  }

  // Need to get the instrument name from the file
  std::string instrumentName;
  file.openPath(entryPath); // reset path in case of unusual behavior
  file.openGroup("instrument", "NXinstrument");
  try {
    file.openData("name");
    instrumentName = file.getStrData();
    // Now let's close the file as we don't need it anymore to load the
    // instrument.
    file.closeData();
    file.closeGroup(); // Close the NXentry
    file.close();

  } catch (std::runtime_error &) // no correct instrument definition (old ISIS
                                 // file, fall back to isis_vms_compat)
  {
    file.closeGroup(); // Close the instrument NXentry
    instrumentName = LoadEventNexus::readInstrumentFromISIS_VMSCompat(file);
    file.close();
  }

  g_log.debug() << "Instrument name read from NeXus file is " << instrumentName << '\n';

  m_workspace->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
  m_workspace->setYUnit("Counts");

  // Load the logs
  this->runLoadLogs(m_filename, m_workspace);

  // Old SNS files don't have this
  try {
    // The run_start will be loaded from the pulse times.
    Types::Core::DateAndTime run_start(0, 0);
    run_start = m_workspace->getFirstPulseTime();
    m_workspace->mutableRun().addProperty("run_start", run_start.toISO8601String(), true);
  } catch (...) {
    // Old files have the start_time defined, so all SHOULD be good.
    // The start_time, however, has been known to be wrong in old files.
  }
  // Load the instrument
  LoadEventNexus::loadInstrument(m_filename, m_workspace, m_top_entry_name, this);

  // Load the meta data, but don't stop on errors
  g_log.debug() << "Loading metadata\n";
  try {
    LoadEventNexus::loadEntryMetadata<API::MatrixWorkspace_sptr>(m_filename, m_workspace, m_top_entry_name, descriptor);
  } catch (std::exception &e) {
    g_log.warning() << "Error while loading meta data: " << e.what() << '\n';
  }

  // add filename
  m_workspace->mutableRun().addProperty("Filename", m_filename);

  // if multiperiod histogram data
  if ((numPeriods > 1) && (!useEventMon)) {
    splitMutiPeriodHistrogramData(numPeriods);
  } else {
    this->setProperty("OutputWorkspace", m_workspace);
  }
}

//------------------------------------------------------------------------------
/**
 * Fix the detector numbers if the defaults are not correct. Currently checks
 * the isis_vms_compat block and reads them from there if possible.
 *
 * @param file :: A reference to the NeXus file opened at the root entry
 */
void LoadNexusMonitors2::fixUDets(::NeXus::File &file) {
  const size_t nmonitors = m_monitorInfo.size();
  boost::scoped_array<detid_t> det_ids(new detid_t[nmonitors]);
  boost::scoped_array<specnum_t> spec_ids(new specnum_t[nmonitors]);
  // convert the monitor info into two arrays for resorting
  for (size_t i = 0; i < nmonitors; ++i) {
    spec_ids[i] = m_monitorInfo[i].specNum;
    det_ids[i] = m_monitorInfo[i].detNum;
  }

  try {
    file.openGroup("isis_vms_compat", "IXvms");
  } catch (::NeXus::Exception &) {
    return;
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

  // This is a little complicated: Each value in the spec_id array is a value
  // found in the
  // SPEC block of isis_vms_compat. The index that this value is found at then
  // corresponds
  // to the index within the UDET block that holds the detector ID
  std::vector<int32_t>::const_iterator beg = spec.begin();
  for (size_t mon_index = 0; mon_index < nmonitors; ++mon_index) {
    std::vector<int32_t>::const_iterator itr = std::find(spec.begin(), spec.end(), spec_ids[mon_index]);
    if (itr == spec.end()) {
      det_ids[mon_index] = -1;
      continue;
    }
    std::vector<int32_t>::difference_type udet_index = std::distance(beg, itr);
    det_ids[mon_index] = udet[udet_index];
  }
  file.closeGroup();

  // copy the information back into the monitorinfo
  for (size_t i = 0; i < nmonitors; ++i) {
    m_monitorInfo[i].specNum = spec_ids[i];
    m_monitorInfo[i].detNum = det_ids[i];
  }
}

void LoadNexusMonitors2::runLoadLogs(const std::string &filename, const API::MatrixWorkspace_sptr &localWorkspace) {
  // get the properties for which logs to use
  const std::vector<std::string> allow_list = getProperty(PropertyNames::LOGS_ALLOW);
  const std::vector<std::string> block_list = getProperty(PropertyNames::LOGS_BLOCK);

  // do the actual work
  auto loadLogs = createChildAlgorithm("LoadNexusLogs");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    g_log.information() << "Loading logs from NeXus file...\n";
    loadLogs->setPropertyValue("Filename", filename);
    loadLogs->setProperty<API::MatrixWorkspace_sptr>("Workspace", localWorkspace);
    // copy properties for which logs to use/not use
    loadLogs->setProperty<std::vector<std::string>>(PropertyNames::LOGS_ALLOW, allow_list);
    loadLogs->setProperty<std::vector<std::string>>(PropertyNames::LOGS_BLOCK, block_list);
    loadLogs->execute();
  } catch (...) {
    g_log.error() << "Error while loading Logs from Nexus. Some sample logs "
                     "may be missing.\n";
  }
}

//------------------------------------------------------------------------------
/**
 * Helper method to make sure that a file is / can be openend as a NeXus file
 *
 * @param fname :: name of the file
 * @return True if opening the file as NeXus and retrieving entries succeeds
 **/
bool LoadNexusMonitors2::canOpenAsNeXus(const std::string &fname) {
  bool res = true;
  std::unique_ptr<::NeXus::File> filePointer;
  try {
    filePointer = std::make_unique<::NeXus::File>(fname);
    if (filePointer)
      filePointer->getEntries();
  } catch (::NeXus::Exception &e) {
    g_log.error() << "Failed to open as a NeXus file: '" << fname << "', error description: " << e.what() << '\n';
    res = false;
  }
  return res;
}

//------------------------------------------------------------------------------
/**
 * Splits multiperiod histogram data into seperate workspaces and puts them in
 * a group
 *
 * @param numPeriods :: number of periods
 **/
void LoadNexusMonitors2::splitMutiPeriodHistrogramData(const size_t numPeriods) {
  // protection - we should not have entered the routine if these are not true
  // More than 1 period
  if (numPeriods < 2) {
    g_log.warning() << "Attempted to split multiperiod histogram workspace with " << numPeriods
                    << "periods, aborted.\n";
    return;
  }

  // Y array should be divisible by the number of periods
  if (m_multiPeriodCounts[0].size() % numPeriods != 0) {
    g_log.warning() << "Attempted to split multiperiod histogram workspace with " << m_multiPeriodCounts[0].size()
                    << "data entries, into " << numPeriods
                    << "periods."
                       " Aborted.\n";
    return;
  }

  WorkspaceGroup_sptr wsGroup(new WorkspaceGroup);
  size_t yLength = m_multiPeriodCounts[0].size() / numPeriods;
  size_t xLength = yLength + 1;
  size_t numSpectra = m_workspace->getNumberHistograms();
  API::ISISRunLogs monLogCreator(m_workspace->run());

  BinEdges edges = m_multiPeriodBinEdges[0];

  for (size_t i = 0; i < numPeriods; i++) {
    // create the period workspace
    API::MatrixWorkspace_sptr wsPeriod =
        API::WorkspaceFactory::Instance().create(m_workspace, numSpectra, xLength, yLength);

    auto offset = yLength * i;

    for (size_t wsIndex = 0; wsIndex < numSpectra; wsIndex++) {
      auto inYBegin = m_multiPeriodCounts[wsIndex].cbegin() + offset;

      wsPeriod->setHistogram(wsIndex, edges, Counts(inYBegin, inYBegin + yLength));
    }
    // add period logs
    monLogCreator.addStatusLog(wsPeriod->mutableRun());
    monLogCreator.addPeriodLogs(static_cast<int>(i + 1), wsPeriod->mutableRun());

    // add to workspace group
    wsGroup->addWorkspace(wsPeriod);
  }

  // set the output workspace
  this->setProperty("OutputWorkspace", wsGroup);
}

size_t LoadNexusMonitors2::getMonitorInfo(::NeXus::File &file, size_t &numPeriods) {
  // should already be open to the correct NXentry

  m_monitorInfo.clear();

  using string_map_t = std::map<std::string, std::string>;

  // Now we want to go through and find the monitors
  string_map_t entries = file.getEntries();
  numPeriods = 0;
  // we want to sort monitors by monitor_number if they are present

  API::Progress prog2(this, 0.2, 0.6, entries.size());

  string_map_t::const_iterator it = entries.begin();
  for (; it != entries.end(); ++it) {
    std::string entry_name(it->first);
    std::string entry_class(it->second);
    if ((entry_class == "NXmonitor")) {
      MonitorInfo info;

      // check for event/histogram monitor
      // -> This will prefer event monitors over histogram
      //    if they are found in the same group.
      file.openGroup(entry_name, "NXmonitor");
      info.name = entry_name;
      info.hasEvent = isEventMonitor(file);
      info.hasHisto = isHistoMonitor(file);

      // get the detector number
      string_map_t inner_entries = file.getEntries(); // get list of entries
      if (inner_entries.find("monitor_number") != inner_entries.end()) {
        // get monitor number from field in file
        const auto detNum = NeXus::NeXusIOHelper::readNexusValue<int64_t>(file, "monitor_number");
        if (detNum > std::numeric_limits<detid_t>::max()) {
          throw std::runtime_error("Monitor number too larger to represent");
        }
        info.detNum = static_cast<detid_t>(detNum);
      } else {
        // default creates it from monitor name
        Poco::Path monPath(entry_name);
        std::string monitorName = monPath.getBaseName();

        // check for monitor name - in our case will be of the form either
        // monitor1
        // or monitor_1
        std::string::size_type loc = monitorName.rfind('_');
        if (loc == std::string::npos) {
          loc = monitorName.rfind('r');
        }

        info.detNum = -1 * boost::lexical_cast<int>(monitorName.substr(loc + 1)); // SNS default
      }

      // get the spectrum number
      if (inner_entries.find("spectrum_index") != inner_entries.end()) {
        file.openData("spectrum_index");
        file.getData(&info.specNum);
        file.closeData();
      } else {
        // default is to match the detector number
        info.specNum = std::abs(info.detNum);
      }

      if (info.hasHisto && (numPeriods == 0) && (inner_entries.find("period_index") != inner_entries.end())) {
        MantidVec period_data;
        file.openData("period_index");
        file.getDataCoerce(period_data);
        file.closeData();
        numPeriods = period_data.size();
      }

      file.closeGroup(); // close NXmonitor
      m_monitorInfo.emplace_back(info);
    }
    prog2.report();
  }

  // sort based on the absolute value of the monitor number
  // this takes care of the fact that SNS monitors have negative numbers
  std::sort(m_monitorInfo.begin(), m_monitorInfo.end(), [](const MonitorInfo &left, const MonitorInfo &right) {
    return std::abs(left.detNum) < std::abs(right.detNum);
  });

  return m_monitorInfo.size();
}

bool LoadNexusMonitors2::createOutputWorkspace(std::vector<bool> &loadMonitorFlags) {
  loadMonitorFlags.clear();

  size_t numEventMon =
      std::count_if(m_monitorInfo.begin(), m_monitorInfo.end(), [](const MonitorInfo &info) { return info.hasEvent; });
  size_t numHistoMon =
      std::count_if(m_monitorInfo.begin(), m_monitorInfo.end(), [](const MonitorInfo &info) { return info.hasHisto; });

  bool useEventMon; // which type of workspace to create/is created
  const std::string loadType = getProperty("LoadOnly");
  if (loadType == LOAD_EVENTS) {
    useEventMon = true;
    if (numEventMon == 0) { // make sure there are some
      throw std::runtime_error("Loading event data. Trying to load event data but failed to "
                               "find event monitors. This file may be corrupted or it may not be "
                               "supported");
    }
  } else if (loadType == LOAD_HISTO) {
    useEventMon = false;
    if (numHistoMon == 0) { // make sure there are some
      throw std::runtime_error("Not loading event data. Trying to load histogram data but failed to "
                               "find monitors with histogram data or could not interpret the data. "
                               "This file may be corrupted or it may not be supported");
    }
  } else { // only other option is to go with the default
    if (numEventMon > 0 && numHistoMon > 0) {
      std::stringstream errmsg;
      errmsg << "There are " << numHistoMon << " histogram monitors and " << numEventMon
             << " event monitors. Loading Histogram by default. "
             << "Use \"LoadOnly\" or \"MonitorLoadOnly\" to specify which to "
                "load.";
      m_log.warning(errmsg.str());
      useEventMon = false;
    } else {
      // more than one event monitor means use that since both can't be nonzero
      useEventMon = (numEventMon > 0);
    }
  }

  // set up the flags to load monitor
  if (useEventMon) {
    // load event
    for (size_t i_mon = 0; i_mon < m_monitor_count; ++i_mon) {
      loadMonitorFlags.emplace_back(m_monitorInfo[i_mon].hasEvent);
    }
  } else {
    // load histogram
    for (size_t i_mon = 0; i_mon < m_monitor_count; ++i_mon) {
      loadMonitorFlags.emplace_back(m_monitorInfo[i_mon].hasHisto);
    }
  }

  // create workspace
  if (useEventMon) {
    // Use event monitors and create event workspace

    // only used if using event monitors
    EventWorkspace_sptr eventWS = EventWorkspace_sptr(new EventWorkspace());
    eventWS->initialize(m_monitorInfo.size(), 1, 1);

    // Set the units
    eventWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");
    eventWS->setYUnit("Counts");
    m_workspace = eventWS;
  } else { // only other option is histograms
    // Use histogram monitors and event monitors' histogram data.
    // And thus create a Workspace2D.

    m_workspace = API::WorkspaceFactory::Instance().create("Workspace2D", m_monitorInfo.size(), 2, 1);
  }

  return useEventMon;
}

void LoadNexusMonitors2::readEventMonitorEntry(::NeXus::File &file, size_t ws_index) {
  // setup local variables
  EventWorkspace_sptr eventWS = std::dynamic_pointer_cast<EventWorkspace>(m_workspace);
  std::string tof_units, event_time_zero_units;

  // read in the data
  auto event_index = NeXus::NeXusIOHelper::readNexusVector<uint64_t>(file, "event_index");

  file.openData("event_time_offset"); // time of flight
  MantidVec time_of_flight = NeXus::NeXusIOHelper::readNexusVector<double>(file);
  file.getAttr("units", tof_units);
  Kernel::Units::timeConversionVector(time_of_flight, tof_units, "microseconds");
  file.closeData();

  // warn the user if no events were found
  if (time_of_flight.empty()) {
    g_log.error() << "No events found in \"" << m_monitorInfo[ws_index].name << "\"\n";
    return; // early
  }

  file.openData("event_time_zero"); // pulse time
  MantidVec seconds = NeXus::NeXusIOHelper::readNexusVector<double>(file);
  file.getAttr("units", event_time_zero_units);
  Kernel::Units::timeConversionVector(seconds, event_time_zero_units, "seconds");
  Mantid::Types::Core::DateAndTime pulsetime_offset;
  {
    std::string startTime;
    file.getAttr("offset", startTime);
    pulsetime_offset = createFromSanitizedISO8601(startTime);
  }
  file.closeData();

  // load up the event list
  DataObjects::EventList &event_list = eventWS->getSpectrum(ws_index);

  Mantid::Types::Core::DateAndTime pulsetime;
  Mantid::Types::Core::DateAndTime lastpulsetime(0);
  std::size_t numEvents = time_of_flight.size();
  bool pulsetimesincreasing = true;
  size_t pulse_index(0);
  size_t numPulses = seconds.size();
  for (std::size_t j = 0; j < numEvents; ++j) {
    while (!((j >= event_index[pulse_index]) && (j < event_index[pulse_index + 1]))) {
      pulse_index += 1;
      if (pulse_index > (numPulses + 1))
        break;
    }
    if (pulse_index >= (numPulses))
      pulse_index = numPulses - 1; // fix it
    pulsetime = pulsetime_offset + seconds[pulse_index];
    if (pulsetime < lastpulsetime)
      pulsetimesincreasing = false;
    lastpulsetime = pulsetime;
    event_list.addEventQuickly(Types::Event::TofEvent(time_of_flight[j], pulsetime));
  }
  if (pulsetimesincreasing)
    event_list.setSortOrder(DataObjects::PULSETIME_SORT);
}

void LoadNexusMonitors2::readHistoMonitorEntry(::NeXus::File &file, size_t ws_index, size_t numPeriods) {
  // Now, actually retrieve the necessary data
  file.openData("data");
  MantidVec data;
  file.getDataCoerce(data);
  file.closeData();

  // Get the TOF axis
  file.openData("time_of_flight");
  MantidVec tof;
  file.getDataCoerce(tof);
  file.closeData();

  if (numPeriods > 1) {
    m_multiPeriodBinEdges[ws_index] = std::move(tof);
    m_multiPeriodCounts[ws_index] = std::move(data);
  } else {
    m_workspace->setHistogram(ws_index, Histogram(BinEdges(std::move(tof)), Counts(std::move(data))));
  }
}

} // namespace Mantid::DataHandling
