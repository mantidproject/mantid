// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadISISNexus2.h"
#include "MantidDataHandling/DataBlockGenerator.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidDataHandling/LoadISISNexusHelper.h"
#include "MantidDataHandling/LoadRawHelper.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"

// clang-format off
#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>
// clang-format on

#include <algorithm>
#include <cctype>
#include <climits>
#include <cmath>
#include <functional>
#include <sstream>
#include <vector>

namespace {
Mantid::DataHandling::DataBlockComposite getMonitorsFromComposite(Mantid::DataHandling::DataBlockComposite &composite,
                                                                  Mantid::DataHandling::DataBlockComposite &monitors) {
  auto dataBlocks = composite.getDataBlocks();
  auto monitorBlocks = monitors.getDataBlocks();
  auto matchesMonitorBlock = [&monitorBlocks](Mantid::DataHandling::DataBlock &dataBlock) {
    return std::find(std::begin(monitorBlocks), std::end(monitorBlocks), dataBlock) != std::end(monitorBlocks);
  };

  Mantid::DataHandling::DataBlockComposite newComposite;
  for (auto &dataBlock : dataBlocks) {
    if (matchesMonitorBlock(dataBlock)) {
      newComposite.addDataBlock(dataBlock);
    }
  }

  return newComposite;
}
} // namespace

namespace Mantid::DataHandling {

DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadISISNexus2)

using namespace Kernel;
using namespace API;
using namespace NeXus;
using namespace HistogramData;
using std::size_t;

/// Empty default constructor
LoadISISNexus2::LoadISISNexus2()
    : m_filename(), m_instrument_name(), m_samplename(), m_detBlockInfo(), m_monBlockInfo(), m_loadBlockInfo(),
      m_have_detector(false), m_hasVMSBlock(false), m_load_selected_spectra(false), m_wsInd2specNum_map(),
      m_spec2det_map(), m_entrynumber(0), m_tof_data(), m_spec(), m_spec_end(nullptr), m_monitors(), m_logCreator(),
      m_progress(), m_nexusFile() {}

/**
 * Return the confidence criteria for this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadISISNexus2::confidence(Kernel::NexusDescriptor &descriptor) const {
  if (descriptor.pathOfTypeExists("/raw_data_1", "NXentry")) {
    // It also could be an Event Nexus file or a TOFRaw file,
    // so confidence is set to less than 80.
    return 75;
  }
  return 0;
}

/// Initialization method.
void LoadISISNexus2::init() {
  const std::vector<std::string> exts{".nxs", ".n*"};
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, exts),
                  "The name of the Nexus file to load");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output));

  auto mustBePositiveSpectrum = std::make_shared<BoundedValidator<specnum_t>>();
  mustBePositiveSpectrum->setLower(0);
  declareProperty("SpectrumMin", static_cast<specnum_t>(0), mustBePositiveSpectrum);
  declareProperty("SpectrumMax", static_cast<specnum_t>(EMPTY_INT()), mustBePositiveSpectrum);
  declareProperty(std::make_unique<ArrayProperty<specnum_t>>("SpectrumList"));
  auto mustBePositive = std::make_shared<BoundedValidator<int64_t>>();
  declareProperty("EntryNumber", static_cast<int64_t>(0), mustBePositive,
                  "0 indicates that every entry is loaded, into a separate "
                  "workspace within a group. "
                  "A positive number identifies one entry to be loaded, into "
                  "one worskspace");

  std::vector<std::string> monitorOptions{"Include", "Exclude", "Separate"};
  std::map<std::string, std::string> monitorOptionsAliases;
  monitorOptionsAliases["1"] = "Separate";
  monitorOptionsAliases["0"] = "Exclude";
  declareProperty("LoadMonitors", "Include",
                  std::make_shared<Kernel::StringListValidator>(monitorOptions, monitorOptionsAliases),
                  "Option to control the loading of monitors.\n"
                  "Allowed options are Include,Exclude, Separate.\n"
                  "Include:The default is Include option would load monitors with the "
                  "workspace if monitors spectra are within the range of loaded "
                  "detectors.\n"
                  "If the time binning for the monitors is different from the\n"
                  "binning of the detectors this option is equivalent to the Separate "
                  "option\n"
                  "Exclude:Exclude option excludes monitors from the output workspace.\n"
                  "Separate:Separate option loads monitors into a separate workspace "
                  "called: OutputWorkspace_monitors.\n"
                  "Defined aliases:\n"
                  "1:  Equivalent to Separate.\n"
                  "0:  Equivalent to Exclude.\n");
}

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw Exception::FileError If the Nexus file cannot be found/opened
 *  @throw std::invalid_argument If the optional properties are set to invalid
 *values
 */
