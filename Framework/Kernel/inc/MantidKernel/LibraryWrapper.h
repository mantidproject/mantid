// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_LIBRARY_WRAPPER_H_
#define MANTID_KERNEL_LIBRARY_WRAPPER_H_

#include <string>

#include "MantidKernel/DllConfig.h"

namespace Mantid {
namespace Kernel {

/** @class LibraryWrapper LibraryWrapper.h Kernel/LibraryWrapperr.h

 Class for wrapping a shared library.

 @author ISIS, STFC
 @date 10/01/2008
 */
class MANTID_KERNEL_DLL LibraryWrapper {
public:
  // Move-only class. The internal module pointer
  // is not safe to copy around.
  LibraryWrapper() = default;
  // No copy
  LibraryWrapper(const LibraryWrapper &) = delete;
  LibraryWrapper &operator=(const LibraryWrapper &) = delete;

  LibraryWrapper(LibraryWrapper &&src) noexcept;
  LibraryWrapper &operator=(LibraryWrapper &&rhs) noexcept;
  ~LibraryWrapper();

  bool openLibrary(const std::string &filepath);

private:
  /** An untyped pointer to the loaded library.
   * This is created and deleted by this class.
   **/
  void *m_module = nullptr;
};

} // namespace Kernel
} // namespace Mantid

#endif // MANTID_KERNEL_LIBRARY_WRAPPER_H_
