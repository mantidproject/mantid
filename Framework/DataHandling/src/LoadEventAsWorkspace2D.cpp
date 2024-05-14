// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/LoadEventAsWorkspace2D.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidDataHandling/LoadEventNexusIndexSetup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidNexus/NexusIOHelper.h"

#include <regex>

namespace Mantid {
namespace DataHandling {
using namespace API;
using Mantid::DataObjects::Workspace2D;
using Mantid::HistogramData::HistogramX;
using Mantid::Kernel::Direction;
using Mantid::Kernel::PropertyWithValue;
using Mantid::Kernel::StringListValidator;
using Mantid::Kernel::TimeSeriesProperty;
using Mantid::Kernel::UnitFactory;
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadEventAsWorkspace2D)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadEventAsWorkspace2D::name() const { return "LoadEventAsWorkspace2D"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadEventAsWorkspace2D::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadEventAsWorkspace2D::category() const { return "DataHandling\\Nexus"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadEventAsWorkspace2D::summary() const {
  return "Load event data, integrating the events during loading. Also set the X-axis based on log data.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithms properties.
 */
void LoadEventAsWorkspace2D::init() {
  const std::vector<std::string> exts{".nxs.h5", ".nxs", "_event.nxs"};
  this->declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, exts),
                        "The name of the Event NeXus file to read, including its full or "
                        "relative path. ");
  declareProperty(std::make_unique<PropertyWithValue<double>>("FilterByTofMin", EMPTY_DBL(), Direction::Input),
                  "To exclude events that do not fall within a range "
                  "of times-of-flight. "
                  "This is the minimum accepted value in microseconds. Keep "
                  "blank to load all events.");
  declareProperty(std::make_unique<PropertyWithValue<double>>("FilterByTofMax", EMPTY_DBL(), Direction::Input),
                  "To exclude events that do not fall within a range "
                  "of times-of-flight. "
                  "This is the maximum accepted value in microseconds. Keep "
                  "blank to load all events.");
  declareProperty(std::make_unique<PropertyWithValue<std::vector<std::string>>>(
                      "LogAllowList", std::vector<std::string>(), Direction::Input),
                  "If specified, only these logs will be loaded from the file (each "
                  "separated by a space).");
  declareProperty(std::make_unique<PropertyWithValue<std::vector<std::string>>>(
                      "LogBlockList", std::vector<std::string>(), Direction::Input),
                  "If specified, these logs will NOT be loaded from the file (each "
                  "separated by a space).");
  declareProperty(std::make_unique<PropertyWithValue<std::string>>("XCenterLog", "wavelength", Direction::Input),
                  "Name of log to take to use as the X-bin center");
  declareProperty(std::make_unique<PropertyWithValue<std::string>>("XWidthLog", "wavelength_spread", Direction::Input),
                  "Name of log to take to use as the X-bin width");
  declareProperty(std::make_unique<PropertyWithValue<double>>("XCenter", EMPTY_DBL(), Direction::Input),
                  "Value to set X-bin center to which overrides XCenterLog");
  declareProperty(std::make_unique<PropertyWithValue<double>>("XWidth", EMPTY_DBL(), Direction::Input),
                  "Value to set X-bin width to which overrides XWidthLog");
  declareProperty("Units", "Wavelength",
                  std::make_shared<StringListValidator>(UnitFactory::Instance().getConvertibleUnits()),
                  "The name of the units to convert to (must be one of those registered in the Unit Factory)");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace2D>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
  declareProperty("LoadNexusInstrumentXML", true,
                  "If true, load the instrument definition file (IDF) from the input NeXus file. "
                  "If false, Mantid will load the most appropriate IDF from the instrument repository.");
}

std::map<std::string, std::string> LoadEventAsWorkspace2D::validateInputs() {
  std::map<std::string, std::string> results;

  const std::vector<std::string> allow_list = getProperty("LogAllowList");
  const std::vector<std::string> block_list = getProperty("LogBlockList");

  if (!allow_list.empty() && !block_list.empty()) {
    results["LogAllowList"] =
        "LogBlockList and LogAllowList are mutually exclusive. Please only enter values for one of these fields.";
    results["LogBlockList"] =
        "LogBlockList and LogAllowList are mutually exclusive. Please only enter values for one of these fields.";
  }

  const double tofMin = getProperty("FilterByTofMin");
  const double tofMax = getProperty("FilterByTofMax");

  if (tofMin != EMPTY_DBL() && tofMax != EMPTY_DBL()) {
    if (tofMin == EMPTY_DBL() || tofMax == EMPTY_DBL()) {
      results["FilterByTofMin"] = "You must specify both min & max or neither TOF filters";
      results["FilterByTofMax"] = "You must specify both min & max or neither TOF filters";
    } else if (tofMin >= tofMax) {
      results["FilterByTofMin"] = "FilterByTofMin must be less than FilterByTofMax";
      results["FilterByTofMax"] = "FilterByTofMax must be greater than FilterByTofMin";
    }
  }

  return results;
}
//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadEventAsWorkspace2D::exec() {
  std::string filename = getPropertyValue("Filename");

  auto prog = std::make_unique<Progress>(this, 0.0, 1.0, 6);

  // temporary workspace to load instrument and metadata
  MatrixWorkspace_sptr WS = WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
  // Load the logs
  prog->doReport("Loading logs");
  int nPeriods = 1;                                                               // Unused
  auto periodLog = std::make_unique<const TimeSeriesProperty<int>>("period_log"); // Unused
  LoadEventNexus::runLoadNexusLogs<MatrixWorkspace_sptr>(filename, WS, *this, false, nPeriods, periodLog,
                                                         getProperty("LogAllowList"), getProperty("LogBlockList"));

  if (nPeriods != 1)
    g_log.warning("This algorithm does not correctly handle period data");

  // set center and width parameters, do it before we try to load the data so if the log doesn't exist we fail fast
  double center = getProperty("XCenter");
  if (center == EMPTY_DBL())
    center = WS->run().getStatistics(getPropertyValue("XCenterLog")).mean;

  double width = getProperty("XWidth");
  if (width == EMPTY_DBL())
    width = WS->run().getStatistics(getPropertyValue("XWidthLog")).mean;
  width *= center;

  if (width == 0.) {
    std::string errmsg(
        "Width was calculated to be 0 (XCenter*XWidth). This will result in a invalid bin with zero width");
    g_log.error(errmsg);
    throw std::runtime_error(errmsg);
  }

  const Kernel::NexusHDF5Descriptor descriptor(filename);

  // Load the instrument
  prog->doReport("Loading instrument");
  LoadEventNexus::loadInstrument<MatrixWorkspace_sptr>(filename, WS, "entry", this, &descriptor);

  // load run metadata
  prog->doReport("Loading metadata");
  try {
    LoadEventNexus::loadEntryMetadata(filename, WS, "entry", descriptor);
  } catch (std::exception &e) {
    g_log.warning() << "Error while loading meta data: " << e.what() << '\n';
  }

  // create IndexInfo
  prog->doReport("Creating IndexInfo");
  const std::vector<int32_t> range;
  LoadEventNexusIndexSetup indexSetup(WS, EMPTY_INT(), EMPTY_INT(), range);
  auto indexInfo = indexSetup.makeIndexInfo();
  const size_t numHist = indexInfo.size();

  // make output workspace with correct number of histograms
  MatrixWorkspace_sptr outWS = WorkspaceFactory::Instance().create(WS, numHist, 2, 1);
  // set spectrum index information
  outWS->setIndexInfo(indexInfo);

  // now load the data
  const auto id_to_wi = outWS->getDetectorIDToWorkspaceIndexMap();
  detid_t min_detid = std::numeric_limits<detid_t>::max();
  detid_t max_detid = std::numeric_limits<detid_t>::min();

  for (const auto &entry : id_to_wi) {
    min_detid = std::min(min_detid, entry.first);
    max_detid = std::max(max_detid, entry.first);
  }

  const double tof_min = getProperty("FilterByTofMin");
  const double tof_max = getProperty("FilterByTofMax");
  const bool tof_filtering = (tof_min != EMPTY_DBL() && tof_max != EMPTY_DBL());

  // vector to stored to integrated counts by detector ID
  std::vector<uint32_t> Y(max_detid - min_detid + 1, 0);

  ::NeXus::File h5file(filename);

  h5file.openPath("/");
  h5file.openGroup("entry", "NXentry");

  // Now we want to go through all the bankN_event entries
  const std::map<std::string, std::set<std::string>> &allEntries = descriptor.getAllEntries();

  prog->doReport("Reading and integrating data");

  auto itClassEntries = allEntries.find("NXevent_data");

  if (itClassEntries != allEntries.end()) {

    const std::set<std::string> &classEntries = itClassEntries->second;
    const std::regex classRegex("(/entry/)([^/]*)");
    std::smatch groups;

    for (const std::string &classEntry : classEntries) {

      if (std::regex_match(classEntry, groups, classRegex)) {
        const std::string entry_name(groups[2].str());

        // skip entries with junk data
        if (entry_name == "bank_error_events" || entry_name == "bank_unmapped_events")
          continue;

        g_log.debug() << "Loading bank " << entry_name << '\n';
        h5file.openGroup(entry_name, "NXevent_data");

        std::vector<uint32_t> event_ids;

        if (descriptor.isEntry("/entry/" + entry_name + "/event_id", "SDS"))
          event_ids = Mantid::NeXus::NeXusIOHelper::readNexusVector<uint32_t>(h5file, "event_id");
        else
          event_ids = Mantid::NeXus::NeXusIOHelper::readNexusVector<uint32_t>(h5file, "event_pixel_id");

        std::vector<float> event_times;
        if (tof_filtering) {
          if (descriptor.isEntry("/entry/" + entry_name + "/event_time_offset", "SDS"))
            event_times = Mantid::NeXus::NeXusIOHelper::readNexusVector<float>(h5file, "event_time_offset");
          else
            event_times = Mantid::NeXus::NeXusIOHelper::readNexusVector<float>(h5file, "event_time_of_flight");
        }

        for (size_t i = 0; i < event_ids.size(); i++) {
          if (tof_filtering) {
            const auto tof = event_times[i];
            if (tof < tof_min || tof > tof_max)
              continue;
          }

          const detid_t det_id = event_ids[i];
          if (det_id < min_detid || det_id > max_detid)
            continue;

          Y[det_id - min_detid]++;
        }

        h5file.closeGroup();
      }
    }
  }

  h5file.closeGroup();
  h5file.close();

  // determine x values
  const auto xBins = {center - width / 2, center + width / 2};

  prog->doReport("Setting data to workspace");
  // set the data on the workspace
  const auto histX = Mantid::Kernel::make_cow<HistogramX>(xBins);
  for (detid_t detid = min_detid; detid <= max_detid; detid++) {
    const auto id_wi = id_to_wi.find(detid);
    if (id_wi == id_to_wi.end())
      continue;

    const auto wi = id_wi->second;
    const auto y = Y[detid - min_detid];

    outWS->mutableY(wi) = y;
    outWS->mutableE(wi) = sqrt(y);
    outWS->setSharedX(wi, histX);
  }

  // set units
  outWS->getAxis(0)->setUnit(getPropertyValue("Units"));
  outWS->setYUnit("Counts");

  // add filename
  outWS->mutableRun().addProperty("Filename", filename);
  setProperty("OutputWorkspace", outWS);
}

} // namespace DataHandling
} // namespace Mantid