void LoadISISNexus2::exec() {

  //**********************************************************************
  // process load monitor options request
  bool bincludeMonitors, bseparateMonitors, bexcludeMonitors;
  LoadRawHelper::ProcessLoadMonitorOptions(bincludeMonitors, bseparateMonitors, bexcludeMonitors, this);

  //**********************************************************************
  m_filename = getPropertyValue("Filename");
  // Create the root Nexus class
  NXRoot root(m_filename);

  // "Open" the same file but with the C++ interface
  m_nexusFile.reset(new ::NeXus::File(root.m_fileID));

  // Open the raw data group 'raw_data_1'
  NXEntry entry = root.openEntry("raw_data_1");

  // Read in the instrument name from the Nexus file
  m_instrument_name = entry.getString("name");

  // Test if we have a vms block
  if (entry.containsGroup("isis_vms_compat")) {
    m_hasVMSBlock = true;
  }

  // Get number of detectors and spectrum list
  size_t ndets{0};
  try {
    NXClass det_class = entry.openNXGroup("detector_1");
    NXInt spectrum_index = det_class.openNXInt("spectrum_index");
    spectrum_index.load();
    ndets = spectrum_index.dim0();
    // We assume that this spectrum list increases monotonically
    m_spec = spectrum_index.vecBuffer();
    m_spec_end = m_spec.data() + ndets;
    m_have_detector = true;
  } catch (std::runtime_error &) {
    ndets = 0;
  }

  // Load detector and spectra ids, and number of monitors + detectors?
  auto [udet, spec] = LoadISISNexusHelper::findDetectorIDsAndSpectrumNumber(entry, m_hasVMSBlock);
  int64_t nsp1 = LoadISISNexusHelper::findNumberOfSpectra(entry, m_hasVMSBlock);

  // Pull out the monitor blocks, if any exist
  size_t nmons{0};

  for (auto it = entry.groups().cbegin(); it != entry.groups().cend(); ++it) {
    if (it->nxclass == "NXmonitor") // Count monitors
    {
      NXInt index = entry.openNXInt(std::string(it->nxname) + "/spectrum_index");
      index.load();
      specnum_t ind = static_cast<specnum_t>(*index());
      // Spectrum index of 0 means no spectrum associated with that monitor,
      // so only count those with index > 0
      if (ind > 0) {
        m_monitors[ind] = it->nxname;
        ++nmons;
      }
    }
  }

  if (ndets == 0 && nmons == 0) {
    if (bexcludeMonitors) {
      g_log.warning() << "Nothing to do. No detectors found and no monitor "
                         "loading requested";
      return;
    } else {
      g_log.error() << "Invalid NeXus structure, cannot find detector or monitor blocks.";
      throw std::runtime_error("Inconsistent NeXus file structure.");
    }
  }

  // Determine the data block for the detectors and monitors
  bseparateMonitors =
      findSpectraDetRangeInFile(entry, m_spec, ndets, nsp1, m_monitors, bexcludeMonitors, bseparateMonitors);

  size_t x_length = m_loadBlockInfo.getNumberOfChannels() + 1;

  // Check input is consistent with the file, throwing if not
  bseparateMonitors = checkOptionalProperties(bseparateMonitors, bexcludeMonitors);
  // Fill up m_spectraBlocks
  size_t total_specs = prepareSpectraBlocks(m_monitors, m_loadBlockInfo);

  m_progress = std::make_shared<API::Progress>(this, 0.0, 1.0, total_specs * m_detBlockInfo.getNumberOfPeriods());

  DataObjects::Workspace2D_sptr local_workspace = std::dynamic_pointer_cast<DataObjects::Workspace2D>(
      WorkspaceFactory::Instance().create("Workspace2D", total_specs, x_length, m_loadBlockInfo.getNumberOfChannels()));
  // Set the units on the workspace to TOF & Counts
  local_workspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  local_workspace->setYUnit("Counts");

  // Load instrument and other data once then copy it later
  m_progress->report("Loading instrument and run details");

  // load run details
  loadRunDetails(local_workspace, entry);

  // Test if IDF exists in Nexus otherwise load default instrument
  bool foundInstrument = LoadEventNexus::runLoadIDFFromNexus(m_filename, local_workspace, "raw_data_1", this);
  if (m_load_selected_spectra)
    m_spec2det_map = SpectrumDetectorMapping(spec(), udet(), udet.dim0());
  else if (bseparateMonitors) {
    m_spec2det_map = SpectrumDetectorMapping(spec(), udet(), udet.dim0());
    local_workspace->updateSpectraUsing(m_spec2det_map);
  } else {
    local_workspace->updateSpectraUsing(SpectrumDetectorMapping(spec(), udet(), udet.dim0()));
  }

  if (!foundInstrument) {
    runLoadInstrument(local_workspace);
  }

  // Load logs and sample information
  m_nexusFile->openPath(entry.path());
  local_workspace->loadSampleAndLogInfoNexus(m_nexusFile.get());

  // Load logs and sample information further information... See maintenance
  // ticket #8697
  loadSampleData(local_workspace, entry);
  m_progress->report("Loading logs");
  loadLogs(local_workspace);

  // Load first period outside loop
  m_progress->report("Loading data");
  // Get X Data
  if (ndets > 0) {
    m_tof_data = LoadISISNexusHelper::loadTimeData(entry);
  }

  int64_t firstentry = (m_entrynumber > 0) ? m_entrynumber : 1;
  loadPeriodData(firstentry, entry, local_workspace, m_load_selected_spectra);

  // Clone the workspace at this point to provide a base object for future
  // workspace generation.
  DataObjects::Workspace2D_sptr period_free_workspace =
      std::dynamic_pointer_cast<DataObjects::Workspace2D>(WorkspaceFactory::Instance().create(local_workspace));

  createPeriodLogs(firstentry, local_workspace);

  WorkspaceGroup_sptr wksp_group(new WorkspaceGroup);
  if (m_loadBlockInfo.getNumberOfPeriods() > 1 && m_entrynumber == 0) {
    wksp_group->setTitle(local_workspace->getTitle());

    // This forms the name of the group
    const std::string base_name = getPropertyValue("OutputWorkspace") + "_";
    const std::string prop_name = "OutputWorkspace_";

    for (int p = 1; p <= m_loadBlockInfo.getNumberOfPeriods(); ++p) {
      std::ostringstream os;
      os << p;
      m_progress->report("Loading period " + os.str());
      if (p > 1) {
        local_workspace = std::dynamic_pointer_cast<DataObjects::Workspace2D>(
            WorkspaceFactory::Instance().create(period_free_workspace));
        loadPeriodData(p, entry, local_workspace, m_load_selected_spectra);
        createPeriodLogs(p, local_workspace);
        // Check consistency of logs data for multi-period workspaces and raise
        // warnings where necessary.
        validateMultiPeriodLogs(local_workspace);
      }
      declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(prop_name + os.str(), base_name + os.str(),
                                                                     Direction::Output));
      wksp_group->addWorkspace(local_workspace);
      setProperty(prop_name + os.str(), std::static_pointer_cast<Workspace>(local_workspace));
    }
    // The group is the root property value
    setProperty("OutputWorkspace", std::dynamic_pointer_cast<Workspace>(wksp_group));
  } else {
    setProperty("OutputWorkspace", std::dynamic_pointer_cast<Workspace>(local_workspace));
  }

  //***************************************************************************************************
  // Workspace or group of workspaces without monitors is loaded. Now we are
  // loading monitors separately.
  if (bseparateMonitors) {
    std::string wsName = getPropertyValue("OutputWorkspace");
    if (m_monBlockInfo.getNumberOfSpectra() > 0) {
      x_length = m_monBlockInfo.getNumberOfChannels() + 1;
      // reset the size of the period free workspace to the monitor size
      period_free_workspace = std::dynamic_pointer_cast<DataObjects::Workspace2D>(WorkspaceFactory::Instance().create(
          period_free_workspace, m_monBlockInfo.getNumberOfSpectra(), x_length, m_monBlockInfo.getNumberOfChannels()));
      auto monitor_workspace = std::dynamic_pointer_cast<DataObjects::Workspace2D>(
          WorkspaceFactory::Instance().create(period_free_workspace));

      m_spectraBlocks.clear();
      m_wsInd2specNum_map.clear();
      // at the moment here we clear this map to enable possibility to load
      // monitors from the spectra block (wiring table bug).
      // if monitor's spectra present in the detectors block due to this bug
      // should be read from monitors, this map should be dealt with properly.
      buildSpectraInd2SpectraNumMap(true /*hasRange*/, false /*hasSpectraList*/, m_monBlockInfo);

      // lo
      prepareSpectraBlocks(m_monitors, m_monBlockInfo);

      firstentry = (m_entrynumber > 0) ? m_entrynumber : 1;
      loadPeriodData(firstentry, entry, monitor_workspace, true);
      local_workspace->setMonitorWorkspace(monitor_workspace);

      ISISRunLogs monLogCreator(monitor_workspace->run());
      monLogCreator.addPeriodLogs(1, monitor_workspace->mutableRun());

      const std::string monitorPropBase = "MonitorWorkspace";
      const std::string monitorWsNameBase = wsName + "_monitors";
      if (m_detBlockInfo.getNumberOfPeriods() > 1 && m_entrynumber == 0) {
        WorkspaceGroup_sptr monitor_group(new WorkspaceGroup);
        monitor_group->setTitle(monitor_workspace->getTitle());

        for (int p = 1; p <= m_detBlockInfo.getNumberOfPeriods(); ++p) {
          std::ostringstream os;
          os << "_" << p;
          m_progress->report("Loading period " + os.str());
          if (p > 1) {
            monitor_workspace = std::dynamic_pointer_cast<DataObjects::Workspace2D>(
                WorkspaceFactory::Instance().create(period_free_workspace));
            loadPeriodData(p, entry, monitor_workspace, m_load_selected_spectra);
            monLogCreator.addPeriodLogs(p, monitor_workspace->mutableRun());
            // Check consistency of logs data for multi-period workspaces and
            // raise
            // warnings where necessary.
            validateMultiPeriodLogs(monitor_workspace);
            auto data_ws = std::static_pointer_cast<API::MatrixWorkspace>(wksp_group->getItem(p - 1));
            data_ws->setMonitorWorkspace(monitor_workspace);
          }
          declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
              monitorPropBase + os.str(), monitorWsNameBase + os.str(), Direction::Output));
          monitor_group->addWorkspace(monitor_workspace);
          setProperty(monitorPropBase + os.str(), std::static_pointer_cast<Workspace>(monitor_workspace));
        }
        // The group is the root property value
        declareProperty(
            std::make_unique<WorkspaceProperty<Workspace>>(monitorPropBase, monitorWsNameBase, Direction::Output));
        setProperty(monitorPropBase, std::dynamic_pointer_cast<Workspace>(monitor_group));

      } else {
        declareProperty(
            std::make_unique<WorkspaceProperty<Workspace>>(monitorPropBase, monitorWsNameBase, Direction::Output));
        setProperty(monitorPropBase, std::static_pointer_cast<Workspace>(monitor_workspace));
      }
    } else {
      g_log.information() << " no monitors to load for workspace: " << wsName << '\n';
    }
  }

  // Clear off the member variable containers
  m_tof_data.reset();
  m_spec.clear();
  m_monitors.clear();
  m_wsInd2specNum_map.clear();
}
/**
Check for a set of synthetic logs associated with multi-period log data. Raise
warnings where necessary.
*/
void LoadISISNexus2::validateMultiPeriodLogs(const Mantid::API::MatrixWorkspace_sptr &ws) {
  const Run &run = ws->run();
  if (!run.hasProperty("current_period")) {
    g_log.warning("Workspace has no current_period log.");
  }
  if (!run.hasProperty("nperiods")) {
    g_log.warning("Workspace has no nperiods log");
  }
  if (!run.hasProperty("proton_charge_by_period")) {
    g_log.warning("Workspace has not proton_charge_by_period log");
  }
}

