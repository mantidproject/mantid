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
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/UnitLabelTypes.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidNexusCpp/NeXusException.hpp"
#include "MantidNexusCpp/NeXusFile.hpp"

// must be after MantidNexusCpp/NeXusFile.hpp
#include "MantidLegacyNexus/NeXusFile.hpp"

#include <Poco/Path.h>
#include <cmath>
#include <memory>
#include <numeric>

using Mantid::Types::Core::DateAndTime;

namespace Mantid::Algorithms {
// Register the algorithm into the algorithm factory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadMuonNexus2)

using namespace Kernel;
using namespace DateAndTimeHelpers;
using namespace API;
using Geometry::Instrument;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::Histogram;
using namespace Mantid::NeXus;
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
    } catch (::NeXus::Exception &) {
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

  Mantid::NeXus::NXInt spectrum_index = dataGroup.openNXInt("spectrum_index");
  spectrum_index.load();
  m_numberOfSpectra = spectrum_index.dim0();

  // Load detector mapping
  const auto &detMapping = loadDetectorMapping(spectrum_index);

  // Call private method to validate the optional parameters, if set
  checkOptionalProperties();

  NXFloat raw_time = dataGroup.openNXFloat("raw_time");
  raw_time.load();
  int nBins = raw_time.dim0();
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

  Mantid::NeXus::NXInt counts = dataGroup.openIntData();
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
Histogram LoadMuonNexus2::loadData(const BinEdges &edges, const Mantid::NeXus::NXInt &counts, int period, int spec) {
  int nBins = 0;
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
void LoadMuonNexus2::loadLogs(const API::MatrixWorkspace_sptr &ws, NXEntry &entry, int period) {
  // Avoid compiler warning
  (void)period;

  std::string start_time = entry.getString("start_time");

  std::string sampleName = entry.getString("sample/name");
  NXMainClass runlogs = entry.openNXClass<NXMainClass>("sample");
  ws->mutableSample().setName(sampleName);

  for (std::vector<NXClassInfo>::const_iterator it = runlogs.groups().begin(); it != runlogs.groups().end(); ++it) {
    NXLog nxLog = runlogs.openNXLog(it->nxname);
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
std::map<int, std::set<int>> LoadMuonNexus2::loadDetectorMapping(const Mantid::NeXus::NXInt &spectrumIndex) {
  std::map<int, std::set<int>> mapping;
  const int nSpectra = spectrumIndex.dim0();

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
      const int nDet = detIndex.dim0();
      for (int i = 0; i < nDet; ++i) {
        const int start = detIndex[i];
        const int nDetectors = detCount[i];
        std::set<int> detIDs;
        for (int jDet = 0; jDet < nDetectors; ++jDet) {
          detIDs.insert(detList[start + jDet]);
        }
        mapping[i] = detIDs;
      }
    } catch (const ::NeXus::Exception &err) {
      // Throw a more user-friendly message
      std::ostringstream message;
      message << "Failed to read detector mapping: " << err.what();
      throw std::runtime_error(message.str());
    }
  } else {
    for (int i = 0; i < nSpectra; ++i) {
      mapping[i] = std::set<int>{spectrumIndex[i]};
    }
  }

  return mapping;
}

} // namespace Mantid::Algorithms
