// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ISISRunLogs.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidDataHandling/SaveNexusProcessedHelper.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeakNoShapeFactory.h"
#include "MantidDataObjects/PeakShapeEllipsoidFactory.h"
#include "MantidDataObjects/PeakShapeSphericalFactory.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidNexusCpp/NeXusException.hpp"

#include <boost/algorithm/string/trim.hpp>
#include <boost/regex.hpp>

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Mantid::DataHandling {

// Register the algorithm into the algorithm factory
DECLARE_NEXUS_HDF5_FILELOADER_ALGORITHM(LoadNexusProcessed)

using namespace Mantid::NeXus;
using namespace DataObjects;
using namespace Kernel;
using namespace API;
using Geometry::Instrument_const_sptr;
using Mantid::Types::Event::TofEvent;
using Types::Core::DateAndTime;

namespace {

// Helper typedef
using IntArray = std::vector<int>;

// Struct to contain spectrum information.
struct SpectraInfo {
  // Number of spectra
  int nSpectra{0};
  // Do we have any spectra
  bool hasSpectra{false};
  // Contains spectrum numbers for each workspace index
  IntArray spectraNumbers;
  // Index of the detector in the workspace.
  IntArray detectorIndex;
  // Number of detectors associated with each spectra
  IntArray detectorCount;
  // Detector list contains a list of all of the detector numbers
  IntArray detectorList;
};

// Helper typdef.
using SpectraInfo_optional = std::optional<SpectraInfo>;

/**
 * Extract ALL the detector, spectrum number and workspace index mapping
 * information.
 * @param mtd_entry
 * @param logger
 * @return
 */
SpectraInfo extractMappingInfo(const NXEntry &mtd_entry, Logger &logger) {
  SpectraInfo spectraInfo;
  // Instrument information

  if (!mtd_entry.containsGroup("instrument")) {
    logger.information() << "No NXinstrument group called `instrument` under "
                            "NXEntry. The workspace will not "
                            "contain any detector information.\n";
    return spectraInfo;
  }
  NXInstrument inst = mtd_entry.openNXInstrument("instrument");
  if (!inst.containsGroup("detector")) {
    logger.information() << "Detector block not found. The workspace will not "
                            "contain any detector information.\n";
    return spectraInfo;
  }

  // Populate the spectra-detector map
  NXDetector detgroup = inst.openNXDetector("detector");

  // Spectra block - Contains spectrum numbers for each workspace index
  // This might not exist so wrap and check. If it doesn't exist create a
  // default mapping
  try {
    NXInt spectra_block = detgroup.openNXInt("spectra");
    spectra_block.load();
    spectraInfo.spectraNumbers = spectra_block.vecBuffer();
    spectraInfo.nSpectra = static_cast<int>(spectra_block.dim0());
    spectraInfo.hasSpectra = true;
  } catch (std::runtime_error &) {
    spectraInfo.hasSpectra = false;
  }

  // Read necessary arrays from the file
  // Detector list contains a list of all of the detector numbers. If it not
  // present then we can't update the spectra
  // map
  try {
    NXInt detlist_group = detgroup.openNXInt("detector_list");
    detlist_group.load();
    spectraInfo.detectorList = detlist_group.vecBuffer();
  } catch (std::runtime_error &) {
    logger.information() << "detector_list block not found. The workspace will "
                            "not contain any detector information.\n";
    return spectraInfo;
  }

  // Detector count contains the number of detectors associated with each
  // spectra
  NXInt det_count = detgroup.openNXInt("detector_count");
  det_count.load();
  spectraInfo.detectorCount = det_count.vecBuffer();
  // Detector index - contains the index of the detector in the workspace
  NXInt det_index = detgroup.openNXInt("detector_index");
  det_index.load();
  spectraInfo.nSpectra = static_cast<int>(det_index.dim0());
  spectraInfo.detectorIndex = det_index.vecBuffer();

  return spectraInfo;
}

/**
 * Is this file from a well-formed multiperiod group workspace.
 * @param nWorkspaceEntries : Number of entries in the group workspace
 * @param sampleWS : Sample workspace to inspect the logs of
 * @param log : Information logger object
 * @return True only if multiperiod.
 */
bool isMultiPeriodFile(int nWorkspaceEntries, const Workspace_sptr &sampleWS, Logger &log) {
  bool isMultiPeriod = false;
  if (ExperimentInfo_sptr expInfo = std::dynamic_pointer_cast<ExperimentInfo>(sampleWS)) {
    const std::string nPeriodsLogEntryName = "nperiods";
    const Run &run = expInfo->run();
    if (run.hasProperty(nPeriodsLogEntryName)) {
      const auto nPeriods = run.getPropertyValueAsType<int>(nPeriodsLogEntryName);
      if (nPeriods == nWorkspaceEntries) {
        isMultiPeriod = true;
        log.information("Loading as MultiPeriod group workspace.");
      }
    }
  }
  return isMultiPeriod;
}

} // namespace

/// Default constructor
LoadNexusProcessed::LoadNexusProcessed()
    : m_shared_bins(false), m_xbins(0), m_axis1vals(), m_list(false), m_interval(false), m_spec_min(0),
      m_spec_max(Mantid::EMPTY_INT()), m_spec_list(), m_filtered_spec_idxs(), m_nexusFile() {}

/// Destructor defined here so that NeXus::File can be forward declared
/// in header
LoadNexusProcessed::~LoadNexusProcessed() = default;

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadNexusProcessed::confidence(Kernel::NexusHDF5Descriptor &descriptor) const {
  if (descriptor.isEntry("/mantid_workspace_1"))
    return 80;
  else
    return 0;
}

void LoadNexusProcessed::readSpectraToDetectorMapping(NXEntry &mtd_entry, Mantid::API::MatrixWorkspace &ws) {
  readInstrumentGroup(mtd_entry, ws);
}

/** Initialisation method.
 *
 */
void LoadNexusProcessed::init() {
  // Declare required input parameters for algorithm
  const std::vector<std::string> exts{".nxs", ".nx5", ".xml"};
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, exts),
                  "The name of the Nexus file to read, as a full or relative path.");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace to be created as the output of "
                  "the algorithm.  A workspace of this name will be created "
                  "and stored in the Analysis Data Service. For multiperiod "
                  "files, one workspace may be generated for each period. "
                  "Currently only one workspace can be saved at a time so "
                  "multiperiod Mantid files are not generated.");

  // optional
  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);

  // Use a static cast as MSVC sometimes gets confused and casts as int64
  declareProperty("SpectrumMin", static_cast<int>(1), mustBePositive, "Number of first spectrum to read.");
  declareProperty("SpectrumMax", static_cast<int>(Mantid::EMPTY_INT()), mustBePositive,
                  "Number of last spectrum to read.");
  declareProperty(std::make_unique<ArrayProperty<int>>("SpectrumList"), "List of spectrum numbers to read.");
  declareProperty("EntryNumber", static_cast<int>(0), mustBePositive,
                  "0 indicates that every entry is loaded, into a separate "
                  "workspace within a group. "
                  "A positive number identifies one entry to be loaded, into "
                  "one workspace");
  declareProperty("LoadHistory", true, "If true, the workspace history will be loaded");
  declareProperty(std::make_unique<PropertyWithValue<bool>>("FastMultiPeriod", true, Direction::Input),
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
Workspace_sptr LoadNexusProcessed::doAccelleratedMultiPeriodLoading(NXRoot &root, const std::string &entryName,
                                                                    MatrixWorkspace_sptr &tempMatrixWorkspace,
                                                                    const size_t nWorkspaceEntries, const size_t p) {

  MatrixWorkspace_sptr periodWorkspace = WorkspaceFactory::Instance().create(tempMatrixWorkspace);

  const size_t nHistograms = periodWorkspace->getNumberHistograms();
  for (size_t i = 0; i < nHistograms; ++i) {
    periodWorkspace->setSharedX(i, tempMatrixWorkspace->sharedX(i));
  }

  // We avoid using `openEntry` or similar here because they're just wrappers
  // around `open`. `open` is slow for large multiperiod datasets, because it
  // does a search upon the entire HDF5 tree. `openLocal` is *much* quicker, as
  // it only searches the current group. It does, however, require that the
  // parent group is currently open.
  // Words of Warning: While the openLocal construct is an optimization,
  // it is very dangerous. Forgetting to close an entry of an NXEntry in a
  // completely unrelated part of the code can result in us opening the
  // wrong NXEntry here!
  NXEntry mtdEntry(root, entryName);
  mtdEntry.openLocal();

  NXData wsEntry(mtdEntry, "workspace");
  if (!wsEntry.openLocal()) {
    std::stringstream buffer;
    buffer << "Group entry " << p - 1
           << " is not a workspace 2D. Retry with "
              "FastMultiPeriod option set off.\n";
    throw std::runtime_error(buffer.str());
  }

  if (wsEntry.isValid("frac_area")) {
    std::stringstream buffer;
    buffer << "Group entry " << p - 1
           << " has fractional area present. Try "
              "reloading with FastMultiPeriod set "
              "off.\n";
    throw std::runtime_error(buffer.str());
  }

  NXDataSetTyped<double> data(wsEntry, "values");
  data.openLocal();
  NXDataSetTyped<double> errors(wsEntry, "errors");
  errors.openLocal();

  const int nChannels = static_cast<int>(data.dim1());

  int blockSize = 8; // Read block size. Set to 8 for efficiency. i.e. read
                     // 8 histograms at a time.
  int nFullBlocks =
      static_cast<int>(nHistograms) / blockSize; // Truncated number of full blocks to read. Remainder removed
  const int readOptimumStop = (nFullBlocks * blockSize);
  const int readStop = m_spec_max - 1;
  const int finalBlockSize = readStop - readOptimumStop;

  int wsIndex = 0;
  int histIndex = m_spec_min - 1;

  for (; histIndex < readStop;) {
    if (histIndex >= readOptimumStop) {
      blockSize = finalBlockSize;
    }

    data.load(blockSize, histIndex);
    errors.load(blockSize, histIndex);

    double *dataStart = data();
    double *dataEnd = dataStart + nChannels;

    double *errorStart = errors();
    double *errorEnd = errorStart + nChannels;

    int final(histIndex + blockSize);
    while (histIndex < final) {
      auto &Y = periodWorkspace->mutableY(wsIndex);
      Y.assign(dataStart, dataEnd);
      dataStart += nChannels;
      dataEnd += nChannels;
      auto &E = periodWorkspace->mutableE(wsIndex);
      E.assign(errorStart, errorEnd);
      errorStart += nChannels;
      errorEnd += nChannels;

      ++wsIndex;
      ++histIndex;
    }
  }

  // We always start one layer too deep
  // go from /workspace_{n}/{something} -> /workspace_{n}
  m_nexusFile->closeGroup();

  // Now move to the correct period group
  // /workspace_{n} -> /workspace_{n+1}
  m_nexusFile->closeGroup();
  m_nexusFile->openGroup(entryName, "NXentry");

  try {
    // This loads logs, sample, and instrument.
    periodWorkspace->loadSampleAndLogInfoNexus(m_nexusFile.get());
  } catch (std::exception &e) {
    g_log.information("Error loading Instrument section of nxs file");
    g_log.information(e.what());
  }

  // We make sure to close the current entries. Failing to do this can cause
  // strange off-by-one errors when loading the spectra.
  wsEntry.close();
  mtdEntry.close();

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
void LoadNexusProcessed::execLoader() {

  API::Workspace_sptr tempWS;
  size_t nWorkspaceEntries = 0;

  // Check for an entry number property
  int entryNumber = getProperty("EntryNumber");
  Property const *const entryNumberProperty = this->getProperty("EntryNumber");
  bool bDefaultEntryNumber = entryNumberProperty->isDefault();

  // Start scoped block
  {
    progress(0, "Opening file...");

    // Throws an appropriate exception if there is a problem with file access
    const std::string filename = getPropertyValue("Filename");
    NXRoot root(filename);

    // "Open" the same file but with the C++ interface
    m_nexusFile = std::make_unique<::NeXus::File>(root.m_fileID);

    // Find out how many NXentry groups there are in the file.
    nWorkspaceEntries = std::count_if(root.groups().cbegin(), root.groups().cend(),
                                      [](const auto &g) { return g.nxclass == "NXentry"; });

    if (!bDefaultEntryNumber && static_cast<size_t>(entryNumber) > nWorkspaceEntries) {
      g_log.error() << "Invalid entry number: " << entryNumber
                    << " specified. File only contains: " << nWorkspaceEntries << " entries.\n";
      throw std::invalid_argument("Invalid entry number specified.");
    }

    const std::string basename = "mantid_workspace_";

    std::ostringstream os;
    if (bDefaultEntryNumber) {
      // Set the entry number to 1 if not provided.
      entryNumber = 1;
    }
    os << basename << entryNumber;
    const std::string targetEntryName = os.str();

    // Take the first real workspace obtainable. We need it even if loading
    // groups.
    tempWS = loadEntry(root, targetEntryName, 0, 1);

    if (nWorkspaceEntries == 1 || !bDefaultEntryNumber) {
      // We have what we need.
      applyLogFiltering(tempWS);
      setProperty("OutputWorkspace", tempWS);
    } else {
      // We already know that this is a group workspace. Is it a true
      // multiperiod workspace.
      const bool bFastMultiPeriod = this->getProperty("FastMultiPeriod");
      const bool bIsMultiPeriod = isMultiPeriodFile(static_cast<int>(nWorkspaceEntries), tempWS, g_log);
      const Property *specListProp = this->getProperty("SpectrumList");
      m_list = !specListProp->isDefault();

      // Load all first level entries
      auto wksp_group = std::make_shared<WorkspaceGroup>();
      // This forms the name of the group
      std::string base_name = getPropertyValue("OutputWorkspace");
      // First member of group should be the group itself, for some reason!

      // load names of each of the workspaces. Note that if we have duplicate
      // names then we don't select them
      auto names = extractWorkspaceNames(root, nWorkspaceEntries);

      // remove existing workspace and replace with the one being loaded
      bool wsExists = AnalysisDataService::Instance().doesExist(base_name);
      if (wsExists) {
        Algorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged("DeleteWorkspace");
        alg->initialize();
        alg->setChild(true);
        alg->setProperty("Workspace", base_name);
        alg->execute();
      }

      base_name += "_";
      const std::string prop_name = "OutputWorkspace_";

      MatrixWorkspace_sptr tempMatrixWorkspace = std::dynamic_pointer_cast<Workspace2D>(tempWS);
      bool bAccelleratedMultiPeriodLoading = false;
      if (tempMatrixWorkspace) {
        // We only accelerate for simple scenarios for now. Spectrum lists are
        // too complicated to bother with.
        bAccelleratedMultiPeriodLoading = bIsMultiPeriod && bFastMultiPeriod && !m_list;
        // Strip out any loaded logs. That way we don't pay for copying that
        // information around.
        tempMatrixWorkspace->mutableRun().clearLogs();
      }

      if (bAccelleratedMultiPeriodLoading) {
        g_log.information("Accelerated multiperiod loading");
      } else {
        g_log.information("Individual group loading");
      }

      for (size_t p = 1; p <= nWorkspaceEntries; ++p) {
        const auto indexStr = std::to_string(p);

        // decide what the workspace should be called
        std::string wsName = buildWorkspaceName(names[p], base_name, p);

        Workspace_sptr local_workspace;

        /*
        For multiperiod workspaces we can accelerate the loading by making
        resonable assumptions about the differences between the workspaces
        Only Y, E and log data entries should vary. Therefore we can clone our
        temp workspace, and overwrite those things we are interested in.
        */
        if (bAccelleratedMultiPeriodLoading) {
          local_workspace =
              doAccelleratedMultiPeriodLoading(root, basename + indexStr, tempMatrixWorkspace, nWorkspaceEntries, p);
        } else // Fall-back for generic loading
        {
          const auto nWorkspaceEntries_d = static_cast<double>(nWorkspaceEntries);
          local_workspace = loadEntry(root, basename + indexStr, static_cast<double>(p - 1) / nWorkspaceEntries_d,
                                      1. / nWorkspaceEntries_d);
        }

        applyLogFiltering(local_workspace);
        declareProperty(
            std::make_unique<WorkspaceProperty<API::Workspace>>(prop_name + indexStr, wsName, Direction::Output));
        wksp_group->addWorkspace(local_workspace);
        setProperty(prop_name + indexStr, local_workspace);
      }

      // The group is the root property value
      setProperty("OutputWorkspace", std::static_pointer_cast<Workspace>(wksp_group));
    }

    root.close();
  }

  // All file resources should be scoped to here. All previous file handles
  //   must be cleared to release locks.

  // NexusGeometry uses direct HDF5 access, and not the `NexusFileIO` methods.
  // For this reason, a separate section is required to load the instrument[s] into the output workspace[s].

  if (nWorkspaceEntries == 1 || !bDefaultEntryNumber)
    loadNexusGeometry(*getValue<API::Workspace_sptr>("OutputWorkspace"), static_cast<size_t>(entryNumber), g_log,
                      std::string(getProperty("Filename")));
  else {
    for (size_t nEntry = 1; nEntry <= static_cast<size_t>(nWorkspaceEntries); ++nEntry) {
      std::ostringstream wsPropertyName;
      wsPropertyName << "OutputWorkspace_" << nEntry;
      loadNexusGeometry(*getValue<API::Workspace_sptr>(wsPropertyName.str()), nEntry, g_log,
                        std::string(getProperty("Filename")));
    }
  }

  m_axis1vals.clear();
}

/**
 * Decides what to call a child of a group workspace.
 *
 * This function builds the workspace name based on either a workspace name
 * which was stored in the file or the base name.
 *
 * @param name :: The name loaded from the file (possibly the empty string if
 *none was loaded)
 * @param baseName :: The name group workspace
 * @param wsIndex :: The current index of this workspace
 *
 * @return The name of the workspace
 */
std::string LoadNexusProcessed::buildWorkspaceName(const std::string &name, const std::string &baseName,
                                                   size_t wsIndex) {
  std::string wsName;
  std::string index = std::to_string(wsIndex);

  if (!name.empty()) {
    wsName = name;
  } else {
    // we have a common stem so rename accordingly
    boost::smatch results;
    const boost::regex exp(".*_(\\d+$)");
    // if we have a common name stem then name is <OutputWorkspaceName>_n
    if (boost::regex_search(name, results, exp)) {
      wsName = baseName + std::string(results[1].first, results[1].second);
    } else {
      // if the name property wasn't defined just use <OutputWorkspaceName>_n
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
    std::string wsIndex; // dont use an index if there is no other
                         // workspace
    if (i > 0) {
      wsIndex = "_" + std::to_string(i);
    }

    bool wsExists = AnalysisDataService::Instance().doesExist(wsName + wsIndex);
    if (!wsExists) {
      wsName += wsIndex;
      noClash = true;
    }
  }
}

/**
 * Extract the workspace names from the file (if any are stored)
 *
 * @param root :: the root for the NeXus document
 * @param nWorkspaceEntries :: the number of workspace entries
 */
std::vector<std::string> LoadNexusProcessed::extractWorkspaceNames(NXRoot &root, size_t nWorkspaceEntries) {
  std::vector<std::string> names(nWorkspaceEntries + 1);
  for (size_t p = 1; p <= nWorkspaceEntries; ++p) {
    auto period = std::to_string(p);
    names[p] = loadWorkspaceName(root, "mantid_workspace_" + period);
  }

  // Check that there are no duplicates in the workspace name
  // This can cause severe problems
  auto it = std::unique(names.begin(), names.end());
  if (it != names.end()) {
    auto size = names.size();
    names.clear();
    names.resize(size);
  }
  return names;
}

/**
 * Load the workspace name, if the attribute exists
 *
 * @param root :: Root of NeXus file
 * @param entry_name :: Entry in NeXus file to look at
 * @return The workspace name. If none found an empty string is returned.
 */
std::string LoadNexusProcessed::loadWorkspaceName(NXRoot &root, const std::string &entry_name) {
  NXEntry mtd_entry = root.openEntry(entry_name);
  std::string workspaceName = std::string();
  try {
    workspaceName = mtd_entry.getString("workspace_name");
  } catch (std::runtime_error &) {
  }
  mtd_entry.close();
  return workspaceName;
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
API::MatrixWorkspace_sptr LoadNexusProcessed::loadEventEntry(NXData &wksp_cls, NXDouble &xbins,
                                                             const double &progressStart, const double &progressRange) {
  NXDataSetTyped<int64_t> indices_data = wksp_cls.openNXDataSet<int64_t>("indices");
  indices_data.load();
  size_t numspec = indices_data.dim0() - 1;

  // process optional spectrum parameters, if set
  checkOptionalProperties(numspec);
  // Actual number of spectra in output workspace (if only a user-specified
  // range and/or list was going to be loaded)
  numspec = calculateWorkspaceSize(numspec, true);

  auto num_xbins = xbins.dim0();
  if (xbins.rank() == 2) {
    num_xbins = xbins.dim1();
  }
  if (num_xbins < 2)
    num_xbins = 2;
  EventWorkspace_sptr ws = std::dynamic_pointer_cast<EventWorkspace>(
      WorkspaceFactory::Instance().create("EventWorkspace", numspec, num_xbins, num_xbins - 1));

  // Set the YUnit label
  ws->setYUnit(indices_data.attributes("units"));
  std::string unitLabel = indices_data.attributes("unit_label");
  if (unitLabel.empty())
    unitLabel = indices_data.attributes("units");
  ws->setYUnitLabel(unitLabel);

  // Handle optional fields.
  // TODO: Handle inconsistent sizes
  std::vector<int64_t> pulsetimes;
  if (wksp_cls.isValid("pulsetime")) {
    NXDataSetTyped<int64_t> pulsetime = wksp_cls.openNXDataSet<int64_t>("pulsetime");
    pulsetime.load();
    pulsetimes = pulsetime.vecBuffer();
  }

  std::vector<double> tofs;
  if (wksp_cls.isValid("tof")) {
    NXDouble tof = wksp_cls.openNXDouble("tof");
    tof.load();
    tofs = tof.vecBuffer();
  }

  std::vector<float> error_squareds;
  if (wksp_cls.isValid("error_squared")) {
    NXFloat error_squared = wksp_cls.openNXFloat("error_squared");
    error_squared.load();
    error_squareds = error_squared.vecBuffer();
  }

  std::vector<float> weights;
  if (wksp_cls.isValid("weight")) {
    NXFloat weight = wksp_cls.openNXFloat("weight");
    weight.load();
    weights = weight.vecBuffer();
  }

  // What type of event lists?
  EventType type = TOF;
  if (!tofs.empty() && !pulsetimes.empty() && !weights.empty() && !error_squareds.empty())
    type = WEIGHTED;
  else if ((!tofs.empty() && !weights.empty() && !error_squareds.empty()))
    type = WEIGHTED_NOTIME;
  else if (!pulsetimes.empty() && !tofs.empty())
    type = TOF;
  else
    throw std::runtime_error("Could not figure out the type of event list!");

  // indices of events
  std::vector<int64_t> indices = indices_data.vecBuffer();
  // Create all the event lists
  auto max = static_cast<int64_t>(m_filtered_spec_idxs.size());
  Progress progress(this, progressStart, progressStart + progressRange, max);
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t j = 0; j < max; ++j) {
    PARALLEL_START_INTERRUPT_REGION
    size_t wi = m_filtered_spec_idxs[j] - 1;
    int64_t index_start = indices[wi];
    int64_t index_end = indices[wi + 1];
    if (index_end >= index_start) {
      EventList &el = ws->getSpectrum(j);
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
          el.addEventQuickly(WeightedEvent(tofs[i], DateAndTime(pulsetimes[i]), weights[i], error_squareds[i]));
          break;
        case WEIGHTED_NOTIME:
          el.addEventQuickly(WeightedEventNoTime(tofs[i], weights[i], error_squareds[i]));
          break;
        }

      // Set the X axis
      if (this->m_shared_bins)
        el.setHistogram(this->m_xbins);
      else {
        MantidVec x(xbins.dim1());

        for (int i = 0; i < xbins.dim1(); i++)
          x[i] = xbins(static_cast<int>(wi), i);

        // for ragged workspace we need to remove all NaN value from end of vector
        const auto idx =
            std::distance(x.rbegin(), std::find_if_not(x.rbegin(), x.rend(), [](auto val) { return std::isnan(val); }));
        if (idx > 0)
          x.resize(x.size() - idx);
        // Workspace and el was just created, so we can just set a new histogram
        // We can move x as it is not longer used after this point
        el.setHistogram(HistogramData::BinEdges(std::move(x)));
      }
    }
    progress.report();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

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
void LoadNexusProcessed::loadNumericColumn(const Mantid::NeXus::NXData &tableData, const std::string &dataSetName,
                                           const API::ITableWorkspace_sptr &tableWs, const std::string &columnType) {
  NXDataSetTyped<NexusType> data = tableData.openNXDataSet<NexusType>(dataSetName);
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
API::Workspace_sptr LoadNexusProcessed::loadTableEntry(const NXEntry &entry) {
  API::ITableWorkspace_sptr workspace;
  workspace = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");

  NXData nx_tw = entry.openNXData("table_workspace");

  int columnNumber = 1;
  do {
    std::string dataSetName = "column_" + std::to_string(columnNumber);

    NXInfo info = nx_tw.getDataSetInfo(dataSetName);
    if (info.stat == NXstatus::NX_ERROR) {
      // Assume we done last column of table
      break;
    }

    if (info.rank == 1) {
      if (info.type == NXnumtype::FLOAT64) {
        loadNumericColumn<double, double>(nx_tw, dataSetName, workspace, "double");
      } else if (info.type == NXnumtype::INT32) {
        loadNumericColumn<int, int32_t>(nx_tw, dataSetName, workspace, "int");
      } else if (info.type == NXnumtype::UINT32) {
        loadNumericColumn<uint32_t, uint32_t>(nx_tw, dataSetName, workspace, "uint");
      } else if (info.type == NXnumtype::INT64) {
        loadNumericColumn<int64_t, int64_t>(nx_tw, dataSetName, workspace, "long64");
      } else if (info.type == NXnumtype::UINT64) {
        loadNumericColumn<size_t, uint64_t>(nx_tw, dataSetName, workspace, "size_t");
      } else if (info.type == NXnumtype::FLOAT32) {
        loadNumericColumn<float, float>(nx_tw, dataSetName, workspace, "float");
      } else if (info.type == NXnumtype::UINT8) {
        loadNumericColumn<bool, bool>(nx_tw, dataSetName, workspace, "bool");
      } else {
        throw std::logic_error("Column with Nexus data type " + std::to_string(static_cast<int>(info.type)) +
                               " cannot be loaded.");
      }
    } else if (info.rank == 2) {
      if (info.type == NXnumtype::CHAR) {
        NXChar data = nx_tw.openNXChar(dataSetName);
        std::string columnTitle = data.attributes("name");
        if (!columnTitle.empty()) {
          workspace->addColumn("str", columnTitle);
          nxdimsize_t nRows = info.dims[0];
          workspace->setRowCount(nRows);

          nxdimsize_t const maxStr = info.dims[1];
          data.load();
          for (int64_t iR = 0; iR < nRows; ++iR) {
            auto &cellContents = workspace->cell<std::string>(iR, columnNumber - 1);
            auto startPoint = data() + maxStr * iR;
            cellContents.assign(startPoint, startPoint + maxStr);
            boost::trim_right(cellContents);
          }
        }
      } else if (info.type == NXnumtype::INT32) {
        loadVectorColumn<int>(nx_tw, dataSetName, workspace, "vector_int");
      } else if (info.type == NXnumtype::FLOAT64) {
        auto data = nx_tw.openNXDouble(dataSetName);
        if (data.attributes("interpret_as") == "V3D") {
          loadV3DColumn(data, workspace);
        } else {
          loadVectorColumn<double>(nx_tw, dataSetName, workspace, "vector_double");
        }
      }
    }

    columnNumber++;

  } while (true);

  return std::static_pointer_cast<API::Workspace>(workspace);
}

/**
 * Loads a vector column to the TableWorkspace.
 * @param tableData   :: Table data to load from
 * @param dataSetName :: Name of the data set to use to get column data
 * @param tableWs     :: Workspace to add column to
 * @param columnType  :: Name of the column type to create
 */
template <typename Type>
void LoadNexusProcessed::loadVectorColumn(const NXData &tableData, const std::string &dataSetName,
                                          const ITableWorkspace_sptr &tableWs, const std::string &columnType) {
  NXDataSetTyped<Type> data = tableData.openNXDataSet<Type>(dataSetName);
  std::string columnTitle = data.attributes("name");
  if (!columnTitle.empty()) {
    tableWs->addColumn(columnType, columnTitle);

    NXInfo info = tableData.getDataSetInfo(dataSetName);
    const size_t rowCount = info.dims[0];
    const size_t blockSize = info.dims[1];

    // This might've been done already, but doing it twice should't do any harm
    tableWs->setRowCount(rowCount);

    data.load();

    for (size_t i = 0; i < rowCount; ++i) {
      auto &cell = tableWs->cell<std::vector<Type>>(i, tableWs->columnCount() - 1);

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
void LoadNexusProcessed::loadV3DColumn(Mantid::NeXus::NXDouble &data, const API::ITableWorkspace_sptr &tableWs) {
  std::string columnTitle = data.attributes("name");
  if (!columnTitle.empty()) {
    ColumnVector<V3D> col = tableWs->addColumn("V3D", columnTitle);

    const int64_t rowCount = data.dim0();

    // This might've been done already, but doing it twice shouldn't do any harm
    tableWs->setRowCount(rowCount);

    data.load();

    for (int64_t i = 0; i < rowCount; ++i) {
      auto &cell = col[i]; // cppcheck-suppress constVariableReference
      cell(data(i, 0), data(i, 1), data(i, 2));
    }
  }
}

/**
 * @brief Load LeanElasticPeakWorkspace
 *
 * @param entry
 * @return API::Workspace_sptr
 */
API::Workspace_sptr LoadNexusProcessed::loadLeanElasticPeaksEntry(const NXEntry &entry) {
  g_log.notice("Load as LeanElasticPeaks");

  // API::IPeaksWorkspace_sptr workspace;
  API::ITableWorkspace_sptr tWorkspace;
  // PeaksWorkspace_sptr workspace;
  tWorkspace = Mantid::API::WorkspaceFactory::Instance().createTable("LeanElasticPeaksWorkspace");

  IPeaksWorkspace_sptr peakWS = std::dynamic_pointer_cast<LeanElasticPeaksWorkspace>(tWorkspace);

  NXData nx_tw = entry.openNXData("peaks_workspace");

  int columnNumber = 1;
  int64_t numberPeaks = 0;
  std::vector<std::string> columnNames;
  do {
    std::string str = "column_" + std::to_string(columnNumber);

    NXInfo info = nx_tw.getDataSetInfo(str);
    if (info.stat == NXstatus::NX_ERROR) {
      // Assume we done last column of table
      break;
    }

    // store column names
    columnNames.emplace_back(str);

    // determine number of peaks
    // here we assume that a peaks_table has always one column of doubles

    if (info.type == NXnumtype::FLOAT64) {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
      std::string columnTitle = nxDouble.attributes("name");
      if (!columnTitle.empty() && numberPeaks == 0) {
        numberPeaks = nxDouble.dim0();
      }
    }

    columnNumber++;

  } while (true);

  // Hop to the right point /mantid_workspace_1
  try {
    m_nexusFile->openPath(entry.path()); // This is
  } catch (std::runtime_error &re) {
    throw std::runtime_error("Error while opening a path in a Peaks entry in a "
                             "Nexus processed file. "
                             "This path is wrong: " +
                             entry.path() + ". Lower level error description: " + re.what());
  }
  try {
    // Get information from all but data group
    std::string parameterStr;
    // This loads logs, sample, and instrument.
    peakWS->loadExperimentInfoNexus(getPropertyValue("Filename"), m_nexusFile.get(), parameterStr);
    // Populate the instrument parameters in this workspace
    peakWS->readParameterMap(parameterStr);
  } catch (std::exception &e) {
    g_log.information("Error loading Instrument section of nxs file");
    g_log.information(e.what());
  }

  // Coordinates - Older versions did not have the separate field but used a log
  // value
  const std::string peaksWSName = "peaks_workspace";
  try {
    m_nexusFile->openGroup(peaksWSName, "NXentry");
  } catch (std::runtime_error &re) {
    throw std::runtime_error("Error while opening a peaks workspace in a Nexus processed file. "
                             "Cannot open gropu " +
                             peaksWSName + ". Lower level error description: " + re.what());
  }
  try {
    uint32_t loadCoord(0);
    m_nexusFile->readData("coordinate_system", loadCoord);
    peakWS->setCoordinateSystem(static_cast<Kernel::SpecialCoordinateSystem>(loadCoord));
  } catch (::NeXus::Exception &) {
    // Check for a log value
    auto logs = peakWS->logs();
    if (logs->hasProperty("CoordinateSystem")) {
      auto *prop = dynamic_cast<PropertyWithValue<int> *>(logs->getProperty("CoordinateSystem"));
      if (prop) {
        int value((*prop)());
        peakWS->setCoordinateSystem(static_cast<Kernel::SpecialCoordinateSystem>(value));
      }
    }
  }

  std::string m_QConvention = "Inelastic";
  try {
    m_nexusFile->getAttr("QConvention", m_QConvention);
  } catch (std::exception &) {
  }

  // peaks_workspace
  m_nexusFile->closeGroup();

  // Change convention of loaded file to that in Preferen
  double qSign = 1.0;
  std::string convention = ConfigService::Instance().getString("Q.convention");
  if (convention != m_QConvention)
    qSign = -1.0;

  for (int r = 0; r < numberPeaks; r++) {
    // Create individual LeanElasticPeak
    const auto &goniometer = peakWS->run().getGoniometer();
    LeanElasticPeak peak;
    peak.setGoniometerMatrix(goniometer.getR());
    peak.setRunNumber(peakWS->getRunNumber());
    peakWS->addPeak(peak);
  }

  for (const auto &str : columnNames) {
    if (str == "column_1") {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = qSign * nxDouble[r];
        peakWS->getPeak(r).setH(val);
      }
    } else if (str == "column_2") {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = qSign * nxDouble[r];
        peakWS->getPeak(r).setK(val);
      }
    } else if (str == "column_3") {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = qSign * nxDouble[r];
        peakWS->getPeak(r).setL(val);
      }
    } else if (str == "column_4") {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setIntensity(val);
      }
    } else if (str == "column_5") {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setSigmaIntensity(val);
      }
    } else if (str == "column_6") {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setBinCount(val);
      }
    } else if (str == "column_7") {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setWavelength(val);
      }
    } else if (str == "column_10") {
      NXInt nxInt = nx_tw.openNXInt(str);
      nxInt.load();

      for (int r = 0; r < numberPeaks; r++) {
        int ival = nxInt[r];
        if (ival != -1)
          peakWS->getPeak(r).setRunNumber(ival);
      }
    } else if (str == "column_11") {
      NXInt nxInt = nx_tw.openNXInt(str);
      nxInt.load();

      for (int r = 0; r < numberPeaks; r++) {
        int ival = nxInt[r];
        peakWS->getPeak(r).setPeakNumber(ival);
      }
    } else if (str == "column_12") {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setAbsorptionWeightedPathLength(val);
      }
    } else if (str == "column_13") {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
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
    } else if (str == "column_14") {
      // Read shape information
      using namespace Mantid::DataObjects;

      PeakShapeFactory_sptr peakFactoryEllipsoid = std::make_shared<PeakShapeEllipsoidFactory>();
      PeakShapeFactory_sptr peakFactorySphere = std::make_shared<PeakShapeSphericalFactory>();
      PeakShapeFactory_sptr peakFactoryNone = std::make_shared<PeakNoShapeFactory>();

      peakFactoryEllipsoid->setSuccessor(peakFactorySphere);
      peakFactorySphere->setSuccessor(peakFactoryNone);

      NXInfo info = nx_tw.getDataSetInfo(str);
      NXChar data = nx_tw.openNXChar(str);

      nxdimsize_t const maxShapeJSONLength = info.dims[1];
      data.load();
      for (int i = 0; i < numberPeaks; ++i) {

        // iR = peak row number
        auto startPoint = data() + (maxShapeJSONLength * i);
        std::string shapeJSON(startPoint, startPoint + maxShapeJSONLength);
        boost::trim_right(shapeJSON);

        // Make the shape
        Mantid::Geometry::PeakShape *peakShape = peakFactoryEllipsoid->create(shapeJSON);

        // Set the shape
        peakWS->getPeak(i).setPeakShape(peakShape);
      }
    } else if (str == "column_15") {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
      nxDouble.load();
      V3D qlab;
      for (int r = 0; r < numberPeaks; ++r) {
        qlab = V3D(nxDouble[r * 3], nxDouble[r * 3 + 1], nxDouble[r * 3 + 2]);
        peakWS->getPeak(r).setQLabFrame(qlab, 0.0);
      }
    } else if (str == "column_16") {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
      nxDouble.load();
      V3D hkl;
      for (int r = 0; r < numberPeaks; ++r) {
        hkl = V3D(nxDouble[r * 3], nxDouble[r * 3 + 1], nxDouble[r * 3 + 2]);
        peakWS->getPeak(r).setIntHKL(hkl);
      }
    } else if (str == "column_17") {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
      nxDouble.load();
      V3D mnp;
      for (int r = 0; r < numberPeaks; ++r) {
        mnp = V3D(nxDouble[r * 3], nxDouble[r * 3 + 1], nxDouble[r * 3 + 2]);
        peakWS->getPeak(r).setIntMNP(mnp);
      }
    }

    // After all columns read set IntHKL if not set
    for (int r = 0; r < numberPeaks; r++) {
      V3D intHKL = peakWS->getPeak(r).getIntHKL();
      if (intHKL.norm2() == 0) {
        intHKL = V3D(peakWS->getPeak(r).getH(), peakWS->getPeak(r).getK(), peakWS->getPeak(r).getL());
        peakWS->getPeak(r).setIntHKL(intHKL);
      }
    }
  }

  return std::static_pointer_cast<API::Workspace>(peakWS);
}

