//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/PeakNoShapeFactory.h"
#include "MantidDataObjects/PeakShapeSphericalFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidNexus/NexusFileIO.h"

#include <boost/shared_ptr.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_array.hpp>
#include <cmath>
#include <Poco/Path.h>
#include <Poco/StringTokenizer.h>
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidDataObjects/PeakNoShapeFactory.h"
#include "MantidDataObjects/PeakShapeSphericalFactory.h"
#include "MantidDataObjects/PeakShapeEllipsoidFactory.h"

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the algorithm factory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadNexusProcessed);

using namespace Mantid::NeXus;
using namespace DataObjects;
using namespace Kernel;
using namespace API;
using Geometry::Instrument_const_sptr;

namespace {

// Helper typedef
typedef boost::shared_array<int> IntArray_shared;

// Struct to contain spectrum information.
struct SpectraInfo {
  // Number of spectra
  int nSpectra;
  // Do we have any spectra
  bool hasSpectra;
  // Contains spectrum numbers for each workspace index
  IntArray_shared spectraNumbers;
  // Index of the detector in the workspace.
  IntArray_shared detectorIndex;
  // Number of detectors associated with each spectra
  IntArray_shared detectorCount;
  // Detector list contains a list of all of the detector numbers
  IntArray_shared detectorList;

  SpectraInfo() : nSpectra(0), hasSpectra(false) {}

  SpectraInfo(int _nSpectra, bool _hasSpectra, IntArray_shared _spectraNumbers,
              IntArray_shared _detectorIndex, IntArray_shared _detectorCount,
              IntArray_shared _detectorList)
      : nSpectra(_nSpectra), hasSpectra(_hasSpectra),
        spectraNumbers(_spectraNumbers), detectorIndex(_detectorIndex),
        detectorCount(_detectorCount), detectorList(_detectorList) {}

  SpectraInfo(const SpectraInfo &other)
      : nSpectra(other.nSpectra), hasSpectra(other.hasSpectra),
        spectraNumbers(other.spectraNumbers),
        detectorIndex(other.detectorIndex), detectorCount(other.detectorCount),
        detectorList(other.detectorList) {}

  /*
  SpectraInfo& operator=(const SpectraInfo& other)
  {
    if (&other != this)
    {
      nSpectra = other.nSpectra;
      hasSpectra = other.hasSpectra;
      spectraNumbers = other.spectraNumbers;
      detectorIndex = other.detectorIndex;
      detectorCount = other.detectorCount;
      detectorList = other.detectorList;
    }
    return *this;
  }
  */
};

// Helper typdef.
typedef boost::optional<SpectraInfo> SpectraInfo_optional;

/**
 * Extract ALL the detector, spectrum number and workspace index mapping
 * information.
 * @param mtd_entry
 * @param logger
 * @return
 */
SpectraInfo extractMappingInfo(NXEntry &mtd_entry, Logger &logger) {
  // Instrument information
  NXInstrument inst = mtd_entry.openNXInstrument("instrument");
  if (!inst.containsGroup("detector")) {
    logger.information() << "Detector block not found. The workspace will not "
                            "contain any detector information.\n";
    return SpectraInfo();
  }

  // Populate the spectra-detector map
  NXDetector detgroup = inst.openNXDetector("detector");

  // Read necessary arrays from the file
  // Detector list contains a list of all of the detector numbers. If it not
  // present then we can't update the spectra
  // map
  boost::shared_array<int> detectorList;
  try {
    NXInt detlist_group = detgroup.openNXInt("detector_list");
    detlist_group.load();
    detectorList = detlist_group.sharedBuffer();
  } catch (std::runtime_error &) {
    logger.information() << "detector_list block not found. The workspace will "
                            "not contain any detector information."
                         << std::endl;
    return SpectraInfo();
  }

  // Detector count contains the number of detectors associated with each
  // spectra
  NXInt det_count = detgroup.openNXInt("detector_count");
  det_count.load();
  boost::shared_array<int> detectorCount = det_count.sharedBuffer();
  // Detector index - contains the index of the detector in the workspace
  NXInt det_index = detgroup.openNXInt("detector_index");
  det_index.load();
  int nspectra = det_index.dim0();
  boost::shared_array<int> detectorIndex = det_index.sharedBuffer();

  // Spectra block - Contains spectrum numbers for each workspace index
  // This might not exist so wrap and check. If it doesn't exist create a
  // default mapping
  bool have_spectra(true);
  boost::shared_array<int> spectra;
  try {
    NXInt spectra_block = detgroup.openNXInt("spectra");
    spectra_block.load();
    spectra = spectra_block.sharedBuffer();
  } catch (std::runtime_error &) {
    have_spectra = false;
  }
  return SpectraInfo(nspectra, have_spectra, spectra, detectorIndex,
                     detectorCount, detectorList);
}

/**
 * Is this file from a well-formed multiperiod group workspace.
 * @param nWorkspaceEntries : Number of entries in the group workspace
 * @param sampleWS : Sample workspace to inspect the logs of
 * @param log : Information logger object
 * @return True only if multiperiod.
 */
bool isMultiPeriodFile(int nWorkspaceEntries, Workspace_sptr sampleWS,
                       Logger &log) {
  bool isMultiPeriod = false;
  if (ExperimentInfo_sptr expInfo =
          boost::dynamic_pointer_cast<ExperimentInfo>(sampleWS)) {
    const std::string nPeriodsLogEntryName = "nperiods";
    const Run &run = expInfo->run();
    if (run.hasProperty(nPeriodsLogEntryName)) {
      const int nPeriods =
          run.getPropertyValueAsType<int>(nPeriodsLogEntryName);
      if (nPeriods == nWorkspaceEntries) {
        isMultiPeriod = true;
        log.information("Loading as MultiPeriod group workspace.");
      }
    }
  }
  return isMultiPeriod;
}
}

/// Default constructor
LoadNexusProcessed::LoadNexusProcessed()
    : m_shared_bins(false), m_xbins(), m_axis1vals(), m_list(false),
      m_interval(false), m_spec_min(0), m_spec_max(Mantid::EMPTY_INT()),
      m_spec_list(), m_filtered_spec_idxs(), m_cppFile(NULL) {}

/// Delete NexusFileIO in destructor
LoadNexusProcessed::~LoadNexusProcessed() { delete m_cppFile; }

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadNexusProcessed::confidence(Kernel::NexusDescriptor &descriptor) const {
  if (descriptor.pathExists("/mantid_workspace_1"))
    return 80;
  else
    return 0;
}

/** Initialisation method.
 *
 */
void LoadNexusProcessed::init() {
  // Declare required input parameters for algorithm
  std::vector<std::string> exts;
  exts.push_back(".nxs");
  exts.push_back(".nx5");
  exts.push_back(".xml");
  declareProperty(
      new FileProperty("Filename", "", FileProperty::Load, exts),
      "The name of the Nexus file to read, as a full or relative path.");
  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name of the workspace to be created as the output of "
                  "the algorithm.  A workspace of this name will be created "
                  "and stored in the Analysis Data Service. For multiperiod "
                  "files, one workspace may be generated for each period. "
                  "Currently only one workspace can be saved at a time so "
                  "multiperiod Mantid files are not generated.");

  // optional
  auto mustBePositive = boost::make_shared<BoundedValidator<int64_t>>();
  mustBePositive->setLower(0);

  declareProperty("SpectrumMin", (int64_t)1, mustBePositive,
                  "Number of first spectrum to read.");
  declareProperty("SpectrumMax", (int64_t)Mantid::EMPTY_INT(), mustBePositive,
                  "Number of last spectrum to read.");
  declareProperty(new ArrayProperty<int64_t>("SpectrumList"),
                  "List of spectrum numbers to read.");
  declareProperty("EntryNumber", (int64_t)0, mustBePositive,
                  "The particular entry number to read. Default load all "
                  "workspaces and creates a workspacegroup (default: read all "
                  "entries).");
  declareProperty("LoadHistory", true,
                  "If true, the workspace history will be loaded");
  declareProperty(
      new PropertyWithValue<bool>("FastMultiPeriod", true, Direction::Input),
      "For multiperiod workspaces. Copy instrument, parameter and x-data "
      "rather than loading it directly for each workspace. Y, E and log "
      "information is always loaded.");
}

/**
 * Loading specifically for mulitperiod group workspaces
 * @param root : NXRoot ref
 * @param entryName : Entry name to load.
 * @param tempMatrixWorkspace : Template workspace to base the next multiperiod
 * entry off.
 * @param nWorkspaceEntries : N entries in the file
 * @param p : index + 1 being processed.
 * @return Next multiperiod group workspace
 */
