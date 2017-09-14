#ifndef MANTID_PARALLEL_IO_NXEVENTDATALOADER_H_
#define MANTID_PARALLEL_IO_NXEVENTDATALOADER_H_

#include <vector>
#include <H5Cpp.h>

#include "MantidParallel/DllConfig.h"

namespace Mantid {
namespace Parallel {
namespace IO {

/** NXEventDataLoader is used to load entries from the the Nexus NXevent_data
  group, in particular event_index, event_time_zero, event_id, and
  event_time_offset. The class is templated such that the types of
  event_time_zero and event_time_offset can be set as required.

  @author Simon Heybrock
  @date 2017

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
template <class TimeZeroType, class TimeOffsetType>
class MANTID_PARALLEL_DLL NXEventDataLoader {
public:
  NXEventDataLoader(const H5::H5File &file, const std::string &nxEventDataPath);

  const std::vector<int64_t> &eventIndex() const;
  const std::vector<TimeZeroType> &eventTimeZero() const;
  void readEventID(int32_t *event_id, size_t start, size_t count) const;
  void readEventTimeOffset(TimeOffsetType *event_time_offset, size_t start,
                           size_t count) const;

private:
  const H5::H5File &m_file;
  const std::string m_rootPath;
  const std::string m_id_path;
  const std::string m_time_offset_path;
  std::vector<int64_t> m_index;
  std::vector<TimeZeroType> m_time_zero;
};

namespace detail {
/// Read complete data set from file and return the contents as a vector.
template <class T>
std::vector<T> read(const H5::H5File &file, const std::string &dataSetName) {
  H5::DataSet dataSet = file.openDataSet(dataSetName);
  H5::DataType dataType = dataSet.getDataType();
  H5::DataSpace dataSpace = dataSet.getSpace();
  std::vector<T> result;
  result.resize(dataSpace.getSelectNpoints());
  dataSet.read(result.data(), dataType);
  return result;
}

/** Read subset of data set from file and write the result into buffer.
 *
 * The subset is given by a start index and a count. */
template <class T>
void read(T *buffer, const H5::H5File &file, const std::string &dataSetName,
          size_t start, size_t count) {
  auto hstart = static_cast<hsize_t>(start);
  auto hcount = static_cast<hsize_t>(count);
  H5::DataSet dataSet = file.openDataSet(dataSetName);
  H5::DataType dataType = dataSet.getDataType();
  H5::DataSpace dataSpace = dataSet.getSpace();
  if ((static_cast<int64_t>(dataSpace.getSelectNpoints()) -
       static_cast<int64_t>(hstart)) <= 0)
    throw std::out_of_range("Start index is beyond end of file");
  if (hcount > dataSpace.getSelectNpoints() - hstart)
    throw std::out_of_range("End index is beyond end of file");
  dataSpace.selectHyperslab(H5S_SELECT_SET, &hcount, &hstart);
  H5::DataSpace memSpace(1, &hcount);
  dataSet.read(buffer, dataType, memSpace, dataSpace);
}
}

/// Constructor from file and path to NXevent_data group to load from.
template <class TimeZeroType, class TimeOffsetType>
NXEventDataLoader<TimeZeroType, TimeOffsetType>::NXEventDataLoader(
    const H5::H5File &file, const std::string &nxEventDataPath)
    : m_file(file), m_rootPath(nxEventDataPath),
      m_id_path(m_rootPath + "/event_id"),
      m_time_offset_path(m_rootPath + "/event_time_offset") {
  m_index = detail::read<int64_t>(file, m_rootPath + "/event_index");
  m_time_zero =
      detail::read<TimeZeroType>(file, m_rootPath + "/event_time_zero");
}

/// Returns a reference to the vector read from event_index.
template <class TimeZeroType, class TimeOffsetType>
const std::vector<int64_t> &
NXEventDataLoader<TimeZeroType, TimeOffsetType>::eventIndex() const {
  return m_index;
}

/// Returns a reference to the vector read from event_time_zero.
template <class TimeZeroType, class TimeOffsetType>
const std::vector<TimeZeroType> &
NXEventDataLoader<TimeZeroType, TimeOffsetType>::eventTimeZero() const {
  return m_time_zero;
}

/// Read subset given by start and count from event_id and write it into buffer.
template <class TimeZeroType, class TimeOffsetType>
void NXEventDataLoader<TimeZeroType, TimeOffsetType>::readEventID(
    int32_t *buffer, size_t start, size_t count) const {
  detail::read<int32_t>(buffer, m_file, m_id_path, start, count);
}

/// Read subset given by start and count from event_time_offset and write it
/// into buffer.
template <class TimeZeroType, class TimeOffsetType>
void NXEventDataLoader<TimeZeroType, TimeOffsetType>::readEventTimeOffset(
    TimeOffsetType *buffer, size_t start, size_t count) const {
  detail::read<TimeOffsetType>(buffer, m_file, m_time_offset_path, start,
                               count);
}

} // namespace IO
} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_IO_NXEVENTDATALOADER_H_ */
