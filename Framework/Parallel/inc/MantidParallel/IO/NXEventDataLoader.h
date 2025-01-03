// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <H5Cpp.h>

#include <vector>

#include "MantidParallel/DllConfig.h"
#include "MantidParallel/IO/NXEventDataSource.h"
#include "MantidParallel/IO/PulseTimeGenerator.h"
#include "MantidTypes/Core/DateAndTime.h"

namespace Mantid {
namespace Parallel {
namespace IO {

/** NXEventDataLoader is used to load entries from the Nexus NXevent_data
  group, in particular event_index, event_time_zero, event_id, and
  event_time_offset. The class is templated such that the types of
  event_index, event_time_zero, and event_time_offset can be set as required.

  @author Simon Heybrock
  @date 2017
*/
template <class TimeOffsetType> class NXEventDataLoader : public NXEventDataSource<TimeOffsetType> {
public:
  NXEventDataLoader(const int numWorkers, const H5::Group &group, std::vector<std::string> bankNames);

  std::unique_ptr<AbstractEventDataPartitioner<TimeOffsetType>> setBankIndex(const size_t bank) override;

  void readEventID(int32_t *event_id, size_t start, size_t count) const override;
  void readEventTimeOffset(TimeOffsetType *event_time_offset, size_t start, size_t count) const override;
  std::string readEventTimeOffsetUnit() const override;

private:
  const int m_numWorkers;
  const H5::Group m_root;
  H5::Group m_group;
  const std::vector<std::string> m_bankNames;
  H5::DataSet m_id;
  H5::DataSet m_time_offset;
};

namespace detail {
/// Read complete data set from group and return the contents as a vector.
template <class T> std::vector<T> read(const H5::Group &group, const std::string &dataSetName) {
  H5::DataSet dataSet = group.openDataSet(dataSetName);
  H5::DataType dataType = dataSet.getDataType();
  H5::DataSpace dataSpace = dataSet.getSpace();
  std::vector<T> result;
  result.resize(dataSpace.getSelectNpoints());
  dataSet.read(result.data(), dataType);
  return result;
}

/** Read subset of data set and write the result into buffer.
 *
 * The subset is given by a start index and a count. */
template <class T> void read(T *buffer, const H5::DataSet &dataSet, size_t start, size_t count) {
  auto hstart = static_cast<hsize_t>(start);
  auto hcount = static_cast<hsize_t>(count);
  H5::DataType dataType = dataSet.getDataType();
  H5::DataSpace dataSpace = dataSet.getSpace();
  if ((static_cast<int64_t>(dataSpace.getSelectNpoints()) - static_cast<int64_t>(hstart)) <= 0)
    throw std::out_of_range("Start index is beyond end of file");
  if (hcount > dataSpace.getSelectNpoints() - hstart)
    throw std::out_of_range("End index is beyond end of file");
  dataSpace.selectHyperslab(H5S_SELECT_SET, &hcount, &hstart);
  H5::DataSpace memSpace(1, &hcount);
  dataSet.read(buffer, dataType, memSpace, dataSpace);
}

/** Read subset of data set from group and write the result into buffer.
 *
 * The subset is given by a start index and a count. */
template <class T>
void read(T *buffer, const H5::Group &group, const std::string &dataSetName, size_t start, size_t count) {
  H5::DataSet dataSet = group.openDataSet(dataSetName);
  read(buffer, dataSet, start, count);
}

std::string MANTID_PARALLEL_DLL readAttribute(const H5::DataSet &dataSet, const std::string &attributeName);

template <class TimeOffsetType, class IndexType, class TimeZeroType>
std::unique_ptr<AbstractEventDataPartitioner<TimeOffsetType>> makeEventDataPartitioner(const H5::Group &group,
                                                                                       const int numWorkers) {
  const auto timeZero = group.openDataSet("event_time_zero");
  int64_t time_zero_offset{0};
  if (timeZero.attrExists("offset")) {
    const auto &offset = readAttribute(timeZero, "offset");
    time_zero_offset = Types::Core::DateAndTime(offset).totalNanoseconds();
  }
  return std::make_unique<EventDataPartitioner<IndexType, TimeZeroType, TimeOffsetType>>(
      numWorkers, PulseTimeGenerator<IndexType, TimeZeroType>{read<IndexType>(group, "event_index"),
                                                              read<TimeZeroType>(group, "event_time_zero"),
                                                              readAttribute(timeZero, "units"), time_zero_offset});
}

template <class R, class... T1, class... T2>
std::unique_ptr<AbstractEventDataPartitioner<R>> makeEventDataPartitioner(const H5::DataType &type, T2 &&...args);

template <class R, class... T1> struct ConditionalFloat {
  template <class... T2>
  static std::unique_ptr<AbstractEventDataPartitioner<R>> forward(const H5::DataType &type, T2 &&...args) {
    if (type == H5::PredType::NATIVE_FLOAT)
      return makeEventDataPartitioner<R, T1..., float>(args...);
    if (type == H5::PredType::NATIVE_DOUBLE)
      return makeEventDataPartitioner<R, T1..., double>(args...);
    throw std::runtime_error("Unsupported H5::DataType for entry in NXevent_data");
  }
};

// Specialization for empty T1, i.e., first type argument `event_index`, which
// must be integer.
template <class R> struct ConditionalFloat<R> {
  template <class... T2>
  static std::unique_ptr<AbstractEventDataPartitioner<R>> forward(const H5::DataType &, T2 &&...) {
    throw std::runtime_error("Unsupported H5::DataType for event_index in "
                             "NXevent_data, must be integer");
  }
};

template <class R, class... T1, class... T2>
std::unique_ptr<AbstractEventDataPartitioner<R>> makeEventDataPartitioner(const H5::DataType &type, T2 &&...args) {
  // Translate from H5::DataType to actual type. Done step by step to avoid
  // combinatoric explosion. The T1 parameter pack holds the final template
  // arguments we want. The T2 parameter pack represents the remaining
  // H5::DataType arguments and any other arguments. In every call we peel off
  // the first entry from the T2 pack and append it to T1. This stops once the
  // next argument in args is not of type H5::DataType anymore, allowing us to
  // pass arbitrary extra arguments in the second part of args.
  if (type == H5::PredType::NATIVE_INT32)
    return makeEventDataPartitioner<R, T1..., int32_t>(args...);
  if (type == H5::PredType::NATIVE_INT64)
    return makeEventDataPartitioner<R, T1..., int64_t>(args...);
  if (type == H5::PredType::NATIVE_UINT32)
    return makeEventDataPartitioner<R, T1..., uint32_t>(args...);
  if (type == H5::PredType::NATIVE_UINT64)
    return makeEventDataPartitioner<R, T1..., uint64_t>(args...);
  // Compile-time branching for float types.
  return ConditionalFloat<R, T1...>::forward(type, args...);
}
} // namespace detail

/** Constructor from group and bank names in group to load from.
 *
 * Template TimeOffsetType -> type used for reading event_time_offset */
template <class TimeOffsetType>
NXEventDataLoader<TimeOffsetType>::NXEventDataLoader(const int numWorkers, const H5::Group &group,
                                                     std::vector<std::string> bankNames)
    : m_numWorkers(numWorkers), m_root(group), m_bankNames(std::move(bankNames)) {}

/// Set the bank index and return a EventDataPartitioner for that bank.
template <class TimeOffsetType>
std::unique_ptr<AbstractEventDataPartitioner<TimeOffsetType>>
NXEventDataLoader<TimeOffsetType>::setBankIndex(const size_t bank) {
  m_group = m_root.openGroup(m_bankNames[bank]);
  m_id = m_group.openDataSet("event_id");
  m_time_offset = m_group.openDataSet("event_time_offset");
  return detail::makeEventDataPartitioner<TimeOffsetType>(m_group.openDataSet("event_index").getDataType(),
                                                          m_group.openDataSet("event_time_zero").getDataType(), m_group,
                                                          m_numWorkers);
}

/// Read subset given by start and count from event_id and write it into buffer.
template <class TimeOffsetType>
void NXEventDataLoader<TimeOffsetType>::readEventID(int32_t *buffer, size_t start, size_t count) const {
  detail::read(buffer, m_id, start, count);
}

/// Read subset given by start and count from event_time_offset and write it
/// into buffer.
template <class TimeOffsetType>
void NXEventDataLoader<TimeOffsetType>::readEventTimeOffset(TimeOffsetType *buffer, size_t start, size_t count) const {
  detail::read(buffer, m_time_offset, start, count);
}

/// Read and return the `units` attribute from event_time_offset.
template <class TimeOffsetType> std::string NXEventDataLoader<TimeOffsetType>::readEventTimeOffsetUnit() const {
  return detail::readAttribute(m_time_offset, "units");
}

} // namespace IO
} // namespace Parallel
} // namespace Mantid
