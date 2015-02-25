//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/FileProperty.h"
#include "LoadRaw/isisraw2.h"
#include "MantidDataHandling/LoadLog.h"
#include "MantidAPI/SpectrumDetectorMapping.h"

#include <boost/shared_ptr.hpp>
#include <Poco/Path.h>
#include <cmath>
#include <cstdio> //Required for gcc 4.4

namespace Mantid {
namespace DataHandling {
DECLARE_FILELOADER_ALGORITHM(LoadRaw3);

using namespace Kernel;
using namespace API;

/// Constructor
LoadRaw3::LoadRaw3()
  : m_filename(), m_numberOfSpectra(), m_cache_options(),
    m_specTimeRegimes(), m_noTimeRegimes(0), m_prog(0.0),
    m_prog_start(0.0), m_prog_end(1.0), m_lengthIn(0),
    m_timeChannelsVec(), m_total_specs(0), m_periodList() {
}

LoadRaw3::~LoadRaw3() {}

/// Initialization method.
void LoadRaw3::init() {
  LoadRawHelper::init();
  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(1);
  declareProperty(
      "SpectrumMin", 1, mustBePositive,
      "The index number of the first spectrum to read.  Only used if\n"
      "SpectrumMax is set.");
  declareProperty(
      "SpectrumMax", EMPTY_INT(), mustBePositive,
      "The number of the last spectrum to read. Only used if explicitly\n"
      "set.");
  declareProperty(
      new ArrayProperty<specid_t>("SpectrumList"),
      "A comma-separated list of individual spectra to read.  Only used if\n"
      "explicitly set.");
  declareProperty(
      new ArrayProperty<int>("PeriodList"),
      "A comma-separated list of individual periods to read.  Only used if\n"
      "explicitly set.");

  std::vector<std::string> monitorOptions;
  monitorOptions.push_back("Include");
  monitorOptions.push_back("Exclude");
  monitorOptions.push_back("Separate");
  std::map<std::string, std::string> monitorOptionsAliases;
  monitorOptionsAliases["1"] = "Separate";
  monitorOptionsAliases["0"] = "Exclude";
  declareProperty("LoadMonitors", "Include",
                  boost::make_shared<StringListValidator>(
                      monitorOptions, monitorOptionsAliases),
                  "Option to control the loading of monitors.\n"
                  "Allowed options are Include,Exclude, Separate.\n"
                  "Include:The default is Include option which loads the "
                  "monitors into the output workspace.\n"
                  "Exclude:The Exclude option excludes monitors from the "
                  "output workspace.\n"
                  "Separate:The Separate option loads monitors into a separate "
                  "workspace called OutputWorkspace_monitor.\n"
                  "Defined aliases:\n"
                  "1:  Equivalent to Separate.\n"
                  "0:  Equivalent to Exclude.\n");
}

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *  @throw Exception::FileError If the RAW file cannot be found/opened
 *  @throw std::invalid_argument If the optional properties are set to invalid
 * values
 */
void LoadRaw3::exec() {
  // Retrieve the filename from the properties
  m_filename = getPropertyValue("Filename");
  // open the raw file
  FILE *file = openRawFile(m_filename);

  bool bLoadlogFiles = getProperty("LoadLogFiles");

  bool bincludeMonitors, bseparateMonitors, bexcludeMonitors;
  LoadRawHelper::ProcessLoadMonitorOptions(bincludeMonitors, bseparateMonitors,
                                           bexcludeMonitors, this);

  std::string title;
  // read workspace title from raw file
  readTitle(file, title);

  // read workspace dimensions,number of periods etc from the raw file.
  readworkspaceParameters(m_numberOfSpectra, m_numberOfPeriods, m_lengthIn,
                          m_noTimeRegimes);

  setOptionalProperties();
  // to validate the optional parameters, if set
  checkOptionalProperties();

  // Calculate the size of a workspace, given its number of periods & spectra to
  // read
  m_total_specs = calculateWorkspaceSize();

  // Get the time channel array(s) and store in a vector inside a shared pointer
  m_timeChannelsVec = getTimeChannels(m_noTimeRegimes, m_lengthIn);

  // The first period to load
  int firstPeriod = isSelectedPeriods() ? m_periodList.front() - 1 : 0;

  // Create the 2D workspace for the output
  DataObjects::Workspace2D_sptr localWorkspace =
      createWorkspace(m_total_specs, m_lengthIn, m_lengthIn - 1, title);

  // Only run the Child Algorithms once
  loadRunParameters(localWorkspace);
  const SpectrumDetectorMapping detectorMapping(isisRaw->spec, isisRaw->udet,
                                                isisRaw->i_det);
  localWorkspace->updateSpectraUsing(detectorMapping);

  runLoadInstrument(m_filename, localWorkspace, 0.0, 0.4);
  m_prog_start = 0.4;
  Run &run = localWorkspace->mutableRun();
  if (bLoadlogFiles) {
    runLoadLog(m_filename, localWorkspace, 0.4, 0.5);
    m_prog_start = 0.5;
    const int period_number = firstPeriod + 1;
    createPeriodLogs(period_number, localWorkspace);
  }
  // Set the total proton charge for this run
  setProtonCharge(run);
  setRunNumber(run);
  run.addProperty("Filename", m_filename);

  // Set progress to start of range taking account of ChildAlgorithms
  setProg(0.0);

  // populate instrument parameters
  g_log.debug("Populating the instrument parameters...");
  progress(m_prog, "Populating the instrument parameters...");
  localWorkspace->populateInstrumentParameters();

  WorkspaceGroup_sptr ws_grp = createGroupWorkspace();
  WorkspaceGroup_sptr monitorws_grp;
  DataObjects::Workspace2D_sptr monitorWorkspace;
  specid_t normalwsSpecs = 0;
  specid_t monitorwsSpecs = 0;
  std::vector<specid_t> monitorSpecList;

  if (bincludeMonitors) {
    setWorkspaceProperty("OutputWorkspace", title, ws_grp, localWorkspace,
                         m_numberOfPeriods, false, this);
  } else {
    // gets the monitor spectra list from workspace
    monitorSpecList = getmonitorSpectrumList(detectorMapping);
    // calculate the workspace size for normal workspace and monitor workspace
    calculateWorkspacesizes(monitorSpecList, normalwsSpecs, monitorwsSpecs);
    try {
      validateWorkspaceSizes(bexcludeMonitors, bseparateMonitors, normalwsSpecs,
                             monitorwsSpecs);
    } catch (std::out_of_range &) {
      fclose(file);
      throw;
    }

    // now create a workspace of size normalwsSpecs and set it as output
    // workspace
    if (normalwsSpecs > 0) {
      localWorkspace = createWorkspace(localWorkspace, normalwsSpecs,
                                       m_lengthIn, m_lengthIn - 1);
      setWorkspaceProperty("OutputWorkspace", title, ws_grp, localWorkspace,
                           m_numberOfPeriods, false, this);
    }
    // now create monitor workspace if separateMonitors selected
    if (bseparateMonitors) {
      createMonitorWorkspace(monitorWorkspace, localWorkspace, monitorws_grp,
                             monitorwsSpecs, normalwsSpecs, m_numberOfPeriods,
                             m_lengthIn, title, this);
    }
  }

  if (bseparateMonitors && normalwsSpecs == 0) {
    // Ensure we fill the correct group as if we are only loading monitors then
    // we essentially want normal behavior
    // with no extra _monitors workspace
    ws_grp = monitorws_grp;
  }

  // Loop over the number of periods in the raw file, putting each period in a
  // separate workspace
  for (int period = 0; period < m_numberOfPeriods; ++period) {
    // skipping the first spectra in each period
    skipData(file, static_cast<int>(period * (m_numberOfSpectra + 1)));

    // check for excluded periods
    if (!isPeriodIncluded(period)) {
      skipPeriod(file, period);
      continue;
    }

    if (period > firstPeriod) {
      if (localWorkspace) {
        localWorkspace = createWorkspace(localWorkspace);
      }

      if (bLoadlogFiles) {
        const int period_number = period + 1;
        // remove previous period data
        std::stringstream prevPeriod;
        prevPeriod << "PERIOD " << (getPreviousPeriod(period_number));
        if (localWorkspace) {
          Run &runObj = localWorkspace->mutableRun();
          runObj.removeLogData(prevPeriod.str());
          runObj.removeLogData("current_period");
          // add current period data
          createPeriodLogs(period_number, localWorkspace);
        }
        if (monitorWorkspace) {
          Run &runObj = monitorWorkspace->mutableRun();
          runObj.removeLogData(prevPeriod.str());
          runObj.removeLogData("current_period");
          // add current period data
          createPeriodLogs(period_number, monitorWorkspace);
        }
      } // end of if loop for loadlogfiles
      if (bseparateMonitors) {
        try {
          monitorWorkspace = createWorkspace(monitorWorkspace, monitorwsSpecs,
                                             m_lengthIn, m_lengthIn - 1);
        } catch (std::out_of_range &) {
          g_log.information() << "Separate Monitors option is selected and no "
                                 "monitors in the selected specra range."
                              << std::endl;
          g_log.information()
              << "Error in creating one of the output workspaces" << std::endl;
        } catch (std::runtime_error &) {
          g_log.information() << "Separate Monitors option is selected,Error "
                                 "in creating one of the output workspaces"
                              << std::endl;
        }
      } // end of separate Monitors
    }

    if (bexcludeMonitors) {
      excludeMonitors(file, period, monitorSpecList, localWorkspace);
    }
    if (bincludeMonitors) {
      includeMonitors(file, period, localWorkspace);
    }
    if (bseparateMonitors) {
      separateMonitors(file, period, monitorSpecList, localWorkspace,
                       monitorWorkspace);
    }

    // Re-update spectra etc.
    if (localWorkspace)
      localWorkspace->updateSpectraUsing(detectorMapping);

    if (monitorWorkspace)
      monitorWorkspace->updateSpectraUsing(detectorMapping);

    // Assign the result to the output workspace property
    if (m_numberOfPeriods > 1) {
      if (bseparateMonitors) {
        if (normalwsSpecs > 0) {
          // declare and set monitor workspace for each period
          setWorkspaceProperty(monitorWorkspace, monitorws_grp, period, true,
                               this);
        } else {
          localWorkspace = monitorWorkspace;
        }
        // declare and set output workspace for each period
        setWorkspaceProperty(localWorkspace, ws_grp, period, false, this);
      } else {
        setWorkspaceProperty(localWorkspace, ws_grp, period, false, this);
      }
      // progress for workspace groups
      setProg(static_cast<double>(period) /
              static_cast<double>(m_numberOfPeriods - 1));
    }

  } // loop over periods
  // Clean up

  reset();
  fclose(file);
}
/** This method creates outputworkspace excluding monitors
 *@param file :: -pointer to file
 *@param period :: period number
 *@param monitorList :: a list containing the spectrum numbers for monitors
 *@param ws_sptr :: shared pointer to workspace
 */
void LoadRaw3::excludeMonitors(FILE *file, const int &period,
                               const std::vector<specid_t> &monitorList,
                               DataObjects::Workspace2D_sptr ws_sptr) {
  int64_t histCurrent = -1;
  int64_t wsIndex = 0;
  double histTotal = static_cast<double>(m_total_specs * m_numberOfPeriods);
  // loop through the spectra
  for (specid_t i = 1; i <= m_numberOfSpectra; ++i) {
    specid_t histToRead = i + period * (m_numberOfSpectra + 1);
    if ((i >= m_spec_min && i < m_spec_max) ||
        (m_list &&
         find(m_spec_list.begin(), m_spec_list.end(), i) !=
             m_spec_list.end())) {
      progress(m_prog, "Reading raw file data...");
      // skip monitor spectrum
      if (isMonitor(monitorList, i)) {
        skipData(file, histToRead);
        continue;
      }

      // read spectrum
      if (!readData(file, histToRead)) {
        throw std::runtime_error("Error reading raw file");
      }
      // set the workspace data
      setWorkspaceData(ws_sptr, m_timeChannelsVec, wsIndex, i, m_noTimeRegimes,
                       m_lengthIn, 1);
      // increment workspace index
      ++wsIndex;

      if (m_numberOfPeriods == 1) {
        if (++histCurrent % 100 == 0) {
          setProg(static_cast<double>(histCurrent) / histTotal);
        }
        interruption_point();
      }

    } // end of if loop for spec min,max check
    else {
      skipData(file, histToRead);
    }
  } // end of for loop
}

/**This method creates outputworkspace including monitors
 *@param file :: -pointer to file
 *@param period :: period number
 *@param ws_sptr :: shared pointer to workspace
 */
void LoadRaw3::includeMonitors(FILE *file, const int64_t &period,
                               DataObjects::Workspace2D_sptr ws_sptr) {

  int64_t histCurrent = -1;
  int64_t wsIndex = 0;
  double histTotal = static_cast<double>(m_total_specs * m_numberOfPeriods);
  // loop through spectra
  for (specid_t i = 1; i <= m_numberOfSpectra; ++i) {
    int64_t histToRead = i + period * (m_numberOfSpectra + 1);
    if ((i >= m_spec_min && i < m_spec_max) ||
        (m_list &&
         find(m_spec_list.begin(), m_spec_list.end(), i) !=
             m_spec_list.end())) {
      progress(m_prog, "Reading raw file data...");

      // read spectrum from raw file
      if (!readData(file, histToRead)) {
        throw std::runtime_error("Error reading raw file");
      }
      // set workspace data
      setWorkspaceData(ws_sptr, m_timeChannelsVec, wsIndex, i, m_noTimeRegimes,
                       m_lengthIn, 1);
      ++wsIndex;

      if (m_numberOfPeriods == 1) {
        if (++histCurrent % 100 == 0) {
          setProg(double(histCurrent) / histTotal);
        }
        interruption_point();
      }

    } else {
      skipData(file, histToRead);
    }
  }
  // loadSpectra(file,period,m_total_specs,ws_sptr,m_timeChannelsVec);
}

/** This method separates monitors and creates two outputworkspaces
 *@param file :: -pointer to file
 *@param period :: period number
 *@param monitorList :: -a list containing the spectrum numbers for monitors
 *@param ws_sptr :: -shared pointer to workspace
 *@param mws_sptr :: -shared pointer to monitor workspace
 */

void LoadRaw3::separateMonitors(FILE *file, const int64_t &period,
                                const std::vector<specid_t> &monitorList,
                                DataObjects::Workspace2D_sptr ws_sptr,
                                DataObjects::Workspace2D_sptr mws_sptr) {
  int64_t histCurrent = -1;
  int64_t wsIndex = 0;
  int64_t mwsIndex = 0;
  double histTotal = static_cast<double>(m_total_specs * m_numberOfPeriods);
  // loop through spectra
  for (specid_t i = 1; i <= m_numberOfSpectra; ++i) {
    int64_t histToRead = i + period * (m_numberOfSpectra + 1);
    if ((i >= m_spec_min && i < m_spec_max) ||
        (m_list &&
         find(m_spec_list.begin(), m_spec_list.end(), i) !=
             m_spec_list.end())) {
      progress(m_prog, "Reading raw file data...");

      // read spectrum from raw file
      if (!readData(file, histToRead)) {
        throw std::runtime_error("Error reading raw file");
      }
      // if this a monitor  store that spectrum to monitor workspace
      if (isMonitor(monitorList, i)) {
        setWorkspaceData(mws_sptr, m_timeChannelsVec, mwsIndex, i,
                         m_noTimeRegimes, m_lengthIn, 1);
        ++mwsIndex;
      } else {
        // not a monitor,store the spectrum to normal output workspace
        setWorkspaceData(ws_sptr, m_timeChannelsVec, wsIndex, i,
                         m_noTimeRegimes, m_lengthIn, 1);
        ++wsIndex;
      }

      if (m_numberOfPeriods == 1) {
        if (++histCurrent % 100 == 0) {
          setProg(static_cast<double>(histCurrent) / histTotal);
        }
        interruption_point();
      }

    } else {
      skipData(file, histToRead);
    }
  }
}

/**
 * Skip all spectra in a period.
 * @param file :: -pointer to file
 * @param period :: period number
 */
void LoadRaw3::skipPeriod(FILE *file, const int64_t &period) {
  for (specid_t i = 1; i <= m_numberOfSpectra; ++i) {
    int64_t histToRead = i + period * (m_numberOfSpectra + 1);
    skipData(file, histToRead);
  }
}

/** Check if a period should be loaded.
 * @param period :: A period to check (0-based).
 */
bool LoadRaw3::isPeriodIncluded(int period) const {
  return !isSelectedPeriods() ||
         std::find(m_periodList.begin(), m_periodList.end(), period + 1) !=
             m_periodList.end();
}

/**
 * Get the period number loaded before given.
 * @param period :: A period number being loaded (1-based).
 */
int LoadRaw3::getPreviousPeriod(int period) const {
  if (isSelectedPeriods()) {
    // find period number preceding the argument in the period list
    auto pitr = std::find(m_periodList.begin(), m_periodList.end(), period);
    if (pitr == m_periodList.end() || pitr == m_periodList.begin()) {
      throw std::logic_error("Unexpected period number found.");
    }
    return *(--pitr);
  }
  return period - 1;
}

/// This sets the optional property to the LoadRawHelper class
void LoadRaw3::setOptionalProperties() {
  // read in the settings passed to the algorithm
  m_spec_list = getProperty("SpectrumList");
  m_spec_max = getProperty("SpectrumMax");
  m_spec_min = getProperty("SpectrumMin");
  m_periodList = getProperty("PeriodList");
  if (!m_periodList.empty()) {
    // periods will be expected in ascending order
    std::sort(m_periodList.begin(), m_periodList.end());
    // check that the periods are within their range: 1 <= p <=
    // m_numberOfPeriods
    auto minElement =
        std::min_element(m_periodList.begin(), m_periodList.end());
    auto maxElement =
        std::max_element(m_periodList.begin(), m_periodList.end());
    if (*minElement < 1 || *maxElement > m_numberOfPeriods) {
      throw std::runtime_error("Values in PeriodList must be between 1 and "
                               "total number of periods.");
    }
  }
}

/// This sets the progress taking account of progress time taken up by
/// ChildAlgorithms
void LoadRaw3::setProg(double prog) {
  m_prog = m_prog_start + (m_prog_end - m_prog_start) * prog;
}

/**This method validates workspace sizes if exclude monitors or separate
 *monitors options is selected
 *@param bexcludeMonitors :: boolean option for exclude monitors
 *@param bseparateMonitors :: boolean option for separate monitors
 *@param normalwsSpecs :: number of spectra in the output workspace excluding
 *monitors
 *@param monitorwsSpecs :: number of monitor spectra
 */
void LoadRaw3::validateWorkspaceSizes(bool bexcludeMonitors,
                                      bool bseparateMonitors,
                                      const int64_t normalwsSpecs,
                                      const int64_t monitorwsSpecs) {
  if (normalwsSpecs <= 0 && bexcludeMonitors) {
    throw std::out_of_range("All the spectra in the selected range for this "
                            "workspace are monitors and Exclude monitors "
                            "option selected ");
  }
  if (bseparateMonitors) {
    if (normalwsSpecs <= 0 && monitorwsSpecs <= 0) {
      throw std::out_of_range(
          "Workspace size is zero,Error in creating output workspace.");
    }
  }
}

/** This method checks given spectrum is a monitor
 *  @param monitorIndexes :: a vector holding the list of monitors
 *  @param spectrumNum ::  the requested spectrum number
 *  @return true if it's a monitor
 */
bool LoadRaw3::isMonitor(const std::vector<specid_t> &monitorIndexes,
                         specid_t spectrumNum) {
  bool bMonitor;
  std::vector<specid_t>::const_iterator itr;
  itr = find(monitorIndexes.begin(), monitorIndexes.end(), spectrumNum);
  (itr != monitorIndexes.end()) ? (bMonitor = true) : (bMonitor = false);
  return bMonitor;
}

} // namespace DataHandling
} // namespace Mantid
