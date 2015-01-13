#include "MantidKernel/DllOpen.h"
#include "MantidKernel/LibraryWrapper.h"

namespace Mantid {
namespace Kernel {

/// Constructor
LibraryWrapper::LibraryWrapper() : module(0) {}

/// Destructor
LibraryWrapper::~LibraryWrapper() {
  // Close lib
  if (module) {
    DllOpen::CloseDll(module);
    module = 0;
  }
}

/** Opens a DLL.
 *  @param libName :: The name of the file to open (not including the
 * lib/so/dll).
 *  @return True if DLL is opened or already open.
 */
bool LibraryWrapper::OpenLibrary(const std::string &libName) {
  if (!module) {
    // Load dynamically loaded library
    module = DllOpen::OpenDll(libName);

    if (!module) {
      return false;
    }
  }

  return true;
}

/** Opens a DLL.
 *  @param libName :: The name of the file to open (not including the
 * lib/so/dll).
 *  @param filePath :: The filepath to the directory where the library is.
 *  @return True if DLL is opened or already open
 */
bool LibraryWrapper::OpenLibrary(const std::string &libName,
                                 const std::string &filePath) {
  if (!module) {
    // Load dynamically loaded library
    module = DllOpen::OpenDll(libName, filePath);
    if (!module) {
      return false;
    }
  }

  return true;
}

} // namespace Kernel
} // namespace Mantid