//-------------------------------------------------------------------------------------------------
/**
 * Load peaks
 */
API::Workspace_sptr LoadNexusProcessed::loadPeaksEntry(const NXEntry &entry) {
  // API::IPeaksWorkspace_sptr workspace;
  API::ITableWorkspace_sptr tWorkspace;
  // PeaksWorkspace_sptr workspace;
  tWorkspace = Mantid::API::WorkspaceFactory::Instance().createTable("PeaksWorkspace");

  PeaksWorkspace_sptr peakWS = std::dynamic_pointer_cast<PeaksWorkspace>(tWorkspace);

  NXData nx_tw = entry.openNXData("peaks_workspace");

  int columnNumber = 1;
  int64_t numberPeaks = 0;
  std::vector<std::string> columnNames;
  do {
    std::string str = "column_" + std::to_string(columnNumber);

    NXInfo info = nx_tw.getDataSetInfo(str);
    if (info.stat == NXstatus::NX_ERROR) {
      // Assume we done last column of table
      break;
    }

    // store column names
    columnNames.emplace_back(str);

    // determine number of peaks
    // here we assume that a peaks_table has always one column of doubles

    if (info.type == NXnumtype::FLOAT64) {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
      std::string columnTitle = nxDouble.attributes("name");
      if (!columnTitle.empty() && numberPeaks == 0) {
        numberPeaks = nxDouble.dim0();
      }
    }

    columnNumber++;

  } while (true);

  // Hop to the right point /mantid_workspace_1
  try {
    m_nexusFile->openPath(entry.path()); // This is
  } catch (std::runtime_error &re) {
    throw std::runtime_error("Error while opening a path in a Peaks entry in a "
                             "Nexus processed file. "
                             "This path is wrong: " +
                             entry.path() + ". Lower level error description: " + re.what());
  }
  try {
    // Get information from all but data group
    std::string parameterStr;
    // This loads logs, sample, and instrument.
    peakWS->loadExperimentInfoNexus(getPropertyValue("Filename"), m_nexusFile.get(), parameterStr);
    // Populate the instrument parameters in this workspace
    peakWS->readParameterMap(parameterStr);
  } catch (std::exception &e) {
    g_log.information("Error loading Instrument section of nxs file");
    g_log.information(e.what());
  }

  // Coordinates - Older versions did not have the separate field but used a log
  // value
  const std::string peaksWSName = "peaks_workspace";
  try {
    m_nexusFile->openGroup(peaksWSName, "NXentry");
  } catch (std::runtime_error &re) {
    throw std::runtime_error("Error while opening a peaks workspace in a Nexus processed file. "
                             "Cannot open gropu " +
                             peaksWSName + ". Lower level error description: " + re.what());
  }
  try {
    uint32_t loadCoord(0);
    m_nexusFile->readData("coordinate_system", loadCoord);
    peakWS->setCoordinateSystem(static_cast<Kernel::SpecialCoordinateSystem>(loadCoord));
  } catch (::NeXus::Exception &) {
    // Check for a log value
    auto logs = peakWS->logs();
    if (logs->hasProperty("CoordinateSystem")) {
      auto *prop = dynamic_cast<PropertyWithValue<int> *>(logs->getProperty("CoordinateSystem"));
      if (prop) {
        int value((*prop)());
        peakWS->setCoordinateSystem(static_cast<Kernel::SpecialCoordinateSystem>(value));
      }
    }
  }

  std::string m_QConvention = "Inelastic";
  try {
    m_nexusFile->getAttr("QConvention", m_QConvention);
  } catch (std::exception &) {
  }

  // peaks_workspace
  m_nexusFile->closeGroup();

  // Change convention of loaded file to that in Preferen
  double qSign = 1.0;
  std::string convention = ConfigService::Instance().getString("Q.convention");
  if (convention != m_QConvention)
    qSign = -1.0;

  for (int r = 0; r < numberPeaks; r++) {
    // Warning! Do not use anything other than the default constructor here
    // It is currently important (10/05/17) that the DetID (set in the loop
    // below this one) is set before QLabFrame as this causes Peak to ray trace
    // to find the location of the detector, which significantly increases
    // loading times.
    const auto &goniometer = peakWS->run().getGoniometer();
    Peak peak;
    peak.setInstrument(peakWS->getInstrument());
    peak.setGoniometerMatrix(goniometer.getR());
    peak.setRunNumber(peakWS->getRunNumber());
    peakWS->addPeak(std::move(peak));
  }

  for (const auto &str : columnNames) {
    if (str == "column_1") {
      NXInt nxInt = nx_tw.openNXInt(str);
      nxInt.load();

      for (int r = 0; r < numberPeaks; r++) {
        int ival = nxInt[r];
        if (ival != -1)
          peakWS->getPeak(r).setDetectorID(ival);
      }
    } else if (str == "column_2") {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = qSign * nxDouble[r];
        peakWS->getPeak(r).setH(val);
      }
    } else if (str == "column_3") {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = qSign * nxDouble[r];
        peakWS->getPeak(r).setK(val);
      }
    } else if (str == "column_4") {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = qSign * nxDouble[r];
        peakWS->getPeak(r).setL(val);
      }
    } else if (str == "column_5") {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setIntensity(val);
      }
    } else if (str == "column_6") {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setSigmaIntensity(val);
      }
    } else if (str == "column_7") {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setBinCount(val);
      }
    } else if (str == "column_10") {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setWavelength(val);
      }
    } else if (str == "column_14") {
      NXInt nxInt = nx_tw.openNXInt(str);
      nxInt.load();

      for (int r = 0; r < numberPeaks; r++) {
        int ival = nxInt[r];
        if (ival != -1)
          peakWS->getPeak(r).setRunNumber(ival);
      }
    } else if (str == "column_17") {
      NXInt nxInt = nx_tw.openNXInt(str);
      nxInt.load();

      for (int r = 0; r < numberPeaks; r++) {
        int ival = nxInt[r];
        peakWS->getPeak(r).setPeakNumber(ival);
      }
    } else if (str == "column_18") {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setAbsorptionWeightedPathLength(val);
      }
    } else if (str == "column_15") {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
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
    } else if (str == "column_16") {
      // Read shape information
      using namespace Mantid::DataObjects;

      PeakShapeFactory_sptr peakFactoryEllipsoid = std::make_shared<PeakShapeEllipsoidFactory>();
      PeakShapeFactory_sptr peakFactorySphere = std::make_shared<PeakShapeSphericalFactory>();
      PeakShapeFactory_sptr peakFactoryNone = std::make_shared<PeakNoShapeFactory>();

      peakFactoryEllipsoid->setSuccessor(peakFactorySphere);
      peakFactorySphere->setSuccessor(peakFactoryNone);

      NXInfo info = nx_tw.getDataSetInfo(str);
      NXChar data = nx_tw.openNXChar(str);

      nxdimsize_t const maxShapeJSONLength = info.dims[1];
      data.load();
      for (int i = 0; i < numberPeaks; ++i) {

        // iR = peak row number
        auto startPoint = data() + (maxShapeJSONLength * i);
        std::string shapeJSON(startPoint, startPoint + maxShapeJSONLength);
        boost::trim_right(shapeJSON);

        // Make the shape
        Mantid::Geometry::PeakShape *peakShape = peakFactoryEllipsoid->create(shapeJSON);

        // Set the shape
        peakWS->getPeak(i).setPeakShape(peakShape);
      }
    } else if (str == "column_19") {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
      nxDouble.load();
      V3D hkl;
      for (int r = 0; r < numberPeaks; ++r) {
        hkl = V3D(nxDouble[r * 3], nxDouble[r * 3 + 1], nxDouble[r * 3 + 2]);
        peakWS->getPeak(r).setIntHKL(hkl);
      }
    } else if (str == "column_20") {
      NXDouble nxDouble = nx_tw.openNXDouble(str);
      nxDouble.load();
      V3D mnp;
      for (int r = 0; r < numberPeaks; ++r) {
        mnp = V3D(nxDouble[r * 3], nxDouble[r * 3 + 1], nxDouble[r * 3 + 2]);
        peakWS->getPeak(r).setIntMNP(mnp);
      }
    }
  }
  // After all columns read set IntHKL if not set
  for (int r = 0; r < numberPeaks; r++) {
    V3D intHKL = peakWS->getPeak(r).getIntHKL();
    if (intHKL.norm2() == 0) {
      intHKL = V3D(peakWS->getPeak(r).getH(), peakWS->getPeak(r).getK(), peakWS->getPeak(r).getL());
      peakWS->getPeak(r).setIntHKL(intHKL);
    }
  }

  return std::static_pointer_cast<API::Workspace>(peakWS);
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
 * @param workspaceType Takes values like "Workspace2D", "RebinnedOutput",
 *etc.
 *
 * @return workspace object containing loaded data
 */
