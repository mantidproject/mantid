#ifndef MANTID_DATAHANDLING_H5UTIL_H_
#define MANTID_DATAHANDLING_H5UTIL_H_

#include "MantidDataHandling/DllConfig.h"

#include <string>
#include <vector>
#include <map>

// forward declarations
namespace H5 {
class DataSpace;
class DataSet;
class DSetCreatPropList;
class DataType;
class Group;
class H5File;
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

/// Create a 1D data-space to hold data of length.
MANTID_DATAHANDLING_DLL H5::DataSpace getDataSpace(const size_t length);

/// Create a 1D data-space that will hold the supplied vector.
template <typename NumT>
H5::DataSpace getDataSpace(const std::vector<NumT> &data);

/// Convert a primitive type to the appropriate H5::DataType.
template <typename NumT> H5::DataType getType();

MANTID_DATAHANDLING_DLL H5::Group createGroupNXS(H5::H5File &file,
                                                 const std::string &name,
                                                 const std::string &nxtype);

MANTID_DATAHANDLING_DLL H5::Group createGroupNXS(H5::Group &group,
                                                 const std::string &name,
                                                 const std::string &nxtype);

MANTID_DATAHANDLING_DLL H5::Group createGroupCanSAS(H5::Group &group,
                                                    const std::string &name,
                                                    const std::string &nxtype,
                                                    const std::string &cstype);

MANTID_DATAHANDLING_DLL H5::Group createGroupCanSAS(H5::H5File &file,
                                                    const std::string &name,
                                                    const std::string &nxtype,
                                                    const std::string &cstype);

/**
 * Sets up the chunking and compression rate.
 * @param length
 * @param deflateLevel
 * @return The configured property list
 */
MANTID_DATAHANDLING_DLL H5::DSetCreatPropList
setCompressionAttributes(const std::size_t length, const int deflateLevel = 6);

template <typename LocationType>
void writeStrAttribute(LocationType &location, const std::string &name,
                       const std::string &value);

template <typename NumT, typename LocationType>
void writeNumAttribute(LocationType &location, const std::string &name,
                       const NumT &value);

template <typename NumT, typename LocationType>
void writeNumAttribute(LocationType &location, const std::string &name,
                       const std::vector<NumT> &value);

MANTID_DATAHANDLING_DLL void write(H5::Group &group, const std::string &name,
                                   const std::string &value);

template <typename T>
void writeScalarDataSetWithStrAttributes(
    H5::Group &group, const std::string &name, const T &value,
    const std::map<std::string, std::string> &attributes);

template <typename NumT>
void writeArray1D(H5::Group &group, const std::string &name,
                  const std::vector<NumT> &values);

MANTID_DATAHANDLING_DLL std::string readString(H5::H5File &file,
                                               const std::string &path);

MANTID_DATAHANDLING_DLL std::string readString(H5::Group &group,
                                               const std::string &name);

MANTID_DATAHANDLING_DLL std::string readString(H5::DataSet &dataset);

MANTID_DATAHANDLING_DLL std::vector<std::string>
readStringVector(H5::Group &, const std::string &);

template <typename LocationType>
std::string readAttributeAsString(LocationType &dataset,
                                  const std::string &attributeName);

template <typename NumT, typename LocationType>
NumT readNumAttributeCoerce(LocationType &location,
                            const std::string &attributeName);

template <typename NumT, typename LocationType>
std::vector<NumT> readNumArrayAttributeCoerce(LocationType &location,
                                              const std::string &attributeName);

template <typename NumT>
std::vector<NumT> readArray1DCoerce(H5::Group &group, const std::string &name);

template <typename NumT>
std::vector<NumT> readArray1DCoerce(H5::DataSet &dataset);

} // namespace H5Util
} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_H5UTIL_H_ */
