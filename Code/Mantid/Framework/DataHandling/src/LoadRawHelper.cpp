//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadRawHelper.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Glob.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/ListValidator.h"
#include "LoadRaw/isisraw2.h"
#include "MantidDataHandling/LoadLog.h"
#include "MantidDataHandling/LoadAscii.h"
#include "MantidDataHandling/RawFileInfo.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/shared_ptr.hpp>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/DateTimeParser.h>
#include <Poco/DateTimeFormat.h>
#include <cmath>
#include <cstdio> //Required for gcc 4.4
#include "MantidKernel/Strings.h"

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;

/// Constructor
LoadRawHelper::LoadRawHelper()
    : isisRaw(new ISISRAW2), m_list(false), m_interval(false), m_spec_list(),
      m_spec_min(0), m_spec_max(EMPTY_INT()), m_numberOfPeriods(0), m_cache_options(),
      m_specTimeRegimes(), m_prog(0.0), m_numberOfSpectra(0), m_monitordetectorList(),
      m_bmspeclist(false), m_total_specs(0), m_logCreator() {
}

LoadRawHelper::~LoadRawHelper() {}

/// Initialisation method.
void LoadRawHelper::init() {
  std::vector<std::string> exts;
  exts.push_back(".raw");
  exts.push_back(".s*");
  exts.push_back(".add");
  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
                  "The name of the RAW file to read, including its full or "
                  "relative path. The file extension must be .raw or .RAW "
                  "(N.B. case sensitive if running on Linux).");
  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name of the workspace that will be created, filled with "
                  "the read-in data and stored in the Analysis Data Service. "
                  "If the input RAW file contains multiple periods higher "
                  "periods will be stored in separate workspaces called "
                  "OutputWorkspace_PeriodNo.");

  m_cache_options.push_back("If Slow");
  m_cache_options.push_back("Always");
  m_cache_options.push_back("Never");
  declareProperty("Cache", "If Slow",
                  boost::make_shared<StringListValidator>(m_cache_options),
                  "An option allowing the algorithm to cache a remote file on "
                  "the local drive before loading. When \"If Slow\" is set the "
                  "download speed is estimated and if is deemed as slow the "
                  "file is cached. \"Always\" means always cache a remote file "
                  "and \"Never\" - never cache.");

  declareProperty("LoadLogFiles", true,
                  "Boolean option to load or skip log files. If this option is "
                  "set all the log files associated with the selected raw file "
                  "are loaded into workspace and can be displayed using right "
                  "click  menu item Sample Logs...on the selected "
                  "workspace.\nNote: If the log files contain motor positions, "
                  "etc. that would affect the instrument geometry this option "
                  "must be set to true for these adjustments to be applied to "
                  "the instrument geometry.");
}
/**opens the raw file and returns the file pointer
 *@param fileName :: name of the raw file
 *@return file pointer
 */
FILE *LoadRawHelper::openRawFile(const std::string &fileName) {
  FILE *file = fopen(fileName.c_str(), "rb");
  if (file == NULL) {
    g_log.error("Unable to open file " + fileName);
    throw Exception::FileError("Unable to open File:", fileName);
  }
  // Need to check that the file is not a text file as the ISISRAW routines
  // don't deal with these very well, i.e
  // reading continues until a bad_alloc is encountered.
  if (isAscii(file)) {
    g_log.error() << "File \"" << fileName << "\" is not a valid RAW file.\n";
    fclose(file);
    throw std::invalid_argument("Incorrect file type encountered.");
  }

  return file;
}
/** Reads the run title and creates a string from it
 * @param file :: pointer to the raw file
 * @param title ::  An output parameter that will contain the workspace title
 */
void LoadRawHelper::readTitle(FILE *file, std::string &title) {
  ioRaw(file, true);
  title = std::string(isisRaw->r_title, 80);
  g_log.information("*** Run title: " + title + " ***");
}
/**skips the histogram from raw file
 *@param file :: pointer to the raw file
 *@param hist :: postion in the file to skip
 */
void LoadRawHelper::skipData(FILE *file, int hist) {
  isisRaw->skipData(file, hist);
}
void LoadRawHelper::skipData(FILE *file, int64_t hist) {
  skipData(file, static_cast<int>(hist));
}
/// calls isisRaw ioRaw.
/// @param file :: the file pointer
/// @param from_file :: unknown
void LoadRawHelper::ioRaw(FILE *file, bool from_file) {
  isisRaw->ioRAW(file, from_file);
}
int LoadRawHelper::getNumberofTimeRegimes() { return isisRaw->daep.n_tr_shift; }

void LoadRawHelper::reset() { isisRaw.reset(); }

/**reads the histogram from raw file
 * @param file :: pointer to the raw file
 * @param hist :: postion in the file to read
 * @return flag is data is read
 */
bool LoadRawHelper::readData(FILE *file, int hist) {
  return isisRaw->readData(file, hist);
}
bool LoadRawHelper::readData(FILE *file, int64_t hist) {
  return readData(file, static_cast<int>(hist));
}

float LoadRawHelper::getProtonCharge() const {
  return isisRaw->rpb.r_gd_prtn_chrg;
}

/**
 * Set the proton charge on the run object
 * @param run :: The run object
 */
void LoadRawHelper::setProtonCharge(API::Run &run) {
  run.setProtonCharge(getProtonCharge());
}
/** Stores the run number in the run logs
 *  @param run :: the workspace's run object
 */
void LoadRawHelper::setRunNumber(API::Run &run) {
  std::string run_num = boost::lexical_cast<std::string>(isisRaw->r_number);
  run.addLogData(new PropertyWithValue<std::string>("run_number", run_num));
}
/**reads workspace dimensions,number of periods etc from raw data
 * @param numberOfSpectra :: number of spectra
 * @param numberOfPeriods :: number of periods
 * @param lengthIn :: size of workspace vectors
 * @param noTimeRegimes :: number of time regime.
 */
void LoadRawHelper::readworkspaceParameters(specid_t &numberOfSpectra,
                                            int &numberOfPeriods,
                                            int64_t &lengthIn,
                                            int64_t &noTimeRegimes) {
  // Read in the number of spectra in the RAW file
  m_numberOfSpectra = numberOfSpectra = static_cast<specid_t>(isisRaw->t_nsp1);
  // Read the number of periods in this file
  numberOfPeriods = isisRaw->t_nper;
  // Read the number of time channels (i.e. bins) from the RAW file
  const int64_t channelsPerSpectrum = isisRaw->t_ntc1;
  // Read in the time bin boundaries
  lengthIn = channelsPerSpectrum + 1;
  // Now check whether there is more than one time regime in use
  noTimeRegimes = isisRaw->daep.n_tr_shift;
}
/**This method creates shared pointer to a workspace
 * @param ws_sptr :: shared pointer to the parent workspace
 * @param nVectors :: number of histograms in the workspace
 * @param xLengthIn :: size of workspace X vector
 * @param yLengthIn :: size of workspace Y vector
 * @return an empty workspace of the given parameters
 */
