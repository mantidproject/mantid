// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadGeometry.h"
#include "MantidKernel/FileDescriptor.h"
#include "MantidKernel/NexusDescriptor.h"
#include "MantidNexusGeometry/NexusGeometryDefinitions.h"

namespace Mantid::DataHandling {

/// Determine if the Geometry file type is IDF
bool LoadGeometry::isIDF(const std::string &filename) {
  if (!filename.empty()) {
    Mantid::Kernel::FileDescriptor descriptor(filename);
    return descriptor.isXML();
  }
  return false;
}

/// Determine if the Geometry file type is Nexus
bool LoadGeometry::isNexus(const std::string &filename) {
  if (filename.empty() || Mantid::Kernel::FileDescriptor(filename).isAscii(filename)) {
    return false;
  }
  Mantid::Kernel::NexusDescriptor descriptor(filename);
  if (!descriptor.isReadable(filename) ||
      (!descriptor.classTypeExists("NXcylindrical_geometry") && !descriptor.classTypeExists("NXoff_geometry") &&
       !descriptor.classTypeExists("NXtransformations")) ||
      !descriptor.classTypeExists(Mantid::NexusGeometry::NX_SOURCE) ||
      !descriptor.classTypeExists(Mantid::NexusGeometry::NX_SAMPLE)) {
    return false;
  }
  std::vector<std::string> detectors = descriptor.allPathsOfType(Mantid::NexusGeometry::NX_DETECTOR);
  if (std::any_of(detectors.begin(), detectors.end(), [&descriptor](const std::string &det) {
        return !descriptor.pathExists(det + "/" + Mantid::NexusGeometry::DETECTOR_IDS);
      })) {
    return false;
  }
  detectors = descriptor.allPathsOfType(Mantid::NexusGeometry::NX_MONITOR);
  return std::all_of(detectors.begin(), detectors.end(), [&descriptor](const std::string &det) {
    return descriptor.pathExists(det + "/" + Mantid::NexusGeometry::DETECTOR_ID);
  });
}

bool LoadGeometry::isNexus(const std::string &filename,
                           const std::map<std::string, std::set<std::string>> &allEntries) {
  if (filename.empty() || Mantid::Kernel::FileDescriptor(filename).isAscii(filename)) {
    return false;
  }
  Mantid::Kernel::NexusDescriptor descriptor(filename, false);
  // Checks if the nexus filename contains a valid Mantid geometry
  if (!descriptor.isReadable(filename) ||
      (allEntries.count("NXcylindrical_geometry") != 1 && allEntries.count("NXoff_geometry") != 1 &&
       allEntries.count("NXtransformations") != 1) ||
      allEntries.count(Mantid::NexusGeometry::NX_SOURCE) != 1 ||
      allEntries.count(Mantid::NexusGeometry::NX_SAMPLE) != 1 ||
      allEntries.count(Mantid::NexusGeometry::NX_DETECTOR) != 1 ||
      allEntries.count(Mantid::NexusGeometry::NX_MONITOR) != 1) {
    return false;
  }
  std::set<std::string> data_entries = allEntries.at("SDS");
  std::set<std::string> det_entries = allEntries.at(Mantid::NexusGeometry::NX_DETECTOR);
  for (auto it = det_entries.begin(); it != det_entries.end(); ++it) {
    std::string id_name = *it + "/" + Mantid::NexusGeometry::DETECTOR_IDS;
    if (data_entries.find(id_name) == data_entries.end()) {
      return false;
    }
  }
  std::set<std::string> mon_entries = allEntries.at(Mantid::NexusGeometry::NX_MONITOR);
  for (auto it = mon_entries.begin(); it != mon_entries.end(); ++it) {
    std::string id_name = *it + "/" + Mantid::NexusGeometry::DETECTOR_ID;
    if (data_entries.find(id_name) == data_entries.end()) {
      return false;
    }
  }
  return true;
}

/// List allowed file extensions for geometry
const std::vector<std::string> LoadGeometry::validExtensions() { return {".xml", ".nxs", ".hdf5"}; }

} // namespace Mantid::DataHandling
