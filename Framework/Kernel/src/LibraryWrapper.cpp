#include "MantidKernel/LibraryWrapper.h"
#include "MantidKernel/DllOpen.h"

namespace Mantid {
namespace Kernel {

/**
 * Move constructor
 * @param src Constructor from this temporary
 */
LibraryWrapper::LibraryWrapper(LibraryWrapper &&src) noexcept {
  *this = std::move(src);
}

/**
 * Move assignment
 * @param rhs Temporary object as source of assignment
 */
LibraryWrapper &LibraryWrapper::operator=(LibraryWrapper &&rhs) noexcept {
  using std::swap;
  swap(m_module, rhs.m_module);
  return *this;
}

/// Destructor
LibraryWrapper::~LibraryWrapper() {
  // Close lib
  if (m_module) {
    DllOpen::closeDll(m_module);
    m_module = nullptr;
  }
}

/** Opens a DLL.
 *  @param filepath :: The filepath to the directory where the library is.
 *  @return True if DLL is opened or already open
 */
bool LibraryWrapper::openLibrary(const std::string &filepath) {
  if (!m_module) {
    // Load dynamically loaded library
    m_module = DllOpen::openDll(filepath);
    if (!m_module) {
      return false;
    }
  }
  return true;
}

} // namespace Kernel
} // namespace Mantid