DataObjects::Workspace2D_sptr
LoadRawHelper::createWorkspace(DataObjects::Workspace2D_sptr ws_sptr,
                               int64_t nVectors, int64_t xLengthIn,
                               int64_t yLengthIn) {
  DataObjects::Workspace2D_sptr empty;
  if (!ws_sptr)
    return empty;
  DataObjects::Workspace2D_sptr workspace =
      boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          WorkspaceFactory::Instance().create(ws_sptr, nVectors, xLengthIn,
                                              yLengthIn));
  return workspace;
}

/** This method creates pointer to workspace
 *  @param nVectors :: The number of vectors/histograms in the workspace
 *  @param xlengthIn :: The number of X data points/bin boundaries in each
 * vector
 *  @param ylengthIn :: The number of Y data points/bin boundaries in each
 * vector
 *  @param title :: title of the workspace
 *  @return Workspace2D_sptr shared pointer to the workspace
 */
DataObjects::Workspace2D_sptr
LoadRawHelper::createWorkspace(int64_t nVectors, int64_t xlengthIn,
                               int64_t ylengthIn, const std::string &title) {
  DataObjects::Workspace2D_sptr workspace;
  if (nVectors > 0) {
    workspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
        WorkspaceFactory::Instance().create("Workspace2D", nVectors, xlengthIn,
                                            ylengthIn));
    // Set the units
    workspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    workspace->setYUnit("Counts");
    workspace->setTitle(title);
  }
  return workspace;
}

/**creates monitor workspace
 *@param monws_sptr :: shared pointer to monitor workspace
 *@param normalws_sptr :: shared pointer to output workspace
 *@param mongrp_sptr :: shared pointer to monitor group workspace
 *@param mwsSpecs :: number of spectra in the monitor workspace
 *@param nwsSpecs :: number of spectra in the output workspace
 *@param numberOfPeriods :: total number of periods from raw file
 *@param lengthIn :: size of workspace vectors
 *@param title :: title of the workspace
 *@param pAlg   :: pointer to the algorithm, this method works with.
 */
void LoadRawHelper::createMonitorWorkspace(
    DataObjects::Workspace2D_sptr &monws_sptr,
    DataObjects::Workspace2D_sptr &normalws_sptr,
    WorkspaceGroup_sptr &mongrp_sptr, const int64_t mwsSpecs,
    const int64_t nwsSpecs, const int64_t numberOfPeriods,
    const int64_t lengthIn, const std::string title,
    API::Algorithm *const pAlg) {
  try {
    // create monitor group workspace
    mongrp_sptr = createGroupWorkspace(); // create workspace
    // create monitor workspace
    if (mwsSpecs > 0) {
      if (normalws_sptr) {
        monws_sptr =
            createWorkspace(normalws_sptr, mwsSpecs, lengthIn, lengthIn - 1);

      } else {
        monws_sptr = createWorkspace(mwsSpecs, lengthIn, lengthIn - 1, title);
      }
    }
    if (!monws_sptr)
      return;

    std::string wsName = pAlg->getPropertyValue("OutputWorkspace");
    // if the normal output workspace size>0 then set the workspace as
    // "MonitorWorkspace"
    // otherwise  set the workspace as "OutputWorkspace"
    if (nwsSpecs > 0) {
      std::string monitorwsName = wsName + "_monitors";
      pAlg->declareProperty(new WorkspaceProperty<Workspace>(
          "MonitorWorkspace", monitorwsName, Direction::Output));
      setWorkspaceProperty("MonitorWorkspace", title, mongrp_sptr, monws_sptr,
                           numberOfPeriods, true, pAlg);
    } else {
      // if only monitors range selected
      // then set the monitor workspace as the output workspace
      setWorkspaceProperty("OutputWorkspace", title, mongrp_sptr, monws_sptr,
                           numberOfPeriods, false, pAlg);
    }

  } catch (std::out_of_range &) {
    pAlg->getLogger().debug() << "Error in creating monitor workspace"
                              << std::endl;
  } catch (std::runtime_error &) {
    pAlg->getLogger().debug() << "Error in creating monitor workspace"
                              << std::endl;
  }
}

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw Exception::FileError If the RAW file cannot be found/opened
 *  @throw std::invalid_argument If the optional properties are set to invalid
 *values
 */
void LoadRawHelper::exec() {}

/** sets the workspace properties
 *  @param ws_sptr ::  shared pointer to  workspace
 *  @param grpws_sptr :: shared pointer to  group workspace
 *  @param  period period number
 *  @param bmonitors :: boolean flag to name  the workspaces
 *  @param pAlg      :: pointer to algorithm this method works with.
 */
void LoadRawHelper::setWorkspaceProperty(DataObjects::Workspace2D_sptr ws_sptr,
                                         WorkspaceGroup_sptr grpws_sptr,
                                         const int64_t period, bool bmonitors,
                                         API::Algorithm *const pAlg) {
  if (!ws_sptr)
    return;
  if (!grpws_sptr)
    return;
  std::string wsName;
  std::string outws;
  std::string outputWorkspace;
  std::string localWSName = pAlg->getProperty("OutputWorkspace");
  std::stringstream suffix;
  suffix << (period + 1);
  if (bmonitors) {
    wsName = localWSName + "_monitors" + "_" + suffix.str();
    outputWorkspace = "MonitorWorkspace";
  } else {
    wsName = localWSName + "_" + suffix.str();
    outputWorkspace = "OutputWorkspace";
  }
  outws = outputWorkspace + "_" + suffix.str();
  pAlg->declareProperty(
      new WorkspaceProperty<Workspace>(outws, wsName, Direction::Output));
  pAlg->setProperty(outws, boost::static_pointer_cast<Workspace>(ws_sptr));
  grpws_sptr->addWorkspace(ws_sptr);
}

/** This method sets the workspace property
 *  @param propertyName :: property name for the workspace
 *  @param title :: title of the workspace
 *  @param grpws_sptr ::  shared pointer to group workspace
 *  @param ws_sptr ::  shared pointer to workspace
 *  @param numberOfPeriods :: number periods in the raw file
 *  @param  bMonitor to identify the workspace is an output workspace or monitor
 * workspace
 *  @param pAlg         :: pointer to algorithm this method works with.
 */
