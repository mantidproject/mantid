#ifndef MANTID_KERNEL_CHECKSUMHELPER_H_
#define MANTID_KERNEL_CHECKSUMHELPER_H_

#include "MantidKernel/DllConfig.h"


namespace Mantid
{
namespace Kernel
{

/** ChecksumHelper : A selection of helper methods for calculating checksums

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
namespace ChecksumHelper 
{
  std::string MANTID_KERNEL_DLL sha1FromString(const std::string& input);
  std::string MANTID_KERNEL_DLL sha1FromFile(const std::string& filepath);
  std::string MANTID_KERNEL_DLL gitSha1FromFile(const std::string& filepath);

  
  std::string MANTID_KERNEL_DLL processSha1(const char* data, const size_t dataLength, const char* header = NULL, const size_t headerLength = 0);
  std::string MANTID_KERNEL_DLL sha1ToString(const char *hash);
};


} // namespace Kernel
} // namespace Mantid

#endif  /* MANTID_KERNEL_CHECKSUMHELPER_H_ */