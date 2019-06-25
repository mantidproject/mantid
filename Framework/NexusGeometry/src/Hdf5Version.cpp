// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidNexusGeometry/Hdf5Version.h"
#include <H5Cpp.h>

namespace Mantid {
namespace NexusGeometry {

// Utility for checking hdf5 library version
uint32_t Hdf5Version::makeHdf5VersionNumber(uint32_t maj, uint32_t min,
                                            uint32_t relnum) {
  return 100000 * maj + 1000 * min + relnum;
}

// Utility for checking variable length string support
bool Hdf5Version::checkVariableLengthStringSupport() {
  uint32_t maj, min, relnum;
  H5get_libversion(&maj, &min, &relnum);
  const auto actual = makeHdf5VersionNumber(maj, min, relnum);
  const auto expected = makeHdf5VersionNumber(1, 8, 16); // Minimum expected
  return (actual > expected);
}

} // namespace NexusGeometry
} // namespace Mantid
