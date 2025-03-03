// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMuon/LoadMuonNexus2.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTimeHelpers.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/UnitLabelTypes.h"
#include "MantidLegacyNexus/NeXusException.hpp"
#include "MantidLegacyNexus/NeXusFile.hpp"
#include "MantidLegacyNexus/NexusClasses.h"

#include <Poco/Path.h>
#include <cmath>
#include <memory>
#include <numeric>

using Mantid::Types::Core::DateAndTime;

namespace {

/**  Implements NXlog Nexus class.
 */
class NXLog : public NXClass {
public:
  /**  Constructor.
   *   @param parent :: The parent Nexus class. In terms of HDF it is the group
   * containing the NXClass.
   *   @param name :: The name of the NXClass relative to its parent
   */
  NXLog(const NXClass &parent, const std::string &name) : NXClass(parent, name) {}
  /// Nexus class id
  std::string NX_class() const override { return "NXlog"; }

  /** createTimeSeries
   * Create a TimeSeries property form the records of the NXLog group. Times are
   * in dataset "time"
   * and the values are in dataset "value"
   * @param start_time :: If the "time" dataset does not have the "start"
   * attribute sets the
   *   start time for the series.
   * @param new_name :: If not empty it is used as the TimeSeries property name
   *   @return The property or NULL
   */
  Mantid::Kernel::Property *createTimeSeries(const std::string &start_time = "", const std::string &new_name = "") {
    const std::string &logName = new_name.empty() ? name() : new_name;
    NXInfo vinfo = getDataSetInfo("time");
    if (vinfo.type == NXnumtype::FLOAT64) {
      NXDouble times(*this, "time");
      times.openLocal();
      times.load();
      std::string units = times.attributes("units");
      if (units == "minutes") {
        using std::placeholders::_1;
        std::transform(times(), times() + times.dim0(), times(), std::bind(std::multiplies<double>(), _1, 60));
      } else if (!units.empty() && units.substr(0, 6) != "second") {
        return nullptr;
      }
      return parseTimeSeries(logName, times, start_time);
    } else if (vinfo.type == NXnumtype::FLOAT32) {
      NXFloat times(*this, "time");
      times.openLocal();
      times.load();
      std::string units = times.attributes("units");
      if (units == "minutes") {
        std::for_each(times(), times() + times.dim0(), [](float &val) { val *= 60.0f; });
      } else if (!units.empty() && units.substr(0, 6) != "second") {
        return nullptr;
      }
      return parseTimeSeries(logName, times, start_time);
    }

    return nullptr;
  }

private:
  /** Creates a single value property of the log
   * @returns A pointer to a newly created property wrapped around the log entry
   */
  Mantid::Kernel::Property *createSingleValueProperty() {
    const std::string valAttr("value");
    NXInfo vinfo = getDataSetInfo(valAttr);
    Mantid::Kernel::Property *prop;
    NXnumtype nxType = vinfo.type;
    if (nxType == NXnumtype::FLOAT64) {
      prop = new Mantid::Kernel::PropertyWithValue<double>(name(), getDouble(valAttr));
    } else if (nxType == NXnumtype::INT32) {
      prop = new Mantid::Kernel::PropertyWithValue<int>(name(), getInt(valAttr));
    } else if (nxType == NXnumtype::CHAR) {
      prop = new Mantid::Kernel::PropertyWithValue<std::string>(name(), getString(valAttr));
    } else if (nxType == NXnumtype::UINT8) {
      NXDataSetTyped<unsigned char> value(*this, valAttr);
      value.load();
      bool state = value[0] != 0;
      prop = new Mantid::Kernel::PropertyWithValue<bool>(name(), state);
    } else {
      prop = nullptr;
    }

    return prop;
  }
  /// Parse a time series
  template <class TYPE>
  Mantid::Kernel::Property *parseTimeSeries(const std::string &logName, const TYPE &times,
                                            const std::string &time0 = "") {
    std::string start_time = (!time0.empty()) ? time0 : times.attributes("start");
    if (start_time.empty()) {
      start_time = "2000-01-01T00:00:00";
    }
    auto start_t = Mantid::Kernel::DateAndTimeHelpers::createFromSanitizedISO8601(start_time);
    NXInfo vinfo = getDataSetInfo("value");
    if (!vinfo)
      return nullptr;

    if (vinfo.dims[0] != times.dim0())
      return nullptr;

    if (vinfo.type == NXnumtype::CHAR) {
      auto logv = new Mantid::Kernel::TimeSeriesProperty<std::string>(logName);
      NXChar value(*this, "value");
      value.openLocal();
      value.load();
      for (int i = 0; i < value.dim0(); i++) {
        auto t = start_t + boost::posix_time::seconds(int(times[i]));
        for (int j = 0; j < value.dim1(); j++) {
          char *c = &value(i, j);
          if (!isprint(*c))
            *c = ' ';
        }
        logv->addValue(t, std::string(value() + i * value.dim1(), value.dim1()));
      }
      return logv;
    } else if (vinfo.type == NXnumtype::FLOAT64) {
      if (logName.find("running") != std::string::npos || logName.find("period ") != std::string::npos) {
        auto logv = new Mantid::Kernel::TimeSeriesProperty<bool>(logName);
        NXDouble value(*this, "value");
        value.openLocal();
        value.load();
        for (int i = 0; i < value.dim0(); i++) {
          auto t = start_t + boost::posix_time::seconds(int(times[i]));
          logv->addValue(t, (value[i] == 0 ? false : true));
        }
        return logv;
      }
      NXDouble value(*this, "value");
      return loadValues<NXDouble, TYPE>(logName, value, start_t, times);
    } else if (vinfo.type == NXnumtype::FLOAT32) {
      NXFloat value(*this, "value");
      return loadValues<NXFloat, TYPE>(logName, value, start_t, times);
    } else if (vinfo.type == NXnumtype::INT32) {
      NXInt value(*this, "value");
      return loadValues<NXInt, TYPE>(logName, value, start_t, times);
    }
    return nullptr;
  }

