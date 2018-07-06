#ifndef MANTID_KERNEL_LIBRARY_MANAGER_H_
#define MANTID_KERNEL_LIBRARY_MANAGER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <string>
#include <unordered_map>
#include <vector>

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/LibraryWrapper.h"
#include "MantidKernel/SingletonHolder.h"

namespace Poco {
class File;
class Path;
}

namespace Mantid {
namespace Kernel {
/**
Class for opening shared libraries.

@author ISIS, STFC
@date 15/10/2007

Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
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
  int openLibraries(const Poco::File &libpath, LoadLibraries loadingBehaviour,
                    const std::vector<std::string> &excludes);
  /// Check if the library should be loaded
  bool shouldBeLoaded(const std::string &filename,
                      const std::vector<std::string> &excludes) const;
  /// Check if the library has already been loaded
  bool isLoaded(const std::string &filename) const;
  /// Returns true if the library has been requested to be excluded
  bool isExcluded(const std::string &filename,
                  const std::vector<std::string> &excludes) const;
  /// Load a given library
  int openLibrary(const Poco::File &filepath, const std::string &cacheKey);

  /// Storage for the LibraryWrappers.
  std::unordered_map<std::string, LibraryWrapper> m_openedLibs;
};

EXTERN_MANTID_KERNEL template class MANTID_KERNEL_DLL
    Mantid::Kernel::SingletonHolder<LibraryManagerImpl>;
using LibraryManager = Mantid::Kernel::SingletonHolder<LibraryManagerImpl>;

} // namespace Kernel
} // namespace Mantid

#endif // MANTID_KERNEL_LIBRARY_MANAGER_H_
