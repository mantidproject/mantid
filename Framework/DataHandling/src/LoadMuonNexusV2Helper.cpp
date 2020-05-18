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
      NXInt goodFrames = entry.openNXInt("good_frames");
      goodFrames.load();
      return goodFrames;
    } catch (std::runtime_error) {
      throw std::runtime_error(
          "Could not load good frames data from nexus file, check Nexus file");
    }
  } else {
    try {
      NXClass periodClass = entry.openNXGroup("periods");
      // For multiperiod datasets, read raw_data_1/periods/good_frames
      NXInt goodFrames = periodClass.openNXInt("good_frames");
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
  NXClass detectorGroup = entry.openNXGroup("instrument/detector_1");
  if (detectorGroup.containsDataSet("grouping")) {
    NXInt groupingData = detectorGroup.openNXInt("grouping");
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
    NXChar orientation =
        entry.openNXChar("run/instrument/detector/orientation");
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
  NXClass detectorGroup = entry.openNXGroup("instrument/detector_1");
  if (detectorGroup.containsDataSet("dead_time")) {
    NXFloat deadTimesData = detectorGroup.openNXFloat("dead_time");
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
    NXClass detectorEntry = entry.openNXGroup("instrument/detector_1");
    NXInfo infoResolution = detectorEntry.getDataSetInfo("resolution");
    NXInt counts = detectorEntry.openNXInt("counts");
    std::string firstGoodBin = counts.attributes("first_good_bin");
    double resolution;
    switch (infoResolution.type) {
    case NX_FLOAT32:
      resolution = static_cast<double>(detectorEntry.getFloat("resolution"));
      break;
    case NX_INT32:
      resolution = static_cast<double>(detectorEntry.getInt("resolution"));
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
    NXClass detectorEntry = entry.openNXGroup("instrument/detector_1");
    double timeZero = static_cast<double>(detectorEntry.getFloat("time_zero"));
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
