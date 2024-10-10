// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/LibraryManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DllOpen.h"
#include "MantidKernel/LibraryWrapper.h"
#include "MantidKernel/Logger.h"

#include <filesystem>

namespace Mantid::Kernel {
namespace {
/// static logger
Logger g_log("LibraryManager");
} // namespace

/// Constructor
LibraryManagerImpl::LibraryManagerImpl() : m_openedLibs() { g_log.debug("LibraryManager created."); }

/**
 * Opens suitable DLLs on a given path.
 *  @param filepath The filepath to the directory where the libraries are.
 *  @param loadingBehaviour Control how libraries are searched for
 *  @param excludes If not empty then each string is considered as a substring
 * to search within each library to be opened. If the substring is found then
 * the library is not opened.
 *  @return The number of libraries opened.
 */
int LibraryManagerImpl::openLibraries(const std::string &filepath, LoadLibraries loadingBehaviour,
                                      const std::vector<std::string> &excludes) {
  g_log.debug("Opening all libraries in " + filepath + "\n");
  try {
    return openLibraries(std::filesystem::path(filepath), loadingBehaviour, excludes);
  } catch (std::exception &exc) {
    g_log.debug() << "Error occurred while opening libraries: " << exc.what() << "\n";
    return 0;
  } catch (...) {
    g_log.error("An unknown error occurred while opening libraries.");
    return 0;
  }
}

//-------------------------------------------------------------------------
// Private members
//-------------------------------------------------------------------------
/**
 * Opens suitable DLLs on a given path.
 *  @param libpath An std::filesystem::path object pointing to a directory where the
 * libraries are.
 *  @param loadingBehaviour Control how libraries are searched for
 *  @param excludes If not empty then each string is considered as a substring
 * to search within each library to be opened. If the substring is found then
 * the library is not opened.
 *  @return The number of libraries opened.
 */
int LibraryManagerImpl::openLibraries(const std::filesystem::path &libpath,
                                      LibraryManagerImpl::LoadLibraries loadingBehaviour,
                                      const std::vector<std::string> &excludes) {
  int libCount(0);
  if (std::filesystem::exists(libpath) && std::filesystem::is_directory(libpath)) {
    // Iterate over the available files
    for (const auto &file : std::filesystem::directory_iterator(libpath)) {
      const auto &path = file.path();
      if (std::filesystem::is_regular_file(path)) {
        if (shouldBeLoaded(path.filename().string(), excludes))
          libCount += openLibrary(path.string(), path.filename().string());
        else
          continue;
      } else if (loadingBehaviour == LoadLibraries::Recursive) {
        // it must be a directory
        libCount += openLibraries(path, LoadLibraries::Recursive, excludes);
      }
    }
  } else {
    g_log.error("In OpenAllLibraries: " + libpath.string() + " must be a directory.");
  }
  return libCount;
}

/**
 * Check if the library should be loaded
 * @param filename The filename of the library, i.e no directory
 * @param excludes If not empty then each string is considered as a substring
 * to search within each library to be opened. If the substring is found then
 * the library is not opened.
 * @return True if loading should be attempted
 */
bool LibraryManagerImpl::shouldBeLoaded(const std::string &filename, const std::vector<std::string> &excludes) const {
  return !isLoaded(filename) && DllOpen::isValidFilename(filename) && !isExcluded(filename, excludes);
}

/**
 * Check if the library been loaded already?
 * @param filename The filename of the library, i.e no directory
 * @return True if the library has been seen before
 */
bool LibraryManagerImpl::isLoaded(const std::string &filename) const {
  return m_openedLibs.find(filename) != m_openedLibs.cend();
}

/**
 * Returns true if the name contains one of the strings given in the
 * exclude list. Each string from the variable is
 * searched for with the filename so an exact match is not necessary. This
 * avoids having to specify prefixes and suffixes for different platforms,
 * i.e. 'plugins.exclude = MantidKernel' will exclude libMantidKernel.so
 * @param filename The filename of the library (no directory)
 * @param excludes A list of substrings to exclude library from loading
 * @return True if the library should be skipped
 */
bool LibraryManagerImpl::isExcluded(const std::string &filename, const std::vector<std::string> &excludes) const {
  return std::any_of(excludes.cbegin(), excludes.cend(),
                     [&filename](const auto exclude) { return filename.find(exclude) != std::string::npos; });
}

/**
 * Load a library
 * @param filepath :: An std::filesystem::path The full path to a library as a string
 * @param cacheKey :: An identifier for the cache if loading is successful
 * @return 1 if the file loaded successfully, 0 otherwise
 */
int LibraryManagerImpl::openLibrary(const std::filesystem::path &filepath, const std::string &cacheKey) {
  // Try to open the library. The wrapper will unload the library when it
  // is deleted
  LibraryWrapper dlwrap;
  if (dlwrap.openLibrary(filepath.string())) {
    // Successfully opened, so add to map
    if (g_log.is(Poco::Message::PRIO_DEBUG)) {
      g_log.debug("Opened library: " + filepath.string() + ".\n");
    }
    m_openedLibs.emplace(cacheKey, std::move(dlwrap));
    return 1;
  } else
    return 0;
}

} // namespace Mantid::Kernel