Workspace_sptr LoadNexusProcessed::doAccelleratedMultiPeriodLoading(
    NXRoot &root, const std::string &entryName,
    MatrixWorkspace_sptr &tempMatrixWorkspace, const int64_t nWorkspaceEntries,
    const int64_t p) {

  MatrixWorkspace_sptr periodWorkspace =
      WorkspaceFactory::Instance().create(tempMatrixWorkspace);

  const size_t nHistograms = periodWorkspace->getNumberHistograms();
  for (size_t i = 0; i < nHistograms; ++i) {
    periodWorkspace->setX(i, tempMatrixWorkspace->refX(i));
  }

  NXEntry mtdEntry = root.openEntry(entryName);
  const std::string groupName = "workspace";
  if (!mtdEntry.containsGroup(groupName)) {
    std::stringstream buffer;
    buffer
        << "Group entry " << p - 1
        << " is not a workspace 2D. Retry with FastMultiPeriod option set off."
        << std::endl;
    throw std::runtime_error(buffer.str());
  }

  NXData wsEntry = mtdEntry.openNXData(groupName);
  if (wsEntry.isValid("frac_area")) {
    std::stringstream buffer;
    buffer << "Group entry " << p - 1 << " has fractional area present. Try "
                                         "reloading with FastMultiPeriod set "
                                         "off." << std::endl;
    throw std::runtime_error(buffer.str());
  }

  NXDataSetTyped<double> data = wsEntry.openDoubleData();
  NXDataSetTyped<double> errors = wsEntry.openNXDouble("errors");

  const int nChannels = data.dim1();

  int64_t blockSize = 8; // Read block size. Set to 8 for efficiency. i.e. read
                         // 8 histograms at a time.
  const int64_t nFullBlocks =
      nHistograms /
      blockSize; // Truncated number of full blocks to read. Remainder removed
  const int64_t readOptimumStop = (nFullBlocks * blockSize);
  const int64_t readStop = m_spec_max - 1;
  const int64_t finalBlockSize = readStop - readOptimumStop;

  int64_t wsIndex = 0;
  int64_t histIndex = m_spec_min - 1;

  for (; histIndex < readStop;) {
    if (histIndex >= readOptimumStop) {
      blockSize = finalBlockSize;
    }

    data.load(static_cast<int>(blockSize), static_cast<int>(histIndex));
    errors.load(static_cast<int>(blockSize), static_cast<int>(histIndex));

    double *dataStart = data();
    double *dataEnd = dataStart + nChannels;

    double *errorStart = errors();
    double *errorEnd = errorStart + nChannels;

    int64_t final(histIndex + blockSize);
    while (histIndex < final) {
      MantidVec &Y = periodWorkspace->dataY(wsIndex);
      Y.assign(dataStart, dataEnd);
      dataStart += nChannels;
      dataEnd += nChannels;
      MantidVec &E = periodWorkspace->dataE(wsIndex);
      E.assign(errorStart, errorEnd);
      errorStart += nChannels;
      errorEnd += nChannels;

      ++wsIndex;
      ++histIndex;
    }
  }

  m_cppFile->openPath(mtdEntry.path());
  try {
    // This loads logs, sample, and instrument.
    periodWorkspace->loadSampleAndLogInfoNexus(m_cppFile);
  } catch (std::exception &e) {
    g_log.information("Error loading Instrument section of nxs file");
    g_log.information(e.what());
  }

  const double fractionComplete = double(p - 1) / double(nWorkspaceEntries);
  progress(fractionComplete, "Loading multiperiod entry");
  return periodWorkspace;
}

//-------------------------------------------------------------------------------------------------
/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void LoadNexusProcessed::exec() {
  progress(0, "Opening file...");

  // Throws an approriate exception if there is a problem with file access
  NXRoot root(getPropertyValue("Filename"));

  // "Open" the same file but with the C++ interface
  m_cppFile = new ::NeXus::File(root.m_fileID);

  // Find out how many first level entries there are
  int64_t nWorkspaceEntries = static_cast<int64_t>(root.groups().size());

  // Check for an entry number property
  int64_t entrynumber = static_cast<int64_t>(getProperty("EntryNumber"));
  Property const *const entryNumberProperty = this->getProperty("EntryNumber");
  bool bDefaultEntryNumber = entryNumberProperty->isDefault();

  if (!bDefaultEntryNumber && entrynumber > nWorkspaceEntries) {
    g_log.error() << "Invalid entry number specified. File only contains "
                  << nWorkspaceEntries << " entries.\n";
    throw std::invalid_argument("Invalid entry number specified.");
  }

  const std::string basename = "mantid_workspace_";

  std::ostringstream os;
  if (bDefaultEntryNumber) {
    // Set the entry number to 1 if not provided.
    entrynumber = 1;
  }
  os << basename << entrynumber;
  const std::string targetEntryName = os.str();

  // Take the first real workspace obtainable. We need it even if loading
  // groups.
  API::Workspace_sptr tempWS = loadEntry(root, targetEntryName, 0, 1);

  if (nWorkspaceEntries == 1 || !bDefaultEntryNumber) {
    // We have what we need.
    setProperty("OutputWorkspace", tempWS);
  } else {
    // We already know that this is a group workspace. Is it a true multiperiod
    // workspace.
    const bool bFastMultiPeriod = this->getProperty("FastMultiPeriod");
    const bool bIsMultiPeriod =
        isMultiPeriodFile(static_cast<int>(nWorkspaceEntries), tempWS, g_log);
    Property *specListProp = this->getProperty("SpectrumList");
    m_list = !specListProp->isDefault();

    // Load all first level entries
    WorkspaceGroup_sptr wksp_group(new WorkspaceGroup);
    // This forms the name of the group
    std::string base_name = getPropertyValue("OutputWorkspace");
    // First member of group should be the group itself, for some reason!

    // load names of each of the workspaces and check for a common stem
    std::vector<std::string> names(nWorkspaceEntries + 1);
    bool commonStem = bIsMultiPeriod || checkForCommonNameStem(root, names);

    // remove existing workspace and replace with the one being loaded
    bool wsExists = AnalysisDataService::Instance().doesExist(base_name);
    if (wsExists) {
      Algorithm_sptr alg =
          AlgorithmManager::Instance().createUnmanaged("DeleteWorkspace");
      alg->initialize();
      alg->setChild(true);
      alg->setProperty("Workspace", base_name);
      alg->execute();
    }

    base_name += "_";
    const std::string prop_name = "OutputWorkspace_";

    MatrixWorkspace_sptr tempMatrixWorkspace =
        boost::dynamic_pointer_cast<Workspace2D>(tempWS);
    bool bAccelleratedMultiPeriodLoading = false;
    if (tempMatrixWorkspace) {
      // We only accelerate for simple scenarios for now. Spectrum lists are too
      // complicated to bother with.
      bAccelleratedMultiPeriodLoading =
          bIsMultiPeriod && bFastMultiPeriod && !m_list;
      tempMatrixWorkspace->mutableRun().clearLogs(); // Strip out any loaded
                                                     // logs. That way we don't
                                                     // pay for copying that
                                                     // information around.
    }

    if (bAccelleratedMultiPeriodLoading) {
      g_log.information("Accelerated multiperiod loading");
    } else {
      g_log.information("Individual group loading");
    }

    for (int64_t p = 1; p <= nWorkspaceEntries; ++p) {
      std::ostringstream os;
      os << p;

      // decide what the workspace should be called
      std::string wsName =
          buildWorkspaceName(names[p], base_name, p, commonStem);

      Workspace_sptr local_workspace;

      /*
       For multiperiod workspaces we can accelerate the loading by making
       resonable assumptions about the differences between the workspaces
       Only Y, E and log data entries should vary. Therefore we can clone our
       temp workspace, and overwrite those things we are interested in.
       */
      if (bAccelleratedMultiPeriodLoading) {
        local_workspace = doAccelleratedMultiPeriodLoading(
            root, basename + os.str(), tempMatrixWorkspace, nWorkspaceEntries,
            p);
      } else // Fall-back for generic loading
      {
        const double nWorkspaceEntries_d =
            static_cast<double>(nWorkspaceEntries);
        local_workspace =
            loadEntry(root, basename + os.str(),
                      static_cast<double>(p - 1) / nWorkspaceEntries_d,
                      1. / nWorkspaceEntries_d);
      }

      declareProperty(new WorkspaceProperty<API::Workspace>(
          prop_name + os.str(), wsName, Direction::Output));

      wksp_group->addWorkspace(local_workspace);
      setProperty(prop_name + os.str(), local_workspace);
    }

    // The group is the root property value
    setProperty("OutputWorkspace",
                boost::static_pointer_cast<Workspace>(wksp_group));
  }

  m_axis1vals.clear();
}

/**
 * Decides what to call a child of a group workspace.
 *
 * This function uses information about if the child workspace has a common stem
 * and checks if the file contained a workspace name to decide what it should be
 *called
 *
 * @param name :: The name loaded from the file (possibly the empty string if
 *none was loaded)
 * @param baseName :: The name group workspace
 * @param wsIndex :: The current index of this workspace
 * @param commonStem :: Whether the workspaces share a common name stem
 *
 * @return The name of the workspace
 */