void LoadRawHelper::setWorkspaceProperty(const std::string &propertyName,
                                         const std::string &title,
                                         WorkspaceGroup_sptr grpws_sptr,
                                         DataObjects::Workspace2D_sptr ws_sptr,
                                         int64_t numberOfPeriods, bool bMonitor,
                                         API::Algorithm *const pAlg) {
  UNUSED_ARG(bMonitor);
  Property *ws = pAlg->getProperty("OutputWorkspace");
  if (!ws)
    return;
  if (!grpws_sptr)
    return;
  if (!ws_sptr)
    return;
  ws_sptr->setTitle(title);
  ws_sptr->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  if (numberOfPeriods > 1) {
    pAlg->setProperty(propertyName,
                      boost::dynamic_pointer_cast<Workspace>(grpws_sptr));
  } else {
    pAlg->setProperty(propertyName,
                      boost::dynamic_pointer_cast<Workspace>(ws_sptr));
  }
}

/** This method sets the raw file data to workspace vectors
 *  @param newWorkspace ::  shared pointer to the  workspace
 *  @param timeChannelsVec ::  vector holding the X data
 *  @param  wsIndex  variable used for indexing the output workspace
 *  @param  nspecNum  spectrum number
 *  @param noTimeRegimes ::   regime no.
 *  @param lengthIn :: length of the workspace
 *  @param binStart :: start of bin
 */
void LoadRawHelper::setWorkspaceData(
    DataObjects::Workspace2D_sptr newWorkspace,
    const std::vector<boost::shared_ptr<MantidVec>> &timeChannelsVec,
    int64_t wsIndex, specid_t nspecNum, int64_t noTimeRegimes, int64_t lengthIn,
    int64_t binStart) {
  if (!newWorkspace)
    return;
  typedef double (*uf)(double);
  uf dblSqrt = std::sqrt;
  // But note that the last (overflow) bin is kept
  MantidVec &Y = newWorkspace->dataY(wsIndex);
  Y.assign(isisRaw->dat1 + binStart, isisRaw->dat1 + lengthIn);
  // Fill the vector for the errors, containing sqrt(count)
  MantidVec &E = newWorkspace->dataE(wsIndex);
  std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);

  newWorkspace->getSpectrum(wsIndex)->setSpectrumNo(nspecNum);
  // for loadrawbin0
  if (binStart == 0) {
    newWorkspace->setX(wsIndex, timeChannelsVec[0]);
    return;
  }
  // for loadrawspectrum 0
  if (nspecNum == 0) {
    newWorkspace->setX(wsIndex, timeChannelsVec[0]);
    return;
  }
  // Set the X vector pointer and spectrum number
  if (noTimeRegimes < 2)
    newWorkspace->setX(wsIndex, timeChannelsVec[0]);
  else {

    // Use std::vector::at just incase spectrum missing from spec array
    newWorkspace->setX(wsIndex,
                       timeChannelsVec.at(m_specTimeRegimes[nspecNum] - 1));
  }
}

/** This method returns the monitor spectrum list
 *  @param mapping The spectrum number to detector mapping
 *  @return monitorSpecList The spectrum numbers of the monitors
 */
std::vector<specid_t>
LoadRawHelper::getmonitorSpectrumList(const SpectrumDetectorMapping &mapping) {
  std::vector<specid_t> spectrumIndices;

  if (!m_monitordetectorList.empty()) {
    const auto &map = mapping.getMapping();
    for (auto it = map.begin(); it != map.end(); ++it) {
      auto detIDs = it->second;
      // Both m_monitordetectorList & detIDs should be (very) short so the
      // nested loop shouldn't be too evil
      for (auto detIt = detIDs.begin(); detIt != detIDs.end(); ++detIt) {
        if (std::find(m_monitordetectorList.begin(),
                      m_monitordetectorList.end(),
                      *detIt) != m_monitordetectorList.end()) {
          spectrumIndices.push_back(it->first);
        }
      }
    }
  } else {
    g_log.error()
        << "monitor detector id list is empty  for the selected workspace"
        << std::endl;
  }

  return spectrumIndices;
}

/** This method creates pointer to group workspace
 *  @return WorkspaceGroup_sptr shared pointer to the workspace
 */
WorkspaceGroup_sptr LoadRawHelper::createGroupWorkspace() {
  WorkspaceGroup_sptr workspacegrp(new WorkspaceGroup);
  return workspacegrp;
}

/**
 * Check if a file is a text file
 * @param file :: The file pointer
 * @returns true if the file an ascii text file, false otherwise
 */
bool LoadRawHelper::isAscii(FILE *file) const {
  return Kernel::FileDescriptor::isAscii(file);
}

/** Constructs the time channel (X) vector(s)
 *  @param regimes ::  The number of time regimes (if 1 regime, will actually
 * contain 0)
 *  @param lengthIn :: The number of time channels
 *  @return The vector(s) containing the time channel boundaries, in a vector of
 * shared ptrs
 */
std::vector<boost::shared_ptr<MantidVec>>
LoadRawHelper::getTimeChannels(const int64_t &regimes,
                               const int64_t &lengthIn) {
  float *const timeChannels = new float[lengthIn];
  isisRaw->getTimeChannels(timeChannels, static_cast<int>(lengthIn));

  std::vector<boost::shared_ptr<MantidVec>> timeChannelsVec;
  if (regimes >= 2) {
    g_log.debug() << "Raw file contains " << regimes << " time regimes\n";
    // If more than 1 regime, create a timeChannelsVec for each regime
    for (int64_t i = 0; i < regimes; ++i) {
      // Create a vector with the 'base' time channels
      boost::shared_ptr<MantidVec> channelsVec(
          new MantidVec(timeChannels, timeChannels + lengthIn));
      const double shift = isisRaw->daep.tr_shift[i];
      g_log.debug() << "Time regime " << i + 1 << " shifted by " << shift
                    << " microseconds\n";
      // Add on the shift for this vector
      std::transform(channelsVec->begin(), channelsVec->end(),
                     channelsVec->begin(),
                     std::bind2nd(std::plus<double>(), shift));
      timeChannelsVec.push_back(channelsVec);
    }
    // In this case, also need to populate the map of spectrum-regime
    // correspondence
    const int64_t ndet = static_cast<int64_t>(isisRaw->i_det);
    std::map<specid_t, specid_t>::iterator hint = m_specTimeRegimes.begin();
    for (int64_t j = 0; j < ndet; ++j) {
      // No checking for consistency here - that all detectors for given
      // spectrum
      // are declared to use same time regime. Will just use first encountered
      hint = m_specTimeRegimes.insert(
          hint, std::make_pair(isisRaw->spec[j], isisRaw->timr[j]));
    }
  } else // Just need one in this case
  {
    boost::shared_ptr<MantidVec> channelsVec(
        new MantidVec(timeChannels, timeChannels + lengthIn));
    timeChannelsVec.push_back(channelsVec);
  }
  // Done with the timeChannels C array so clean up
  delete[] timeChannels;
  return timeChannelsVec;
}

