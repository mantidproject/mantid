#ifndef MANTID_PARALLEL_IO_NXEVENTDATALOADER_H_
#define MANTID_PARALLEL_IO_NXEVENTDATALOADER_H_

#include <vector>
#include <H5Cpp.h>

#include "MantidParallel/DllConfig.h"
#include "MantidParallel/IO/NXEventDataSource.h"
#include "MantidTypes/Core/DateAndTime.h"

namespace Mantid {
namespace Parallel {
namespace IO {

/** NXEventDataLoader is used to load entries from the the Nexus NXevent_data
  group, in particular event_index, event_time_zero, event_id, and
  event_time_offset. The class is templated such that the types of
  event_index, event_time_zero, and event_time_offset can be set as required.

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
template <class IndexType, class TimeZeroType, class TimeOffsetType>
class NXEventDataLoader
    : public NXEventDataSource<IndexType, TimeZeroType, TimeOffsetType> {
public:
  NXEventDataLoader(const H5::Group &group, std::vector<std::string> bankNames);

  void setBankIndex(const size_t bank) override;

  const std::vector<IndexType> &eventIndex() const override;
  const std::vector<TimeZeroType> &eventTimeZero() const override;
  int64_t eventTimeZeroOffset() const override;
  void readEventID(int32_t *event_id, size_t start,
                   size_t count) const override;
  void readEventTimeOffset(TimeOffsetType *event_time_offset, size_t start,
                           size_t count) const override;

private:
  const H5::Group m_root;
  H5::Group m_group;
  const std::vector<std::string> m_bankNames;
  const std::string m_id_path;
  const std::string m_time_offset_path;
  std::vector<IndexType> m_index;
  std::vector<TimeZeroType> m_time_zero;
  int64_t m_time_zero_offset{0};
};

namespace detail {
/// Read complete data set from group and return the contents as a vector.
template <class T>
std::vector<T> read(const H5::Group &group, const std::string &dataSetName) {
  H5::DataSet dataSet = group.openDataSet(dataSetName);
  H5::DataType dataType = dataSet.getDataType();
  H5::DataSpace dataSpace = dataSet.getSpace();
  std::vector<T> result;
  result.resize(dataSpace.getSelectNpoints());
  dataSet.read(result.data(), dataType);
  return result;
}

/** Read subset of data set from group and write the result into buffer.
 *
 * The subset is given by a start index and a count. */
template <class T>
void read(T *buffer, const H5::Group &group, const std::string &dataSetName,
          size_t start, size_t count) {
  auto hstart = static_cast<hsize_t>(start);
  auto hcount = static_cast<hsize_t>(count);
  H5::DataSet dataSet = group.openDataSet(dataSetName);
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

/** Constructor from group and bank names in group to load from.
 *
 * Template arguments are:
 * - IndexType      -> type used for reading event_index
 * - TimeZeroType   -> type used for reading event_time_zero
 * - TimeOffsetType -> type used for reading event_time_offset */
template <class IndexType, class TimeZeroType, class TimeOffsetType>
NXEventDataLoader<IndexType, TimeZeroType, TimeOffsetType>::NXEventDataLoader(
    const H5::Group &group, std::vector<std::string> bankNames)
    : m_root(group), m_bankNames(std::move(bankNames)) {}

template <class IndexType, class TimeZeroType, class TimeOffsetType>
void NXEventDataLoader<IndexType, TimeZeroType, TimeOffsetType>::setBankIndex(
    const size_t bank) {
  m_group = m_root.openGroup(m_bankNames[bank]);
  m_index = detail::read<IndexType>(m_group, "event_index");
  m_time_zero = detail::read<TimeZeroType>(m_group, "event_time_zero");
  const auto dataSet = m_group.openDataSet("event_time_zero");
  if (dataSet.attrExists("offset")) {
    const auto &attr = dataSet.openAttribute("offset");
    std::string offset;
    attr.read(attr.getDataType(), offset);
    m_time_zero_offset = Types::Core::DateAndTime(offset).totalNanoseconds();
  }
}

/// Returns a reference to the vector read from event_index.
template <class IndexType, class TimeZeroType, class TimeOffsetType>
const std::vector<IndexType> &
NXEventDataLoader<IndexType, TimeZeroType, TimeOffsetType>::eventIndex() const {
  return m_index;
}

/// Returns a reference to the vector read from event_time_zero.
template <class IndexType, class TimeZeroType, class TimeOffsetType>
const std::vector<TimeZeroType> &
NXEventDataLoader<IndexType, TimeZeroType, TimeOffsetType>::eventTimeZero()
    const {
  return m_time_zero;
}

/// Returns the offset attribute read from event_time_zero, in nano seconds.
template <class IndexType, class TimeZeroType, class TimeOffsetType>
int64_t NXEventDataLoader<IndexType, TimeZeroType,
                          TimeOffsetType>::eventTimeZeroOffset() const {
  return m_time_zero_offset;
}

/// Read subset given by start and count from event_id and write it into buffer.
template <class IndexType, class TimeZeroType, class TimeOffsetType>
void NXEventDataLoader<IndexType, TimeZeroType, TimeOffsetType>::readEventID(
    int32_t *buffer, size_t start, size_t count) const {
  detail::read<int32_t>(buffer, m_group, "event_id", start, count);
}

/// Read subset given by start and count from event_time_offset and write it
/// into buffer.
template <class IndexType, class TimeZeroType, class TimeOffsetType>
void NXEventDataLoader<IndexType, TimeZeroType, TimeOffsetType>::
    readEventTimeOffset(TimeOffsetType *buffer, size_t start,
                        size_t count) const {
  detail::read<TimeOffsetType>(buffer, m_group, "event_time_offset", start,
                               count);
}

} // namespace IO
} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_IO_NXEVENTDATALOADER_H_ */