/**
 * Check the validity of the optional properties of the algorithm and identify
 * if partial data should be loaded.
 * @param bseparateMonitors: flag indicating if the monitors are to be loaded
 * separately
 * @param bexcludeMonitor: flag indicating if the monitors are to be excluded
 */
bool LoadISISNexus2::checkOptionalProperties(bool bseparateMonitors, bool bexcludeMonitor) {
  // optional properties specify that only some spectra have to be loaded
  bool range_supplied(false);

  // Get the spectrum selection which were specfied by the user
  specnum_t spec_min = getProperty("SpectrumMin");
  specnum_t spec_max = getProperty("SpectrumMax");

  // If spearate monitors or excluded monitors is selected then we
  // need to build up a wsIndex to spectrum number map as well,
  // since we cannot rely on contiguous blocks of detectors
  if (bexcludeMonitor || bseparateMonitors) {
    m_load_selected_spectra = true;
  }

  if (spec_min == 0)
    spec_min = m_loadBlockInfo.getMinSpectrumID();
  else {
    range_supplied = true;
    m_load_selected_spectra = true;
  }

  if (spec_max == EMPTY_INT())
    spec_max = m_loadBlockInfo.getMaxSpectrumID();
  else {
    range_supplied = true;
    m_load_selected_spectra = true;
  }

  // Sanity check for min/max
  if (spec_min > spec_max) {
    throw std::invalid_argument("Inconsistent range properties. SpectrumMin is "
                                "larger than SpectrumMax.");
  }

  if (spec_max > m_loadBlockInfo.getMaxSpectrumID()) {
    std::string err = "Inconsistent range property. SpectrumMax is larger than number of "
                      "spectra: " +
                      std::to_string(m_loadBlockInfo.getMaxSpectrumID());
    throw std::invalid_argument(err);
  }

  // Check the entry number
  m_entrynumber = getProperty("EntryNumber");
  if (static_cast<int>(m_entrynumber) > m_loadBlockInfo.getNumberOfPeriods() || m_entrynumber < 0) {
    std::string err = "Invalid entry number entered. File contains " +
                      std::to_string(m_loadBlockInfo.getNumberOfPeriods()) + " period. ";
    throw std::invalid_argument(err);
  }

  if (m_loadBlockInfo.getNumberOfPeriods() == 1) {
    m_entrynumber = 1;
  }

  // Did the user provide a spectrum list
  std::vector<specnum_t> spec_list = getProperty("SpectrumList");
  auto hasSpecList = false;

  if (!spec_list.empty()) {
    m_load_selected_spectra = true;

    // Sort the list so that we can check it's range
    std::sort(spec_list.begin(), spec_list.end());

    // Check if the spectra list entries are outside of the bounds
    // If we load the monitors separately, then we need to make sure that we
    // take them into account
    bool isSpectraListTooLarge;
    bool isSpectraListTooSmall;
    auto maxLoadBlock = m_loadBlockInfo.getMaxSpectrumID();
    auto minLoadBlock = m_loadBlockInfo.getMinSpectrumID();
    if (bseparateMonitors) {
      auto maxMonBlock = m_monBlockInfo.getMaxSpectrumID();
      auto minMonBlock = m_monBlockInfo.getMinSpectrumID();
      isSpectraListTooLarge = spec_list.back() > std::max(maxMonBlock, maxLoadBlock);
      isSpectraListTooSmall = spec_list.front() < std::min(minMonBlock, minLoadBlock);

    } else {
      isSpectraListTooLarge = spec_list.back() > maxLoadBlock;
      isSpectraListTooSmall = spec_list.front() < minLoadBlock;
    }

    if (isSpectraListTooLarge) {
      std::string err = "The specified spectrum list contains a spectrum number which is "
                        "larger "
                        "than the largest loadable spectrum number for your selection of "
                        "excluded/included/separate monitors. The largest loadable "
                        "spectrum number is " +
                        std::to_string(m_loadBlockInfo.getMinSpectrumID());
      throw std::invalid_argument(err);
    }
    if (isSpectraListTooSmall) {
      std::string err = "The specified spectrum list contains a spectrum number which is "
                        "smaller "
                        "than the smallest loadable spectrum number for your selection of "
                        "excluded/included/separate monitors. The smallest loadable "
                        "spectrum number is " +
                        std::to_string(m_loadBlockInfo.getMinSpectrumID());
      throw std::invalid_argument(err);
    }

    // The users can provide a spectrum list and and a spectrum range. Handle
    // this here.
    if (range_supplied) {
      // First remove all entries which are inside of the min and max spectrum,
      // to avoid duplicates
      auto isInRange = [&spec_min, &spec_max](specnum_t x) { return (spec_min <= x) && (x <= spec_max); };

      spec_list.erase(remove_if(spec_list.begin(), spec_list.end(), isInRange), spec_list.end());

      // The spec_min - spec_max range needs to be added to the spec list
      for (auto i = spec_min; i < spec_max + 1; ++i) {
        spec_list.emplace_back(i);
      }
      std::sort(spec_list.begin(), spec_list.end());
    }

    auto monitorSpectra = m_monBlockInfo.getAllSpectrumNumbers();
    // Create DataBlocks from the spectrum list
    DataBlockComposite composite;
    populateDataBlockCompositeWithContainer(composite, spec_list, spec_list.size(),
                                            m_loadBlockInfo.getNumberOfPeriods(), m_loadBlockInfo.getNumberOfChannels(),
                                            monitorSpectra);

    // If the monitors are to be loaded separately, then we have
    // to remove them at this point, but we also have to check if the
    if (bexcludeMonitor || bseparateMonitors) {
      auto newMonitors = getMonitorsFromComposite(composite, m_monBlockInfo);
      composite.removeSpectra(m_monBlockInfo);

      // This is important. If there are no monitors which were specifically
      // selected,
      // then we load the full monitor range, else respect the selection.
      if (!newMonitors.isEmpty()) {
        m_monBlockInfo = newMonitors;
      }

      // Handle case where the composite is empty since it only contained
      // monitors, but we want to load the monitors sepearately. In this case we
      // should set the loadBlock to the selected monitors.
      if (bseparateMonitors && composite.isEmpty()) {
        composite = m_monBlockInfo;
        bseparateMonitors = false;
      }
    }

    m_loadBlockInfo = composite;

    hasSpecList = true;
  } else {
    // At this point we don't have a spectrum list but there might have been a
    // spectrum range which we need to take into account, by truncating
    // the current range. If we load the monitors separately, we need to
    // truncate them as well (provided they are affected)
    if (range_supplied) {
      m_loadBlockInfo.truncate(spec_min, spec_max);

      auto new_monitors = m_monBlockInfo;
      new_monitors.truncate(spec_min, spec_max);
      m_monBlockInfo = new_monitors;
    }
  }

  if (m_load_selected_spectra) {
    buildSpectraInd2SpectraNumMap(range_supplied, hasSpecList, m_loadBlockInfo);
  }

  // Check that the load blocks contain anything at all.
  if (m_loadBlockInfo.isEmpty()) {
    throw std::invalid_argument("Your spectrum number selection was not valid. "
                                "Make sure that you select spectrum numbers "
                                "and ranges which are compatible with your "
                                "selection of excluded/included/separate monitors. ");
  }

  return bseparateMonitors;
}

