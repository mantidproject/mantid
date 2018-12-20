#include "ALCLatestFileFinder.h"
#include <Poco/DirectoryIterator.h>
#include <Poco/Exception.h>
#include <algorithm>
#include <cctype>

namespace MantidQt {
namespace CustomInterfaces {

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
    Poco::Path path(m_firstRunFileName);
    std::vector<std::string> fileNames;
    try {
      // Directory iterator - check if we were passed a file or a directory
      Poco::DirectoryIterator iter(path.isDirectory() ? path : path.parent());
      Poco::DirectoryIterator end; // the end iterator
      while (iter != end) {
        if (isValid(iter->path())) {
          fileNames.push_back(iter->path());
        }
        ++iter;
      }
    } catch (const Poco::Exception &) {
      // There was some problem iterating through the directory.
      // Return the file we were given.
      return path.toString();
    }

    // Sort by run number
    std::sort(fileNames.begin(), fileNames.end());

    return fileNames.back();
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
  const Poco::Path filePath(path);
  const Poco::Path firstPath(m_firstRunFileName);

  // Get the instrument and run number from "INST0001234"
  auto getInstrumentAndRun = [](const std::string &name) {
    // No muon instruments have numbers in their names
    size_t numPos = name.find_first_of("0123456789");
    if (numPos == std::string::npos) {
      return std::make_pair<std::string, std::string>(std::string(name), "");
    } else {
      return std::make_pair<std::string, std::string>(name.substr(0, numPos),
                                                      name.substr(numPos));
    }
  };

  auto firstRunInstrument = getInstrumentAndRun(firstPath.getBaseName()).first;

  // 0. Must be a file
  if (filePath.isFile()) {
    // 1. Must be a NeXus file
    if (filePath.getExtension() == "nxs") {
      // 2. Instrument must be correct
      auto fileName = filePath.getBaseName();
      if (fileName.size() > 5) {
        auto fileSplit = getInstrumentAndRun(fileName);
        if (fileSplit.first == firstRunInstrument) {
          // 3. Must end in a number
          valid = std::all_of(fileSplit.second.begin(), fileSplit.second.end(),
                              ::isdigit);
        }
      }
    }
  }
  return valid;
}

} // namespace CustomInterfaces
} // namespace MantidQt
