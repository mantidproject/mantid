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

 Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