  /// Loads the values in the log into the workspace
  ///@param logName :: the name of the log
  ///@param value :: the value
  ///@param start_t :: the start time
  ///@param times :: the array of time offsets
  ///@returns a property pointer
  template <class NX_TYPE, class TIME_TYPE>
  Mantid::Kernel::Property *loadValues(const std::string &logName, NX_TYPE &value,
                                       Mantid::Types::Core::DateAndTime start_t, const TIME_TYPE &times) {
    value.openLocal();
    auto logv = new Mantid::Kernel::TimeSeriesProperty<double>(logName);
    value.load();
    for (int i = 0; i < value.dim0(); i++) {
      if (i == 0 || value[i] != value[i - 1] || times[i] != times[i - 1]) {
        auto t = start_t + boost::posix_time::seconds(int(times[i]));
        logv->addValue(t, value[i]);
      }
    }
    return logv;
  }
};

// static method to create an NXlog from nexus
NXLog openNXLog(const NXClass &nxclass, const std::string &name) { return nxclass.openNXClass<NXLog>(name); }
} // namespace

namespace Mantid::Algorithms {

// Register the algorithm into the algorithm factory
DECLARE_LEGACY_NEXUS_FILELOADER_ALGORITHM(LoadMuonNexus2)

using namespace Kernel;
using namespace DateAndTimeHelpers;
using namespace API;
using Geometry::Instrument;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::Histogram;
using Mantid::Types::Core::DateAndTime;

LoadMuonNexus2::LoadMuonNexus2() : LoadMuonNexus() {}

/** Executes the right version of the Muon nexus loader
 *  @throw Exception::FileError If the Nexus file cannot be found/opened
 *  @throw std::invalid_argument If the optional properties are set to invalid
 *values
 */
void LoadMuonNexus2::exec() {
  // Create the root Nexus class
  NXRoot root(getPropertyValue("Filename"));

  int64_t iEntry = getProperty("EntryNumber");
  if (iEntry >= static_cast<int64_t>(root.groups().size())) {
    throw std::invalid_argument("EntryNumber is out of range");
  }

  // Open the data entry
  m_entry_name = root.groups()[iEntry].nxname;
  NXEntry entry = root.openEntry(m_entry_name);

  // Read in the instrument name from the Nexus file
  m_instrument_name = entry.getString("instrument/name");

  // Read the number of periods in this file
  if (entry.containsGroup("run")) {
    try {
      m_numberOfPeriods = entry.getInt("run/number_periods");
    } catch (LegacyNexus::Exception &) {
      // assume 1
      m_numberOfPeriods = 1;
    }
  } else {
    m_numberOfPeriods = 1;
  }

  // Need to extract the user-defined output workspace name
  const Property *ws = getProperty("OutputWorkspace");
  std::string localWSName = ws->value();
  // If multiperiod, will need to hold the Instrument & Sample for copying
  std::shared_ptr<Instrument> instrument;
  std::shared_ptr<Sample> sample;

  std::string detectorName;
  // Only the first NXdata found
  for (const auto &group : entry.groups()) {
    std::string className = group.nxclass;
    if (className == "NXdata") {
      detectorName = group.nxname;
      break;
    }
  }
  NXData dataGroup = entry.openNXData(detectorName);

  LegacyNexus::NXInt spectrum_index = dataGroup.openNXInt("spectrum_index");
  spectrum_index.load();
  m_numberOfSpectra = static_cast<specnum_t>(spectrum_index.dim0());

  // Load detector mapping
  const auto &detMapping = loadDetectorMapping(spectrum_index);

  // Call private method to validate the optional parameters, if set
  checkOptionalProperties();

  NXFloat raw_time = dataGroup.openNXFloat("raw_time");
  raw_time.load();
  auto nBins = raw_time.dim0();
  std::vector<double> timeBins;
  timeBins.assign(raw_time(), raw_time() + nBins);
  timeBins.emplace_back(raw_time[nBins - 1] + raw_time[1] - raw_time[0]);

  // Calculate the size of a workspace, given its number of periods & spectra to
  // read
  int total_specs;
  if (m_interval || m_list) {
    total_specs = static_cast<int>(m_spec_list.size());
    if (m_interval) {
      total_specs += static_cast<int>((m_spec_max - m_spec_min + 1));
    } else {
      m_spec_max = -1; // to stop entering the min - max loop
    }
  } else {
    total_specs = static_cast<int>(m_numberOfSpectra);
    // for nexus return all spectra
    m_spec_min = 1;
    m_spec_max = m_numberOfSpectra; // was +1?
  }

  // Create the 2D workspace for the output
  DataObjects::Workspace2D_sptr localWorkspace = std::dynamic_pointer_cast<DataObjects::Workspace2D>(
      WorkspaceFactory::Instance().create("Workspace2D", total_specs, nBins + 1, nBins));
  // Set the unit on the workspace to muon time, for now in the form of a Label
  // Unit
  std::shared_ptr<Kernel::Units::Label> lblUnit =
      std::dynamic_pointer_cast<Kernel::Units::Label>(UnitFactory::Instance().create("Label"));
  lblUnit->setLabel("Time", Units::Symbol::Microsecond);
  localWorkspace->getAxis(0)->unit() = lblUnit;
  // Set y axis unit
  localWorkspace->setYUnit("Counts");

  // g_log.error()<<" number of perioids= "<<m_numberOfPeriods<<'\n';
  WorkspaceGroup_sptr wsGrpSptr = WorkspaceGroup_sptr(new WorkspaceGroup);
  if (entry.containsDataSet("title")) {
    wsGrpSptr->setTitle(entry.getString("title"));
  }

  if (entry.containsDataSet("notes")) {
    wsGrpSptr->setComment(entry.getString("notes"));
  }

  if (m_numberOfPeriods > 1) {
    setProperty("OutputWorkspace", std::dynamic_pointer_cast<Workspace>(wsGrpSptr));
  }

  // period_index is currently unused
  // Mantid::NeXus::NXInt period_index = dataGroup.openNXInt("period_index");
  // period_index.load();

  Mantid::LegacyNexus::NXInt counts = dataGroup.openIntData();
  counts.load();

  NXInstrument instr = entry.openNXInstrument("instrument");

  if (instr.containsGroup("detector_fb")) {
    NXDetector detector = instr.openNXDetector("detector_fb");
    if (detector.containsDataSet("time_zero")) {
      double dum = detector.getFloat("time_zero");
      setProperty("TimeZero", dum);
    }
    if (detector.containsDataSet("first_good_time")) {
      double dum = detector.getFloat("first_good_time");
      setProperty("FirstGoodData", dum);
    }

    if (detector.containsDataSet("last_good_time")) {
      double dum = detector.getFloat("last_good_time");
      setProperty("LastGoodData", dum);
    }
  }

  API::Progress progress(this, 0.0, 1.0, m_numberOfPeriods * total_specs);
  // Loop over the number of periods in the Nexus file, putting each period in a
  // separate workspace
  for (int period = 0; period < m_numberOfPeriods; ++period) {

    if (period == 0) {
      // Only run the Child Algorithms once
      loadRunDetails(localWorkspace);
      runLoadInstrument(localWorkspace);
      loadLogs(localWorkspace, entry, period);
    } else // We are working on a higher period of a multiperiod raw file
    {
      localWorkspace =
          std::dynamic_pointer_cast<DataObjects::Workspace2D>(WorkspaceFactory::Instance().create(localWorkspace));
    }

    std::string outws;
    if (m_numberOfPeriods > 1) {
      std::string outputWorkspace = "OutputWorkspace";
      std::stringstream suffix;
      suffix << (period + 1);
      outws = outputWorkspace + "_" + suffix.str();
      std::string WSName = localWSName + "_" + suffix.str();
      declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(outws, WSName, Direction::Output));
      if (wsGrpSptr)
        wsGrpSptr->addWorkspace(localWorkspace);
    }

    // create spectrum -> index correspondence
    std::map<int, int> index_spectrum;
    for (int i = 0; i < m_numberOfSpectra; ++i) {
      index_spectrum[spectrum_index[i]] = i;
    }

    int wsIndex = 0;
    localWorkspace->mutableX(0) = timeBins;
    for (auto spec = static_cast<int>(m_spec_min); spec <= static_cast<int>(m_spec_max); ++spec) {
      int i = index_spectrum[spec]; // if spec not found i is 0
      localWorkspace->setHistogram(wsIndex, loadData(localWorkspace->binEdges(0), counts, period, i));
      localWorkspace->getSpectrum(wsIndex).setSpectrumNo(spectrum_index[i]);
      localWorkspace->getSpectrum(wsIndex).setDetectorIDs(detMapping.at(i));
      wsIndex++;
      progress.report();
    }

    // Read in the spectra in the optional list parameter, if set
    if (m_list) {
      for (auto spec : m_spec_list) {
        int k = index_spectrum[spec]; // if spec not found k is 0
        localWorkspace->setHistogram(wsIndex, loadData(localWorkspace->binEdges(0), counts, period, k));
        localWorkspace->getSpectrum(wsIndex).setSpectrumNo(spectrum_index[k]);
        localWorkspace->getSpectrum(wsIndex).setDetectorIDs(detMapping.at(k));
        wsIndex++;
        progress.report();
      }
    }
    // Just a sanity check
    assert(wsIndex == total_specs);

    bool autogroup = getProperty("AutoGroup");

    if (autogroup) {
      g_log.warning("Autogrouping is not implemented for muon NeXus version 2 files");
    }

    // Assign the result to the output workspace property
    if (m_numberOfPeriods > 1)
      setProperty(outws, std::static_pointer_cast<Workspace>(localWorkspace));
    else {
      setProperty("OutputWorkspace", std::dynamic_pointer_cast<Workspace>(localWorkspace));
    }

  } // loop over periods
}

