#ifndef MANTID_KERNEL_DLLOPEN_H_
#define MANTID_KERNEL_DLLOPEN_H_

#include <string>

#include "MantidKernel/DllConfig.h"

namespace Mantid {
namespace Kernel {

/** @class DllOpen DllOpen.h

 Simple class for opening shared libraries at run-time. Works for Windows and
 Linux.

 @author ISIS, STFC
 @date 25/10/2007

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
class MANTID_KERNEL_DLL DllOpen {
public:
  // Unconstructible
  DllOpen() = delete;
  // Not copyable
  DllOpen(const DllOpen &) = delete;
  ~DllOpen() = delete;

public:
  /// Check if the filename conforms to the expected style for this platform
  static bool isValidFilename(const std::string &filename);

  /// Static method for opening the shared library
  static void *openDll(const std::string &filepath);

  /// Static method for closing the shared library
  static void closeDll(void *handle);

private:
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_DLLOPEN_H_*/
