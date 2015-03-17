#ifndef MANTID_KERNEL_CHECKSUMHELPER_H_
#define MANTID_KERNEL_CHECKSUMHELPER_H_

#include "MantidKernel/DllConfig.h"
#include <string>

namespace Mantid {
namespace Kernel {

/** ChecksumHelper : A selection of helper methods for calculating checksums

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
namespace ChecksumHelper {
/// create a md5 checksum from a string
MANTID_KERNEL_DLL std::string md5FromString(const std::string &input);

/// create a SHA-1 checksum from a string
MANTID_KERNEL_DLL std::string sha1FromString(const std::string &input);
/// create a SHA-1 checksum from a file
MANTID_KERNEL_DLL std::string sha1FromFile(const std::string &filepath);
/// create a git checksum from a file (these match the git hash-object command)
MANTID_KERNEL_DLL std::string gitSha1FromFile(const std::string &filepath);
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_CHECKSUMHELPER_H_ */
