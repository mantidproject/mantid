#include "MantidQtCustomInterfaces/Muon/ALCLatestFileFinder.h"
#include <Poco/DirectoryIterator.h>

namespace MantidQt {
namespace CustomInterfaces {

/**
 * Gets the most recently modified Nexus file in the same directory as the first
 * run
 * @returns Path to most recently modified file
 */
std::string ALCLatestFileFinder::getMostRecentFile() const {
  if (m_firstRunFileName.empty()) {
    return "";
  } else {
    Poco::Path path(m_firstRunFileName);
    Poco::File latestFile(path);
    Poco::Timestamp lastModified(0); // start at Epoch
    try {
      // Directory iterator - check if we were passed a file or a directory
      Poco::DirectoryIterator iter(latestFile.isDirectory() ? path
                                                            : path.parent());
      Poco::DirectoryIterator end; // the end iterator
      while (iter != end) {
        if (Poco::Path(iter->path()).getExtension() == "nxs") {
          if (iter->getLastModified() > lastModified) {
            latestFile = *iter;
            lastModified = iter->getLastModified();
          }
        }
        ++iter;
      }
    } catch (const Poco::Exception &) {
      // There was some problem iterating through the directory.
      // Return the file we were given.
      return path.toString();
    }
    return latestFile.path();
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
