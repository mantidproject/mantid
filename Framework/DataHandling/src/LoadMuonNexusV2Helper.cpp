// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadMuonNexusV2Helper.h"
#include "MantidAPI/GroupingLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid {
namespace DataHandling {
namespace LoadMuonNexusV2Helper {

namespace NeXusEntry {
const std::string GOODFRAMES{"good_frames"};
const std::string DETECTOR{"instrument/detector_1"};
const std::string PERIOD{"periods"};
const std::string ORIENTATON{"instrument/detector_1/orientation"};
const std::string RESOLUTION{"resolution"};
const std::string GROUPING{"grouping"};
const std::string DEADTIME{"dead_time"};
const std::string COUNTS{"counts"};
const std::string FIRSTGOODBIN{"first_good_bin"};
const std::string TIMEZERO{"time_zero"};
const std::string SAMPLE{"sample"};
const std::string TEMPERATURE{"temperature"};
const std::string MAGNETICFIELD{"magnetic_field"};
} // namespace NeXusEntry

using namespace NeXus;
using namespace Kernel;
using namespace API;
using namespace NeXus;
using namespace HistogramData;
using std::size_t;
using namespace DataObjects;

// Loads the good frames from the Muon Nexus V2 entry
NXInt loadGoodFramesDataFromNexus(const NXEntry &entry,
                                  bool isFileMultiPeriod) {

  if (!isFileMultiPeriod) {
    try {
      NXInt goodFrames = entry.openNXInt(NeXusEntry::GOODFRAMES);
      goodFrames.load();
      return goodFrames;
    } catch (std::runtime_error) {
      throw std::runtime_error(
          "Could not load good frames data from nexus file, check Nexus file");
    }
  } else {
    try {
      NXClass periodClass = entry.openNXGroup(NeXusEntry::PERIOD);
      // For multiperiod datasets, read raw_data_1/periods/good_frames
      NXInt goodFrames = periodClass.openNXInt(NeXusEntry::GOODFRAMES);
      goodFrames.load();
      return goodFrames;
    } catch (std::runtime_error) {
      throw std::runtime_error(
          "Could not load good frames data from nexus file, check Nexus file");
    }
  }
}
// Loads the detector grouping from the Muon Nexus V2 entry
std::vector<detid_t>
loadDetectorGroupingFromNexus(const NXEntry &entry,
                              const std::vector<detid_t> &detectorsLoaded,
                              bool isFileMultiPeriod, const int periodNumber) {
  // We cast the numLoadedDetectors to an int, which is the index type used for
  // Nexus Data sets. As detectorsLoaded is derived from a nexus entry we
  // can be certain we won't overflow the integer type.
  int numLoadedDetectors = static_cast<int>(detectorsLoaded.size());
  std::vector<detid_t> grouping;
  grouping.reserve(numLoadedDetectors);
  // Open nexus entry
  NXClass detectorGroup = entry.openNXGroup(NeXusEntry::DETECTOR);
  if (detectorGroup.containsDataSet(NeXusEntry::GROUPING)) {
    NXInt groupingData = detectorGroup.openNXInt(NeXusEntry::GROUPING);
    groupingData.load();
    int groupingOffset =
        !isFileMultiPeriod ? 0 : (numLoadedDetectors) * (periodNumber - 1);
    for (const auto &detectorNumber : detectorsLoaded) {
      grouping.emplace_back(groupingData[detectorNumber - 1 + groupingOffset]);
    }
  }
  return grouping;
}
std::string loadMainFieldDirectionFromNexus(const NeXus::NXEntry &entry) {
  std::string mainFieldDirection = "Longitudinal"; // default
  try {
    NXChar orientation = entry.openNXChar(NeXusEntry::ORIENTATON);
    // some files have no data there
    orientation.load();
    if (orientation[0] == 't') {
      mainFieldDirection = "Transverse";
    }
  } catch (std::runtime_error) {
    // no data - assume main field was longitudinal and continue.
  }
  return mainFieldDirection;
}
// Loads dead times from the nexus file
// Assumes one grouping entry per detector
std::vector<double>
loadDeadTimesFromNexus(const NeXus::NXEntry &entry,
                       const std::vector<detid_t> &loadedDetectors,
                       const bool isFileMultiPeriod, const int periodNumber) {
  // We cast the numLoadedDetectors to an int, which is the index type used for
  // Nexus Data sets. As loadedDectors is derived from a nexus entry we
  // can be certain we won't overflow the integer type.
  int numLoadedDetectors = static_cast<int>(loadedDetectors.size());
  std::vector<double> deadTimes;
  deadTimes.reserve(numLoadedDetectors);
  NXClass detectorGroup = entry.openNXGroup(NeXusEntry::DETECTOR);
  if (detectorGroup.containsDataSet(NeXusEntry::DEADTIME)) {
    NXFloat deadTimesData = detectorGroup.openNXFloat(NeXusEntry::DEADTIME);
    deadTimesData.load();
    int deadTimeOffset =
        !isFileMultiPeriod ? 0 : (numLoadedDetectors) * (periodNumber - 1);
    for (const auto &detectorNumber : loadedDetectors) {
      deadTimes.emplace_back(
          deadTimesData[detectorNumber - 1 + deadTimeOffset]);
    }
  }
  return deadTimes;
}

double loadFirstGoodDataFromNexus(const NeXus::NXEntry &entry) {
  try {
    NXClass detectorEntry = entry.openNXGroup(NeXusEntry::DETECTOR);
    NXInfo infoResolution =
        detectorEntry.getDataSetInfo(NeXusEntry::RESOLUTION);
    NXInt counts = detectorEntry.openNXInt(NeXusEntry::COUNTS);
    std::string firstGoodBin = counts.attributes(NeXusEntry::FIRSTGOODBIN);
    double resolution;
    switch (infoResolution.type) {
    case NX_FLOAT32:
      resolution =
          static_cast<double>(detectorEntry.getFloat(NeXusEntry::RESOLUTION));
      break;
    case NX_INT32:
      resolution =
          static_cast<double>(detectorEntry.getInt(NeXusEntry::RESOLUTION));
      break;
    default:
      throw std::runtime_error("Unsupported data type for resolution");
    }
    double bin = static_cast<double>(boost::lexical_cast<int>(firstGoodBin));
    double bin_size = resolution / 1000000.0;
    return bin * bin_size;
  } catch (std::runtime_error) {
    throw std::runtime_error("Error loading FirstGoodData, check Nexus file");
  }
}

double loadTimeZeroFromNexusFile(const NeXus::NXEntry &entry) {
  try {
    NXClass detectorEntry = entry.openNXGroup(NeXusEntry::DETECTOR);
    double timeZero =
        static_cast<double>(detectorEntry.getFloat(NeXusEntry::TIMEZERO));
    return timeZero;
  } catch (std::runtime_error) {
    throw std::runtime_error("Could not load time zero, check Nexus file");
  }
}

std::vector<double> loadTimeZeroListFromNexusFile(const NeXus::NXEntry &entry,
                                                  size_t numSpectra) {
  NXClass det_class = entry.openNXGroup(NeXusEntry::DETECTOR);

  NXDouble timeZeroClass = det_class.openNXDouble(NeXusEntry::TIMEZERO);
  std::vector<double> timeZeroVector = timeZeroClass.vecBuffer();
  if (timeZeroVector.size() == 0) {
    double timeZero =
        static_cast<double>(det_class.getFloat(NeXusEntry::TIMEZERO));
    timeZeroVector = std::vector<double>(numSpectra, timeZero);
  } else if (timeZeroVector.size() != numSpectra) {
    throw std::runtime_error("Time zero list size does not match number of "
                             "spectra, check Nexus file.");
  }
  // We assume that this spectrum list increases monotonically
  return timeZeroVector;
}

std::vector<detid_t>
getLoadedDetectors(const DataObjects::Workspace2D_sptr &localWorkspace) {

  std::vector<detid_t> loadedDetectors;
  size_t numberOfSpectra = localWorkspace->getNumberHistograms();

  for (size_t spectraIndex = 0; spectraIndex < numberOfSpectra;
       spectraIndex++) {
    const auto detIdSet =
        localWorkspace->getSpectrum(spectraIndex).getDetectorIDs();
    // each spectrum should only point to one detector in the Muon file
    loadedDetectors.emplace_back(*detIdSet.begin());
  }
  return loadedDetectors;
}

MuonNexus::SampleInformation
loadSampleInformationFromNexus(const NeXus::NXEntry &entry) {
  auto runSample = entry.openNXGroup(NeXusEntry::SAMPLE);
  MuonNexus::SampleInformation sampleInformation;
  try {
    sampleInformation.magneticField =
        runSample.getFloat(NeXusEntry::MAGNETICFIELD);
    sampleInformation.temperature = runSample.getFloat(NeXusEntry::TEMPERATURE);
  } catch (std::runtime_error) {
    throw std::runtime_error("Could not load sample information (temperature "
                             "and magnetic field) from nexus entry");
  }
  return sampleInformation;
}
/**
 * Loads default detector grouping, if this isn't present
 * return dummy grouping
 * @returns :: Grouping table
 */
Workspace_sptr loadDefaultDetectorGrouping(
    const DataObjects::Workspace2D_sptr &localWorkspace) {

  auto instrument = localWorkspace->getInstrument();
  auto &run = localWorkspace->run();
  std::string mainFieldDirection =
      run.getLogData("main_field_direction")->value();
  API::GroupingLoader groupLoader(instrument, mainFieldDirection);
  try {
    const auto idfGrouping = groupLoader.getGroupingFromIDF();
    return idfGrouping->toTable();
  } catch (const std::runtime_error &) {
    auto dummyGrouping = std::make_shared<Grouping>();
    if (instrument->getNumberDetectors() != 0) {
      dummyGrouping = groupLoader.getDummyGrouping();
    } else {
      // Make sure it uses the right number of detectors
      std::ostringstream oss;
      oss << "1-" << localWorkspace->getNumberHistograms();
      dummyGrouping->groups.emplace_back(oss.str());
      dummyGrouping->groupNames.emplace_back("all");
    }
    return dummyGrouping->toTable();
  }
}

std::vector<detid_t> getLoadedDetectorsFromWorkspace(
    const DataObjects::Workspace2D_sptr &localWorkspace) {
  size_t numberOfSpectra = localWorkspace->getNumberHistograms();
  std::vector<detid_t> loadedDetectors;
  loadedDetectors.reserve(numberOfSpectra);
  for (size_t spectraIndex = 0; spectraIndex < numberOfSpectra;
       spectraIndex++) {
    const auto detIdSet =
        localWorkspace->getSpectrum(spectraIndex).getDetectorIDs();
    // each spectrum should only point to one detector in the Muon file
    loadedDetectors.emplace_back(*detIdSet.begin());
  }
  return loadedDetectors;
}

} // namespace LoadMuonNexusV2Helper
} // namespace DataHandling
} // namespace Mantid
