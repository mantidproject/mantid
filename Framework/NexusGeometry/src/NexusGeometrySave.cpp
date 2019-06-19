// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidNexusGeometry/NexusGeometrySave.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include <fstream>

namespace Mantid {
namespace NexusGeometry {

void saveInstrument( const Geometry::ComponentInfo& compInfo, const std::string& fullPath) {
  std::ifstream tmp(fullPath);
  if (!tmp) {
    throw std::invalid_argument(
        "The path provided for the file saving is invalid: " + fullPath); //throw error message for invalid path.
  };

};
} // namespace NexusGeometry
} // namespace Mantid
