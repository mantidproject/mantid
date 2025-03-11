// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/LibraryWrapper.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid {
namespace Kernel {
/**
Class for opening shared libraries.

@author ISIS, STFC
@date 15/10/2007
*/
class MANTID_KERNEL_DLL LibraryManagerImpl {
public:
  enum LoadLibraries { Recursive, NonRecursive };
  int openLibraries(const std::string &libpath, LoadLibraries loadingBehaviour,
                    const std::vector<std::string> &excludes);
  LibraryManagerImpl(const LibraryManagerImpl &) = delete;
  LibraryManagerImpl &operator=(const LibraryManagerImpl &) = delete;

private:
  friend struct Mantid::Kernel::CreateUsingNew<LibraryManagerImpl>;

  /// Private Constructor
  LibraryManagerImpl();
  /// Private Destructor
  ~LibraryManagerImpl() = default;

  /// Load libraries from the given Poco::File path
  /// Private so Poco::File doesn't leak to the public interface
  int openLibraries(const std::filesystem::path &libpath, LoadLibraries loadingBehaviour,
                    const std::vector<std::string> &excludes);
  /// Check if the library should be loaded
  bool shouldBeLoaded(const std::string &filename, const std::vector<std::string> &excludes) const;
  /// Check if the library has already been loaded
  bool isLoaded(const std::string &filename) const;
  /// Returns true if the library has been requested to be excluded
  bool isExcluded(const std::string &filename, const std::vector<std::string> &excludes) const;
  /// Load a given library
  int openLibrary(const std::filesystem::path &filepath, const std::string &cacheKey);

  /// Storage for the LibraryWrappers.
  std::unordered_map<std::string, LibraryWrapper> m_openedLibs;
};

EXTERN_MANTID_KERNEL template class MANTID_KERNEL_DLL Mantid::Kernel::SingletonHolder<LibraryManagerImpl>;
using LibraryManager = Mantid::Kernel::SingletonHolder<LibraryManagerImpl>;

} // namespace Kernel
} // namespace Mantid