/**
Build the list of spectra to load and include into spectra-detectors map.
The map should be built if the user either specified a range or if the user
provided a list of spectrum numbers.
@param range_supplied: if true specifies that the range of values provided
below have to be processed rather then spectra list
@param hasSpectraList: did the user specify a spectrum list
@param dataBlockComposite: a data block composite specfiying the spectra
intervals
**/
void LoadISISNexus2::buildSpectraInd2SpectraNumMap(bool range_supplied, bool hasSpectraList,
                                                   DataBlockComposite &dataBlockComposite) {

  if (range_supplied || hasSpectraList || true) {
    auto generator = dataBlockComposite.getGenerator();
    int64_t hist = 0;
    for (; !generator->isDone(); generator->next()) {
      auto spec_num = static_cast<specnum_t>(generator->getValue());
      m_wsInd2specNum_map.emplace(hist, spec_num);
      ++hist;
    }
  }
}

/**
 * Analyze the spectra ranges and prepare a list contiguous blocks. Each monitor
 * must be
 * in a separate block.
 * @return :: Number of spectra to load.
 */
size_t LoadISISNexus2::prepareSpectraBlocks(std::map<specnum_t, std::string> &monitors, DataBlockComposite &LoadBlock) {
  std::vector<specnum_t> includedMonitors;
  // Setup the SpectraBlocks based on the DataBlocks
  auto dataBlocks = LoadBlock.getDataBlocks();
  auto isMonitor = [&monitors](specnum_t spectrumNumber) { return monitors.find(spectrumNumber) != monitors.end(); };
  for (const auto &dataBlock : dataBlocks) {
    auto min = dataBlock.getMinSpectrumID();
    if (isMonitor(min)) {
      m_spectraBlocks.emplace_back(SpectraBlock(min, min, true, monitors.find(min)->second));
      includedMonitors.emplace_back(min);
    } else {
      auto max = dataBlock.getMaxSpectrumID();
      m_spectraBlocks.emplace_back(min, max, false, "");
    }
  }

  // sort and check for overlapping
  if (m_spectraBlocks.size() > 1) {
    std::sort(m_spectraBlocks.begin(), m_spectraBlocks.end(),
              [](const LoadISISNexus2::SpectraBlock &block1, const LoadISISNexus2::SpectraBlock &block2) {
                return block1.last < block2.first;
              });
    checkOverlappingSpectraRange();
  }

  // Remove monitors that have been used.
  auto allMonitorsIncluded = monitors.size() == includedMonitors.size();
  if (!includedMonitors.empty() && !allMonitorsIncluded) {
    for (auto it = monitors.begin(); it != monitors.end();) {
      if (std::find(includedMonitors.begin(), includedMonitors.end(), it->first) != includedMonitors.end()) {
        auto it1 = it;
        ++it;
        monitors.erase(it1);
      } else {
        ++it;
      }
    }
  }

  // Count the number of spectra.
  const auto nSpec = std::accumulate(
      m_spectraBlocks.cbegin(), m_spectraBlocks.cend(), static_cast<size_t>(0),
      [](size_t sum, const auto &spectraBlock) { return sum + spectraBlock.last - spectraBlock.first + 1; });
  return nSpec;
}

