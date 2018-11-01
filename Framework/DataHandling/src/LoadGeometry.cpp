// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadGeometry.h"
#include "MantidKernel/FileDescriptor.h"
#include "MantidKernel/NexusDescriptor.h"

namespace Mantid {
namespace DataHandling {

bool LoadGeometry::isIDF(const std::string &filename, const std::string &instrumentname) {
  if (!filename.empty()) {
    Mantid::Kernel::FileDescriptor descriptor(filename);
    return ((descriptor.isAscii() && descriptor.extension() == ".xml"));
  }
  return !instrumentname.empty();
}

bool LoadGeometry::isNexus(const std::string &filename) {
  if (!filename.empty() && !Mantid::Kernel::FileDescriptor(filename).isAscii(filename)) {
    Mantid::Kernel::NexusDescriptor descriptor(filename);
    return descriptor.isHDF(filename) &&
           (descriptor.classTypeExists("NXcylindrical_geometry") ||
            descriptor.classTypeExists("NXoff_geometry"));
  }
  return false;
}

} // namespace DataHandling
} // namespace Mantid
