// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/DllConfig.h"
#include "MantidDataHandling/LoadAsciiStl.h"
#include "MantidDataHandling/LoadBinaryStl.h"

namespace Mantid {
namespace DataHandling {

class MANTID_DATAHANDLING_DLL LoadStlFactory {

  // clang-format off
public :
static std::unique_ptr<LoadStl> createReader(const std::string &filename, ScaleUnits scaleType) {

  std::unique_ptr<LoadStl> reader = nullptr;
  if (LoadBinaryStl::isBinarySTL(filename)) {
    reader = std::make_unique<LoadBinaryStl>(filename, scaleType);
  } else if (LoadAsciiStl::isAsciiSTL(filename)) {
    reader = std::make_unique<LoadAsciiStl>(filename, scaleType);
  } else {
    throw Kernel::Exception::ParseError("Could not read file, did not match either STL Format", filename, 0);
  }
  return reader;
}
  // clang-format on

}; // namespace Mantid

} // namespace DataHandling
} // namespace Mantid