/**
 * Check if any spectra block ranges overlap.
 *
 * Iterate over the sorted list of spectra blocks and check
 * if the last element of the preceeding block is less than
 * the first element of the next block.
 */
void LoadISISNexus2::checkOverlappingSpectraRange() {
  for (size_t i = 1; i < m_spectraBlocks.size(); ++i) {
    const auto &block1 = m_spectraBlocks[i - 1];
    const auto &block2 = m_spectraBlocks[i];
    if (block1.first > block1.last && block2.first > block2.last)
      throw std::runtime_error("LoadISISNexus2: inconsistent spectra ranges");
    if (block1.last >= block2.first) {
      throw std::runtime_error("LoadISISNexus2: the range of SpectraBlocks must not overlap");
    }
  }
}

/**
 * Load a given period into the workspace
 * @param period :: The period number to load (starting from 1)
 * @param entry :: The opened root entry node for accessing the monitor and data
 * nodes
 * @param local_workspace :: The workspace to place the data in
 * @param update_spectra2det_mapping :: reset spectra-detector map to the one
 * calculated earlier. (Warning! -- this map has to be calculated correctly!)
 */
void LoadISISNexus2::loadPeriodData(int64_t period, NXEntry &entry, DataObjects::Workspace2D_sptr &local_workspace,
                                    bool update_spectra2det_mapping) {
  int64_t hist_index = 0;
  int64_t period_index(period - 1);

  for (const auto &spectraBlock : m_spectraBlocks) {
    if (spectraBlock.isMonitor) {
      NXData monitor = entry.openNXData(spectraBlock.monName);
      NXInt mondata = monitor.openIntData();
      m_progress->report("Loading monitor");
      mondata.load(1, static_cast<int>(period - 1)); // TODO this is just wrong
      NXFloat timeBins = monitor.openNXFloat("time_of_flight");
      timeBins.load();
      local_workspace->setHistogram(hist_index, BinEdges(timeBins(), timeBins() + timeBins.dim0()),
                                    Counts(mondata(), mondata() + m_monBlockInfo.getNumberOfChannels()));

      if (update_spectra2det_mapping) {
        auto &spec = local_workspace->getSpectrum(hist_index);
        specnum_t specNum = m_wsInd2specNum_map.at(hist_index);
        spec.setDetectorIDs(m_spec2det_map.getDetectorIDsForSpectrumNo(specNum));
        spec.setSpectrumNo(specNum);
      }
      hist_index++;
    } else if (m_have_detector) {
      NXData nxdata = entry.openNXData("detector_1");
      NXDataSetTyped<int> data = nxdata.openIntData();
      data.open();
      // Start with the list members that are lower than the required spectrum
      const int *const spec_begin = m_spec.data();
      // When reading in blocks we need to be careful that the range is exactly
      // divisible by the block-size
      // and if not have an extra read of the left overs
      const int64_t blocksize = 8;
      const int64_t rangesize = spectraBlock.last - spectraBlock.first + 1;
      const int64_t fullblocks = rangesize / blocksize;
      int64_t spectra_no = spectraBlock.first;

      // For this to work correctly, we assume that the spectrum list increases
      // monotonically
      int64_t filestart = std::lower_bound(spec_begin, m_spec_end, spectra_no) - spec_begin;
      if (fullblocks > 0) {
        for (int64_t i = 0; i < fullblocks; ++i) {
          loadBlock(data, blocksize, period_index, filestart, hist_index, spectra_no, local_workspace);
          filestart += blocksize;
        }
      }
      int64_t finalblock = rangesize - (fullblocks * blocksize);
      if (finalblock > 0) {
        loadBlock(data, finalblock, period_index, filestart, hist_index, spectra_no, local_workspace);
      }
    }
  }

  try {
    const std::string title = entry.getString("title");
    local_workspace->setTitle(title);
    // write the title into the log file (run object)
    local_workspace->mutableRun().addProperty("run_title", title, true);
  } catch (std::runtime_error &) {
    g_log.debug() << "No title was found in the input file, " << getPropertyValue("Filename") << '\n';
  }

  std::string notes = "";
  try {
    notes = entry.getString("notes");
  } catch (std::runtime_error &) {
    // if no notes, add empty string
  }
  local_workspace->setComment(notes);
}