/// Run the Child Algorithm LoadInstrument (or LoadInstrumentFromRaw)
/// @param fileName :: the raw file filename
/// @param localWorkspace :: The workspace to load the instrument for
/// @param progStart :: progress at start
/// @param progEnd :: progress at end
void
LoadRawHelper::runLoadInstrument(const std::string &fileName,
                                 DataObjects::Workspace2D_sptr localWorkspace,
                                 double progStart, double progEnd) {
  g_log.debug("Loading the instrument definition...");
  m_prog = progStart;
  progress(m_prog, "Loading the instrument geometry...");

  std::string instrumentID = isisRaw->i_inst; // get the instrument name
  size_t i = instrumentID.find_first_of(' '); // cut trailing spaces
  if (i != std::string::npos)
    instrumentID.erase(i);

  IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");
  // Enable progress reporting by Child Algorithm -
  loadInst->addObserver(this->progressObserver());
  setChildStartProgress(progStart);
  setChildEndProgress((progStart + progEnd) / 2);
  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  bool executionSuccessful(true);
  try {
    loadInst->setPropertyValue("InstrumentName", instrumentID);
    loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", localWorkspace);
    loadInst->setProperty(
        "RewriteSpectraMap",
        false); // No point as we will load the one from the file
    loadInst->execute();
  } catch (std::invalid_argument &) {
    g_log.information("Invalid argument to LoadInstrument Child Algorithm");
    executionSuccessful = false;
  } catch (std::runtime_error &) {
    g_log.information(
        "Unable to successfully run LoadInstrument Child Algorithm");
    executionSuccessful = false;
  }

  // If loading instrument definition file fails, run LoadInstrumentFromRaw
  // instead
  if (!executionSuccessful) {
    g_log.information() << "Instrument definition file "
                        << " not found. Attempt to load information about \n"
                        << "the instrument from raw data file.\n";
    runLoadInstrumentFromRaw(fileName, localWorkspace);
  } else {
    // If requested update the instrument to positions in the raw file
    const Geometry::ParameterMap &pmap = localWorkspace->instrumentParameters();
    if (pmap.contains(localWorkspace->getInstrument()->getComponentID(),
                      "det-pos-source")) {
      boost::shared_ptr<Geometry::Parameter> updateDets = pmap.get(
          localWorkspace->getInstrument()->getComponentID(), "det-pos-source");
      std::string value = updateDets->value<std::string>();
      if (value.substr(0, 8) == "datafile") {
        IAlgorithm_sptr updateInst =
            createChildAlgorithm("UpdateInstrumentFromFile");
        updateInst->setProperty<MatrixWorkspace_sptr>("Workspace",
                                                      localWorkspace);
        updateInst->setPropertyValue("Filename", fileName);
        updateInst->addObserver(this->progressObserver()); // Enable progress
                                                           // reporting by
                                                           // ChildAlgorithm
        setChildStartProgress((progStart + progEnd) / 2);
        setChildEndProgress(progEnd);
        if (value == "datafile-ignore-phi") {
          updateInst->setProperty("IgnorePhi", true);
          g_log.information("Detector positions in IDF updated with positions "
                            "in the data file except for the phi values");
        } else {
          g_log.information("Detector positions in IDF updated with positions "
                            "in the data file");
        }
        // We want this to throw if it fails to warn the user that the
        // information is not correct.
        updateInst->execute();
      }
    }
    // Debugging code??
    m_monitordetectorList = loadInst->getProperty("MonitorList");
    std::vector<specid_t>::const_iterator itr;
    for (itr = m_monitordetectorList.begin();
         itr != m_monitordetectorList.end(); ++itr) {
      g_log.debug() << "Monitor detector id is " << (*itr) << std::endl;
    }
  }
}

/// Run LoadInstrumentFromRaw as a Child Algorithm (only if loading from
/// instrument definition file fails)
/// @param fileName :: the raw file filename
/// @param localWorkspace :: The workspace to load the instrument for
void LoadRawHelper::runLoadInstrumentFromRaw(
    const std::string &fileName, DataObjects::Workspace2D_sptr localWorkspace) {
  IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrumentFromRaw");
  loadInst->setPropertyValue("Filename", fileName);
  // Set the workspace property to be the same one filled above
  loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", localWorkspace);

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    loadInst->execute();
  } catch (std::runtime_error &) {
    g_log.error(
        "Unable to successfully run LoadInstrumentFromRaw Child Algorithm");
  }
  m_monitordetectorList = loadInst->getProperty("MonitorList");
  std::vector<specid_t>::const_iterator itr;
  for (itr = m_monitordetectorList.begin(); itr != m_monitordetectorList.end();
       ++itr) {
    g_log.debug() << "Monitor dtector id is " << (*itr) << std::endl;
  }
  if (!loadInst->isExecuted()) {
    g_log.error("No instrument definition loaded");
  }
}

/// Run the LoadMappingTable Child Algorithm to fill the SpectraToDetectorMap
/// @param fileName :: the raw file filename
/// @param localWorkspace :: The workspace to load the mapping table for
void LoadRawHelper::runLoadMappingTable(
    const std::string &fileName, DataObjects::Workspace2D_sptr localWorkspace) {
  g_log.debug("Loading the spectra-detector mapping...");
  progress(m_prog, "Loading the spectra-detector mapping...");
  // Now determine the spectra to detector map calling Child Algorithm
  // LoadMappingTable
  // There is a small penalty in re-opening the raw file but nothing major.
  IAlgorithm_sptr loadmap = createChildAlgorithm("LoadMappingTable");
  loadmap->setPropertyValue("Filename", fileName);
  loadmap->setProperty<MatrixWorkspace_sptr>("Workspace", localWorkspace);
  try {
    loadmap->execute();
  } catch (std::runtime_error &) {
    g_log.error(
        "Unable to successfully execute LoadMappingTable Child Algorithm");
  }

  if (!loadmap->isExecuted()) {
    g_log.error("LoadMappingTable Child Algorithm is not executed");
  }
}