std::string LoadNexusProcessed::buildWorkspaceName(const std::string &name,
                                                   const std::string &baseName,
                                                   int64_t wsIndex,
                                                   bool commonStem) {
  std::string wsName;
  std::string index = boost::lexical_cast<std::string>(wsIndex);

  // if we don't have a common stem then use name tag
  if (!commonStem) {
    if (!name.empty()) {
      // use name loaded from file there's no common stem
      wsName = name;
    } else {
      // if the name property wasn't defined just use <OutputWorkspaceName>_n
      wsName = baseName + index;
    }
  } else {
    // we have a common stem so rename accordingly
    boost::smatch results;
    const boost::regex exp(".*_(\\d+$)");
    // if we have a common name stem then name is <OutputWorkspaceName>_n
    if (boost::regex_search(name, results, exp)) {
      wsName = baseName + std::string(results[1].first, results[1].second);
    } else {
      // use default name if we couldn't match for some reason
      wsName = baseName + index;
    }
  }

  correctForWorkspaceNameClash(wsName);

  return wsName;
}

/**
 * Append an index to the name if it already exists in the AnalysisDataService
 *
 * @param wsName :: Name to call the workspace
 */
void LoadNexusProcessed::correctForWorkspaceNameClash(std::string &wsName) {
  bool noClash(false);

  for (int i = 0; !noClash; ++i) {
    std::string wsIndex = ""; // dont use an index if there is no other
                              // workspace
    if (i > 0) {
      wsIndex = "_" + boost::lexical_cast<std::string>(i);
    }

    bool wsExists = AnalysisDataService::Instance().doesExist(wsName + wsIndex);
    if (!wsExists) {
      wsName += wsIndex;
      noClash = true;
    }
  }
}

/**
 * Check if the workspace name contains a common stem and load the workspace
 *names
 *
 * @param root :: the root for the NeXus document
 * @param names :: vector to store the names to be loaded.
 * @return Whether there was a common stem.
 */
bool
LoadNexusProcessed::checkForCommonNameStem(NXRoot &root,
                                           std::vector<std::string> &names) {
  bool success(true);
  int64_t nWorkspaceEntries = static_cast<int64_t>(root.groups().size());
  for (int64_t p = 1; p <= nWorkspaceEntries; ++p) {
    std::ostringstream os;
    os << p;

    names[p] = loadWorkspaceName(root, "mantid_workspace_" + os.str());

    boost::smatch results;
    const boost::regex exp(".*_\\d+$");

    // check if the workspace name has an index on the end
    if (!boost::regex_match(names[p], results, exp)) {
      success = false;
    }
  }

  return success;
}

/**
 * Load the workspace name, if the attribute exists
 *
 * @param root :: Root of NeXus file
 * @param entry_name :: Entry in NeXus file to look at
 * @return The workspace name. If none found an empty string is returned.
 */
std::string
LoadNexusProcessed::loadWorkspaceName(NXRoot &root,
                                      const std::string &entry_name) {
  NXEntry mtd_entry = root.openEntry(entry_name);
  try {
    return mtd_entry.getString("workspace_name");
  } catch (std::runtime_error &) {
    return std::string();
  }
}

//-------------------------------------------------------------------------------------------------
/**
 * Load an event_workspace field
 *
 * @param wksp_cls  Nexus data for "event_workspace"
 * @param xbins bins on the "X" axis
 * @param progressStart algorithm progress (from 0)
 * @param progressRange  progress made after loading an entry
 *
 * @return event_workspace object with data
 */
API::MatrixWorkspace_sptr
LoadNexusProcessed::loadEventEntry(NXData &wksp_cls, NXDouble &xbins,
                                   const double &progressStart,
                                   const double &progressRange) {
  NXDataSetTyped<int64_t> indices_data =
      wksp_cls.openNXDataSet<int64_t>("indices");
  indices_data.load();
  size_t numspec = indices_data.dim0() - 1;

  // process optional spectrum parameters, if set
  checkOptionalProperties(numspec);
  // Actual number of spectra in output workspace (if only a user-specified
  // range and/or list was going to be loaded)
  numspec = calculateWorkspaceSize(numspec, true);

  int num_xbins = xbins.dim0();
  if (num_xbins < 2)
    num_xbins = 2;
  EventWorkspace_sptr ws = boost::dynamic_pointer_cast<EventWorkspace>(
      WorkspaceFactory::Instance().create("EventWorkspace", numspec, num_xbins,
                                          num_xbins - 1));

  // Set the YUnit label
  ws->setYUnit(indices_data.attributes("units"));
  std::string unitLabel = indices_data.attributes("unit_label");
  if (unitLabel.empty())
    unitLabel = indices_data.attributes("units");
  ws->setYUnitLabel(unitLabel);

  // Handle optional fields.
  // TODO: Handle inconsistent sizes
  boost::shared_array<int64_t> pulsetimes;
  if (wksp_cls.isValid("pulsetime")) {
    NXDataSetTyped<int64_t> pulsetime =
        wksp_cls.openNXDataSet<int64_t>("pulsetime");
    pulsetime.load();
    pulsetimes = pulsetime.sharedBuffer();
  }

  boost::shared_array<double> tofs;
  if (wksp_cls.isValid("tof")) {
    NXDouble tof = wksp_cls.openNXDouble("tof");
    tof.load();
    tofs = tof.sharedBuffer();
  }

  boost::shared_array<float> error_squareds;
  if (wksp_cls.isValid("error_squared")) {
    NXFloat error_squared = wksp_cls.openNXFloat("error_squared");
    error_squared.load();
    error_squareds = error_squared.sharedBuffer();
  }

  boost::shared_array<float> weights;
  if (wksp_cls.isValid("weight")) {
    NXFloat weight = wksp_cls.openNXFloat("weight");
    weight.load();
    weights = weight.sharedBuffer();
  }

  // What type of event lists?
  EventType type = TOF;
  if (tofs && pulsetimes && weights && error_squareds)
    type = WEIGHTED;
  else if ((tofs && weights && error_squareds))
    type = WEIGHTED_NOTIME;
  else if (pulsetimes && tofs)
    type = TOF;
  else
    throw std::runtime_error("Could not figure out the type of event list!");

  // indices of events
  boost::shared_array<int64_t> indices = indices_data.sharedBuffer();
  // Create all the event lists
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t j = 0; j < static_cast<int64_t>(m_filtered_spec_idxs.size()); j++) {
    PARALLEL_START_INTERUPT_REGION
    size_t wi = m_filtered_spec_idxs[j] - 1;
    int64_t index_start = indices[wi];
    int64_t index_end = indices[wi + 1];
    if (index_end >= index_start) {
      EventList &el = ws->getEventList(j);
      el.switchTo(type);

      // Allocate all the required memory
      el.reserve(index_end - index_start);
      el.clearDetectorIDs();

      for (int64_t i = index_start; i < index_end; i++)
        switch (type) {
        case TOF:
          el.addEventQuickly(TofEvent(tofs[i], DateAndTime(pulsetimes[i])));
          break;
        case WEIGHTED:
          el.addEventQuickly(WeightedEvent(tofs[i], DateAndTime(pulsetimes[i]),
                                           weights[i], error_squareds[i]));
          break;
        case WEIGHTED_NOTIME:
          el.addEventQuickly(
              WeightedEventNoTime(tofs[i], weights[i], error_squareds[i]));
          break;
        }

      // Set the X axis
      if (this->m_shared_bins)
        el.setX(this->m_xbins);
      else {
        MantidVec x;
        x.resize(xbins.dim0());
        for (int i = 0; i < xbins.dim0(); i++)
          x[i] = xbins(static_cast<int>(wi), i);
        el.setX(x);
      }
    }

    progress(progressStart + progressRange *
             (1.0 / static_cast<double>(numspec)));
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  return ws;
}

//-------------------------------------------------------------------------------------------------
/**
 * Load a numeric column to the TableWorkspace.
 * @param tableData   :: Table data to load from
 * @param dataSetName :: Name of the data set to use to get column data
 * @param tableWs     :: Workspace to add column to
 * @param columnType  :: Name of the column type to create
 */
template <typename ColumnType, typename NexusType>
void LoadNexusProcessed::loadNumericColumn(
    const Mantid::NeXus::NXData &tableData, const std::string &dataSetName,
    const API::ITableWorkspace_sptr &tableWs, const std::string &columnType) {
  NXDataSetTyped<NexusType> data =
      tableData.openNXDataSet<NexusType>(dataSetName);
  std::string columnTitle = data.attributes("name");
  if (!columnTitle.empty()) {
    data.load();
    auto length = static_cast<size_t>(data.dim0());
    auto rowCount = tableWs->rowCount();
    // check that the row count is OK
    if (rowCount == 0) {
      tableWs->setRowCount(length);
    } else if (rowCount != length) {
      throw std::runtime_error("Columns have different sizes.");
    }
    // copy the data
    auto column = tableWs->addColumn(columnType, columnTitle);
    for (size_t i = 0; i < length; i++) {
      column->cell<ColumnType>(i) = static_cast<ColumnType>(*(data() + i));
    }
  }
}

