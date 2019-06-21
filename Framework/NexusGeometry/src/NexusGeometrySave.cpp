// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidNexusGeometry/NexusGeometrySave.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidKernel/ProgressBase.h"
#include <boost/filesystem/operations.hpp>
#include <iostream>

namespace Mantid {
namespace NexusGeometry {

void saveInstrument(const Geometry::ComponentInfo &compInfo,
                    const std::string &fullPath,
                    Kernel::ProgressBase *reporter) {

  if (fullPath.empty()) {

    throw std::invalid_argument(
        "no path is provided.\n"); // handles case for empty string before
                                   // attempting to open with boost.
  } else {

    // should only attempt this for non-empty fullPath
    boost::filesystem::path tmp(fullPath);
    if (!boost::filesystem::is_directory(tmp)) {
      throw std::invalid_argument(
          "The path provided for the file saving is invalid: " + fullPath +
          "\n");
    }
  }
  if (!compInfo.hasDetectorInfo()) {
    throw std::invalid_argument("The component has no detector info.\n");
  }

  if (reporter != nullptr) {
    reporter->report();
  }

  /*do checks on instrument attributes and classes.*/

  // save file to destination 'fullPath' WIP

  std::string instrumentData;
  std::string filename;
  std::string pathToFile = fullPath + "\\" + filename;
  std::ofstream file(pathToFile); // open file.

  file << instrumentData; // write data to file

}; // saveInstrument

// define HDF5FileTestUtility class here

} // namespace NexusGeometry
} // namespace Mantid
