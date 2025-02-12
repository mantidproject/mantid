// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadGeometry.h"
#include "MantidKernel/FileDescriptor.h"
#include "MantidKernel/NexusHDF5Descriptor.h"
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

  if (Mantid::Kernel::NexusHDF5Descriptor::isReadable(filename)) {
    Mantid::Kernel::NexusHDF5Descriptor descriptor(filename);
    return isNexus(descriptor.getAllEntries());
  }

  return false;
}

bool LoadGeometry::isNexus(const std::map<std::string, std::set<std::string>> &allEntries) {
  // Checks if the nexus filename contains a valid Mantid geometry
  if ((allEntries.count("NXcylindrical_geometry") != 1 && allEntries.count("NXoff_geometry") != 1 &&
       allEntries.count("NXtransformations") != 1) ||
      allEntries.count(Mantid::NexusGeometry::NX_SOURCE) != 1 ||
      allEntries.count(Mantid::NexusGeometry::NX_SAMPLE) != 1) {
    return false;
  }
  std::set<std::string> data_entries = allEntries.at("SDS");

  if (allEntries.contains(Mantid::NexusGeometry::NX_DETECTOR)) {
    std::set<std::string> det_entries = allEntries.at(Mantid::NexusGeometry::NX_DETECTOR);
    for (auto it = det_entries.begin(); it != det_entries.end(); ++it) {
      std::string id_name = *it + "/" + Mantid::NexusGeometry::DETECTOR_IDS;
      if (data_entries.find(id_name) == data_entries.end()) {
        return false;
      }
    }
  }
  if (allEntries.contains(Mantid::NexusGeometry::NX_MONITOR)) {
    std::set<std::string> mon_entries = allEntries.at(Mantid::NexusGeometry::NX_MONITOR);
    for (auto it = mon_entries.begin(); it != mon_entries.end(); ++it) {
      std::string id_name = *it + "/" + Mantid::NexusGeometry::DETECTOR_ID;
      if (data_entries.find(id_name) == data_entries.end()) {
        return false;
      }
    }
  }
  return true;
}

/// List allowed file extensions for geometry
const std::vector<std::string> LoadGeometry::validExtensions() { return {".xml", ".nxs", ".hdf5"}; }

} // namespace Mantid::DataHandling