//-------------------------------------------------------------------------------------------------
/**
 * Load a table
 */
API::Workspace_sptr LoadNexusProcessed::loadTableEntry(NXEntry &entry) {
  API::ITableWorkspace_sptr workspace;
  workspace =
      Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");

  NXData nx_tw = entry.openNXData("table_workspace");

  int columnNumber = 1;
  do {
    std::string dataSetName =
        "column_" + boost::lexical_cast<std::string>(columnNumber);

    NXInfo info = nx_tw.getDataSetInfo(dataSetName.c_str());
    if (info.stat == NX_ERROR) {
      // Assume we done last column of table
      break;
    }

    if (info.rank == 1) {
      if (info.type == NX_FLOAT64) {
        loadNumericColumn<double, double>(nx_tw, dataSetName, workspace,
                                          "double");
      } else if (info.type == NX_INT32) {
        loadNumericColumn<int, int32_t>(nx_tw, dataSetName, workspace, "int");
      } else if (info.type == NX_UINT32) {
        loadNumericColumn<uint32_t, uint32_t>(nx_tw, dataSetName, workspace,
                                              "uint");
      } else if (info.type == NX_INT64) {
        loadNumericColumn<int64_t, int64_t>(nx_tw, dataSetName, workspace,
                                            "long64");
      } else if (info.type == NX_UINT64) {
        loadNumericColumn<size_t, uint64_t>(nx_tw, dataSetName, workspace,
                                            "size_t");
      } else if (info.type == NX_FLOAT32) {
        loadNumericColumn<float, float>(nx_tw, dataSetName, workspace, "float");
      } else if (info.type == NX_UINT8) {
        loadNumericColumn<bool, bool>(nx_tw, dataSetName, workspace, "bool");
      } else {
        throw std::logic_error("Column with Nexus data type " +
                               boost::lexical_cast<std::string>(info.type) +
                               " cannot be loaded.");
      }
    } else if (info.rank == 2) {
      if (info.type == NX_CHAR) {
        NXChar data = nx_tw.openNXChar(dataSetName.c_str());
        std::string columnTitle = data.attributes("name");
        if (!columnTitle.empty()) {
          workspace->addColumn("str", columnTitle);
          int nRows = info.dims[0];
          workspace->setRowCount(nRows);

          const int maxStr = info.dims[1];
          data.load();
          for (int iR = 0; iR < nRows; ++iR) {
            auto &cellContents =
                workspace->cell<std::string>(iR, columnNumber - 1);
            auto startPoint = data() + maxStr * iR;
            cellContents.assign(startPoint, startPoint + maxStr);
            boost::trim_right(cellContents);
          }
        }
      } else if (info.type == NX_INT32) {
        loadVectorColumn<int>(nx_tw, dataSetName, workspace, "vector_int");
      } else if (info.type == NX_FLOAT64) {
        auto data = nx_tw.openNXDouble(dataSetName.c_str());
        if (data.attributes("interpret_as") == "V3D") {
          loadV3DColumn(data, workspace);
        } else {
          loadVectorColumn<double>(nx_tw, dataSetName, workspace,
                                   "vector_double");
        }
      }
    }

    columnNumber++;

  } while (1);

  return boost::static_pointer_cast<API::Workspace>(workspace);
}

/**
 * Loads a vector column to the TableWorkspace.
 * @param tableData   :: Table data to load from
 * @param dataSetName :: Name of the data set to use to get column data
 * @param tableWs     :: Workspace to add column to
 * @param columnType  :: Name of the column type to create
 */
template <typename Type>
void LoadNexusProcessed::loadVectorColumn(const NXData &tableData,
                                          const std::string &dataSetName,
                                          const ITableWorkspace_sptr &tableWs,
                                          const std::string &columnType) {
  NXDataSetTyped<Type> data =
      tableData.openNXDataSet<Type>(dataSetName.c_str());
  std::string columnTitle = data.attributes("name");
  if (!columnTitle.empty()) {
    tableWs->addColumn(columnType, columnTitle);

    NXInfo info = tableData.getDataSetInfo(dataSetName.c_str());
    const size_t rowCount = info.dims[0];
    const size_t blockSize = info.dims[1];

    // This might've been done already, but doing it twice should't do any harm
    tableWs->setRowCount(rowCount);

    data.load();

    for (size_t i = 0; i < rowCount; ++i) {
      auto &cell =
          tableWs->cell<std::vector<Type>>(i, tableWs->columnCount() - 1);

      Type *from = data() + blockSize * i;

      cell.assign(from, from + blockSize);

      std::ostringstream rowSizeAttrName;
      rowSizeAttrName << "row_size_" << i;

      // This is ugly, but I can only get attribute as a string using the API
      std::istringstream rowSizeStr(data.attributes(rowSizeAttrName.str()));

      int rowSize;
      rowSizeStr >> rowSize;

      cell.resize(rowSize);
    }
  }
}

/**
 * Loads a V3D column to the TableWorkspace.
 * @param data   :: Table data to load from
 * @param tableWs     :: Workspace to add column to
 */
void
LoadNexusProcessed::loadV3DColumn(Mantid::NeXus::NXDouble &data,
                                  const API::ITableWorkspace_sptr &tableWs) {
  std::string columnTitle = data.attributes("name");
  if (!columnTitle.empty()) {
    ColumnVector<V3D> col = tableWs->addColumn("V3D", columnTitle);

    const int rowCount = data.dim0();

    // This might've been done already, but doing it twice should't do any harm
    tableWs->setRowCount(rowCount);

    data.load();

    for (int i = 0; i < rowCount; ++i) {
      auto &cell = col[i];
      cell(data(i, 0), data(i, 1), data(i, 2));
    }
  }
}

//-------------------------------------------------------------------------------------------------
/**
 * Load peaks
 */
API::Workspace_sptr LoadNexusProcessed::loadPeaksEntry(NXEntry &entry) {
  // API::IPeaksWorkspace_sptr workspace;
  API::ITableWorkspace_sptr tWorkspace;
  // PeaksWorkspace_sptr workspace;
  tWorkspace =
      Mantid::API::WorkspaceFactory::Instance().createTable("PeaksWorkspace");

  PeaksWorkspace_sptr peakWS =
      boost::dynamic_pointer_cast<PeaksWorkspace>(tWorkspace);

  NXData nx_tw = entry.openNXData("peaks_workspace");

  int columnNumber = 1;
  int numberPeaks = 0;
  std::vector<std::string> columnNames;
  do {
    std::string str =
        "column_" + boost::lexical_cast<std::string>(columnNumber);

    NXInfo info = nx_tw.getDataSetInfo(str.c_str());
    if (info.stat == NX_ERROR) {
      // Assume we done last column of table
      break;
    }

    // store column names
    columnNames.push_back(str);

    // determine number of peaks
    // here we assume that a peaks_table has always one column of doubles

    if (info.type == NX_FLOAT64) {
      NXDouble nxDouble = nx_tw.openNXDouble(str.c_str());
      std::string columnTitle = nxDouble.attributes("name");
      if (!columnTitle.empty() && numberPeaks == 0) {
        numberPeaks = nxDouble.dim0();
      }
    }

    columnNumber++;

  } while (1);

  // Get information from all but data group
  std::string parameterStr;
  // Hop to the right point
  m_cppFile->openPath(entry.path());
  try {
    // This loads logs, sample, and instrument.
    peakWS->loadExperimentInfoNexus(m_cppFile, parameterStr);
  } catch (std::exception &e) {
    g_log.information("Error loading Instrument section of nxs file");
    g_log.information(e.what());
  }

  // std::vector<API::IPeak*> p;
  for (int r = 0; r < numberPeaks; r++) {
    Kernel::V3D v3d;
    v3d[2] = 1.0;
    API::IPeak *p;
    p = peakWS->createPeak(v3d);
    peakWS->addPeak(*p);
  }

  for (size_t i = 0; i < columnNames.size(); i++) {
    const std::string str = columnNames[i];
    if (!str.compare("column_1")) {
      NXInt nxInt = nx_tw.openNXInt(str.c_str());
      nxInt.load();

      for (int r = 0; r < numberPeaks; r++) {
        int ival = nxInt[r];
        if (ival != -1)
          peakWS->getPeak(r).setDetectorID(ival);
      }
    }

    if (!str.compare("column_2")) {
      NXDouble nxDouble = nx_tw.openNXDouble(str.c_str());
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setH(val);
      }
    }

    if (!str.compare("column_3")) {
      NXDouble nxDouble = nx_tw.openNXDouble(str.c_str());
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setK(val);
      }
    }

    if (!str.compare("column_4")) {
      NXDouble nxDouble = nx_tw.openNXDouble(str.c_str());
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setL(val);
      }
    }

    if (!str.compare("column_5")) {
      NXDouble nxDouble = nx_tw.openNXDouble(str.c_str());
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setIntensity(val);
      }
    }

    if (!str.compare("column_6")) {
      NXDouble nxDouble = nx_tw.openNXDouble(str.c_str());
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setSigmaIntensity(val);
      }
    }

    if (!str.compare("column_7")) {
      NXDouble nxDouble = nx_tw.openNXDouble(str.c_str());
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setBinCount(val);
      }
    }

    if (!str.compare("column_10")) {
      NXDouble nxDouble = nx_tw.openNXDouble(str.c_str());
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setWavelength(val);
      }
    }

    if (!str.compare("column_14")) {
      NXInt nxInt = nx_tw.openNXInt(str.c_str());
      nxInt.load();

      for (int r = 0; r < numberPeaks; r++) {
        int ival = nxInt[r];
        if (ival != -1)
          peakWS->getPeak(r).setRunNumber(ival);
      }
    }

    if (!str.compare("column_15")) {
      NXDouble nxDouble = nx_tw.openNXDouble(str.c_str());
      nxDouble.load();
      Kernel::Matrix<double> gm(3, 3, false);
      int k = 0;
      for (int r = 0; r < numberPeaks; r++) {
        for (int j = 0; j < 9; j++) {
          double val = nxDouble[k];
          k++;
          gm[j % 3][j / 3] = val;
        }
        peakWS->getPeak(r).setGoniometerMatrix(gm);
      }
    }

    if (!str.compare("column_16")) {
      // Read shape information
      using namespace Mantid::DataObjects;

      PeakShapeFactory_sptr peakFactoryEllipsoid = boost::make_shared<PeakShapeEllipsoidFactory>();
      PeakShapeFactory_sptr peakFactorySphere = boost::make_shared<PeakShapeSphericalFactory>();
      PeakShapeFactory_sptr peakFactoryNone = boost::make_shared<PeakNoShapeFactory>();

      peakFactoryEllipsoid->setSuccessor(peakFactorySphere);
      peakFactorySphere->setSuccessor(peakFactoryNone);

      NXInfo info = nx_tw.getDataSetInfo(str.c_str());
      NXChar data = nx_tw.openNXChar(str.c_str());

      const int maxShapeJSONLength = info.dims[1];
      data.load();
      for (int i = 0; i < numberPeaks; ++i) {

        // iR = peak row number
        auto startPoint = data() + (maxShapeJSONLength * i);
        std::string shapeJSON(startPoint, startPoint + maxShapeJSONLength);
        boost::trim_right(shapeJSON);

        // Make the shape
        Mantid::Geometry::PeakShape* peakShape = peakFactoryEllipsoid->create(shapeJSON);

        // Set the shape
        peakWS->getPeak(i).setPeakShape(peakShape);

      }
    }
  }

