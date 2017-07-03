#include "MantidDataHandling/LoadNexusMonitors2.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/ISISRunLogs.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/UnitFactory.h"

#include <Poco/File.h>
#include <Poco/Path.h>
#include <boost/lexical_cast.hpp>

#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

using Mantid::DataObjects::EventWorkspace;
using Mantid::DataObjects::EventWorkspace_sptr;
using Mantid::API::WorkspaceGroup;
using Mantid::API::WorkspaceGroup_sptr;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::Histogram;

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadNexusMonitors2)

namespace {
void loadSampleDataISIScompatibilityInfo(
    ::NeXus::File &file, Mantid::API::MatrixWorkspace_sptr const WS) {
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

    WS->mutableSample().setGeometryFlag(
        spb[2]); // the flag is in the third value
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
} // namespace

//------------------------------------------------------------------------------
/// Initialization method.
void LoadNexusMonitors2::init() {
  declareProperty(
      Kernel::make_unique<API::FileProperty>("Filename", "",
                                             API::FileProperty::Load, ".nxs"),
      "The name (including its full or relative path) of the NeXus file to "
      "attempt to load. The file extension must either be .nxs or .NXS");

  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::Workspace>>(
          "OutputWorkspace", "", Kernel::Direction::Output),
      "The name of the output workspace in which to load the NeXus monitors.");

  declareProperty(Kernel::make_unique<Kernel::PropertyWithValue<bool>>(
                      "MonitorsAsEvents", true, Kernel::Direction::Input),
                  "If enabled (by default), load the monitors as events (into "
                  "an EventWorkspace), as long as there is event data. If "
                  "disabled, load monitors as spectra (into a Workspace2D, "
                  "regardless of whether event data is found.");

  declareProperty("LoadEventMonitors", true,
                  "Load event monitor in NeXus file both event monitor and "
                  "histogram monitor found in NeXus file."
                  "If both of LoadEventMonitor and LoadHistoMonitor are true, "
                  "or both of them are false,"
                  "then it is in the auto mode such that any existing monitor "
                  "will be loaded.");