/// Run the LoadLog Child Algorithm
/// @param fileName :: the raw file filename
/// @param localWorkspace :: The workspace to load the logs for
/// @param progStart :: starting progress fraction
/// @param progEnd :: ending progress fraction
void LoadRawHelper::runLoadLog(const std::string &fileName,
                               DataObjects::Workspace2D_sptr localWorkspace,
                               double progStart, double progEnd) {
  // search for the log file to load, and save their names in a set.
  std::list<std::string> logFiles = searchForLogFiles(fileName);

  g_log.debug("Loading the log files...");
  if (progStart < progEnd) {
    m_prog = progStart;
  }

  progress(m_prog, "Reading log files...");

  // Iterate over and load each log file into the localWorkspace.
  std::list<std::string>::const_iterator logPath;
  for (logPath = logFiles.begin(); logPath != logFiles.end(); ++logPath) {
    // check for log files we should just ignore
    std::string ignoreSuffix = "ICPstatus.txt";
    if (boost::algorithm::ends_with(*logPath, ignoreSuffix)) {
      g_log.information("Skipping log file: " + *logPath);
      continue;
    }

    ignoreSuffix = "ICPdebug.txt";
    if (boost::algorithm::ends_with(*logPath, ignoreSuffix)) {
      g_log.information("Skipping log file: " + *logPath);
      continue;
    }

    // Create a new object for each log file.
    IAlgorithm_sptr loadLog = createChildAlgorithm("LoadLog");

    // Pass through the same input filename
    loadLog->setPropertyValue("Filename", *logPath);
    // Set the workspace property to be the same one filled above
    loadLog->setProperty<MatrixWorkspace_sptr>("Workspace", localWorkspace);
    // Pass the name of the log file explicitly to LoadLog.
    loadLog->setPropertyValue("Names", extractLogName(*logPath));

    // Force loading two column file if it's an ISIS ICPevent log
    if (boost::algorithm::ends_with(*logPath, "ICPevent.txt")) {
      loadLog->setPropertyValue("NumberOfColumns", "2");
    }

    // Enable progress reporting by Child Algorithm - if progress range has
    // duration
    if (progStart < progEnd) {
      loadLog->addObserver(this->progressObserver());
      setChildStartProgress(progStart);
      setChildEndProgress(progEnd);
    }
    // Now execute the Child Algorithm. Catch and log any error, but don't stop.
    try {
      loadLog->execute();
    } catch (std::exception &) {
      g_log.error("Unable to successfully run LoadLog Child Algorithm");
    }

    if (!loadLog->isExecuted()) {
      g_log.error("Unable to successfully run LoadLog Child Algorithm");
    }
  }
  // Make log creator object and add the run status log if we have the
  // appropriate ICP log
  m_logCreator.reset(new ISISRunLogs(localWorkspace->run(), m_numberOfPeriods));
  m_logCreator->addStatusLog(localWorkspace->mutableRun());
}

/**
 * Extract the log name from the path to the log file.
 * @param path :: Path to the log file
 * @return logName :: The name of the log file.
 */
std::string LoadRawHelper::extractLogName(const std::string &path) {
  // The log file's name, including workspace (e.g. CSP78173_ICPevent)
  std::string fileName =
      Poco::Path(Poco::Path(path).getFileName()).getBaseName();
  // Return only the log name (excluding workspace, e.g. ICPevent)
  return (fileName.substr(fileName.find('_') + 1));
}

/**
 * Creates period log data in the workspace
 * @param period :: period number
 * @param local_workspace :: workspace to add period log data to.
 */
void
LoadRawHelper::createPeriodLogs(int64_t period,
                                DataObjects::Workspace2D_sptr local_workspace) {
  m_logCreator->addPeriodLogs(static_cast<int>(period),
                              local_workspace->mutableRun());
}

/**
 * Pulls the run parameters from the ISIS Raw RPB structure and stores them as
 * log entries on the
 * workspace run object
 * @param localWorkspace :: The workspace to attach the information to
 * @param rawFile :: The handle to an ISIS Raw file
 */
void LoadRawHelper::loadRunParameters(API::MatrixWorkspace_sptr localWorkspace,
                                      ISISRAW *const rawFile) const {
  ISISRAW *localISISRaw(NULL);
  if (!rawFile) {
    localISISRaw = isisRaw.get();
  } else {
    localISISRaw = rawFile;
  }

  API::Run &runDetails = localWorkspace->mutableRun();

  runDetails.addProperty("run_header", RawFileInfo::runHeader(*localISISRaw));
  // Run title is stored in a different attribute
  runDetails.addProperty("run_title", RawFileInfo::runTitle(*localISISRaw),
                         true);

  runDetails.addProperty("user_name",
                         std::string(localISISRaw->hdr.hd_user, 20));
  runDetails.addProperty("inst_abrv",
                         std::string(localISISRaw->hdr.inst_abrv, 3));
  runDetails.addProperty("hd_dur", std::string(localISISRaw->hdr.hd_dur, 8));

  // Data details on run not the workspace
  runDetails.addProperty("nspectra", static_cast<int>(localISISRaw->t_nsp1));
  runDetails.addProperty("nchannels", static_cast<int>(localISISRaw->t_ntc1));
  runDetails.addProperty("nperiods", static_cast<int>(localISISRaw->t_nper));

  // RPB struct info
  runDetails.addProperty("dur", localISISRaw->rpb.r_dur); // actual run duration
  runDetails.addProperty(
      "durunits", localISISRaw->rpb.r_durunits); // scaler for above (1=seconds)
  runDetails.addProperty(
      "dur_freq",
      localISISRaw->rpb.r_dur_freq); // testinterval for above (seconds)
  runDetails.addProperty("dmp", localISISRaw->rpb.r_dmp); // dump interval
  runDetails.addProperty("dmp_units",
                         localISISRaw->rpb.r_dmp_units); // scaler for above
  runDetails.addProperty("dmp_freq",
                         localISISRaw->rpb.r_dmp_freq); // interval for above
  runDetails.addProperty(
      "freq",
      localISISRaw->rpb.r_freq); // 2**k where source frequency = 50 / 2**k
  runDetails.addProperty(
      "gd_prtn_chrg",
      static_cast<double>(
          localISISRaw->rpb.r_gd_prtn_chrg)); // good proton charge (uA.hour)
  runDetails.addProperty(
      "tot_prtn_chrg",
      static_cast<double>(
          localISISRaw->rpb.r_tot_prtn_chrg)); // total proton charge (uA.hour)
  runDetails.addProperty("goodfrm", localISISRaw->rpb.r_goodfrm); // good frames
  runDetails.addProperty("rawfrm", localISISRaw->rpb.r_rawfrm);   // raw frames
  runDetails.addProperty(
      "dur_wanted", localISISRaw->rpb.r_dur_wanted); // requested run duration
                                                     // (units as for "duration"
                                                     // above)
  runDetails.addProperty(
      "dur_secs",
      localISISRaw->rpb.r_dur_secs); // actual run duration in seconds
  runDetails.addProperty("mon_sum1",
                         localISISRaw->rpb.r_mon_sum1); // monitor sum 1
  runDetails.addProperty("mon_sum2",
                         localISISRaw->rpb.r_mon_sum2); // monitor sum 2
  runDetails.addProperty("mon_sum3",
                         localISISRaw->rpb.r_mon_sum3); // monitor sum 3
  runDetails.addProperty("rb_proposal",
                         localISISRaw->rpb.r_prop); // RB (proposal) number

  // Note isis raw date format which is stored in DD-MMM-YYYY. Store dates in
  // ISO 8601
  std::string isisDate = std::string(localISISRaw->rpb.r_enddate, 11);
  if (isisDate[0] == ' ')
    isisDate[0] = '0';
  runDetails.addProperty(
      "run_end", DateAndTime(isisDate.substr(7, 4) + "-" +
                             convertMonthLabelToIntStr(isisDate.substr(3, 3)) +
                             "-" + isisDate.substr(0, 2) + "T" +
                             std::string(localISISRaw->rpb.r_endtime, 8))
                     .toISO8601String());
  isisDate = std::string(localISISRaw->hdr.hd_date, 11);
  if (isisDate[0] == ' ')
    isisDate[0] = '0';
  runDetails.addProperty(
      "run_start",
      DateAndTime(isisDate.substr(7, 4) + "-" +
                  convertMonthLabelToIntStr(isisDate.substr(3, 3)) + "-" +
                  isisDate.substr(0, 2) + "T" +
                  std::string(localISISRaw->hdr.hd_time, 8)).toISO8601String());
}