return boost::static_pointer_cast<API::Workspace>(peakWS);
}

//-------------------------------------------------------------------------------------------------
/**
 * Load a Workspace2D
 *
 * @param wksp_cls Nexus data for "Workspace2D" (or "offsets_workspace")
 * @param xbins bins on the "X" axis
 * @param progressStart algorithm progress (from 0)
 * @param progressRange progress made after loading an entry
 * @param mtd_entry Nexus entry for "mantid_workspace_..."
 * @param xlength bins in the "X" axis (xbins)
 * @param workspaceType Takes values like "Workspace2D", "RebinnedOutput", etc.
 *
 * @return workspace object containing loaded data
 */
API::MatrixWorkspace_sptr
LoadNexusProcessed::loadNonEventEntry(NXData &wksp_cls,
                                      NXDouble &xbins,
                                      const double &progressStart,
                                      const double &progressRange,
                                      const NXEntry &mtd_entry,
                                      const int xlength,
                                      std::string &workspaceType) {
  // Filter the list of spectra to process, applying min/max/list options
  NXDataSetTyped<double> data = wksp_cls.openDoubleData();
  int64_t nchannels = data.dim1();
  size_t nspectra = data.dim0();
  // process optional spectrum parameters, if set
  checkOptionalProperties(nspectra);
  // Actual number of spectra in output workspace (if only a range was going
  // to be loaded)
  size_t total_specs = calculateWorkspaceSize(nspectra);

  //// Create the 2D workspace for the output
  bool hasFracArea = false;
  if (wksp_cls.isValid("frac_area")) {
    // frac_area entry is the signal for a RebinnedOutput workspace
    hasFracArea = true;
    workspaceType.clear();
    workspaceType = "RebinnedOutput";
  }

  API::MatrixWorkspace_sptr local_workspace =
      boost::dynamic_pointer_cast<API::MatrixWorkspace>(
         WorkspaceFactory::Instance().create(workspaceType, total_specs, xlength,
                                             nchannels));
  try {
    local_workspace->setTitle(mtd_entry.getString("title"));
  } catch (std::runtime_error &) {
    g_log.debug() << "No title was found in the input file, "
                  << getPropertyValue("Filename") << std::endl;
  }

  // Set the YUnit label
  local_workspace->setYUnit(data.attributes("units"));
  std::string unitLabel = data.attributes("unit_label");
  if (unitLabel.empty())
    unitLabel = data.attributes("units");
  local_workspace->setYUnitLabel(unitLabel);

  readBinMasking(wksp_cls, local_workspace);
  NXDataSetTyped<double> errors = wksp_cls.openNXDouble("errors");
  NXDataSetTyped<double> fracarea = errors;
  if (hasFracArea) {
    fracarea = wksp_cls.openNXDouble("frac_area");
  }

  int64_t blocksize(8);
  // const int fullblocks = nspectra / blocksize;
  // size of the workspace
  int64_t fullblocks = total_specs / blocksize;
  int64_t read_stop = (fullblocks * blocksize);
  const double progressBegin = progressStart + 0.25 * progressRange;
  const double progressScaler = 0.75 * progressRange;
  int64_t hist_index = 0;
  int64_t wsIndex = 0;
  if (m_shared_bins) {
    // if spectrum min,max,list properties are set
    if (m_interval || m_list) {
      // if spectrum max,min properties are set read the data as a
      // block(multiple of 8) and
      // then read the remaining data as finalblock
      if (m_interval) {
        // specs at the min-max interval
        int interval_specs = static_cast<int>(m_spec_max - m_spec_min);
        fullblocks = (interval_specs) / blocksize;
        read_stop = (fullblocks * blocksize) + m_spec_min - 1;

        if (interval_specs < blocksize) {
          blocksize = total_specs;
          read_stop = m_spec_max - 1;
        }
        hist_index = m_spec_min - 1;

        for (; hist_index < read_stop;) {
          progress(progressBegin +
                   progressScaler * static_cast<double>(hist_index) /
                   static_cast<double>(read_stop),
                   "Reading workspace data...");
          loadBlock(data, errors, fracarea, hasFracArea, blocksize, nchannels,
                    hist_index, wsIndex, local_workspace);
        }
        int64_t finalblock = m_spec_max - 1 - read_stop;
        if (finalblock > 0) {
          loadBlock(data, errors, fracarea, hasFracArea, finalblock,
                    nchannels, hist_index, wsIndex, local_workspace);
        }
      }
      // if spectrum list property is set read each spectrum separately by
      // setting blocksize=1
      if (m_list) {
        std::vector<int64_t>::iterator itr = m_spec_list.begin();
        for (; itr != m_spec_list.end(); ++itr) {
          int64_t specIndex = (*itr) - 1;
          progress(progressBegin +
                   progressScaler * static_cast<double>(specIndex) /
                   static_cast<double>(m_spec_list.size()),
                   "Reading workspace data...");
          loadBlock(data, errors, fracarea, hasFracArea,
                    static_cast<int64_t>(1), nchannels, specIndex, wsIndex,
                    local_workspace);
        }
      }
    } else {
      for (; hist_index < read_stop;) {
        progress(progressBegin +
                 progressScaler * static_cast<double>(hist_index) /
                 static_cast<double>(read_stop),
                 "Reading workspace data...");
        loadBlock(data, errors, fracarea, hasFracArea, blocksize, nchannels,
                  hist_index, wsIndex, local_workspace);
      }
      int64_t finalblock = total_specs - read_stop;
      if (finalblock > 0) {
        loadBlock(data, errors, fracarea, hasFracArea, finalblock, nchannels,
                  hist_index, wsIndex, local_workspace);
      }
    }

  } else {
    if (m_interval || m_list) {
      if (m_interval) {
        int64_t interval_specs = m_spec_max - m_spec_min;
        fullblocks = (interval_specs) / blocksize;
        read_stop = (fullblocks * blocksize) + m_spec_min - 1;

        if (interval_specs < blocksize) {
          blocksize = interval_specs;
          read_stop = m_spec_max - 1;
        }
        hist_index = m_spec_min - 1;

        for (; hist_index < read_stop;) {
          progress(progressBegin +
                   progressScaler * static_cast<double>(hist_index) /
                   static_cast<double>(read_stop),
                   "Reading workspace data...");
          loadBlock(data, errors, fracarea, hasFracArea, xbins, blocksize,
                    nchannels, hist_index, wsIndex, local_workspace);
        }
        int64_t finalblock = m_spec_max - 1 - read_stop;
        if (finalblock > 0) {
          loadBlock(data, errors, fracarea, hasFracArea, xbins, finalblock,
                    nchannels, hist_index, wsIndex, local_workspace);
        }
      }
      //
      if (m_list) {
        std::vector<int64_t>::iterator itr = m_spec_list.begin();
        for (; itr != m_spec_list.end(); ++itr) {
          int64_t specIndex = (*itr) - 1;
          progress(progressBegin +
                   progressScaler * static_cast<double>(specIndex) /
                   static_cast<double>(read_stop),
                   "Reading workspace data...");
          loadBlock(data, errors, fracarea, hasFracArea, xbins, 1, nchannels,
                    specIndex, wsIndex, local_workspace);
        }
      }
    } else {
      for (; hist_index < read_stop;) {
        progress(progressBegin +
                 progressScaler * static_cast<double>(hist_index) /
                 static_cast<double>(read_stop),
                 "Reading workspace data...");
        loadBlock(data, errors, fracarea, hasFracArea, xbins, blocksize,
                  nchannels, hist_index, wsIndex, local_workspace);
      }
      int64_t finalblock = total_specs - read_stop;
      if (finalblock > 0) {
        loadBlock(data, errors, fracarea, hasFracArea, xbins, finalblock,
                  nchannels, hist_index, wsIndex, local_workspace);
      }
    }
  }
  return local_workspace;
}