/** loadData
 *  Load the counts data from an NXInt into a workspace
 */
Histogram LoadMuonNexus2::loadData(const BinEdges &edges, const Mantid::LegacyNexus::NXInt &counts, int period,
                                   int spec) {
  int64_t nBins = 0;
  const int *data = nullptr;

  if (counts.rank() == 3) {
    nBins = counts.dim2();
    data = &counts(period, spec, 0);
  } else if (counts.rank() == 2) {
    nBins = counts.dim1();
    data = &counts(spec, 0);
  } else {
    throw std::runtime_error("Data have unsupported dimensionality");
  }

  return Histogram(edges, Counts(data, data + nBins));
}

/**  Load logs from Nexus file. Logs are expected to be in
 *   /run/sample group of the file.
 *   @param ws :: The workspace to load the logs to.
 *   @param entry :: The Nexus entry
 *   @param period :: The period of this workspace
 */
void LoadMuonNexus2::loadLogs(const API::MatrixWorkspace_sptr &ws, const NXEntry &entry, int period) {
  // Avoid compiler warning
  UNUSED_ARG(period);

  std::string start_time = entry.getString("start_time");

  std::string sampleName = entry.getString("sample/name");
  NXClass runlogs = entry.openNXClass<NXClass>("sample");
  ws->mutableSample().setName(sampleName);

  for (std::vector<NXClassInfo>::const_iterator it = runlogs.groups().begin(); it != runlogs.groups().end(); ++it) {
    NXLog nxLog = openNXLog(runlogs, it->nxname);
    Kernel::Property *logv = nxLog.createTimeSeries(start_time);
    if (!logv)
      continue;
    ws->mutableRun().addLogData(logv);
  }

  ws->setTitle(entry.getString("title"));

  if (entry.containsDataSet("notes")) {
    ws->setComment(entry.getString("notes"));
  }

  std::string run_num = std::to_string(entry.getInt("run_number"));
  // The sample is left to delete the property
  ws->mutableRun().addLogData(new PropertyWithValue<std::string>("run_number", run_num));

  ws->populateInstrumentParameters();
}