/// To help transforming date stored in ISIS raw file into iso 8601
/// @param month
/// @return month as string integer e.g. 01
std::string LoadRawHelper::convertMonthLabelToIntStr(std::string month) const {
  std::transform(month.begin(), month.end(), month.begin(), toupper);

  if (month == "JAN")
    return "01";
  if (month == "FEB")
    return "02";
  if (month == "MAR")
    return "03";
  if (month == "APR")
    return "04";
  if (month == "MAY")
    return "05";
  if (month == "JUN")
    return "06";
  if (month == "JUL")
    return "07";
  if (month == "AUG")
    return "08";
  if (month == "SEP")
    return "09";
  if (month == "OCT")
    return "10";
  if (month == "NOV")
    return "11";
  if (month == "DEC")
    return "12";

  throw std::runtime_error(
      "LoadRawHelper::convertMonthLabelToIntStr(): Invalid month label found.");
}

/// sets optional properties for the loadraw algorithm
/// @param spec_min :: The minimum spectra number
/// @param spec_max :: The maximum spectra number
/// @param spec_list :: The list of Spectra to be included
void LoadRawHelper::setOptionalProperties(const int &spec_min,
                                          const int &spec_max,
                                          const std::vector<int> &spec_list) {
  m_spec_min = spec_min;
  m_spec_max = spec_max;
  m_spec_list.assign(spec_list.begin(), spec_list.end());
}

/// Validates the optional 'spectra to read' properties, if they have been set
void LoadRawHelper::checkOptionalProperties() {
  // read in the settings passed to the algorithm
  /*m_spec_list = getProperty("SpectrumList");
  m_spec_max = getProperty("SpectrumMax");
  m_spec_min = getProperty("SpectrumMin");*/

  m_list = !m_spec_list.empty();
  m_bmspeclist = !m_spec_list.empty();
  m_interval = (m_spec_max != EMPTY_INT()) || (m_spec_min != 1);
  if (m_spec_max == EMPTY_INT())
    m_spec_max = 1;
  // Check validity of spectra list property, if set
  if (m_list) {
    m_list = true;
    if (m_spec_list.size() == 0) {
      m_list = false;
    } else {
      const int64_t minlist =
          *min_element(m_spec_list.begin(), m_spec_list.end());
      const int64_t maxlist =
          *max_element(m_spec_list.begin(), m_spec_list.end());
      if (maxlist > m_numberOfSpectra || minlist <= 0) {
        g_log.error("Invalid list of spectra");
        throw std::invalid_argument("Inconsistent properties defined");
      }
    }
  }
  // Check validity of spectra range, if set
  if (m_interval) {
    m_interval = true;
    // m_spec_min = getProperty("SpectrumMin");
    if (m_spec_min != 1 && m_spec_max == 1)
      m_spec_max = m_numberOfSpectra;
    if (m_spec_max < m_spec_min || m_spec_max > m_numberOfSpectra) {
      g_log.error("Invalid Spectrum min/max properties");
      throw std::invalid_argument("Inconsistent properties defined");
    }
  }
}
/**
 * Calculates the total number of spectra in the workspace, given the input
 * properties
 * @return the size of the workspace (number of spectra)
 */
specid_t LoadRawHelper::calculateWorkspaceSize() {
  specid_t total_specs(0);
  if (m_interval || m_list) {
    if (m_interval) {
      if (m_spec_min != 1 && m_spec_max == 1)
        m_spec_max = m_numberOfSpectra;

      m_total_specs = total_specs = (m_spec_max - m_spec_min + 1);
      m_spec_max += 1;
    } else
      total_specs = 0;

    if (m_list) {
      if (m_interval) {
        for (std::vector<specid_t>::iterator it = m_spec_list.begin();
             it != m_spec_list.end();)
          if (*it >= m_spec_min && *it < m_spec_max) {
            it = m_spec_list.erase(it);
          } else
            ++it;
      }
      if (m_spec_list.size() == 0)
        m_list = false;
      total_specs += static_cast<specid_t>(m_spec_list.size());
      m_total_specs = total_specs;
    }
  } else {
    total_specs = m_numberOfSpectra;
    m_total_specs = total_specs;
    // In this case want all the spectra, but zeroth spectrum is garbage so go
    // from 1 to NSP1
    m_spec_min = 1;
    m_spec_max = m_numberOfSpectra + 1;
  }
  return total_specs;
}

