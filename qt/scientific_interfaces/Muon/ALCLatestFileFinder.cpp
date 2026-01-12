// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALCLatestFileFinder.h"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <cctype>
#include <filesystem>

namespace MantidQt::CustomInterfaces {

/**
 * Gets the most recently modified valid Nexus file in the same directory as the
 * first run. Assumes files go in run number order.
 * @returns Path to most recently modified file
 */
std::string ALCLatestFileFinder::getMostRecentFile() const {
  if (m_firstRunFileName.empty()) {
    return "";
  } else {
    // List all valid Nexus files in the directory
    std::filesystem::path path(m_firstRunFileName);
    std::vector<std::string> fileNames;
    // if it found no valid files then return an error
    // and retry disable the auto button
    try {
      // Directory iterator - check if we were passed a file or a directory
      std::filesystem::path dirPath = std::filesystem::is_directory(path) ? path : path.parent_path();
      for (const auto &entry : std::filesystem::directory_iterator(dirPath)) {
        if (isValid(entry.path().string())) {
          fileNames.emplace_back(entry.path().string());
        }
      }
    } catch (const std::filesystem::filesystem_error &) {
      // There was some problem iterating through the directory.
      // Return the file we were given.
      return path.string();
    }

    if (!fileNames.empty()) {
      // Sort by run number
      std::sort(fileNames.begin(), fileNames.end());
      return fileNames.back();
    } else {
      // return empty string
      return "";
    }
  }
}

/**
 * Checks the file is valid
 * "valid": of correct form INST000XXXXX.nxs and with
 * correct instrument
 * @param path :: [input] path to file
 * @returns :: validity of file
 */
bool ALCLatestFileFinder::isValid(const std::string &path) const {
  bool valid = false;
  const std::filesystem::path filePath(path);
  const std::filesystem::path firstPath(m_firstRunFileName);

  auto getInstrumentAndRun = [](const std::string &name) {
    // No muon instruments have numbers in their names
    size_t numPos = name.find_first_of("0123456789");
    if (numPos == std::string::npos) {
      return std::make_pair<std::string, std::string>(std::string(name), "");
    } else {
      return std::make_pair<std::string, std::string>(name.substr(0, numPos), name.substr(numPos));
    }
  };

  auto firstRunInstrument = getInstrumentAndRun(firstPath.stem().string()).first;
  // 0. Must be a file
  if (std::filesystem::is_regular_file(filePath)) {
    // 1. Must be a NeXus file
    std::string extension = filePath.extension().string();
    if (!extension.empty() && extension[0] == '.') {
      extension = extension.substr(1); // remove leading dot
    }
    if (extension == "nxs") {
      // 2. Instrument must be correct
      auto fileName = filePath.stem().string();
      if (fileName.size() > 5) {
        auto fileSplit = getInstrumentAndRun(fileName);
        if (boost::iequals(fileSplit.first, firstRunInstrument)) {
          // 3. Must end in a number
          valid = std::all_of(fileSplit.second.begin(), fileSplit.second.end(), ::isdigit);
        }
      }
    }
  }
  return valid;
}

} // namespace MantidQt::CustomInterfaces