/**  Log the run details from the file
 * @param localWorkspace :: The workspace details to use
 */
void LoadMuonNexus2::loadRunDetails(const DataObjects::Workspace2D_sptr &localWorkspace) {
  API::Run &runDetails = localWorkspace->mutableRun();

  runDetails.addProperty("run_title", localWorkspace->getTitle(), true);

  auto numSpectra = static_cast<int>(localWorkspace->getNumberHistograms());
  runDetails.addProperty("nspectra", numSpectra);

  m_filename = getPropertyValue("Filename");
  NXRoot root(m_filename);
  NXEntry entry = root.openEntry(m_entry_name);

  std::string start_time = entry.getString("start_time");
  runDetails.addProperty("run_start", start_time);

  std::string stop_time = entry.getString("end_time");
  runDetails.addProperty("run_end", stop_time);

  if (entry.containsGroup("run")) {
    NXClass runRun = entry.openNXGroup("run");

    if (runRun.containsDataSet("good_total_frames")) {
      int dum = runRun.getInt("good_total_frames");
      runDetails.addProperty("goodfrm", dum);
    }

    if (runRun.containsDataSet("number_periods")) {
      int dum = runRun.getInt("number_periods");
      runDetails.addProperty("nperiods", dum);
    }
  }

  { // Duration taken to be stop_time minus stat_time
    auto start = createFromSanitizedISO8601(start_time);
    auto end = createFromSanitizedISO8601(stop_time);
    double duration_in_secs = DateAndTime::secondsFromDuration(end - start);
    runDetails.addProperty("dur_secs", duration_in_secs);
  }
}

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadMuonNexus2::confidence(Kernel::LegacyNexusDescriptor &descriptor) const {
  const auto &firstEntryNameType = descriptor.firstEntryNameType();
  const std::string root = "/" + firstEntryNameType.first;
  if (!descriptor.pathExists(root + "/definition"))
    return 0;

  bool upperIDF(true);
  if (descriptor.pathExists(root + "/IDF_version"))
    upperIDF = true;
  else {
    if (descriptor.pathExists(root + "/idf_version"))
      upperIDF = false;
    else
      return 0;
  }

  try {
    std::string versionField = "idf_version";
    if (upperIDF)
      versionField = "IDF_version";

    auto &file = descriptor.data();
    file.openPath(root + "/" + versionField);
    int32_t IDFversion = 0;
    file.getData(&IDFversion);
    if (IDFversion != 2)
      return 0;

    file.openPath(root + "/definition");
    std::string def = file.getStrData();
    if (def == "muonTD" || def == "pulsedTD") {
      // If all this succeeded then we'll assume this is an ISIS Muon NeXus file
      // version 2
      return 81;
    }
  } catch (...) {
  }
  return 0;
}