  declareProperty("LoadHistoMonitors", true,
                  "Load histogram monitor in NeXus file both event monitor and "
                  "histogram monitor found in NeXus file."
                  "If both of LoadEventMonitor and LoadHistoMonitor are true, "
                  "or both of them are false,"
                  "then it is in the auto mode such that any existing monitor "
                  "will be loaded.");
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
    throw std::runtime_error(
        "Failed to recognize this file as a NeXus file, cannot continue.");
  }

  // top level file information
  ::NeXus::File file(m_filename);

  // Start with the base entry
  typedef std::map<std::string, std::string> string_map_t;
  string_map_t::const_iterator it;
  string_map_t entries = file.getEntries();
  for (it = entries.begin(); it != entries.end(); ++it) {
    if (((it->first == "entry") || (it->first == "raw_data_1")) &&
        (it->second == "NXentry")) {
      file.openGroup(it->first, it->second);
      m_top_entry_name = it->first;
      break;
    }
  }
  prog1.report();

  size_t numHistMon = 0;
  size_t numEventMon = 0;
  size_t numPeriods = 0;
  std::vector<std::string> monitorNames;
  std::map<int, std::string> monitorNumber2Name;
  std::vector<bool> isEventMonitors;
  m_monitor_count =
      getMonitorInfo(file, monitorNames, numHistMon, numEventMon, numPeriods,
                     monitorNumber2Name, isEventMonitors);

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

  // With this property you can make the exception that even if there's event
  // data, monitors will be loaded
  // as histograms (if set to false)
  bool monitorsAsEvents = getProperty("MonitorsAsEvents");
  // Beware, even if monitorsAsEvents==False (user requests to load monitors as
  // histograms)
  // check if there's histogram data. If not, ignore setting, step back and load
  // monitors as
  // events which is the only possibility left.

  m_allMonitorsHaveHistoData = allMonitorsHaveHistoData(file, monitorNames);
  if (!monitorsAsEvents)
    if (!m_allMonitorsHaveHistoData) {
      g_log.information() << "Cannot load monitors as histogram data. Loading "
                             "as events even if the opposite was requested by "
                             "disabling the property MonitorsAsEvents\n";
      monitorsAsEvents = true;
    }

  // only used if using event monitors
  // EventWorkspace_sptr eventWS;
  // Create the output workspace
  std::vector<bool> loadMonitorFlags;
  bool useEventMon = createOutputWorkspace(
      numHistMon, numEventMon, monitorsAsEvents, monitorNames, isEventMonitors,
      monitorNumber2Name, loadMonitorFlags);

  // a temporary place to put the spectra/detector numbers
  boost::scoped_array<specnum_t> spectra_numbers(
      new specnum_t[m_monitor_count]);
  boost::scoped_array<detid_t> detector_numbers(new detid_t[m_monitor_count]);

  API::Progress prog3(this, 0.6, 1.0, m_monitor_count);

  // TODO-NEXT: load event monitor if it is required to do so
  //            load histogram monitor if it is required to do so
  // Require a tuple: monitorNames[i], loadAsEvent[i], loadAsHistogram[i]
  size_t ws_index = 0;
  for (std::size_t i_mon = 0; i_mon < m_monitor_count; ++i_mon) {

    // TODO 1: SKIP if this monitor is not to be loaded!
    g_log.information() << "Loading " << monitorNames[i_mon];
    if (loadMonitorFlags[i_mon]) {
      g_log.information() << "\n";
    } else {
      g_log.information() << " is skipped.\n";
      continue;
    }

    // TODO 2: CHECK
    if (ws_index == m_workspace->getNumberHistograms())
      throw std::runtime_error(
          "Overcedes the number of histograms in output event "
          "workspace.");

    // TODO 3: REFACTOR to get spectrumNo and momIndex
    // Do not rely on the order in path list
    Poco::Path monPath(monitorNames[i_mon]);
    std::string monitorName = monPath.getBaseName();

    // check for monitor name - in our case will be of the form either monitor1
    // or monitor_1
    std::string::size_type loc = monitorName.rfind('_');
    if (loc == std::string::npos) {
      loc = monitorName.rfind('r');
    }

    detid_t monIndex = -1 * boost::lexical_cast<int>(
                                monitorName.substr(loc + 1)); // SNS default
    file.openGroup(monitorNames[i_mon], "NXmonitor");

    // Check if the spectra index is there
    specnum_t spectrumNo(static_cast<specnum_t>(ws_index + 1));
    try {
      file.openData("spectrum_index");
      file.getData(&spectrumNo);
      file.closeData();
    } catch (::NeXus::Exception &) {
      // Use the default as matching the workspace index
    }

    g_log.debug() << "monIndex = " << monIndex << '\n';
    g_log.debug() << "spectrumNo = " << spectrumNo << '\n';

    spectra_numbers[ws_index] = spectrumNo;
    detector_numbers[ws_index] = monIndex;

    if (useEventMon) {
      // load as an event monitor
      readEventMonitorEntry(file, ws_index);
    } else {
      // load as a histogram monitor
      readHistoMonitorEntry(file, ws_index, numPeriods);
    }

    file.closeGroup(); // NXmonitor

    // Default values, might change later.
    m_workspace->getSpectrum(ws_index).setSpectrumNo(spectrumNo);
    m_workspace->getSpectrum(ws_index).setDetectorID(monIndex);

    ++ws_index;
    prog3.report();
  }

  if (useEventMon) // set the x-range to be the range for min/max events
  {
    EventWorkspace_sptr eventWS =
        boost::dynamic_pointer_cast<EventWorkspace>(m_workspace);
    double xmin, xmax;
    eventWS->getEventXMinMax(xmin, xmax);

    auto axis = HistogramData::BinEdges{xmin - 1, xmax + 1};
    eventWS->setAllX(axis); // Set the binning axis using this.
  }

  // Fix the detector numbers if the defaults above are not correct
  // fixUDets(detector_numbers, file, spectra_numbers, m_monitor_count);
  fixUDets(detector_numbers, file, spectra_numbers,
           m_workspace->getNumberHistograms());

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

  g_log.debug() << "Instrument name read from NeXus file is " << instrumentName
                << '\n';

  m_workspace->getAxis(0)->unit() =
      Kernel::UnitFactory::Instance().create("TOF");
  m_workspace->setYUnit("Counts");

  // Load the logs
  this->runLoadLogs(m_filename, m_workspace);

  // Old SNS files don't have this
  try {
    // The run_start will be loaded from the pulse times.
    Kernel::DateAndTime run_start(0, 0);
    run_start = m_workspace->getFirstPulseTime();
    m_workspace->mutableRun().addProperty("run_start",
                                          run_start.toISO8601String(), true);
  } catch (...) {
    // Old files have the start_time defined, so all SHOULD be good.
    // The start_time, however, has been known to be wrong in old files.
  }
  // Load the instrument
  LoadEventNexus::loadInstrument(m_filename, m_workspace, m_top_entry_name,
                                 this);

  // Load the meta data, but don't stop on errors
  g_log.debug() << "Loading metadata\n";
  try {
    LoadEventNexus::loadEntryMetadata<API::MatrixWorkspace_sptr>(
        m_filename, m_workspace, m_top_entry_name);
  } catch (std::exception &e) {
    g_log.warning() << "Error while loading meta data: " << e.what() << '\n';
  }

  // Fix the detector IDs/spectrum numbers
  for (size_t i = 0; i < m_workspace->getNumberHistograms(); i++) {
    m_workspace->getSpectrum(i).setSpectrumNo(spectra_numbers[i]);
    m_workspace->getSpectrum(i).setDetectorID(detector_numbers[i]);
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
* Can we get a histogram (non event data) for every monitor?
*
* @param file :: NeXus file object (open)
* @param monitorNames :: names of monitors of interest
* @return If there seems to be histograms for all monitors (they have "data")
**/
bool LoadNexusMonitors2::allMonitorsHaveHistoData(
    ::NeXus::File &file, const std::vector<std::string> &monitorNames) {
  bool res = true;

  try {
    for (std::size_t i = 0; i < m_monitor_count; ++i) {
      file.openGroup(monitorNames[i], "NXmonitor");
      file.openData("data");
      file.closeData();
      file.closeGroup();
    }
  } catch (::NeXus::Exception &) {
    file.closeGroup();
    res = false;
  }
  return res;
}

//------------------------------------------------------------------------------
/**
* Fix the detector numbers if the defaults are not correct. Currently checks
* the isis_vms_compat block and reads them from there if possible.
*
* @param det_ids :: An array of prefilled detector IDs
* @param file :: A reference to the NeXus file opened at the root entry
* @param spec_ids :: An array of spectrum numbers that the monitors have
* @param nmonitors :: The size of the det_ids and spec_ids arrays
*/
void LoadNexusMonitors2::fixUDets(
    boost::scoped_array<detid_t> &det_ids, ::NeXus::File &file,
    const boost::scoped_array<specnum_t> &spec_ids,
    const size_t nmonitors) const {
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
    std::vector<int32_t>::const_iterator itr =
        std::find(spec.begin(), spec.end(), spec_ids[mon_index]);
    if (itr == spec.end()) {
      det_ids[mon_index] = -1;
      continue;
    }
    std::vector<int32_t>::difference_type udet_index = std::distance(beg, itr);
    det_ids[mon_index] = udet[udet_index];
  }
  file.closeGroup();
}

