// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/StartAndEndTimeFromNexusFileExtractor.h"
#include "MantidAPI/FileFinder.h"
#include "MantidDataHandling/LoadNexus.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidNexus/NexusFile.h"
#include <vector>

namespace Mantid::DataHandling {

namespace {
Mantid::Kernel::Logger g_log("StartAndEndTimeFromNexusFileExtractor");

enum class NexusType { Muon, Processed, ISIS, TofRaw };
enum class TimeType { StartTime, EndTime };

Mantid::Types::Core::DateAndTime getValue(const std::string &filename, const std::string &absAddress) {
  Nexus::File nxfile(filename, NXaccess::READ);
  nxfile.openAddress(absAddress);
  const auto valueStr = nxfile.getStrData();
  g_log.debug(valueStr + " from " + absAddress + " in " + filename);
  return Mantid::Types::Core::DateAndTime(valueStr);
}

/**
 * Opens the file up to twice to determine what nexus schema is being used.
 */
NexusType whichNexusType(const std::string &filename) {
  std::vector<std::string> entryName;
  std::vector<std::string> definition;
  auto count = LoadNexus::getNexusEntryTypes(filename, entryName, definition);

  // Issues with the file
  if (count <= -1) {
    g_log.error("Error reading file " + filename);
    throw Mantid::Kernel::Exception::FileError("Unable to read data in File:", filename);
  } else if (count == 0) {
    g_log.error("Error no entries found in " + filename);
    throw Mantid::Kernel::Exception::FileError("Error no entries found in ", filename);
  }

  NexusType nexusType;
  if (definition[0] == LoadNexus::muonTD || definition[0] == LoadNexus::pulsedTD) {
    nexusType = NexusType::Muon;
  } else if (entryName[0] == "mantid_workspace_1") {
    nexusType = NexusType::Processed;

  } else if (entryName[0] == "raw_data_1") {
    nexusType = NexusType::ISIS;
  } else {
    Nexus::File nxfile(filename, NXaccess::READ);
    const auto entries = nxfile.getEntries();
    const auto firstEntryName = entries.begin()->first;
    try {
      nxfile.openAddress("/" + firstEntryName + "/instrument/SNSdetector_calibration_id");
    } catch (...) {
      g_log.error("File " + filename + " is a currently unsupported type of NeXus file");
      throw Mantid::Kernel::Exception::FileError("Unable to read File:", filename);
    }
    nexusType = NexusType::TofRaw;
  }

  return nexusType;
}

/**
 * This holds all of the logic for what field in the file contains the DateAndTime information requested.
 */
std::string getDataFieldAddress(const NexusType nexusType, const TimeType type) {
  std::string datafieldaddress;
  switch (nexusType) {
  case NexusType::Muon:
    if (type == TimeType::StartTime) {
      datafieldaddress = "/run/start_time";
    } else {
      datafieldaddress = "/run/stop_time";
    }
    break;
  case NexusType::ISIS:
    if (type == TimeType::StartTime) {
      datafieldaddress = "/raw_data_1/start_time";
    } else {
      datafieldaddress = "/raw_data_1/end_time";
    }
    break;
  case NexusType::Processed:
    if (type == TimeType::StartTime) {
      datafieldaddress = "/mantid_workspace_1/logs/run_start/value";
    } else {
      datafieldaddress = "/mantid_workspace_1/logs/run_end/value";
    }
    break;
  case NexusType::TofRaw:
    if (type == TimeType::StartTime) {
      datafieldaddress = "/entry/start_time";
    } else {
      datafieldaddress = "/entry/end_time";
    }
    break;
  default:
    throw std::runtime_error("Unkown Nexus format. Not able to extract a date and time.");
  };

  return datafieldaddress;
}

/**
 * Main function for converting a filename into a DateAndTime
 */
Mantid::Types::Core::DateAndTime extractDateAndTime(TimeType type, const std::string &filename) {
  // convert relative to absolute path
  auto fullFileName = Mantid::API::FileFinder::Instance().getFullPath(filename);
  // Figure out the type of the Nexus file. We need to handle them individually since they store the datetime
  // differently
  auto nexusType = whichNexusType(fullFileName);
  // convert the type information into a address within the nexus file
  const auto dataAddress = getDataFieldAddress(nexusType, type);
  // return the result
  return getValue(fullFileName, dataAddress);
}
} // namespace

/**
 * Gets the start time from the nexus file
 * @param filename: the file name
 * @return the start time
 * @throws if the start time cannot be extracted
 */
Mantid::Types::Core::DateAndTime extractStartTime(const std::string &filename) {
  return extractDateAndTime(TimeType::StartTime, filename);
}

/**
 * Gets the start time from the nexus file
 * @param filename: the file name
 * @return the start time
 * @throws if the start time cannot be extracted
 */
Mantid::Types::Core::DateAndTime extractEndTime(const std::string &filename) {
  return extractDateAndTime(TimeType::EndTime, filename);
}

} // namespace Mantid::DataHandling