/**
 * Creates period log data in the workspace
 * @param period :: period number
 * @param local_workspace :: workspace to add period log data to.
 */
void LoadISISNexus2::createPeriodLogs(int64_t period, DataObjects::Workspace2D_sptr &local_workspace) {
  m_logCreator->addPeriodLogs(static_cast<int>(period), local_workspace->mutableRun());
}

/**
 * Perform a call to nxgetslab, via the NexusClasses wrapped methods for a given
 * block-size
 * @param data :: The NXDataSet object
 * @param blocksize :: The block-size to use
 * @param period :: The period number
 * @param start :: The index within the file to start reading from (zero based)
 * @param hist :: The workspace index to start reading into
 * @param spec_num :: The spectrum number that matches the hist variable
 * @param local_workspace :: The workspace to fill the data with
 */
void LoadISISNexus2::loadBlock(NXDataSetTyped<int> &data, int64_t blocksize, int64_t period, int64_t start,
                               int64_t &hist, int64_t &spec_num, DataObjects::Workspace2D_sptr &local_workspace) {
  data.load(static_cast<int>(blocksize), static_cast<int>(period),
            static_cast<int>(start)); // TODO this is just wrong
  int *data_start = data();
  int *data_end = data_start + m_loadBlockInfo.getNumberOfChannels();
  int64_t final(hist + blocksize);
  while (hist < final) {
    m_progress->report("Loading data");
    local_workspace->setHistogram(hist, BinEdges(m_tof_data), Counts(data_start, data_end));
    data_start += m_detBlockInfo.getNumberOfChannels();
    data_end += m_detBlockInfo.getNumberOfChannels();
    if (m_load_selected_spectra) {
      auto &spec = local_workspace->getSpectrum(hist);
      specnum_t specNum = m_wsInd2specNum_map.at(hist);
      // set detectors corresponding to spectra Number
      spec.setDetectorIDs(m_spec2det_map.getDetectorIDsForSpectrumNo(specNum));
      // set correct spectra Number
      spec.setSpectrumNo(specNum);
    }

    ++hist;
    ++spec_num;
  }
}