/// calculate workspace sizes.
/// @param monitorSpecList :: the vector of the monitor spectra
/// @param normalwsSpecs :: the spectra for the detector workspace
/// @param monitorwsSpecs :: the spectra for the monitor workspace
void LoadRawHelper::calculateWorkspacesizes(
    const std::vector<specid_t> &monitorSpecList, specid_t &normalwsSpecs,
    specid_t &monitorwsSpecs) {
  if (!m_interval && !m_bmspeclist) {
    monitorwsSpecs = static_cast<specid_t>(monitorSpecList.size());
    normalwsSpecs = m_total_specs - monitorwsSpecs;
    g_log.debug()
        << "normalwsSpecs   when m_interval  & m_bmspeclist are  false is  "
        << normalwsSpecs << "  monitorwsSpecs is " << monitorwsSpecs
        << std::endl;
  } else if (m_interval || m_bmspeclist) {
    if (m_interval) {
      int msize = 0;
      std::vector<specid_t>::const_iterator itr1;
      for (itr1 = monitorSpecList.begin(); itr1 != monitorSpecList.end();
           ++itr1) {
        if (*itr1 >= m_spec_min && *itr1 < m_spec_max)
          ++msize;
      }
      monitorwsSpecs = msize;
      normalwsSpecs = m_total_specs - monitorwsSpecs;
      g_log.debug() << "normalwsSpecs when  m_interval true is  "
                    << normalwsSpecs << "  monitorwsSpecs is " << monitorwsSpecs
                    << std::endl;
    }
    if (m_bmspeclist) {
      if (m_interval) {
        std::vector<specid_t>::iterator itr;
        for (itr = m_spec_list.begin();
             itr != m_spec_list.end();) { // if  the m_spec_list elements are in
                                          // the range between m_spec_min &
                                          // m_spec_max
          if (*itr >= m_spec_min && *itr < m_spec_max)
            itr = m_spec_list.erase(itr);
          else
            ++itr;
        }
        if (m_spec_list.size() == 0) {
          g_log.debug() << "normalwsSpecs is " << normalwsSpecs
                        << "  monitorwsSpecs is " << monitorwsSpecs
                        << std::endl;
        } else { // at this point there are monitors in the list which are not
                 // in the min& max range
          // so find those  monitors  count and calculate the workspace specs
          std::vector<specid_t>::const_iterator itr;
          std::vector<specid_t>::const_iterator monitr;
          specid_t monCounter = 0;
          for (itr = m_spec_list.begin(); itr != m_spec_list.end(); ++itr) {
            monitr = find(monitorSpecList.begin(), monitorSpecList.end(), *itr);
            if (monitr != monitorSpecList.end())
              ++monCounter;
          }
          monitorwsSpecs += monCounter;
          normalwsSpecs = m_total_specs - monitorwsSpecs;
          g_log.debug() << "normalwsSpecs is  " << normalwsSpecs
                        << "  monitorwsSpecs is " << monitorwsSpecs
                        << std::endl;
        }
      }      // end if loop for m_interval
      else { // if only List true
        specid_t mSize = 0;
        std::vector<specid_t>::const_iterator itr;
        std::vector<specid_t>::const_iterator monitr;
        for (itr = m_spec_list.begin(); itr != m_spec_list.end(); ++itr) {
          monitr = find(monitorSpecList.begin(), monitorSpecList.end(), *itr);
          if (monitr != monitorSpecList.end()) {
            ++mSize;
          }
        }
        monitorwsSpecs = mSize;
        normalwsSpecs = m_total_specs - monitorwsSpecs;
      }
    } // end of if loop for m_bmspeclist
  }
}

void LoadRawHelper::loadSpectra(
    FILE *file, const int &period, const int &total_specs,
    DataObjects::Workspace2D_sptr ws_sptr,
    std::vector<boost::shared_ptr<MantidVec>> timeChannelsVec) {
  double progStart = m_prog;
  double progEnd = 1.0; // Assume this function is called last

  int64_t histCurrent = -1;
  int64_t wsIndex = 0;
  int64_t numberOfPeriods = static_cast<int64_t>(isisRaw->t_nper);
  double histTotal = static_cast<double>(total_specs * numberOfPeriods);
  int64_t noTimeRegimes = getNumberofTimeRegimes();
  int64_t lengthIn = static_cast<int64_t>(isisRaw->t_ntc1 + 1);

  const int64_t periodTimesNSpectraP1 = period *
    (static_cast<int64_t>(m_numberOfSpectra) + 1);
  // loop through spectra
  for (specid_t i = 1; i <= m_numberOfSpectra; ++i) {
    int64_t histToRead = i + periodTimesNSpectraP1;
    if ((i >= m_spec_min && i < m_spec_max) ||
        (m_list &&
         find(m_spec_list.begin(), m_spec_list.end(), i) !=
             m_spec_list.end())) {
      progress(m_prog, "Reading raw file data...");

      // read spectrum from raw file
      if (!readData(file, histToRead))
        throw std::runtime_error("Error reading raw file, in "
                                 "LoadRawHelper::loadSpectra, readData failed");
      // set workspace data
      setWorkspaceData(ws_sptr, timeChannelsVec, wsIndex, i, noTimeRegimes,
                       lengthIn, 1);
      ++wsIndex;

      if (numberOfPeriods == 1) {
        if (++histCurrent % 100 == 0) {
          m_prog = progStart +
                   (progEnd - progStart) *
                       (static_cast<double>(histCurrent) / histTotal);
        }
        interruption_point();
      }
    } else {
      skipData(file, histToRead);
    }
  }
}

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadRawHelper::confidence(Kernel::FileDescriptor &descriptor) const {
  auto &stream = descriptor.data();
  // 85th character is a space & 89th character is a ~
  stream.seekg(84, std::ios::beg);
  int c = stream.get();
  int confidence(0);
  if (c == 32) {
    stream.seekg(3, std::ios::cur);
    int c = stream.get();
    if (c == 126)
      confidence = 80;
  }
  return confidence;
}

/**
 * Searches for log files related to RAW file loaded using LoadLog algorithm.
 * @param pathToRawFile The path and name of the raw file.
 * @returns A set containing paths to log files related to RAW file used.
 */
