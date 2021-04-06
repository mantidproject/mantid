// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadGeometry.h"
#include "MantidKernel/FileDescriptor.h"
#include "MantidKernel/NexusDescriptor.h"

namespace Mantid {
namespace DataHandling {

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
  if (!filename.empty() && !Mantid::Kernel::FileDescriptor(filename).isAscii(filename)) {
    Mantid::Kernel::NexusDescriptor descriptor(filename);
    return descriptor.isReadable(filename) &&
           (descriptor.classTypeExists("NXcylindrical_geometry") || descriptor.classTypeExists("NXoff_geometry") ||
            descriptor.classTypeExists("NXtransformations"));
  }
  return false;
}

bool LoadGeometry::isNexus(const std::string &filename,
                           const std::map<std::string, std::set<std::string>> &allEntries) {
  if (!filename.empty() && !Mantid::Kernel::FileDescriptor(filename).isAscii(filename)) {
    Mantid::Kernel::NexusDescriptor descriptor(filename, false);
    return descriptor.isReadable(filename) &&
           (allEntries.count("NXcylindrical_geometry") == 1 || allEntries.count("NXoff_geometry") == 1 ||
            allEntries.count("NXtransformations") == 1);
  }
  return false;
}

/// List allowed file extensions for geometry
const std::vector<std::string> LoadGeometry::validExtensions() { return {".xml", ".nxs", ".hdf5"}; }

} // namespace DataHandling
} // namespace Mantid