/**
 * Loads the mapping between index -> set of detector IDs
 *
 * If "detector_index", "detector_count" and "detector_list" are all present,
 * use these to get the mapping, otherwise spectrum number = detector ID
 * (one-to-one)
 *
 * The spectrum spectrum_index[i] maps to detector_count[i] detectors, whose
 * detector IDs are in detector_list starting at the index detector_index[i]
 *
 * @returns :: map of index -> detector IDs
 * @throws std::runtime_error if fails to read data from file
 */
std::map<int, std::set<int>> LoadMuonNexus2::loadDetectorMapping(const Mantid::LegacyNexus::NXInt &spectrumIndex) {
  std::map<int, std::set<int>> mapping;
  auto const nSpectra = spectrumIndex.dim0();

  // Find and open the data group
  NXRoot root(getPropertyValue("Filename"));
  NXEntry entry = root.openEntry(m_entry_name);
  const std::string detectorName = [&entry]() {
    // Only the first NXdata found
    for (const auto &group : entry.groups()) {
      std::string className = group.nxclass;
      if (className == "NXdata") {
        return group.nxname;
      }
    }
    throw std::runtime_error("No NXdata found in file");
  }();
  NXData dataGroup = entry.openNXData(detectorName);

  // Usually for muon data, detector id = spectrum number
  // If not, the optional groups "detector_index", "detector_list" and
  // "detector_count" will be present to map one to the other
  const bool hasDetectorMapping = dataGroup.containsDataSet("detector_index") &&
                                  dataGroup.containsDataSet("detector_list") &&
                                  dataGroup.containsDataSet("detector_count");
  if (hasDetectorMapping) {
    // Read detector IDs
    try {
      const auto detIndex = dataGroup.openNXInt("detector_index");
      const auto detCount = dataGroup.openNXInt("detector_count");
      const auto detList = dataGroup.openNXInt("detector_list");
      const detid_t nDet = static_cast<detid_t>(detIndex.dim0());
      for (detid_t i = 0; i < nDet; ++i) {
        const auto start = static_cast<detid_t>(detIndex[i]);
        const auto nDetectors = detCount[i];
        std::set<detid_t> detIDs;
        for (detid_t jDet = 0; jDet < nDetectors; ++jDet) {
          detIDs.insert(detList[start + jDet]);
        }
        mapping[i] = detIDs;
      }
    } catch (const LegacyNexus::Exception &err) {
      // Throw a more user-friendly message
      std::ostringstream message;
      message << "Failed to read detector mapping: " << err.what();
      throw std::runtime_error(message.str());
    }
  } else {
    for (specnum_t i = 0; i < nSpectra; ++i) {
      mapping[i] = std::set<detid_t>{spectrumIndex[i]};
    }
  }

  return mapping;
}

} // namespace Mantid::Algorithms