void LoadNexusMonitors2::runLoadLogs(const std::string filename,
                                     API::MatrixWorkspace_sptr localWorkspace) {
  // do the actual work
  API::IAlgorithm_sptr loadLogs = createChildAlgorithm("LoadNexusLogs");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    g_log.information() << "Loading logs from NeXus file...\n";
    loadLogs->setPropertyValue("Filename", filename);
    loadLogs->setProperty<API::MatrixWorkspace_sptr>("Workspace",
                                                     localWorkspace);
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
  ::NeXus::File *f = nullptr;
  try {
    f = new ::NeXus::File(fname);
    if (f)
      f->getEntries();
  } catch (::NeXus::Exception &e) {
    g_log.error() << "Failed to open as a NeXus file: '" << fname
                  << "', error description: " << e.what() << '\n';
    res = false;
  }
  if (f)
    delete f;
  return res;
}

//------------------------------------------------------------------------------
/**
* Splits multiperiod histogram data into seperate workspaces and puts them in
* a group
*
* @param numPeriods :: number of periods
**/
void LoadNexusMonitors2::splitMutiPeriodHistrogramData(
    const size_t numPeriods) {
  // protection - we should not have entered the routine if these are not true
  // More than 1 period
  if (numPeriods < 2) {
    g_log.warning()
        << "Attempted to split multiperiod histogram workspace with "
        << numPeriods << "periods, aborted.\n";
    return;
  }

  // Y array should be divisible by the number of periods
  if (m_multiPeriodCounts[0].size() % numPeriods != 0) {
    g_log.warning()
        << "Attempted to split multiperiod histogram workspace with "
        << m_multiPeriodCounts[0].size() << "data entries, into " << numPeriods
        << "periods."
           " Aborted.\n";
    return;
  }

  WorkspaceGroup_sptr wsGroup(new WorkspaceGroup);
  size_t yLength = m_multiPeriodCounts[0].size() / numPeriods;
  size_t xLength = yLength + 1;
  size_t numSpectra = m_workspace->getNumberHistograms();
  ISISRunLogs monLogCreator(m_workspace->run());

  BinEdges edges = m_multiPeriodBinEdges[0];

  for (size_t i = 0; i < numPeriods; i++) {
    // create the period workspace
    API::MatrixWorkspace_sptr wsPeriod =
        API::WorkspaceFactory::Instance().create(m_workspace, numSpectra,
                                                 xLength, yLength);

    auto offset = yLength * i;

    for (size_t wsIndex = 0; wsIndex < numSpectra; wsIndex++) {
      auto inYBegin = m_multiPeriodCounts[wsIndex].cbegin() + offset;

      wsPeriod->setHistogram(wsIndex, edges,
                             Counts(inYBegin, inYBegin + yLength));
    }
    // add period logs
    monLogCreator.addStatusLog(wsPeriod->mutableRun());
    monLogCreator.addPeriodLogs(static_cast<int>(i + 1),
                                wsPeriod->mutableRun());

    // add to workspace group
    wsGroup->addWorkspace(wsPeriod);
  }

  // set the output workspace
  this->setProperty("OutputWorkspace", wsGroup);
}

