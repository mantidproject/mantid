// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadMuonNexusV2Helper.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
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
                              bool isFileMultiPeriod) {

  std::vector<detid_t> grouping;
  // Open nexus entry
  NXClass detectorGroup = entry.openNXGroup(NeXusEntry::DETECTOR);
  if (detectorGroup.containsDataSet(NeXusEntry::GROUPING)) {
    NXInt groupingData = detectorGroup.openNXInt(NeXusEntry::GROUPING);
    groupingData.load();
    if (!isFileMultiPeriod) {
      for (const auto &detectorNumber : detectorsLoaded) {
        grouping.emplace_back(groupingData[detectorNumber - 1]);
      }
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
std::vector<double>
loadDeadTimesFromNexus(const NeXus::NXEntry &entry,
                       const std::vector<detid_t> &loadedDetectors,
                       const bool isFileMultiPeriod) {

  std::vector<double> deadTimes;
  // Open detector nexus entry
  NXClass detectorGroup = entry.openNXGroup(NeXusEntry::DETECTOR);
  if (detectorGroup.containsDataSet(NeXusEntry::DEADTIME)) {
    NXFloat deadTimesData = detectorGroup.openNXFloat(NeXusEntry::DEADTIME);
    deadTimesData.load();
    if (!isFileMultiPeriod) {
      // Simplest case - one grouping entry per detector
      for (const auto &detectorNumber : loadedDetectors) {
        deadTimes.emplace_back(deadTimesData[detectorNumber - 1]);
      }
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

} // namespace LoadMuonNexusV2Helper
} // namespace DataHandling
} // namespace Mantid