//-------------------------------------------------------------------------------------------------
/**
 * Load a single entry into a workspace (event_workspace or workspace2d)
 *
 * @param root :: The opened root node
 * @param entry_name :: The entry name
 * @param progressStart :: The percentage value to start the progress reporting
 * for this entry
 * @param progressRange :: The percentage range that the progress reporting
 * should cover
 * @returns A 2D workspace containing the loaded data
 */
API::Workspace_sptr LoadNexusProcessed::loadEntry(NXRoot &root,
                                                  const std::string &entry_name,
                                                  const double &progressStart,
                                                  const double &progressRange) {
  progress(progressStart, "Opening entry " + entry_name + "...");

  NXEntry mtd_entry = root.openEntry(entry_name);

  if (mtd_entry.containsGroup("table_workspace")) {
    return loadTableEntry(mtd_entry);
  }

  if (mtd_entry.containsGroup("peaks_workspace")) {
    return loadPeaksEntry(mtd_entry);
  }

  // Determine workspace type and name of group containing workspace
  // characteristics
  bool isEvent = false;
  std::string workspaceType = "Workspace2D";
  std::string group_name = "workspace";
  if (mtd_entry.containsGroup("event_workspace")) {
    isEvent = true;
    group_name = "event_workspace";
  } else if (mtd_entry.containsGroup("offsets_workspace")) {
    workspaceType = "OffsetsWorkspace";
    group_name = "offsets_workspace";
  }

  // Get workspace characteristics
  NXData wksp_cls = mtd_entry.openNXData(group_name);

  // Axis information
  // "X" axis

  NXDouble xbins = wksp_cls.openNXDouble("axis1");
  xbins.load();
  std::string unit1 = xbins.attributes("units");
  // Non-uniform x bins get saved as a 2D 'axis1' dataset
  int xlength(-1);
  if (xbins.rank() == 2) {
    xlength = xbins.dim1();
    m_shared_bins = false;
  } else if (xbins.rank() == 1) {
    xlength = xbins.dim0();
    m_shared_bins = true;
    xbins.load();
    m_xbins.access().assign(xbins(), xbins() + xlength);
  } else {
    throw std::runtime_error("Unknown axis1 dimension encountered.");
  }

  // MatrixWorkspace axis 1
  NXDouble axis2 = wksp_cls.openNXDouble("axis2");
  std::string unit2 = axis2.attributes("units");

  // --- Load workspace (as event_workspace or workspace2d) ---
  API::MatrixWorkspace_sptr local_workspace;
  if (isEvent) {
    local_workspace =
        loadEventEntry(wksp_cls, xbins, progressStart, progressRange);
  } else {
    local_workspace =
        loadNonEventEntry(wksp_cls, xbins, progressStart, progressRange,
                          mtd_entry, xlength, workspaceType);
  }
  size_t nspectra = local_workspace->getNumberHistograms();

  // Units
  bool verticalHistogram(false);
  try {
    local_workspace->getAxis(0)->unit() = UnitFactory::Instance().create(unit1);
    if (unit1 == "Label") {
      auto label = boost::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(
          local_workspace->getAxis(0)->unit());
      auto ax = wksp_cls.openNXDouble("axis1");
      label->setLabel(ax.attributes("caption"), ax.attributes("label"));
    }

    // If this doesn't throw then it is a numeric access so grab the data so we
    // can set it later
    axis2.load();
    if (static_cast<size_t>(axis2.size()) == nspectra + 1)
      verticalHistogram = true;
    m_axis1vals = MantidVec(axis2(), axis2() + axis2.dim0());
  } catch (std::runtime_error &) {
    g_log.information() << "Axis 0 set to unitless quantity \"" << unit1
                        << "\"\n";
  }

  // Setting a unit onto a TextAxis makes no sense.
  if (unit2 == "TextAxis") {
    Mantid::API::TextAxis *newAxis = new Mantid::API::TextAxis(nspectra);
    local_workspace->replaceAxis(1, newAxis);
  } else if (unit2 != "spectraNumber") {
    try {
      auto *newAxis = (verticalHistogram) ? new API::BinEdgeAxis(nspectra + 1)
                                          : new API::NumericAxis(nspectra);
      local_workspace->replaceAxis(1, newAxis);
      newAxis->unit() = UnitFactory::Instance().create(unit2);
      if (unit2 == "Label") {
        auto label = boost::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(
            newAxis->unit());
        auto ax = wksp_cls.openNXDouble("axis2");
        label->setLabel(ax.attributes("caption"), ax.attributes("label"));
      }
    } catch (std::runtime_error &) {
      g_log.information() << "Axis 1 set to unitless quantity \"" << unit2
                          << "\"\n";
    }
  }

  // Are we a distribution
  std::string dist = xbins.attributes("distribution");
  if (dist == "1") {
    local_workspace->isDistribution(true);
  } else {
    local_workspace->isDistribution(false);
  }

  // Get information from all but data group
  std::string parameterStr;

  progress(progressStart + 0.05 * progressRange,
           "Reading the sample details...");

  // Hop to the right point
  m_cppFile->openPath(mtd_entry.path());
  try {
    // This loads logs, sample, and instrument.
    local_workspace->loadExperimentInfoNexus(
        m_cppFile, parameterStr); // REQUIRED PER PERIOD
  } catch (std::exception &e) {
    g_log.information("Error loading Instrument section of nxs file");
    g_log.information(e.what());
  }

  // Now assign the spectra-detector map
  readInstrumentGroup(mtd_entry, local_workspace);

  // Parameter map parsing
  progress(progressStart + 0.11 * progressRange,
           "Reading the parameter maps...");
  local_workspace->readParameterMap(parameterStr);

  if (!local_workspace->getAxis(1)
           ->isSpectra()) { // If not a spectra axis, load the axis data into
                            // the workspace. (MW 25/11/10)
    loadNonSpectraAxis(local_workspace, wksp_cls);
  }

  progress(progressStart + 0.15 * progressRange,
           "Reading the workspace history...");
  m_cppFile->openPath(mtd_entry.path());
  try {
    bool load_history = getProperty("LoadHistory");
    if (load_history)
      local_workspace->history().loadNexus(m_cppFile);
  } catch (std::out_of_range &) {
    g_log.warning() << "Error in the workspaces algorithm list, its processing "
                       "history is incomplete\n";
  }

  progress(progressStart + 0.2 * progressRange,
           "Reading the workspace history...");

  return boost::static_pointer_cast<API::Workspace>(local_workspace);
}

//-------------------------------------------------------------------------------------------------
/**
 * Read the instrument group
 * @param mtd_entry :: The node for the current workspace
 * @param local_workspace :: The workspace to attach the instrument
 */