size_t LoadNexusMonitors2::getMonitorInfo(
    ::NeXus::File &file, std::vector<std::string> &monitorNames,
    size_t &numHistMon, size_t &numEventMon, size_t &numPeriods,
    std::map<int, std::string> &monitorNumber2Name,
    std::vector<bool> &isEventMonitors) {
  typedef std::map<std::string, std::string> string_map_t;

  // Now we want to go through and find the monitors
  string_map_t entries = file.getEntries();
  monitorNames.clear();
  numHistMon = 0;
  numEventMon = 0;
  numPeriods = 0;
  // we want to sort monitors by monitor_number if they are present
  monitorNumber2Name.clear();
  // prog1.report();

  API::Progress prog2(this, 0.2, 0.6, entries.size());

  string_map_t::const_iterator it = entries.begin();
  for (; it != entries.end(); ++it) {
    std::string entry_name(it->first);
    std::string entry_class(it->second);
    if ((entry_class == "NXmonitor")) {
      monitorNames.push_back(entry_name);

      // check for event/histogram monitor
      // -> This will prefer event monitors over histogram
      //    if they are found in the same group.
      file.openGroup(entry_name, "NXmonitor");
      int numEventThings =
          0; // number of things that are eventish - should be 3
      string_map_t inner_entries = file.getEntries(); // get list of entries
      for (auto &entry : inner_entries) {
        if (entry.first == "event_index") {
          numEventThings += 1;
          continue;
        } else if (entry.first == "event_time_offset") {
          numEventThings += 1;
          continue;
        } else if (entry.first == "event_time_zero") {
          numEventThings += 1;
          continue;
        }
      }

      if (numEventThings == 3) {
        // it is an event monitor
        numEventMon += 1;
        isEventMonitors.push_back(true);
      } else {
        // it is a histogram monitor
        numHistMon += 1;
        isEventMonitors.push_back(false);

        if (inner_entries.find("monitor_number") != inner_entries.end()) {
          specnum_t monitorNo;
          file.openData("monitor_number");
          file.getData(&monitorNo);
          file.closeData();
          monitorNumber2Name[monitorNo] = entry_name;
        }
        if ((numPeriods == 0) &&
            (inner_entries.find("period_index") != inner_entries.end())) {
          MantidVec period_data;
          file.openData("period_index");
          file.getDataCoerce(period_data);
          file.closeData();
          numPeriods = period_data.size();
        }
      }
      file.closeGroup(); // close NXmonitor
    }
    prog2.report();
  }

  return monitorNames.size();
}