API::MatrixWorkspace_sptr LoadNexusProcessed::loadNonEventEntry(NXData &wksp_cls, NXDouble &xbins,
                                                                const double &progressStart,
                                                                const double &progressRange, const NXEntry &mtd_entry,
                                                                const int64_t xlength, std::string &workspaceType) {
  // Filter the list of spectra to process, applying min/max/list options
  NXDataSetTyped<double> data = wksp_cls.openDoubleData();
  int64_t nchannels = data.dim1();
  size_t nspectra = data.dim0();
  // process optional spectrum parameters, if set
  checkOptionalProperties(nspectra);
  // Actual number of spectra in output workspace (if only a range was going to be loaded)
  size_t total_specs = calculateWorkspaceSize(nspectra);

  if (nchannels == 1 && nspectra == 1) {
    // if there is only one value of channels and nspectra, it may be a WorkspaceSingleValue
    // check for instrument
    bool hasInstrument = mtd_entry.containsGroup("instrument");
    if (hasInstrument) {
      std::string inst_name = mtd_entry.getString("instrument/name");
      boost::algorithm::trim(inst_name);
      if (inst_name == "")
        hasInstrument = false;
    } else {
      // data saved with SaveNexusESS will have the instrument in a directory named after it
      // check for special types of instrument: "basic_rect" and "unspecified_instrument":
      if (mtd_entry.containsGroup("basic_rect") || mtd_entry.containsGroup("unspecified_instrument")) {
        hasInstrument = true;
      } else {
        // check for other possible instruments
        for (auto facility : ConfigService::Instance().getFacilities()) {
          for (auto instrumentName : facility->instruments()) {
            if (instrumentName.name() != "" && mtd_entry.containsGroup(instrumentName.name())) {
              hasInstrument = true;
              break;
            }
          }
        }
      }
    }
    // check for metadata
    bool hasMetadata = mtd_entry.containsGroup("logs");
    if (hasMetadata) {
      // if there is more than one log (called "goniometer") then it's not a single-valued ws
      const auto nLogs = mtd_entry.openNXGroup("logs").groups().size();
      if (nLogs <= 1) { // only "goniometer" group is present, thus it's a single-valued ws
        hasMetadata = false;
      }
    }
    // a workspace with no instrument and no metadata, and only one entry is a single-valued ws
    if (!hasInstrument && !hasMetadata)
      workspaceType = "WorkspaceSingleValue";
  }
  bool hasFracArea = false;
  if (wksp_cls.isValid("frac_area")) {
    // frac_area entry is the signal for a RebinnedOutput workspace
    hasFracArea = true;
    workspaceType = "RebinnedOutput";
  }

  API::MatrixWorkspace_sptr local_workspace = std::dynamic_pointer_cast<API::MatrixWorkspace>(
      WorkspaceFactory::Instance().create(workspaceType, total_specs, xlength, nchannels));
  try {
    local_workspace->setTitle(mtd_entry.getString("title"));
  } catch (std::runtime_error &) {
    g_log.debug() << "No title was found in the input file, " << getPropertyValue("Filename") << '\n';
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

    // Set the fractional area attributes, default values consistent with
    // previous assumptions: finalized = true, sqrdErrs = false
    auto rbWS = std::dynamic_pointer_cast<RebinnedOutput>(local_workspace);
    auto finalizedValue = fracarea.attributes("finalized");
    auto finalized = (finalizedValue.empty() ? true : finalizedValue == "1");
    rbWS->setFinalized(finalized);
    auto sqrdErrsValue = fracarea.attributes("sqrd_errors");
    auto sqrdErrs = (sqrdErrsValue.empty() ? false : sqrdErrsValue == "1");
    rbWS->setSqrdErrors(sqrdErrs);
  }

  // Check for x errors; as with fracArea we set it to xbins
  // although in this case it would never be used.
  auto hasXErrors = wksp_cls.isValid("xerrors");
  auto xErrors = hasXErrors ? wksp_cls.openNXDouble("xerrors") : errors;
  if (hasXErrors) {
    if (xErrors.dim1() == nchannels + 1)
      g_log.warning() << "Legacy X uncertainty found in input file, i.e., "
                         "delta-Q for each BIN EDGE. Uncertainties will be "
                         "re-interpreted as delta-Q of the BIN CENTRE and the "
                         "last value will be dropped.\n";
  }

  int blocksize = 8;
  // const int fullblocks = nspectra / blocksize;
  // size of the workspace
  // have to cast down to int as later functions require ints
  int fullblocks = static_cast<int>(total_specs) / blocksize;
  int read_stop = (fullblocks * blocksize);
  const double progressBegin = progressStart + 0.25 * progressRange;
  const double progressScaler = 0.75 * progressRange;
  int hist_index = 0;
  int wsIndex = 0;
  if (m_shared_bins) {
    // if spectrum min,max,list properties are set
    if (m_interval || m_list) {
      // if spectrum max,min properties are set read the data as a
      // block(multiple of 8) and
      // then read the remaining data as finalblock
      if (m_interval) {
        // specs at the min-max interval
        int interval_specs = m_spec_max - m_spec_min;
        fullblocks = (interval_specs) / blocksize;
        read_stop = (fullblocks * blocksize) + m_spec_min - 1;

        if (interval_specs < blocksize) {
          blocksize = static_cast<int>(total_specs);
          read_stop = m_spec_max - 1;
        }
        hist_index = m_spec_min - 1;

        for (; hist_index < read_stop;) {
          progress(progressBegin + progressScaler * static_cast<double>(hist_index) / static_cast<double>(read_stop),
                   "Reading workspace data...");
          loadBlock(data, errors, fracarea, hasFracArea, xErrors, hasXErrors, blocksize, nchannels, hist_index, wsIndex,
                    local_workspace);
        }
        size_t finalblock = m_spec_max - 1 - read_stop;
        if (finalblock > 0) {
          loadBlock(data, errors, fracarea, hasFracArea, xErrors, hasXErrors, static_cast<int>(finalblock), nchannels,
                    hist_index, wsIndex, local_workspace);
        }
      }
      // if spectrum list property is set read each spectrum separately by
      // setting blocksize=1
      if (m_list) {
        for (const auto itr : m_spec_list) {
          int specIndex = itr - 1;
          progress(progressBegin +
                       progressScaler * static_cast<double>(specIndex) / static_cast<double>(m_spec_list.size()),
                   "Reading workspace data...");
          loadBlock(data, errors, fracarea, hasFracArea, xErrors, hasXErrors, 1, nchannels, specIndex, wsIndex,
                    local_workspace);
        }
      }
    } else {
      for (; hist_index < read_stop;) {
        progress(progressBegin + progressScaler * static_cast<double>(hist_index) / static_cast<double>(read_stop),
                 "Reading workspace data...");
        loadBlock(data, errors, fracarea, hasFracArea, xErrors, hasXErrors, blocksize, nchannels, hist_index, wsIndex,
                  local_workspace);
      }
      size_t finalblock = total_specs - read_stop;
      if (finalblock > 0) {
        loadBlock(data, errors, fracarea, hasFracArea, xErrors, hasXErrors, static_cast<int>(finalblock), nchannels,
                  hist_index, wsIndex, local_workspace);
      }
    }

  } else {
    if (m_interval || m_list) {
      if (m_interval) {
        int interval_specs = m_spec_max - m_spec_min;
        fullblocks = (interval_specs) / blocksize;
        read_stop = (fullblocks * blocksize) + m_spec_min - 1;

        if (interval_specs < blocksize) {
          blocksize = interval_specs;
          read_stop = m_spec_max - 1;
        }
        hist_index = m_spec_min - 1;

        for (; hist_index < read_stop;) {
          progress(progressBegin + progressScaler * static_cast<double>(hist_index) / static_cast<double>(read_stop),
                   "Reading workspace data...");
          loadBlock(data, errors, fracarea, hasFracArea, xErrors, hasXErrors, xbins, blocksize, nchannels, hist_index,
                    wsIndex, local_workspace);
        }
        size_t finalblock = m_spec_max - 1 - read_stop;
        if (finalblock > 0) {
          loadBlock(data, errors, fracarea, hasFracArea, xErrors, hasXErrors, xbins, static_cast<int>(finalblock),
                    nchannels, hist_index, wsIndex, local_workspace);
        }
      }
      //
      if (m_list) {
        for (const auto itr : m_spec_list) {
          int specIndex = itr - 1;
          progress(progressBegin + progressScaler * static_cast<double>(specIndex) / static_cast<double>(read_stop),
                   "Reading workspace data...");
          loadBlock(data, errors, fracarea, hasFracArea, xErrors, hasXErrors, xbins, 1, nchannels, specIndex, wsIndex,
                    local_workspace);
        }
      }
    } else {
      for (; hist_index < read_stop;) {
        progress(progressBegin + progressScaler * static_cast<double>(hist_index) / static_cast<double>(read_stop),
                 "Reading workspace data...");
        loadBlock(data, errors, fracarea, hasFracArea, xErrors, hasXErrors, xbins, blocksize, nchannels, hist_index,
                  wsIndex, local_workspace);
      }
      size_t finalblock = total_specs - read_stop;
      if (finalblock > 0) {
        loadBlock(data, errors, fracarea, hasFracArea, xErrors, hasXErrors, xbins, static_cast<int>(finalblock),
                  nchannels, hist_index, wsIndex, local_workspace);
      }
    }

    // now check for NaN at end of X which would signify ragged binning
    for (size_t i = 0; i < local_workspace->getNumberHistograms(); i++) {
      const auto &x = local_workspace->readX(i);
      const auto idx =
          std::distance(x.rbegin(), std::find_if_not(x.rbegin(), x.rend(), [](auto val) { return std::isnan(val); }));
      if (idx > 0)
        local_workspace->resizeHistogram(i, local_workspace->histogramSize(i) - idx);
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
 * @param progressStart :: The percentage value to start the progress
 *reporting
 * for this entry
 * @param progressRange :: The percentage range that the progress reporting
 * should cover
 * @returns A 2D workspace containing the loaded data
 */
API::Workspace_sptr LoadNexusProcessed::loadEntry(NXRoot &root, const std::string &entry_name,
                                                  const double &progressStart, const double &progressRange) {
  progress(progressStart, "Opening entry " + entry_name + "...");

  NXEntry mtd_entry = root.openEntry(entry_name);

  if (mtd_entry.containsGroup("table_workspace")) {
    return loadTableEntry(mtd_entry);
  }

  if (mtd_entry.containsGroup("peaks_workspace")) {
    try {
      // try standard PeakWorkspace first
      return loadPeaksEntry(mtd_entry);
    } catch (std::exception &) {
      return loadLeanElasticPeaksEntry(mtd_entry);
    }
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
  } else if (mtd_entry.containsGroup("mask_workspace")) {
    workspaceType = "MaskWorkspace";
    group_name = "mask_workspace";
  } else if (mtd_entry.containsGroup("grouping_workspace")) {
    workspaceType = "GroupingWorkspace";
    group_name = "grouping_workspace";
  }

  // Get workspace characteristics
  NXData wksp_cls = mtd_entry.openNXData(group_name);

  // Axis information
  // "X" axis

  NXDouble xbins = wksp_cls.openNXDouble("axis1");
  xbins.load();
  std::string unit1 = xbins.attributes("units");
  // Non-uniform x bins get saved as a 2D 'axis1' dataset
  int64_t xlength(-1);
  if (xbins.rank() == 2) {
    xlength = xbins.dim1();
    m_shared_bins = false;
  } else if (xbins.rank() == 1) {
    xlength = xbins.dim0();
    m_shared_bins = true;
    xbins.load();
    m_xbins = HistogramData::HistogramX(xbins(), xbins() + xlength);
  } else {
    throw std::runtime_error("Unknown axis1 dimension encountered.");
  }

  // MatrixWorkspace axis 1
  NXDouble axis2 = wksp_cls.openNXDouble("axis2");
  std::string unit2 = axis2.attributes("units");

  // --- Load workspace (as event_workspace or workspace2d) ---
  API::MatrixWorkspace_sptr local_workspace;
  if (isEvent) {
    local_workspace = loadEventEntry(wksp_cls, xbins, progressStart, progressRange);
  } else {
    local_workspace =
        loadNonEventEntry(wksp_cls, xbins, progressStart, progressRange, mtd_entry, xlength, workspaceType);
  }
  size_t nspectra = local_workspace->getNumberHistograms();

  // Units
  bool verticalHistogram(false);
  try {
    local_workspace->getAxis(0)->unit() = UnitFactory::Instance().create(unit1);
    if (unit1 == "Label") {
      auto label = std::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(local_workspace->getAxis(0)->unit());
      auto ax = wksp_cls.openNXDouble("axis1");
      label->setLabel(ax.attributes("caption"), ax.attributes("label"));
    }

    // If this doesn't throw then it is a numeric access so grab the data so
    // we
    // can set it later
    axis2.load();
    if (static_cast<size_t>(axis2.size()) == nspectra + 1)
      verticalHistogram = true;
    m_axis1vals = MantidVec(axis2(), axis2() + axis2.dim0());
  } catch (std::runtime_error &) {
    g_log.information() << "Axis 0 set to unitless quantity \"" << unit1 << "\"\n";
  }

  // Setting a unit onto a TextAxis makes no sense.
  if (unit2 == "TextAxis") {
    auto newAxis = std::make_unique<Mantid::API::TextAxis>(nspectra);
    local_workspace->replaceAxis(1, std::move(newAxis));
  } else if (unit2 != "spectraNumber") {
    try {
      auto newAxis = (verticalHistogram) ? std::make_unique<API::BinEdgeAxis>(nspectra + 1)
                                         : std::make_unique<API::NumericAxis>(nspectra);
      auto newAxisRaw = newAxis.get();
      local_workspace->replaceAxis(1, std::move(newAxis));
      newAxisRaw->unit() = UnitFactory::Instance().create(unit2);
      if (unit2 == "Label") {
        auto label = std::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(newAxisRaw->unit());
        auto ax = wksp_cls.openNXDouble("axis2");
        label->setLabel(ax.attributes("caption"), ax.attributes("label"));
      }
    } catch (std::runtime_error &) {
      g_log.information() << "Axis 1 set to unitless quantity \"" << unit2 << "\"\n";
    }
  }

  // Are we a distribution
  std::string dist = xbins.attributes("distribution");
  if (dist == "1") {
    local_workspace->setDistribution(true);
  } else {
    local_workspace->setDistribution(false);
  }

  progress(progressStart + 0.05 * progressRange, "Reading the sample details...");

  // Hop to the right point
  m_nexusFile->openPath(mtd_entry.path());
  try {
    // Get information from all but data group
    std::string parameterStr;

    // This loads logs, sample, and instrument.
    local_workspace->loadExperimentInfoNexus(getPropertyValue("Filename"), m_nexusFile.get(),
                                             parameterStr); // REQUIRED PER PERIOD

    // Parameter map parsing only if instrument loaded OK.
    progress(progressStart + 0.11 * progressRange, "Reading the parameter maps...");
    local_workspace->readParameterMap(parameterStr);
  } catch (std::exception &e) {
    // For workspaces saved via SaveNexusESS, these warnings are not
    // relevant. Such workspaces will contain an `NXinstrument` entry
    // with the name of the instrument.
    const auto &entries = getFileInfo()->getAllEntries();
    if (version() < 2 || entries.find("NXinstrument") == entries.end()) {
      g_log.warning("Error loading Instrument section of nxs file");
      g_log.warning(e.what());
      g_log.warning("Try running LoadInstrument Algorithm on the Workspace to "
                    "update the geometry");
    }
  }

  readSpectraToDetectorMapping(mtd_entry, *local_workspace);

  if (!local_workspace->getAxis(1)->isSpectra()) { // If not a spectra axis, load the axis data into
                                                   // the workspace. (MW 25/11/10)
    loadNonSpectraAxis(local_workspace, wksp_cls);
  }

  progress(progressStart + 0.15 * progressRange, "Reading the workspace history...");
  m_nexusFile->openPath(mtd_entry.path());
  try {
    bool load_history = getProperty("LoadHistory");
    if (load_history)
      local_workspace->history().loadNexus(m_nexusFile.get());
  } catch (std::out_of_range &) {
    g_log.warning() << "Error in the workspaces algorithm list, its processing "
                       "history is incomplete\n";
  }

  progress(progressStart + 0.2 * progressRange, "Reading the workspace history...");

  try {
    if (local_workspace->getTitle().empty())
      local_workspace->setTitle(mtd_entry.getString("title"));
  } catch (std::runtime_error &) {
    g_log.debug() << "No title was found in the input file, " << getPropertyValue("Filename") << '\n';
  }

  return std::static_pointer_cast<API::Workspace>(local_workspace);
}

//-------------------------------------------------------------------------------------------------
/**
 * Read the instrument group
 * @param mtd_entry :: The node for the current workspace
 * @param local_workspace :: The workspace to attach the instrument
 */
void LoadNexusProcessed::readInstrumentGroup(NXEntry &mtd_entry, API::MatrixWorkspace &local_workspace) {
  // Get spectrum information for the current entry.

  SpectraInfo spectraInfo = extractMappingInfo(mtd_entry, this->g_log);

  // Now build the spectra list
  int index = 0;
  bool haveSpectraAxis = local_workspace.getAxis(1)->isSpectra();

  for (int i = 1; i <= spectraInfo.nSpectra; ++i) {
    int spectrum;
    // prefer the spectra number from the instrument section
    // over anything else. If not there then use a spectra axis
    // number if we have one, else make one up as nothing was
    // written to the file. We should always set it so that
    // CompareWorkspaces gives the expected answer on a Save/Load
    // round trip.
    if (spectraInfo.hasSpectra) {
      spectrum = spectraInfo.spectraNumbers[i - 1];
    } else if (haveSpectraAxis && !m_axis1vals.empty()) {
      spectrum = static_cast<specnum_t>(m_axis1vals[i - 1]);
    } else {
      spectrum = i + 1;
    }

    if ((i >= m_spec_min && i < m_spec_max) ||
        (m_list && find(m_spec_list.begin(), m_spec_list.end(), i) != m_spec_list.end())) {
      auto &spec = local_workspace.getSpectrum(index);
      spec.setSpectrumNo(spectrum);
      ++index;

      if (!spectraInfo.detectorIndex.empty()) {
        const int start = spectraInfo.detectorIndex[i - 1];
        const int end = start + spectraInfo.detectorCount[i - 1];
        spec.setDetectorIDs(
            std::set<detid_t>(spectraInfo.detectorList.data() + start, spectraInfo.detectorList.data() + end));
      }
    }
  }
}

/**
 * Validates SpectrumMin and SpectrumMax conditions
 * @return Returns a map indicating invalid input combinations
 */
std::map<std::string, std::string> LoadNexusProcessed::validateInputs() {
  using namespace std;
  map<string, string> errorList;

  int specMin = getProperty("SpectrumMin");
  int specMax = getProperty("SpectrumMax");

  // Check our range is not reversed
  if (specMax < specMin) {
    errorList["SpectrumMin"] = "SpectrumMin must be smaller than SpectrumMax";
    errorList["SpectrumMax"] = "SpectrumMax must be larger than SpectrumMin";
  }

  // Finished testing return any errors
  return errorList;
}

//-------------------------------------------------------------------------------------------------
/**
 * Loads the information contained in non-Spectra (ie, Text or Numeric) axis
 * in
 * the Nexus
 * file into the workspace.
 * @param local_workspace :: pointer to workspace object
 * @param data :: reference to the NeXuS data for the axis
 */
void LoadNexusProcessed::loadNonSpectraAxis(const API::MatrixWorkspace_sptr &local_workspace, const NXData &data) {
  Axis *axis = local_workspace->getAxis(1);

  if (axis->isNumeric()) {
    NXDouble axisData = data.openNXDouble("axis2");
    axisData.load();
    for (int i = 0; i < static_cast<int>(axis->length()); i++) {
      axis->setValue(i, axisData[i]);
    }
  } else if (axis->isText()) {
    NXChar axisData = data.openNXChar("axis2");
    std::string axisLabels;
    try {
      axisData.load();
      axisLabels = std::string(axisData(), axisData.dim0());
    } catch (std::runtime_error &) {
      axisLabels = "";
    }
    // Use boost::tokenizer to split up the input
    Mantid::Kernel::StringTokenizer tokenizer(axisLabels, "\n", Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
    // We must cast the axis object to TextAxis so we may use ->setLabel
    auto *textAxis = static_cast<TextAxis *>(axis);
    int i = 0;
    for (const auto &tokIter : tokenizer) {
      textAxis->setLabel(i, tokIter);
      i++;
    }
  }
}

/**
 * Binary predicate function object to sort the AlgorithmHistory vector by
 * execution order
 * @param elem1 :: first element in the vector
 * @param elem2 :: second element in the vecor
 */
bool UDlesserExecCount(const NXClassInfo &elem1, const NXClassInfo &elem2) {
  std::string::size_type index1, index2;
  std::string num1, num2;
  // find the number after "_" in algorithm name ( eg:MantidAlogorthm_1)
  index1 = elem1.nxname.find('_');
  if (index1 != std::string::npos) {
    num1 = elem1.nxname.substr(index1 + 1, elem1.nxname.length() - index1);
  }
  index2 = elem2.nxname.find('_');
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

  return execNum1 < execNum2;
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
void LoadNexusProcessed::getWordsInString(const std::string &words3, std::string &w1, std::string &w2,
                                          std::string &w3) {
  Mantid::Kernel::StringTokenizer data(words3, " ", Mantid::Kernel::StringTokenizer::TOK_TRIM);
  if (data.count() != 3) {
    g_log.warning() << "Algorithm list line " + words3 + " is not of the correct format\n";
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
void LoadNexusProcessed::getWordsInString(const std::string &words4, std::string &w1, std::string &w2, std::string &w3,
                                          std::string &w4) {
  Mantid::Kernel::StringTokenizer data(words4, " ", Mantid::Kernel::StringTokenizer::TOK_TRIM);
  if (data.count() != 4) {
    g_log.warning() << "Algorithm list line " + words4 + " is not of the correct format\n";
    throw std::out_of_range(words4);
  }

  w1 = data[0];
  w2 = data[1];
  w3 = data[2];
  w4 = data[3];
}

//-------------------------------------------------------------------------------------------------
/**
 * Read the bin masking information from the mantid_workspace_i/workspace
 * group.
 * @param wksp_cls :: The data group
 * @param local_workspace :: The workspace to read into
 */
void LoadNexusProcessed::readBinMasking(const NXData &wksp_cls, const API::MatrixWorkspace_sptr &local_workspace) {
  if (wksp_cls.getDataSetInfo("masked_spectra").stat == NXstatus::NX_ERROR) {
    return;
  }
  NXInt spec = wksp_cls.openNXInt("masked_spectra");
  spec.load();
  NXSize bins = wksp_cls.openNXSize("masked_bins");
  bins.load();
  NXDouble weights = wksp_cls.openNXDouble("mask_weights");
  weights.load();
  const int64_t n = spec.dim0();
  const int64_t n1 = n - 1;
  for (int i = 0; i < n; ++i) {
    int64_t si = spec(i, 0);
    int64_t j0 = spec(i, 1);
    int64_t j1 = i < n1 ? spec(i + 1, 1) : bins.dim0();
    for (int64_t j = j0; j < j1; ++j) {
      local_workspace->flagMasked(si, bins[j], weights[j]);
    }
  }
}

/**
 * Perform a call to nxgetslab, via the NexusClasses wrapped methods for a
 * given
 * blocksize. This assumes that the
 * xbins have alread been cached
 * @param data :: The NXDataSet object of y values
 * @param errors :: The NXDataSet object of error values
 * @param farea :: The NXDataSet object of fraction area values
 * @param hasFArea :: Flag to signal a RebinnedOutput workspace is in use
 * @param xErrors :: The NXDataSet object of xError values
 * @param hasXErrors :: Flag to signal the File contains x errors
 * @param blocksize :: The blocksize to use
 * @param nchannels :: The number of channels for the block
 * @param hist :: The workspace index to start reading into
 * @param local_workspace :: A pointer to the workspace
 */
void LoadNexusProcessed::loadBlock(NXDataSetTyped<double> &data, NXDataSetTyped<double> &errors,
                                   NXDataSetTyped<double> &farea, bool hasFArea, NXDouble &xErrors, bool hasXErrors,
                                   int blocksize, int nchannels, int &hist,
                                   const API::MatrixWorkspace_sptr &local_workspace) {
  data.load(blocksize, hist);
  errors.load(blocksize, hist);
  double *data_start = data();
  double *data_end = data_start + nchannels;
  double *err_start = errors();
  double *err_end = err_start + nchannels;
  double *farea_start = nullptr;
  double *farea_end = nullptr;
  double *xErrors_start = nullptr;
  double *xErrors_end = nullptr;
  size_t dx_increment = nchannels;
  // NexusFileIO stores Dx data for all spectra (sharing not preserved) so dim0
  // is the histograms, dim1 is Dx length. For old files this is nchannels+1,
  // otherwise nchannels. See #16298.
  // WARNING: We are dropping the last Dx value for old files!
  size_t dx_input_increment = xErrors.dim1();
  RebinnedOutput_sptr rb_workspace;
  if (hasFArea) {
    farea.load(blocksize, hist);
    farea_start = farea();
    farea_end = farea_start + nchannels;
    rb_workspace = std::dynamic_pointer_cast<RebinnedOutput>(local_workspace);
  }
  if (hasXErrors) {
    xErrors.load(blocksize, hist);
    xErrors_start = xErrors();
    xErrors_end = xErrors_start + dx_increment;
  }

  int final(hist + blocksize);
  while (hist < final) {
    auto &Y = local_workspace->mutableY(hist);
    Y.assign(data_start, data_end);
    data_start += nchannels;
    data_end += nchannels;
    auto &E = local_workspace->mutableE(hist);
    E.assign(err_start, err_end);
    err_start += nchannels;
    err_end += nchannels;
    if (hasFArea) {
      MantidVec &F = rb_workspace->dataF(hist);
      F.assign(farea_start, farea_end);
      farea_start += nchannels;
      farea_end += nchannels;
    }
    if (hasXErrors) {
      local_workspace->setSharedDx(hist, Kernel::make_cow<HistogramData::HistogramDx>(xErrors_start, xErrors_end));
      xErrors_start += dx_input_increment;
      xErrors_end += dx_input_increment;
    }

    local_workspace->setSharedX(hist, m_xbins.cowData());
    ++hist;
  }
}

/**
 * Perform a call to nxgetslab, via the NexusClasses wrapped methods for a
 * given
 * blocksize. This assumes that the
 * xbins have alread been cached
 * @param data :: The NXDataSet object of y values
 * @param errors :: The NXDataSet object of error values
 * @param farea :: The NXDataSet object of fraction area values
 * @param hasFArea :: Flag to signal a RebinnedOutput workspace is in use
 * @param xErrors :: The NXDataSet object of xError values
 * @param hasXErrors :: Flag to signal the File contains x errors
 * @param blocksize :: The blocksize to use
 * @param nchannels :: The number of channels for the block
 * @param hist :: The workspace index to start reading into
 * @param wsIndex :: The workspace index to save data into
 * @param local_workspace :: A pointer to the workspace
 */

void LoadNexusProcessed::loadBlock(NXDataSetTyped<double> &data, NXDataSetTyped<double> &errors,
                                   NXDataSetTyped<double> &farea, bool hasFArea, NXDouble &xErrors, bool hasXErrors,
                                   int64_t blocksize, int64_t nchannels, int &hist, int &wsIndex,
                                   const API::MatrixWorkspace_sptr &local_workspace) {
  data.load(blocksize, hist);
  errors.load(blocksize, hist);
  double *data_start = data();
  double *data_end = data_start + nchannels;
  double *err_start = errors();
  double *err_end = err_start + nchannels;
  double *farea_start = nullptr;
  double *farea_end = nullptr;
  double *xErrors_start = nullptr;
  double *xErrors_end = nullptr;
  size_t dx_increment = nchannels;
  // NexusFileIO stores Dx data for all spectra (sharing not preserved) so dim0
  // is the histograms, dim1 is Dx length. For old files this is nchannels+1,
  // otherwise nchannels. See #16298.
  // WARNING: We are dropping the last Dx value for old files!
  size_t dx_input_increment = xErrors.dim1();
  RebinnedOutput_sptr rb_workspace;
  if (hasFArea) {
    farea.load(blocksize, hist);
    farea_start = farea();
    farea_end = farea_start + nchannels;
    rb_workspace = std::dynamic_pointer_cast<RebinnedOutput>(local_workspace);
  }
  if (hasXErrors) {
    xErrors.load(blocksize, hist);
    xErrors_start = xErrors();
    xErrors_end = xErrors_start + dx_increment;
  }

  int64_t final(hist + blocksize);
  while (hist < final) {
    auto &Y = local_workspace->mutableY(wsIndex);
    Y.assign(data_start, data_end);
    data_start += nchannels;
    data_end += nchannels;
    auto &E = local_workspace->mutableE(wsIndex);
    E.assign(err_start, err_end);
    err_start += nchannels;
    err_end += nchannels;
    if (hasFArea) {
      MantidVec &F = rb_workspace->dataF(wsIndex);
      F.assign(farea_start, farea_end);
      farea_start += nchannels;
      farea_end += nchannels;
    }
    if (hasXErrors) {
      local_workspace->setSharedDx(wsIndex, Kernel::make_cow<HistogramData::HistogramDx>(xErrors_start, xErrors_end));
      xErrors_start += dx_input_increment;
      xErrors_end += dx_input_increment;
    }
    local_workspace->setSharedX(wsIndex, m_xbins.cowData());
    ++hist;
    ++wsIndex;
  }
}

/**
 * Perform a call to nxgetslab, via the NexusClasses wrapped methods for a
 * given
 * blocksize. The xbins are read along with
 * each call to the data/error loading
 * @param data :: The NXDataSet object of y values
 * @param errors :: The NXDataSet object of error values
 * @param farea :: The NXDataSet object of fraction area values
 * @param hasFArea :: Flag to signal a RebinnedOutput workspace is in use
 * @param xErrors :: The NXDataSet object of xError values
 * @param hasXErrors :: Flag to signal the File contains x errors
 * @param xbins :: The xbin NXDataSet
 * @param blocksize :: The blocksize to use
 * @param nchannels :: The number of channels for the block
 * @param hist :: The workspace index to start reading into
 * @param wsIndex :: The workspace index to save data into
 * @param local_workspace :: A pointer to the workspace
 */
void LoadNexusProcessed::loadBlock(NXDataSetTyped<double> &data, NXDataSetTyped<double> &errors,
                                   NXDataSetTyped<double> &farea, bool hasFArea, NXDouble &xErrors, bool hasXErrors,
                                   NXDouble &xbins, int64_t blocksize, int64_t nchannels, int &hist, int &wsIndex,
                                   const API::MatrixWorkspace_sptr &local_workspace) {
  data.load(blocksize, hist);
  double *data_start = data();
  double *data_end = data_start + nchannels;
  errors.load(blocksize, hist);
  double *err_start = errors();
  double *err_end = err_start + nchannels;
  double *farea_start = nullptr;
  double *farea_end = nullptr;
  double *xErrors_start = nullptr;
  double *xErrors_end = nullptr;
  size_t dx_increment = nchannels;
  // NexusFileIO stores Dx data for all spectra (sharing not preserved) so dim0
  // is the histograms, dim1 is Dx length. For old files this is nchannels+1,
  // otherwise nchannels. See #16298.
  // WARNING: We are dropping the last Dx value for old files!
  size_t dx_input_increment = xErrors.dim1();
  RebinnedOutput_sptr rb_workspace;
  if (hasFArea) {
    farea.load(blocksize, hist);
    farea_start = farea();
    farea_end = farea_start + nchannels;
    rb_workspace = std::dynamic_pointer_cast<RebinnedOutput>(local_workspace);
  }
  xbins.load(blocksize, hist);
  const int64_t nxbins(xbins.dim1());
  double *xbin_start = xbins();
  double *xbin_end = xbin_start + nxbins;
  int64_t final(hist + blocksize);

  if (hasXErrors) {
    xErrors.load(blocksize, hist);
    xErrors_start = xErrors();
    xErrors_end = xErrors_start + dx_increment;
  }

  while (hist < final) {
    auto &Y = local_workspace->mutableY(wsIndex);
    Y.assign(data_start, data_end);
    data_start += nchannels;
    data_end += nchannels;
    auto &E = local_workspace->mutableE(wsIndex);
    E.assign(err_start, err_end);
    err_start += nchannels;
    err_end += nchannels;
    if (hasFArea) {
      MantidVec &F = rb_workspace->dataF(wsIndex);
      F.assign(farea_start, farea_end);
      farea_start += nchannels;
      farea_end += nchannels;
    }
    if (hasXErrors) {
      local_workspace->setSharedDx(wsIndex, Kernel::make_cow<HistogramData::HistogramDx>(xErrors_start, xErrors_end));
      xErrors_start += dx_input_increment;
      xErrors_end += dx_input_increment;
    }
    auto &X = local_workspace->mutableX(wsIndex);
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
void LoadNexusProcessed::checkOptionalProperties(const std::size_t numberofspectra) {
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
    const int minlist = *min_element(m_spec_list.begin(), m_spec_list.end());
    const int maxlist = *max_element(m_spec_list.begin(), m_spec_list.end());
    // Need to check before casting
    if (maxlist < 0) {
      g_log.error("Invalid list of spectra");
      throw std::invalid_argument("Spectra max is less than 0");
    }

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
      m_spec_max = static_cast<int>(numberofspectra);
    }
    if (m_spec_max < m_spec_min || m_spec_max > static_cast<int>(numberofspectra)) {
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
size_t LoadNexusProcessed::calculateWorkspaceSize(const std::size_t numberofspectra, bool gen_filtered_list) {
  // Calculate the size of a workspace, given its number of spectra to read
  size_t total_specs;
  if (m_interval || m_list) {
    if (m_interval) {
      if (m_spec_min != 1 && m_spec_max == 1) {
        m_spec_max = static_cast<int>(numberofspectra);
      }
      total_specs = m_spec_max - m_spec_min + 1;
      m_spec_max += 1;

      if (gen_filtered_list) {
        m_filtered_spec_idxs.resize(total_specs);
        size_t j = 0;
        for (int si = m_spec_min; si < m_spec_max; si++, j++)
          m_filtered_spec_idxs[j] = si;
      }
    } else {
      total_specs = 0;
    }

    if (m_list) {
      if (m_interval) {
        for (auto it = m_spec_list.begin(); it != m_spec_list.end();)
          if (*it >= m_spec_min && *it < m_spec_max) {
            it = m_spec_list.erase(it);
          } else
            ++it;
      }
      if (m_spec_list.empty())
        m_list = false;
      total_specs += m_spec_list.size();

      if (gen_filtered_list) {
        // range list + spare indices from list
        // example: min: 2, max: 8, list: 3,4,5,10,12;
        //          result: 2,3,...,7,8,10,12
        m_filtered_spec_idxs.insert(m_filtered_spec_idxs.end(), m_spec_list.begin(), m_spec_list.end());
      }
    }
  } else {
    total_specs = numberofspectra;
    m_spec_min = 1;
    m_spec_max = static_cast<int>(numberofspectra) + 1;

    if (gen_filtered_list) {
      m_filtered_spec_idxs.resize(total_specs, 0);
      for (int j = 0; j < static_cast<int>(total_specs); j++)
        m_filtered_spec_idxs[j] = m_spec_min + j;
    }
  }
  return total_specs;
}

/**
 * Applies log filtering to workspaces that require it
 *
 * @param local_workspace :: the workspace containing logs to be filtered
 */
void LoadNexusProcessed::applyLogFiltering(const Mantid::API::Workspace_sptr &local_workspace) {
  auto mWorkspace = std::dynamic_pointer_cast<MatrixWorkspace>(local_workspace);
  if (mWorkspace) {
    auto run = mWorkspace->run();
    // check for presence of filterable logs that suggest this is ISIS data
    if (run.hasProperty(LogParser::statusLogName()) || run.hasProperty(LogParser::periodsLogName())) {
      ISISRunLogs::applyLogFiltering(mWorkspace->mutableRun());
    }
  }
}

} // namespace Mantid::DataHandling
