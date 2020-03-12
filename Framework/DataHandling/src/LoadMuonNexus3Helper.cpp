
// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

// These functions handle the nexus operations needed to load
// the information from the Muon Nexus V2 file
#include "MantidDataHandling/LoadMuonNexus3Helper.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"

#include <iostream>

namespace Mantid {
namespace DataHandling {
namespace LoadMuonNexus3Helper {

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
    } catch (...) {
    }
  } else {
    try {
      NXClass periodClass = entry.openNXGroup("periods");
      // For multiperiod datasets, read raw_data_1/periods/good_frames
      NXInt goodFrames = periodClass.openNXInt("good_frames");
      goodFrames.load();
      return goodFrames;
    } catch (...) {
    }
  }
}
// Loads the detector grouping from the Muon Nexus V2 entry
DataObjects::TableWorkspace_sptr
loadDetectorGroupingFromNexus(NXEntry &entry,
                              DataObjects::Workspace2D_sptr &localWorkspace,
                              bool isFileMultiPeriod) {

  int64_t numberOfSpectra =
      static_cast<int64_t>(localWorkspace->getNumberHistograms());

  // Open nexus entry
  NXClass detectorGroup = entry.openNXGroup("instrument/detector_1");

  if (detectorGroup.containsDataSet("grouping")) {
    NXInt groupingData = detectorGroup.openNXInt("grouping");
    groupingData.load();
    int numGroupingEntries = groupingData.dim0();

    std::vector<detid_t> detectorsLoaded;
    std::vector<detid_t> grouping;
    // Return the detectors which are loaded
    // then find the grouping ID for each detector
    for (int64_t spectraIndex = 0; spectraIndex < numberOfSpectra;
         spectraIndex++) {
      const auto detIdSet =
          localWorkspace->getSpectrum(spectraIndex).getDetectorIDs();
      for (auto detector : detIdSet) {
        detectorsLoaded.emplace_back(detector);
      }
    }
    if (!isFileMultiPeriod) {
      // Simplest case - one grouping entry per detector
      for (const auto &detectorNumber : detectorsLoaded) {
        grouping.emplace_back(groupingData[detectorNumber - 1]);
      }
    }
    DataObjects::TableWorkspace_sptr table =
        createDetectorGroupingTable(detectorsLoaded, grouping);

    return table;
  }
}
/**
 * Creates Detector Grouping Table .
 * @param detectorsLoaded :: Vector containing the list of detectorsLoaded
 * @param grouping :: Vector containing corresponding grouping
 * @return Detector Grouping Table create using the data
 */
DataObjects::TableWorkspace_sptr
createDetectorGroupingTable(std::vector<detid_t> detectorsLoaded,
                            std::vector<detid_t> grouping) {
  auto detectorGroupingTable =
      boost::dynamic_pointer_cast<DataObjects::TableWorkspace>(
          WorkspaceFactory::Instance().createTable("TableWorkspace"));

  detectorGroupingTable->addColumn("vector_int", "Detectors");

  std::map<detid_t, std::vector<detid_t>> groupingMap;

  for (size_t i = 0; i < detectorsLoaded.size(); ++i) {
    // Add detector ID to the list of group detectors. Detector ID is always
    groupingMap[grouping[i]].emplace_back(detectorsLoaded[i]);
  }

  for (auto &group : groupingMap) {
    if (group.first != 0) { // Skip 0 group
      TableRow newRow = detectorGroupingTable->appendRow();
      newRow << group.second;
    }
  }
  return detectorGroupingTable;
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
  } catch (...) {
    // no data - assume main field was longitudinal
  }

  return mainFieldDirection;
}

} // namespace LoadMuonNexus3Helper
} // namespace DataHandling
} // namespace Mantid