/** Create output workspace
* @brief LoadNexusMonitors2::createOutputWorkspace
* @param numHistMon
* @param numEventMon
* @param monitorsAsEvents
* @param monitorNames
* @param isEventMonitors
* @param monitorNumber2Name
* @param loadMonitorFlags
* @return
*/
bool LoadNexusMonitors2::createOutputWorkspace(
    size_t numHistMon, size_t numEventMon, bool monitorsAsEvents,
    std::vector<std::string> &monitorNames, std::vector<bool> &isEventMonitors,
    const std::map<int, std::string> &monitorNumber2Name,
    std::vector<bool> &loadMonitorFlags) {

  // Find out using event monitor or histogram monitor
  bool loadEventMon = getProperty("LoadEventMonitors");
  bool loadHistoMon = getProperty("LoadHistoMonitors");
  if (!loadEventMon && !loadHistoMon) {
    // both of them are false is equivlanet to both of them are true
    loadEventMon = true;
    loadHistoMon = true;
  }
  // create vector for flags to load monitor or not
  loadMonitorFlags.clear();
  loadMonitorFlags.resize(m_monitor_count);

  bool useEventMon;
  // Create the output workspace
  if (numHistMon == m_monitor_count) {
    // all monitors are histogram monitors
    useEventMon = false;
    // with single type of monitor, there is no need to be specified right by
    // user
    loadHistoMon = true;
    loadEventMon = false;

  } else if (numEventMon == m_monitor_count) {
    // all monitors are event monitors
    useEventMon = true;
    // with single type of monitor, there is no need to be specified right by
    // user
    loadHistoMon = false;
    loadEventMon = true;

  } else if (loadEventMon == loadHistoMon && !monitorsAsEvents) {
    // Both event monitors and histogram monitors exist
    // while the user wants the result be read from histogram data
    // in the event monitor

    // check
    if (!m_allMonitorsHaveHistoData) {
      std::stringstream errmsg;
      errmsg << "There are " << numHistMon << " histogram monitors and "
             << numEventMon << " event monitors.  But not all of the event "
             << "monitors have 'data' entry to be converted to histogram.";
      throw std::invalid_argument(errmsg.str());
    }
    // set value
    useEventMon = false;
  } else if (loadEventMon == loadHistoMon && monitorsAsEvents) {
    // Both event monitors are histogram monitor exist,
    // But the user tries to export them as event data.
    std::stringstream errmsg;
    errmsg << "There are " << numHistMon << " histogram monitors and "
           << numEventMon << " event monitors.  It is not allowed to "
           << "read all of them as event monitor.";
    throw std::invalid_argument(errmsg.str());
  } else if (loadEventMon) {
    // coexistence of event monitor and histo monitor. load event monitor only.
    useEventMon = true;
  } else {
    // coexistence of event monitor and histo monitor. load histo monitor only.
    useEventMon = false;
  }

  // set up the flags to load monitor
  if (useEventMon) {
    // load event
    for (size_t i_mon = 0; i_mon < m_monitor_count; ++i_mon) {
      if (isEventMonitors[i_mon])
        loadMonitorFlags[i_mon] = true;
      else
        loadMonitorFlags[i_mon] = false;
    }
  } else {
    // load histogram
    for (size_t i_mon = 0; i_mon < m_monitor_count; ++i_mon) {
      if (!isEventMonitors[i_mon]) {
        // histo
        loadMonitorFlags[i_mon] = true;
      } else if (loadEventMon && loadHistoMon) {
        // event mode but load both
        loadMonitorFlags[i_mon] = true;
      } else {
        loadMonitorFlags[i_mon] = false;
      }
    }
  }

  // create workspace
  if (useEventMon) {
    // Use event monitors and create event workspace
    // check
    if (numEventMon == 0)
      throw std::runtime_error(
          "Loading event data. Trying to load event data but failed to "
          "find event monitors."
          "This file may be corrupted or it may not be supported");

    // only used if using event monitors
    EventWorkspace_sptr eventWS = EventWorkspace_sptr(new EventWorkspace());
    eventWS->initialize(numEventMon, 1, 1);

    // Set the units
    eventWS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("TOF");
    eventWS->setYUnit("Counts");
    m_workspace = eventWS;
  } else {
    // Use histogram monitors and event monitors' histogram data.
    // And thus create a Workspace2D.
    // check
    if (m_monitor_count == 0)
      throw std::runtime_error(
          "Not loading event data. Trying to load histogram data but failed to "
          "find monitors with histogram data or could not interpret the data. "
          "This file may be corrupted or it may not be supported");

    // Create
    size_t numSpec(numHistMon);
    if (loadEventMon)
      numSpec = m_monitor_count;

    m_workspace =
        API::WorkspaceFactory::Instance().create("Workspace2D", numSpec, 2, 1);
    // if there is a distinct monitor number for each monitor sort them by that
    // number
    if (monitorNumber2Name.size() == monitorNames.size()) {
      monitorNames.clear();
      for (auto &numberName : monitorNumber2Name) {
        monitorNames.push_back(numberName.second);
      }
    }
  }

  return useEventMon;
}

