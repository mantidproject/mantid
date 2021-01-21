// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadMuonNexusV2NexusHelper.h"
#include "MantidAPI/GroupingLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid {
namespace DataHandling {

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
const std::string RAWDATA{"/raw_data_1"};
} // namespace NeXusEntry

using namespace NeXus;
using namespace Kernel;
using namespace API;
using namespace NeXus;
using namespace HistogramData;
using std::size_t;
using namespace DataObjects;

LoadMuonNexusV2NexusHelper::LoadMuonNexusV2NexusHelper(const NeXus::NXEntry &entry) : m_entry(entry) {}

// Loads the good frames from the Muon Nexus V2 entry
NXInt LoadMuonNexusV2NexusHelper::loadGoodFramesDataFromNexus(bool isFileMultiPeriod) {

  if (!isFileMultiPeriod) {
    try {
      NXInt goodFrames = m_entry.openNXInt(NeXusEntry::GOODFRAMES);
      goodFrames.load();
      return goodFrames;
    } catch (std::runtime_error &) {
      throw std::runtime_error("Could not load good frames data from nexus file, check Nexus file");
    }
  } else {
    try {
      NXClass periodClass = m_entry.openNXGroup(NeXusEntry::PERIOD);
      // For multiperiod datasets, read raw_data_1/periods/good_frames
      NXInt goodFrames = periodClass.openNXInt(NeXusEntry::GOODFRAMES);
      goodFrames.load();
      return goodFrames;
    } catch (std::runtime_error &) {
      throw std::runtime_error("Could not load good frames data from nexus file, check Nexus file");
    }
  }
}
// Loads the detector grouping from the Muon Nexus V2 entry
// NOTE: Currently, the Muon Nexus V2 files do not implement grouping.
// The method implemented here assumes that once implemented
// each detector will map to a single group. If this is not the case,
// the method will need to be altered.
std::vector<detid_t>
LoadMuonNexusV2NexusHelper::loadDetectorGroupingFromNexus(const std::vector<detid_t> &detectorsLoaded,
                                                          bool isFileMultiPeriod, int periodNumber) {
  // We cast the numLoadedDetectors to an int, which is the index type used for
  // Nexus Data sets. As detectorsLoaded is derived from a nexus entry we
  // can be certain we won't overflow the integer type.
  int numLoadedDetectors = static_cast<int>(detectorsLoaded.size());
  std::vector<detid_t> grouping;
  grouping.reserve(numLoadedDetectors);
  // Open nexus entry
  NXClass detectorGroup = m_entry.openNXGroup(NeXusEntry::DETECTOR);
  if (detectorGroup.containsDataSet(NeXusEntry::GROUPING)) {
    NXInt groupingData = detectorGroup.openNXInt(NeXusEntry::GROUPING);
    groupingData.load();
    int groupingOffset = !isFileMultiPeriod ? 0 : (numLoadedDetectors) * (periodNumber - 1);
    for (auto detectorNumber : detectorsLoaded) {
      grouping.emplace_back(groupingData[detectorNumber - 1 + groupingOffset]);
    }
  }
  return grouping;
}
std::string LoadMuonNexusV2NexusHelper::loadMainFieldDirectionFromNexus() {
  std::string mainFieldDirection = "Longitudinal"; // default
  try {
    NXChar orientation = m_entry.openNXChar(NeXusEntry::ORIENTATON);
    // some files have no data there
    orientation.load();
    if (orientation[0] == 't') {
      mainFieldDirection = "Transverse";
    }
  } catch (std::runtime_error &) {
    // no data - assume main field was longitudinal and continue.
  }
  return mainFieldDirection;
}
// Loads dead times from the nexus file
// Assumes one grouping entry per detector
std::vector<double> LoadMuonNexusV2NexusHelper::loadDeadTimesFromNexus(const std::vector<detid_t> &loadedDetectors,
                                                                       bool isFileMultiPeriod, int periodNumber) {
  // We cast the numLoadedDetectors to an int, which is the index type used for
  // Nexus Data sets. As loadedDectors is derived from a nexus entry we
  // can be certain we won't overflow the integer type.
  int numLoadedDetectors = static_cast<int>(loadedDetectors.size());
  std::vector<double> deadTimes;
  deadTimes.reserve(numLoadedDetectors);

  NXClass detectorGroup = m_entry.openNXGroup(NeXusEntry::DETECTOR);
  if (detectorGroup.containsDataSet(NeXusEntry::DEADTIME)) {
    NXFloat deadTimesData = detectorGroup.openNXFloat(NeXusEntry::DEADTIME);
    deadTimesData.load();
    // If we have a multi period file all the data will be stored in a single
    // nexus entry of length (num_periods * numDetectors)
    // So if we are load the second period we need to offset our indexes by
    // (1*numDetectors)
    int deadTimeOffset = !isFileMultiPeriod ? 0 : (numLoadedDetectors) * (periodNumber - 1);
    for (auto detectorNumber : loadedDetectors) {
      deadTimes.emplace_back(deadTimesData[detectorNumber - 1 + deadTimeOffset]);
    }
  }
  return deadTimes;
}