void LoadNexusProcessed::readInstrumentGroup(
    NXEntry &mtd_entry, API::MatrixWorkspace_sptr local_workspace) {
  // Get spectrum information for the current entry.

  SpectraInfo spectraInfo = extractMappingInfo(mtd_entry, this->g_log);

  // Now build the spectra list
  int index = 0;

  for (int i = 1; i <= spectraInfo.nSpectra; ++i) {
    int spectrum(-1);
    if (spectraInfo.hasSpectra) {
      spectrum = spectraInfo.spectraNumbers[i - 1];
    } else {
      spectrum = i + 1;
    }

    if ((i >= m_spec_min && i < m_spec_max) ||
        (m_list &&
         find(m_spec_list.begin(), m_spec_list.end(), i) !=
             m_spec_list.end())) {
      ISpectrum *spec = local_workspace->getSpectrum(index);
      if (m_axis1vals.empty()) {
        spec->setSpectrumNo(spectrum);
      } else {
        spec->setSpectrumNo(static_cast<specid_t>(m_axis1vals[i - 1]));
      }
      ++index;

      int start = spectraInfo.detectorIndex[i - 1];
      int end = start + spectraInfo.detectorCount[i - 1];
      spec->setDetectorIDs(
          std::set<detid_t>(spectraInfo.detectorList.get() + start,
                            spectraInfo.detectorList.get() + end));
    }
  }
}

//-------------------------------------------------------------------------------------------------
/**
 * Loads the information contained in non-Spectra (ie, Text or Numeric) axis in
 * the Nexus
 * file into the workspace.
 * @param local_workspace :: pointer to workspace object
 * @param data :: reference to the NeXuS data for the axis
 */
void LoadNexusProcessed::loadNonSpectraAxis(
    API::MatrixWorkspace_sptr local_workspace, NXData &data) {
  Axis *axis = local_workspace->getAxis(1);

  if (axis->isNumeric()) {
    NXDouble axisData = data.openNXDouble("axis2");
    axisData.load();
    for (int i = 0; i < static_cast<int>(axis->length()); i++) {
      axis->setValue(i, axisData[i]);
    }
  } else if (axis->isText()) {
    NXChar axisData = data.openNXChar("axis2");
    axisData.load();
    std::string axisLabels = axisData();
    // Use boost::tokenizer to split up the input
    boost::char_separator<char> sep("\n");
    boost::tokenizer<boost::char_separator<char>> tokenizer(axisLabels, sep);
    // We must cast the axis object to TextAxis so we may use ->setLabel
    TextAxis *textAxis = static_cast<TextAxis *>(axis);
    int i = 0;
    for (auto tokIter = tokenizer.begin(); tokIter != tokenizer.end();
         ++tokIter, ++i) {
      textAxis->setLabel(i, *tokIter);
    }
  }
}

/**
 * Binary predicate function object to sort the AlgorithmHistory vector by
 * execution order
 * @param elem1 :: first element in the vector
 * @param elem2 :: second element in the vecor
 */
bool UDlesserExecCount(NXClassInfo elem1, NXClassInfo elem2) {
  std::string::size_type index1, index2;
  std::string num1, num2;
  // find the number after "_" in algorithm name ( eg:MantidAlogorthm_1)
  index1 = elem1.nxname.find("_");
  if (index1 != std::string::npos) {
    num1 = elem1.nxname.substr(index1 + 1, elem1.nxname.length() - index1);
  }
  index2 = elem2.nxname.find("_");
  if (index2 != std::string::npos) {
    num2 = elem2.nxname.substr(index2 + 1, elem2.nxname.length() - index2);
  }
  std::stringstream is1, is2;
  is1 << num1;
  is2 << num2;

  int execNum1 = -1;
  int execNum2 = -1;
  is1 >> execNum1;
  is2 >> execNum2;

  if (execNum1 < execNum2)
    return true;
  else
    return false;
}

//-------------------------------------------------------------------------------------------------
/** If the first string contains exactly three words separated by spaces
 *  these words will be copied into each of the following strings that were
 * passed
 *  @param[in] words3 a string with 3 words separated by spaces
 *  @param[out] w1 the first word in the input string
 *  @param[out] w2 the second word in the input string
 *  @param[out] w3 the third word in the input string
 *  @throw out_of_range if there aren't exaltly three strings in the word
 */
void LoadNexusProcessed::getWordsInString(const std::string &words3,
                                          std::string &w1, std::string &w2,
                                          std::string &w3) {
  Poco::StringTokenizer data(words3, " ", Poco::StringTokenizer::TOK_TRIM);
  if (data.count() != 3) {
    g_log.warning() << "Algorithm list line " + words3 +
                           " is not of the correct format\n";
    throw std::out_of_range(words3);
  }

  w1 = data[0];
  w2 = data[1];
  w3 = data[2];
}

//-------------------------------------------------------------------------------------------------
/** If the first string contains exactly four words separated by spaces
 *  these words will be copied into each of the following strings that were
 * passed
 *  @param[in] words4 a string with 4 words separated by spaces
 *  @param[out] w1 the first word in the input string
 *  @param[out] w2 the second word in the input string
 *  @param[out] w3 the third word in the input string
 *  @param[out] w4 the fourth word in the input string
 *  @throw out_of_range if there aren't exaltly four strings in the word
 */
void LoadNexusProcessed::getWordsInString(const std::string &words4,
                                          std::string &w1, std::string &w2,
                                          std::string &w3, std::string &w4) {
  Poco::StringTokenizer data(words4, " ", Poco::StringTokenizer::TOK_TRIM);
  if (data.count() != 4) {
    g_log.warning() << "Algorithm list line " + words4 +
                           " is not of the correct format\n";
    throw std::out_of_range(words4);
  }

  w1 = data[0];
  w2 = data[1];
  w3 = data[2];
  w4 = data[3];
}

//-------------------------------------------------------------------------------------------------
/**
 * Read the bin masking information from the mantid_workspace_i/workspace group.
 * @param wksp_cls :: The data group
 * @param local_workspace :: The workspace to read into
 */
void
LoadNexusProcessed::readBinMasking(NXData &wksp_cls,
                                   API::MatrixWorkspace_sptr local_workspace) {
  if (wksp_cls.getDataSetInfo("masked_spectra").stat == NX_ERROR) {
    return;
  }
  NXInt spec = wksp_cls.openNXInt("masked_spectra");
  spec.load();
  NXInt bins = wksp_cls.openNXInt("masked_bins");
  bins.load();
  NXDouble weights = wksp_cls.openNXDouble("mask_weights");
  weights.load();
  const int n = spec.dim0();
  const int n1 = n - 1;
  for (int i = 0; i < n; ++i) {
    int si = spec(i, 0);
    int j0 = spec(i, 1);
    int j1 = i < n1 ? spec(i + 1, 1) : bins.dim0();
    for (int j = j0; j < j1; ++j) {
      local_workspace->flagMasked(si, bins[j], weights[j]);
    }
  }
}

/**
 * Perform a call to nxgetslab, via the NexusClasses wrapped methods for a given
 * blocksize. This assumes that the
 * xbins have alread been cached
 * @param data :: The NXDataSet object of y values
 * @param errors :: The NXDataSet object of error values
 * @param farea :: The NXDataSet object of fraction area values
 * @param hasFArea :: Flag to signal a RebinnedOutput workspace is in use
 * @param blocksize :: The blocksize to use
 * @param nchannels :: The number of channels for the block
 * @param hist :: The workspace index to start reading into
 * @param local_workspace :: A pointer to the workspace
 */
void LoadNexusProcessed::loadBlock(NXDataSetTyped<double> &data,
                                   NXDataSetTyped<double> &errors,
                                   NXDataSetTyped<double> &farea, bool hasFArea,
                                   int64_t blocksize, int64_t nchannels,
                                   int64_t &hist,
                                   API::MatrixWorkspace_sptr local_workspace) {
  data.load(static_cast<int>(blocksize), static_cast<int>(hist));
  errors.load(static_cast<int>(blocksize), static_cast<int>(hist));
  double *data_start = data();
  double *data_end = data_start + nchannels;
  double *err_start = errors();
  double *err_end = err_start + nchannels;
  double *farea_start = NULL;
  double *farea_end = NULL;
  RebinnedOutput_sptr rb_workspace;
  if (hasFArea) {
    farea.load(static_cast<int>(blocksize), static_cast<int>(hist));
    farea_start = farea();
    farea_end = farea_start + nchannels;
    rb_workspace = boost::dynamic_pointer_cast<RebinnedOutput>(local_workspace);
  }
  int64_t final(hist + blocksize);
  while (hist < final) {
    MantidVec &Y = local_workspace->dataY(hist);
    Y.assign(data_start, data_end);
    data_start += nchannels;
    data_end += nchannels;
    MantidVec &E = local_workspace->dataE(hist);
    E.assign(err_start, err_end);
    err_start += nchannels;
    err_end += nchannels;
    if (hasFArea) {
      MantidVec &F = rb_workspace->dataF(hist);
      F.assign(farea_start, farea_end);
      farea_start += nchannels;
      farea_end += nchannels;
    }
    local_workspace->setX(hist, m_xbins);
    ++hist;
  }
}

/**
 * Perform a call to nxgetslab, via the NexusClasses wrapped methods for a given
 * blocksize. This assumes that the
 * xbins have alread been cached
 * @param data :: The NXDataSet object of y values
 * @param errors :: The NXDataSet object of error values
 * @param farea :: The NXDataSet object of fraction area values
 * @param hasFArea :: Flag to signal a RebinnedOutput workspace is in use
 * @param blocksize :: The blocksize to use
 * @param nchannels :: The number of channels for the block
 * @param hist :: The workspace index to start reading into
 * @param wsIndex :: The workspace index to save data into
 * @param local_workspace :: A pointer to the workspace
 */

