#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DllOpen.h"
#include "MantidKernel/LibraryManager.h"
#include "MantidKernel/LibraryWrapper.h"
#include "MantidKernel/Logger.h"

#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include <boost/algorithm/string.hpp>

namespace Mantid {
namespace Kernel {
namespace {
/// static logger
Logger g_log("LibraryManager");
}

/// Constructor
LibraryManagerImpl::LibraryManagerImpl() : m_openedLibs() {
  g_log.debug() << "LibraryManager created.\n";
}

/**
 * Opens suitable DLLs on a given path.
 *  @param filePath The filepath to the directory where the libraries are.
 *  @param loadingBehaviour Control how libraries are searched for
 *  @param excludes If not empty then each string is considered as a substring
 * to search within each library to be opened. If the substring is found then
 * the library is not opened.
 *  @return The number of libraries opened.
 */
int LibraryManagerImpl::openLibraries(
    const std::string &filePath, LoadLibraries loadingBehaviour,
    const std::vector<std::string> &excludes) {
  g_log.debug() << "Opening all libraries in " << filePath << "\n";
  try {
    return openLibraries(Poco::File(filePath), loadingBehaviour, excludes);
  } catch (std::exception &exc) {
    g_log.debug() << "Error occurred while opening libraries: " << exc.what()
                  << "\n";
    return 0;
  } catch (...) {
    g_log.error() << "An unknown error occurred while opening libraries.";
    return 0;
  }
}

//-------------------------------------------------------------------------
// Private members
//-------------------------------------------------------------------------
/**
 * Opens suitable DLLs on a given path.
 *  @param filePath A Poco::File object pointing to a directory where the
 * libraries are.
 *  @param loadingBehaviour Control how libraries are searched for
 *  @param excludes If not empty then each string is considered as a substring
 * to search within each library to be opened. If the substring is found then
 * the library is not opened.
 *  @return The number of libraries opened.
 */
int LibraryManagerImpl::openLibraries(
    const Poco::File &libPath,
    LibraryManagerImpl::LoadLibraries loadingBehaviour,
    const std::vector<std::string> &excludes) {
  int libCount(0);
  if (libPath.exists() && libPath.isDirectory()) {
    DllOpen::addSearchDirectory(libPath.path());
    // Iterate over the available files
    Poco::DirectoryIterator end_itr;
    for (Poco::DirectoryIterator itr(libPath); itr != end_itr; ++itr) {
      const Poco::File &item = *itr;
      if (item.isFile()) {
        if (skipLibrary(itr.path().getFileName(), excludes))
          continue;
        if (loadLibrary(itr.path())) {
          ++libCount;
        }
      } else if (loadingBehaviour == LoadLibraries::Recursive) {
        // it must be a directory
        libCount += openLibraries(item, LoadLibraries::Recursive, excludes);
      }
    }
  } else {
    g_log.error("In OpenAllLibraries: " + libPath.path() +
                " must be a directory.");
  }
  return libCount;
}

/**
 * Returns true if the name contains one of the strings given in the
 * exclude list. Each string from the variable is
 * searched for with the filename so an exact match is not necessary. This
 * avoids having to specify prefixes and suffixes for different platforms,
 * i.e. 'plugins.exclude = MantidKernel' will exclude libMantidKernel.so
 * @param filename A string giving the filename/file path
 * @param excludes A list of substrings to exclude library from loading
 * @return True if the library should be skipped
 */
bool LibraryManagerImpl::skipLibrary(const std::string &filename,
                                     const std::vector<std::string> &excludes) {
  bool skipme(false);
  for (const auto &exclude : excludes) {
    if (filename.find(exclude) != std::string::npos) {
      skipme = true;
      break;
    }
  }
  return skipme;
}

/**
* Load a library
* @param filepath :: A Poco::File The full path to a library as a string
*/
bool LibraryManagerImpl::loadLibrary(Poco::Path filepath) {
  // Get the name of the library.
  std::string libName = DllOpen::convertToLibName(filepath.getFileName());
  if (libName.empty())
    return false;
  // Check that a library with this name has not already been loaded
  if (m_openedLibs.find(boost::algorithm::to_lower_copy(libName)) ==
      m_openedLibs.end()) {
    filepath.makeParent();
    // Try to open the library. The wrapper will unload the library when it
    // is deleted
    LibraryWrapper dlwrap;
    if (dlwrap.openLibrary(libName, filepath.toString())) {
      // Successfully opened, so add to map
      if (g_log.is(Poco::Message::PRIO_DEBUG)) {
        g_log.debug("Opened library: " + libName + ".\n");
      }
      m_openedLibs.emplace(libName, std::move(dlwrap));
      return true;
    } else {
      return false;
    }
  } else {
    g_log.debug() << libName << " already opened, skipping load\n";
  }
  return false;
}

} // namespace Kernel
} // namespace Mantid