/// Run the Child Algorithm LoadInstrument (or LoadInstrumentFromNexus)
void LoadISISNexus2::runLoadInstrument(DataObjects::Workspace2D_sptr &localWorkspace) {

  auto loadInst = createChildAlgorithm("LoadInstrument");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  bool executionSuccessful(true);
  try {
    loadInst->setPropertyValue("InstrumentName", m_instrument_name);
    loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", localWorkspace);
    loadInst->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(false));
    loadInst->execute();
  } catch (std::invalid_argument &) {
    g_log.information("Invalid argument to LoadInstrument Child Algorithm");
    executionSuccessful = false;
  } catch (std::runtime_error &) {
    g_log.information("Unable to successfully run LoadInstrument Child Algorithm");
    executionSuccessful = false;
  }
  if (executionSuccessful) {
    // If requested update the instrument to positions in the data file
    const auto &pmap = localWorkspace->constInstrumentParameters();
    if (pmap.contains(localWorkspace->getInstrument()->getComponentID(), "det-pos-source")) {
      std::shared_ptr<Geometry::Parameter> updateDets =
          pmap.get(localWorkspace->getInstrument()->getComponentID(), "det-pos-source");
      std::string value = updateDets->value<std::string>();
      if (value.substr(0, 8) == "datafile") {
        auto updateInst = createChildAlgorithm("UpdateInstrumentFromFile");
        updateInst->setProperty<MatrixWorkspace_sptr>("Workspace", localWorkspace);
        updateInst->setPropertyValue("Filename", m_filename);
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
  }
}

/**
 * Load data about the run
 *   @param local_workspace :: The workspace to load the run information in to
 *   @param entry :: The Nexus entry
 */
void LoadISISNexus2::loadRunDetails(DataObjects::Workspace2D_sptr &local_workspace, NXEntry &entry) {

  API::Run &runDetails = local_workspace->mutableRun();

  // Data details on run not the workspace
  runDetails.addProperty("nspectra", static_cast<int>(m_loadBlockInfo.getNumberOfSpectra()));
  runDetails.addProperty("nchannels", static_cast<int>(m_loadBlockInfo.getNumberOfChannels()));
  runDetails.addProperty("nperiods", static_cast<int>(m_loadBlockInfo.getNumberOfPeriods()));

  LoadISISNexusHelper::loadRunDetails(runDetails, entry, m_hasVMSBlock);
}

/**
 * Load data about the sample
 *   @param local_workspace :: The workspace to load the logs to.
 *   @param entry :: The Nexus entry
 */
void LoadISISNexus2::loadSampleData(DataObjects::Workspace2D_sptr &local_workspace, const NXEntry &entry) {

  // load sample geometry - Id and dimensions
  LoadISISNexusHelper::loadSampleGeometry(local_workspace->mutableSample(), entry, m_hasVMSBlock);
  g_log.debug() << "Sample geometry -  ID: " << local_workspace->mutableSample().getGeometryFlag()
                << ", thickness: " << local_workspace->mutableSample().getThickness()
                << ", height: " << local_workspace->mutableSample().getHeight()
                << ", width: " << local_workspace->mutableSample().getWidth() << "\n";
}

/**  Load logs from Nexus file. Logs are expected to be in
 *   /raw_data_1/runlog group of the file. Call to this method must be done
 *   within /raw_data_1 group.
 *   @param ws :: The workspace to load the logs to.
 */
void LoadISISNexus2::loadLogs(DataObjects::Workspace2D_sptr &ws) {
  auto alg = createChildAlgorithm("LoadNexusLogs", 0.0, 0.5);
  alg->setPropertyValue("Filename", this->getProperty("Filename"));
  alg->setProperty<MatrixWorkspace_sptr>("Workspace", ws);
  try {
    alg->executeAsChildAlg();
  } catch (std::runtime_error &) {
    g_log.warning() << "Unable to load run logs. There will be no log "
                    << "data associated with this workspace\n";
    return;
  }

  // Populate the instrument parameters.
  ws->populateInstrumentParameters();

  // Make log creator object and add the run status log
  m_logCreator.reset(new ISISRunLogs(ws->run()));
  m_logCreator->addStatusLog(ws->mutableRun());
}

double LoadISISNexus2::dblSqrt(double in) { return sqrt(in); }
/**Method takes input parameters which describe  monitor loading and analyze
 *them against spectra/monitor block information in the file.
 * The result is the option if monitors can  be loaded together with spectra or
 *mast be treated separately
 * and additional information on how to treat monitor spectra.
 *
 *@param entry                :: entry to the NeXus file, opened at root folder
 *@param spectrum_index       :: array of spectra indexes of the data present in
 *the file
 *@param ndets                :: size of the spectrum index array
 *@param n_vms_compat_spectra :: number of data entries containing common time
 *bins (e.g. all spectra, or all spectra and monitors or some spectra (this is
 *not fully supported)
 *@param monitors             :: map connecting monitor spectra ID against
 *monitor group name in the file.
 *@param excludeMonitors      :: input property indicating if it is requested to
 *exclude monitors from the target workspace
 *@param separateMonitors     :: input property indicating if it is requested to
 *load monitors separately (and exclude them from target data workspace this
 *way)
 *@return excludeMonitors     :: indicator if monitors should or must be
 *excluded from the main data workspace if they can not be loaded with the data
 *                               (contain different number of time channels)
 *
 */
bool LoadISISNexus2::findSpectraDetRangeInFile(NXEntry &entry, std::vector<specnum_t> &spectrum_index, int64_t ndets,
                                               int64_t n_vms_compat_spectra, std::map<specnum_t, std::string> &monitors,
                                               bool excludeMonitors, bool separateMonitors) {
  size_t nmons = monitors.size();

  if (nmons > 0) {
    NXInt chans = entry.openNXInt(m_monitors.begin()->second + "/data");

    // Iterate over each monitor and create a data block for each monitor
    for (const auto &monitor : monitors) {
      auto monID = monitor.first;
      auto monTemp = DataBlock(chans);
      monTemp.setMinSpectrumID(monID);
      monTemp.setMaxSpectrumID(monID);
      m_monBlockInfo.addDataBlock(monTemp);
    }

    // at this stage we assume that the only going to load monitors
    m_loadBlockInfo = m_monBlockInfo;
  }

  // Check if the there are only monitors in the workspace, in which
  // case the monitors are loaded as the main workspace, ie not as
  // a separate workspace.
  if (ndets == 0) {
    separateMonitors = false;
    return separateMonitors;
  }

  // There are detectors present. The spectrum_index array contains
  // all available spectra of detectors, but these indices might
  // not be contiguous.
  NXData nxData = entry.openNXData("detector_1");
  NXInt data = nxData.openIntData();

  auto monitorSpectra = m_monBlockInfo.getAllSpectrumNumbers();
  populateDataBlockCompositeWithContainer(m_detBlockInfo, spectrum_index, ndets, data.dim0() /*Number of Periods*/,
                                          data.dim2() /*Number of channels*/, monitorSpectra);

  // We should handle legacy files which include the spectrum number of the
  // monitors
  // in the detector group ("raw_data_1/detector_1/spectrum_index")
  // Simple try to remove the monitors. If they are not included nothing should
  // happen
  m_detBlockInfo.removeSpectra(m_monBlockInfo);

  m_loadBlockInfo = m_detBlockInfo;

  // Check what is actually going or can be loaded
  bool removeMonitors = excludeMonitors || separateMonitors;

  // If the monitors are to be pulled into the same workspace as the detector
  // information,
  // then the number of periods and the number of channels has to conincide
  if (nmons > 0 && ((m_detBlockInfo.getNumberOfPeriods() != m_monBlockInfo.getNumberOfPeriods()) ||
                    (m_detBlockInfo.getNumberOfChannels() != m_monBlockInfo.getNumberOfChannels()))) {
    if (!removeMonitors) {
      g_log.warning() << " Performing separate loading as can not load spectra "
                         "and monitors in the single workspace:\n";
      g_log.warning() << " Monitors data contain :" << m_monBlockInfo.getNumberOfChannels()
                      << " time channels and: " << m_monBlockInfo.getNumberOfPeriods() << " period(s)\n";
      g_log.warning() << " Spectra  data contain :" << m_detBlockInfo.getNumberOfChannels()
                      << " time channels and: " << m_detBlockInfo.getNumberOfPeriods() << " period(s)\n";
    }

    // Force the monitors to be removed and separate if the periods and channels
    // don't conincide
    // between monitors and detectors.
    separateMonitors = true;
    removeMonitors = true;
  }

  int64_t spectraID_min = std::min(m_monBlockInfo.getMinSpectrumID(), m_detBlockInfo.getMinSpectrumID());
  int64_t spectraID_max = std::max(m_monBlockInfo.getMaxSpectrumID(), m_detBlockInfo.getMaxSpectrumID());
  size_t totNumOfSpectra = m_monBlockInfo.getNumberOfSpectra() + m_detBlockInfo.getNumberOfSpectra();

  // In case we want to load everything into a one workspace, we should combine
  // the
  // the data blocks of the monitor and the detector
  if (!removeMonitors) {
    m_loadBlockInfo = m_detBlockInfo + m_monBlockInfo;
  }

  // If the monitors are to be loaded separately, then we set the loadblocks to
  // the detblocks,
  // since we want to deal with the detectors (the main workspace) first.
  if (separateMonitors)
    m_loadBlockInfo = m_detBlockInfo;

  // Perform a sanity check of the spectrum numbers
  if ((totNumOfSpectra != static_cast<size_t>(n_vms_compat_spectra)) ||
      (spectraID_max - spectraID_min + 1 != static_cast<int64_t>(n_vms_compat_spectra))) {
    // At this point we normally throw since there is a mismatch between the
    // number
    // spectra of the detectors+monitors and the entry in NSP1, but in the
    // case of multiple time regimes this comparison is not any longer valid.
    // Hence we only throw if the file does not correspond to a multiple time
    // regime file.
    if (!isMultipleTimeRegimeFile(entry)) {
      throw std::runtime_error("LoadISISNexus: There seems to be an "
                               "inconsistency in the spectrum numbers.");
    }
  }

  return separateMonitors;
}

/**
 * Determine if a file is a multiple time regime file. Note that for a true
 * multi-time regime file we need at least three time regime entries, since
 * two time regimes are handled by vms_compat.
 * @param entry a handle to the Nexus file
 * @return if the file has multiple time regimes or not
 */
bool LoadISISNexus2::isMultipleTimeRegimeFile(const NeXus::NXEntry &entry) const {
  auto hasMultipleTimeRegimes(false);
  try {
    NXClass instrument = entry.openNXGroup("instrument");
    NXClass dae = instrument.openNXGroup("dae");
    hasMultipleTimeRegimes = dae.containsGroup("time_channels_3");
  } catch (...) {
  }
  return hasMultipleTimeRegimes;
}

} // namespace Mantid::DataHandling
