#ifndef MANTID_DATAHANDLING_H5UTIL_H_
#define MANTID_DATAHANDLING_H5UTIL_H_

#include "MantidDataHandling/DllConfig.h"

#include <string>
#include <vector>

namespace H5 {
class DataSpace;
class DSetCreatPropList;
class Group;
}

namespace Mantid {
namespace DataHandling {
namespace H5Util {
/** H5Util : TODO: DESCRIPTION

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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


MANTID_DATAHANDLING_DLL H5::DataSpace getDataSpace(const size_t length);

template <typename NumT> H5::DataSpace getDataSpace(const std::vector<NumT> &data);

/**
 * Sets up the chunking and compression rate.
 * @param length
 * @return The configured property list
 * TODO needs a better name
 */
MANTID_DATAHANDLING_DLL H5::DSetCreatPropList getPropList(const std::size_t length,
                                                          const int deflateLevel=6);

MANTID_DATAHANDLING_DLL void writeStrAttribute(H5::Group &location,
                                               const std::string &name,
                                               const std::string &value);

MANTID_DATAHANDLING_DLL void writeArray(H5::Group &group, const std::string &name,
                const std::string &value);

MANTID_DATAHANDLING_DLL void writeArray(H5::Group &group, const std::string &name,
                const std::vector<double> &values);

MANTID_DATAHANDLING_DLL void writeArray(H5::Group &group, const std::string &name,
                const std::vector<int32_t> &values);

} // namespace H5Util
} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_H5UTIL_H_ */