double LoadMuonNexusV2NexusHelper::loadFirstGoodDataFromNexus() {
  try {
    NXClass detectorEntry = m_entry.openNXGroup(NeXusEntry::DETECTOR);
    NXInfo infoResolution = detectorEntry.getDataSetInfo(NeXusEntry::RESOLUTION);
    NXInt counts = detectorEntry.openNXInt(NeXusEntry::COUNTS);
    std::string firstGoodBin = counts.attributes(NeXusEntry::FIRSTGOODBIN);
    double resolution;
    switch (infoResolution.type) {
    case NX_FLOAT32:
      resolution = static_cast<double>(detectorEntry.getFloat(NeXusEntry::RESOLUTION));
      break;
    case NX_INT32:
      resolution = static_cast<double>(detectorEntry.getInt(NeXusEntry::RESOLUTION));
      break;
    default:
      throw std::runtime_error("Unsupported data type for resolution");
    }
    double bin = static_cast<double>(boost::lexical_cast<int>(firstGoodBin));
    double bin_size = resolution / 1000000.0;
    return bin * bin_size;
  } catch (std::runtime_error &) {
    throw std::runtime_error("Error loading FirstGoodData, check Nexus file");
  }
}

double LoadMuonNexusV2NexusHelper::loadTimeZeroFromNexusFile() {
  try {
    NXClass detectorEntry = m_entry.openNXGroup(NeXusEntry::DETECTOR);
    double timeZero = static_cast<double>(detectorEntry.getFloat(NeXusEntry::TIMEZERO));
    return timeZero;
  } catch (std::runtime_error &) {
    throw std::runtime_error("Could not load time zero, check Nexus file");
  }
}

std::vector<double> LoadMuonNexusV2NexusHelper::loadTimeZeroListFromNexusFile(size_t numSpectra) {
  NXClass det_class = m_entry.openNXGroup(NeXusEntry::DETECTOR);

  NXDouble timeZeroClass = det_class.openNXDouble(NeXusEntry::TIMEZERO);
  std::vector<double> timeZeroVector = timeZeroClass.vecBuffer();
  if (timeZeroVector.size() == 0) {
    double timeZero = static_cast<double>(det_class.getFloat(NeXusEntry::TIMEZERO));
    timeZeroVector = std::vector<double>(numSpectra, timeZero);
  } else if (timeZeroVector.size() != numSpectra) {
    throw std::runtime_error("Time zero list size does not match number of "
                             "spectra, check Nexus file.");
  }
  // We assume that this spectrum list increases monotonically
  return timeZeroVector;
}

MuonNexus::SampleInformation LoadMuonNexusV2NexusHelper::loadSampleInformationFromNexus() {
  auto runSample = m_entry.openNXGroup(NeXusEntry::SAMPLE);
  MuonNexus::SampleInformation sampleInformation;
  try {
    sampleInformation.magneticField = runSample.getFloat(NeXusEntry::MAGNETICFIELD);
    sampleInformation.temperature = runSample.getFloat(NeXusEntry::TEMPERATURE);
  } catch (std::runtime_error &) {
    throw std::runtime_error("Could not load sample information (temperature "
                             "and magnetic field) from nexus entry");
  }
  return sampleInformation;
}

int LoadMuonNexusV2NexusHelper::getNumberOfPeriods() const {
  NXClass periodClass = m_entry.openNXGroup(NeXusEntry::PERIOD);
  return periodClass.getInt("number");
}
} // namespace DataHandling
} // namespace Mantid