void LoadNexusProcessed::loadBlock(NXDataSetTyped<double> &data,
                                   NXDataSetTyped<double> &errors,
                                   NXDataSetTyped<double> &farea, bool hasFArea,
                                   int64_t blocksize, int64_t nchannels,
                                   int64_t &hist, int64_t &wsIndex,
                                   API::MatrixWorkspace_sptr local_workspace) {
  data.load(static_cast<int>(blocksize), static_cast<int>(hist));
  errors.load(static_cast<int>(blocksize), static_cast<int>(hist));
  double *data_start = data();
  double *data_end = data_start + nchannels;
  double *err_start = errors();
  double *err_end = err_start + nchannels;
  double *farea_start = NULL;
  double *farea_end = NULL;
  RebinnedOutput_sptr rb_workspace;
  if (hasFArea) {
    farea.load(static_cast<int>(blocksize), static_cast<int>(hist));
    farea_start = farea();
    farea_end = farea_start + nchannels;
    rb_workspace = boost::dynamic_pointer_cast<RebinnedOutput>(local_workspace);
  }
  int64_t final(hist + blocksize);
  while (hist < final) {
    MantidVec &Y = local_workspace->dataY(wsIndex);
    Y.assign(data_start, data_end);
    data_start += nchannels;
    data_end += nchannels;
    MantidVec &E = local_workspace->dataE(wsIndex);
    E.assign(err_start, err_end);
    err_start += nchannels;
    err_end += nchannels;
    if (hasFArea) {
      MantidVec &F = rb_workspace->dataF(wsIndex);
      F.assign(farea_start, farea_end);
      farea_start += nchannels;
      farea_end += nchannels;
    }
    local_workspace->setX(wsIndex, m_xbins);
    ++hist;
    ++wsIndex;
  }
}

/**
 * Perform a call to nxgetslab, via the NexusClasses wrapped methods for a given
 * blocksize. The xbins are read along with
 * each call to the data/error loading
 * @param data :: The NXDataSet object of y values
 * @param errors :: The NXDataSet object of error values
 * @param farea :: The NXDataSet object of fraction area values
 * @param hasFArea :: Flag to signal a RebinnedOutput workspace is in use
 * @param xbins :: The xbin NXDataSet
 * @param blocksize :: The blocksize to use
 * @param nchannels :: The number of channels for the block
 * @param hist :: The workspace index to start reading into
 * @param wsIndex :: The workspace index to save data into
 * @param local_workspace :: A pointer to the workspace
 */
void LoadNexusProcessed::loadBlock(NXDataSetTyped<double> &data,
                                   NXDataSetTyped<double> &errors,
                                   NXDataSetTyped<double> &farea, bool hasFArea,
                                   NXDouble &xbins, int64_t blocksize,
                                   int64_t nchannels, int64_t &hist,
                                   int64_t &wsIndex,
                                   API::MatrixWorkspace_sptr local_workspace) {
  data.load(static_cast<int>(blocksize), static_cast<int>(hist));
  double *data_start = data();
  double *data_end = data_start + nchannels;
  errors.load(static_cast<int>(blocksize), static_cast<int>(hist));
  double *err_start = errors();
  double *err_end = err_start + nchannels;
  double *farea_start = NULL;
  double *farea_end = NULL;
  RebinnedOutput_sptr rb_workspace;
  if (hasFArea) {
    farea.load(static_cast<int>(blocksize), static_cast<int>(hist));
    farea_start = farea();
    farea_end = farea_start + nchannels;
    rb_workspace = boost::dynamic_pointer_cast<RebinnedOutput>(local_workspace);
  }
  xbins.load(static_cast<int>(blocksize), static_cast<int>(hist));
  const int64_t nxbins(nchannels + 1);
  double *xbin_start = xbins();
  double *xbin_end = xbin_start + nxbins;
  int64_t final(hist + blocksize);
  while (hist < final) {
    MantidVec &Y = local_workspace->dataY(wsIndex);
    Y.assign(data_start, data_end);
    data_start += nchannels;
    data_end += nchannels;
    MantidVec &E = local_workspace->dataE(wsIndex);
    E.assign(err_start, err_end);
    err_start += nchannels;
    err_end += nchannels;
    if (hasFArea) {
      MantidVec &F = rb_workspace->dataF(wsIndex);
      F.assign(farea_start, farea_end);
      farea_start += nchannels;
      farea_end += nchannels;
    }
    MantidVec &X = local_workspace->dataX(wsIndex);
    X.assign(xbin_start, xbin_end);
    xbin_start += nxbins;
    xbin_end += nxbins;
    ++hist;
    ++wsIndex;
  }
}

/**
 *Validates the optional 'spectra to read' properties, if they have been set
 * @param numberofspectra :: number of spectrum
 */
void
LoadNexusProcessed::checkOptionalProperties(const std::size_t numberofspectra) {
  // read in the settings passed to the algorithm
  m_spec_list = getProperty("SpectrumList");
  m_spec_max = getProperty("SpectrumMax");
  m_spec_min = getProperty("SpectrumMin");
  // Are we using a list of spectra or all the spectra in a range?
  m_list = !m_spec_list.empty();
  m_interval = (m_spec_max != Mantid::EMPTY_INT()) || (m_spec_min != 1);
  if (m_spec_max == Mantid::EMPTY_INT())
    m_spec_max = 1;

  // Check validity of spectra list property, if set
  if (m_list) {
    m_list = true;
    const int64_t minlist =
        *min_element(m_spec_list.begin(), m_spec_list.end());
    const int64_t maxlist =
        *max_element(m_spec_list.begin(), m_spec_list.end());
    if (maxlist > static_cast<int>(numberofspectra) || minlist == 0) {
      g_log.error("Invalid list of spectra");
      throw std::invalid_argument("Inconsistent properties defined");
    }
  }

  // Check validity of spectra range, if set
  if (m_interval) {
    m_interval = true;
    m_spec_min = getProperty("SpectrumMin");
    if (m_spec_min != 1 && m_spec_max == 1) {
      m_spec_max = numberofspectra;
    }
    if (m_spec_max < m_spec_min ||
        m_spec_max > static_cast<int>(numberofspectra)) {
      g_log.error("Invalid Spectrum min/max properties");
      throw std::invalid_argument("Inconsistent properties defined");
    }
  }
}

/**
 * Calculate the size of a workspace
 *
 * @param numberofspectra :: count of spectra found in the file being loaded
 *
 * @param gen_filtered_list :: process SpectrumList and SpectrumMin/Max
 *                             and save resulting explicit list of
 *                             spectra indices into a vector data
 *                             member, presently used only when loading
 *                             into event_workspace
 *
 * @return the size of a workspace
 */
size_t
LoadNexusProcessed::calculateWorkspaceSize(const std::size_t numberofspectra,
                                           bool gen_filtered_list) {
  // Calculate the size of a workspace, given its number of spectra to read
  int64_t total_specs;
  if (m_interval || m_list) {
    if (m_interval) {
      if (m_spec_min != 1 && m_spec_max == 1) {
        m_spec_max = numberofspectra;
      }
      total_specs = static_cast<int>(m_spec_max - m_spec_min + 1);
      m_spec_max += 1;

      if (gen_filtered_list) {
        m_filtered_spec_idxs.resize(total_specs);
        size_t j = 0;
        for(int64_t si = m_spec_min; si < m_spec_max; si++, j++)
          m_filtered_spec_idxs[j] = si;
      }
    } else {
      total_specs = 0;
    }

    if (m_list) {
      if (m_interval) {
        for (std::vector<int64_t>::iterator it = m_spec_list.begin();
             it != m_spec_list.end();)
          if (*it >= m_spec_min && *it < m_spec_max) {
            it = m_spec_list.erase(it);
          } else
            ++it;
      }
      if (m_spec_list.size() == 0)
        m_list = false;
      total_specs += static_cast<int>(m_spec_list.size());

      if (gen_filtered_list) {
        // range list + spare indices from list
        // example: min: 2, max: 8, list: 3,4,5,10,12;
        //          result: 2,3,...,7,8,10,12
        m_filtered_spec_idxs.insert(m_filtered_spec_idxs.end(),
                                  m_spec_list.begin(),
                                  m_spec_list.end());
      }
    }
  } else {
    total_specs = static_cast<int>(numberofspectra);
    m_spec_min = 1;
    m_spec_max = static_cast<int>(numberofspectra) + 1;

    if (gen_filtered_list) {
      m_filtered_spec_idxs.resize(total_specs, 0);
      for(int64_t j = 0; j < total_specs; j++)
        m_filtered_spec_idxs[j] = m_spec_min+j;
    }
  }
  return total_specs;
}

} // namespace DataHandling
} // namespace Mantid
