#include "MantidKernel/DllOpen.h"
#include "MantidKernel/LibraryWrapper.h"

namespace Mantid {
namespace Kernel {

/// Destructor
LibraryWrapper::~LibraryWrapper() {
  // Close lib
  if (m_module) {
    DllOpen::closeDll(m_module);
    m_module = nullptr;
  }
}

/** Opens a DLL.
 *  @param libName :: The name of the file to open (not including the
 * lib/so/dll).
 *  @return True if DLL is opened or already open.
 */
bool LibraryWrapper::openLibrary(const std::string &libName) {
  if (!m_module) {
    // Load dynamically loaded library
    m_module = DllOpen::openDll(libName);

    if (!m_module) {
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
bool LibraryWrapper::openLibrary(const std::string &libName,
                                 const std::string &filePath) {
  if (!m_module) {
    // Load dynamically loaded library
    m_module = DllOpen::openDll(libName, filePath);
    if (!m_module) {
      return false;
    }
  }

  return true;
}

} // namespace Kernel
} // namespace Mantid