std::list<std::string>
LoadRawHelper::searchForLogFiles(const std::string &pathToRawFile) {
  // If pathToRawFile is the filename of a raw datafile then search for
  // potential log files
  // in the directory of this raw datafile. Otherwise check if it is a potential
  // log file. Add the filename of these potential log files to:
  // potentialLogFiles.
  std::set<std::string> potentialLogFiles;
  // Using a list instead of a set to preserve order. The three column names
  // will
  // be added to the end of the list. This means if a column exists in the two
  // and three column file then it will be overridden correctly.
  std::list<std::string> potentialLogFilesList;

  // File property checks whether the given path exists, just check that is
  // actually a file
  Poco::File l_path(pathToRawFile);
  if (l_path.isDirectory()) {
    g_log.error("In LoadLog: " + pathToRawFile +
                " must be a filename not a directory.");
    throw Exception::FileError("Filename is a directory:", pathToRawFile);
  }

  // start the process or populating potential log files into the container:
  // potentialLogFiles
  std::string l_filenamePart =
      Poco::Path(l_path.path()).getFileName(); // get filename part only
  if (isAscii(pathToRawFile) &&
      l_filenamePart.rfind("_") != std::string::npos) {
    // then we will assume that the file is an ISIS log file
    potentialLogFiles.insert(pathToRawFile);
  } else {
    // then we will assume that the file is an ISIS raw file. The file validator
    // will have warned the user if the extension is not one of the suggested
    // ones.

    // strip out the raw data file identifier
    std::string l_rawID("");
    size_t idx = l_filenamePart.rfind('.');

    if (idx != std::string::npos) {
      l_rawID = l_filenamePart.substr(0, l_filenamePart.rfind('.'));
    } else {
      l_rawID = l_filenamePart;
    }
    /// check for alternate data stream exists for raw file
    /// if exists open the stream and read  log files name  from ADS
    if (adsExists(pathToRawFile)) {
      potentialLogFiles = getLogFilenamesfromADS(pathToRawFile);
    } else {
      // look for log files in the directory of the raw datafile
      std::string pattern(l_rawID + "_*.txt");
      Poco::Path dir(pathToRawFile);
      dir.makeParent();

      try {
        Kernel::Glob::glob(Poco::Path(dir).resolve(pattern), potentialLogFiles);
      } catch (std::exception &) {
      }
    }

    // push potential log files from set to list.
    potentialLogFilesList.insert(potentialLogFilesList.begin(),
                                  potentialLogFiles.begin(),
                                  potentialLogFiles.end());

    // Remove extension from path, and append .log to path.
    std::string logName =
        pathToRawFile.substr(0, pathToRawFile.rfind('.')) + ".log";
    // Check if log file exists in current directory.
    std::ifstream fileExists(logName.c_str());
    if (fileExists) {
      // Push three column filename to end of list.
      potentialLogFilesList.insert(potentialLogFilesList.end(), logName);
    }
  }
  return (potentialLogFilesList);
}

/**
 * This method looks for ADS with name checksum exists
 * @param pathToFile The path and name of the file.
 * @return True if ADS stream checksum exists
 */
bool LoadRawHelper::adsExists(const std::string &pathToFile) {
#ifdef _WIN32
  std::string adsname(pathToFile + ":checksum");
  std::ifstream adstream(adsname.c_str());
  if (!adstream) {
    return false;
  }
  adstream.close();
  return true;
#else
  UNUSED_ARG(pathToFile);
  return (false);
#endif
}

/**
 * This method reads the checksum ADS associated with the raw file and returns
 * the filenames of the log files
 * @param pathToRawFile The path and name of the raw file.
 * @return list of logfile names.
 */
std::set<std::string>
LoadRawHelper::getLogFilenamesfromADS(const std::string &pathToRawFile) {
  std::string adsname(pathToRawFile + ":checksum");
  std::ifstream adstream(adsname.c_str());
  if (!adstream) {
    return (std::set<std::string>());
  }

  std::string str;
  std::string path;
  std::string logFile;
  std::set<std::string> logfilesList;
  Poco::Path logpath(pathToRawFile);
  size_t pos = pathToRawFile.find_last_of("/");
  if (pos == std::string::npos) {
    pos = pathToRawFile.find_last_of("\\");
  }
  if (pos != std::string::npos) {
    path = pathToRawFile.substr(0, pos);
  }
  while (Mantid::Kernel::Strings::extractToEOL(adstream, str)) {
    std::string fileName;
    pos = str.find("*");
    if (pos == std::string::npos)
      continue;
    fileName = str.substr(pos + 1, str.length() - pos);
    pos = fileName.find("txt");
    if (pos == std::string::npos)
      continue;
    logFile = path + "/" + fileName;
    if (logFile.empty())
      continue;
    logfilesList.insert(logFile);
  }
  return (logfilesList);
}

/**
 * Checks whether filename is a simple text file
 * @param filename :: The filename to inspect
 * @returns true if the filename has the .txt extension
 */
bool LoadRawHelper::isAscii(const std::string &filename) {
  FILE *file = fopen(filename.c_str(), "rb");
  char data[256];
  size_t n = fread(data, 1, sizeof(data), file);
  fclose(file);
  char *pend = &data[n];
  // Call it a binary file if we find a non-ascii character in the first 256
  // bytes of the file.
  for (char *p = data; p < pend; ++p) {
    unsigned long ch = (unsigned long)*p;
    if (!(ch <= 0x7F)) {
      return false;
    }
  }
  return true;
}

/** This method checks the value of LoadMonitors property and returns true or
 * false
 *  @return true if Exclude Monitors option is selected,otherwise false
 */
bool LoadRawHelper::isExcludeMonitors(const std::string &monitorOption) {
  bool bExclude;
  monitorOption.compare("Exclude") ? (bExclude = false) : (bExclude = true);
  return bExclude;
}

/**This method checks the value of LoadMonitors property and returns true or
 * false
 * @return true if Include Monitors option is selected,otherwise false
 */
bool LoadRawHelper::isIncludeMonitors(const std::string &monitorOption) {
  bool bExclude;
  monitorOption.compare("Include") ? (bExclude = false) : (bExclude = true);

  return bExclude;
}

/** This method checks the value of LoadMonitors property and returns true or
 * false
 *  @return true if Separate Monitors option is selected,otherwise false
 */
bool LoadRawHelper::isSeparateMonitors(const std::string &monitorOption) {
  bool bSeparate;
  monitorOption.compare("Separate") ? (bSeparate = false) : (bSeparate = true);
  return bSeparate;
}
/**The method to interpret LoadMonitors property options and convert then into
 * boolean values
 * @param bincludeMonitors  :: if monitors requested to be included with
 * workspace
 * @param bseparateMonitors :: if monitors requested to be loaded separately
 * from the workspace
 * @param bexcludeMonitors  :: if monitors should not be loaded at all.
 * @param pAlgo             :: pointer to the algorithm, which has LoadMonitors
 * property.
 */
void LoadRawHelper::ProcessLoadMonitorOptions(bool &bincludeMonitors,
                                              bool &bseparateMonitors,
                                              bool &bexcludeMonitors,
                                              API::Algorithm *pAlgo) {
  // process monitor option
  std::string monitorOption = pAlgo->getProperty("LoadMonitors");
  if (monitorOption == "1")
    monitorOption = "Separate";
  if (monitorOption == "0")
    monitorOption = "Exclude";

  bincludeMonitors = LoadRawHelper::isIncludeMonitors(monitorOption);
  bseparateMonitors = false;
  bexcludeMonitors = false;
  if (!bincludeMonitors) {
    bseparateMonitors = LoadRawHelper::isSeparateMonitors(monitorOption);
    bexcludeMonitors = LoadRawHelper::isExcludeMonitors(monitorOption);
  }
  //
}

} // namespace DataHandling
} // namespace Mantid