void LoadNexusMonitors2::readEventMonitorEntry(NeXus::File &file, size_t i) {
  // setup local variables
  EventWorkspace_sptr eventWS =
      boost::dynamic_pointer_cast<EventWorkspace>(m_workspace);

  std::vector<uint64_t> event_index;
  MantidVec time_of_flight;
  std::string tof_units;
  MantidVec seconds;

  // read in the data
  file.openData("event_index");
  file.getData(event_index);
  file.closeData();
  file.openData("event_time_offset");
  file.getDataCoerce(time_of_flight);
  file.getAttr("units", tof_units);
  file.closeData();
  file.openData("event_time_zero");
  file.getDataCoerce(seconds);
  Mantid::Kernel::DateAndTime pulsetime_offset;
  {
    std::string startTime;
    file.getAttr("offset", startTime);
    pulsetime_offset = Mantid::Kernel::DateAndTime(startTime);
  }
  file.closeData();

  // load up the event list
  DataObjects::EventList &event_list = eventWS->getSpectrum(i);

  Mantid::Kernel::DateAndTime pulsetime(0);
  Mantid::Kernel::DateAndTime lastpulsetime(0);
  std::size_t numEvents = time_of_flight.size();
  bool pulsetimesincreasing = true;
  size_t pulse_index(0);
  size_t numPulses = seconds.size();
  for (std::size_t j = 0; j < numEvents; ++j) {
    while (!((j >= event_index[pulse_index]) &&
             (j < event_index[pulse_index + 1]))) {
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
    event_list.addEventQuickly(
        DataObjects::TofEvent(time_of_flight[j], pulsetime));
  }
  if (pulsetimesincreasing)
    event_list.setSortOrder(DataObjects::PULSETIME_SORT);
}

void LoadNexusMonitors2::readHistoMonitorEntry(NeXus::File &file, size_t i,
                                               size_t numPeriods) {
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
    m_multiPeriodBinEdges[i] = std::move(tof);
    m_multiPeriodCounts[i] = std::move(data);
  } else {
    m_workspace->setHistogram(
        i, Histogram(BinEdges(std::move(tof)), Counts(std::move(data))));
  }
}

} // end DataHandling
} // end Mantid
