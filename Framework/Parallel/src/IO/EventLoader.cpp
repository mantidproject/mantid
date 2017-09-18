#include <H5Cpp.h>

#include "MantidKernel/make_unique.h"
#include "MantidParallel/Communicator.h"
#include "MantidParallel/IO/Chunker.h"
#include "MantidParallel/IO/EventLoader.h"
#include "MantidParallel/IO/NXEventDataLoader.h"

namespace Mantid {
namespace Parallel {
namespace IO {

namespace {
/// Read number of events in given banks from file.
std::vector<size_t> readBankSizes(const H5::Group &group,
                                  const std::vector<std::string> &bankNames) {
  std::vector<size_t> bankSizes;
  for (const auto &bankName : bankNames) {
    const H5::DataSet dataset = group.openDataSet(bankName + "/event_id");
    const H5::DataSpace dataSpace = dataset.getSpace();
    bankSizes.push_back(dataSpace.getSelectNpoints());
  }
  return bankSizes;
}

H5::DataType readDataType(const H5::Group &group,
                          const std::vector<std::string> &bankNames,
                          const std::string &name) {
  return group.openDataSet(bankNames.front() + "/" + name).getDataType();
}

// TODO make this factory functions for EventLoader? Do we need non-templated base class? -> next non-templated base for EventLoader?
// why do we need an instance at all? just pass args through loader functions?
/*
void load(const EventLoader &eventLoader, const H5::DataType &indexType,
          const H5::DataType &timeZeroType,
          const H5::DataType &timeOffsetType) {
  // Translate from DataType to actual type. Done in three steps to avoid
  // combinatoric explosion. Step 1: event_index.
  if (indexType == H5::PredType::NATIVE_INT64)
    return load<int64_t>(eventLoader, timeZeroType, timeOffsetType);
  if (indexType == H5::PredType::NATIVE_DOUBLE)
    return load<int32_t>(eventLoader, timeZeroType, timeOffsetType);
  throw std::runtime_error("Unsupported H5::DataType of event_index");
}

template <class IndexType>
void load(const EventLoader &eventLoader, const H5::DataType &timeZeroType,
          const H5::DataType &timeOffsetType) {
  // DataType translation step 2: event_time_zero.
  if (timeZeroType == H5::PredType::NATIVE_INT64)
    return load<IndexType, int64_t>(eventLoader, timeOffsetType);
  if (timeZeroType == H5::PredType::NATIVE_DOUBLE)
    return load<IndexType, double>(eventLoader, timeOffsetType);
  throw std::runtime_error("Unsupported H5::DataType of event_time_zero");
}

template <class IndexType, class TimeZeroType>
void load(const EventLoader &eventLoader, const H5::DataType &timeOffsetType) {
  // DataType translation step 3: event_time_offset.
  if (timeOffsetType == H5::PredType::NATIVE_INT32)
    return eventLoader.load<IndexType, TimeZeroType, int32_t>();
  if (timeOffsetType == H5::PredType::NATIVE_FLOAT)
    return eventLoader.load<IndexType, TimeZeroType, float>();
  throw std::runtime_error("Unsupported H5::DataType of event_time_offset");
}
*/

template <class IndexType, class TimeZeroType, class TimeOffsetType>
void load(
    const Chunker &chunker,
    NXEventDataSource<IndexType, TimeZeroType, TimeOffsetType> &dataSource) {
  // TODO automatically(?) determine good chunk size
  // TODO automatically(?) determine good number of ranks to use for load
  const size_t chunkSize = 1024 * 1024;
  const auto &ranges = chunker.makeLoadRanges();
  // const auto &rankGroups = chunker.makeRankGroups();
  std::vector<int32_t> event_id(2 * chunkSize);
  std::vector<TimeOffsetType> event_time_offset(2 * chunkSize);

  // TODO Create parser. Constructor arguments:
  // - rankGroups (parser must insert events received from ranks within group
  //   always in that order, to preserve pulse time ordering)
  // - m_bankOffsets (use to translate from event_id to global spectrum index)
  // - m_eventLists (index is workspace index)
  // - Later: Pass something to translate (global spectrum index -> (rank,
  //   workspace index))
  int64_t previousBank = -1;
  for (const auto &range : ranges) {
    if (static_cast<int64_t>(range.bankIndex) != previousBank) {
      dataSource.setBankIndex(range.bankIndex);
      // TODO
      // parser.setEventIndex(loader->eventIndex());
      // parser.setEventTimeZero(loader->eventTimeZero());
    }
    // TODO use double buffer or something
    // parser.wait()
    // TODO use and manage bufferOffset
    dataSource.readEventID(event_id.data(), range.eventOffset,
                           range.eventCount);
    dataSource.readEventTimeOffset(event_time_offset.data(), range.eventOffset,
                                   range.eventCount);
    // TODO
    // parser.startProcessChunk(event_id.data() + bufferOffset,
    //                          event_time_offset.data() + bufferOffset,
    //                          range.eventCount);
    // parser can assume that event_index and event_time_zero stay the same and
    // chunks are ordered, i.e., current position in event_index can be reused,
    // no need to iterate in event_index from the start for every chunk.
  }
}

template <class IndexType, class TimeZeroType, class TimeOffsetType>
void load(const H5::Group &group, const std::vector<std::string> &bankNames,
          const std::vector<int32_t> &bankOffsets,
          std::vector<std::vector<TofEvent> *> eventLists) {
  const size_t chunkSize = 1024 * 1024;
  Communicator comm;
  const Chunker chunker(comm.size(), comm.rank(),
                        readBankSizes(group, bankNames), chunkSize);
  NXEventDataLoader<IndexType, TimeZeroType, TimeOffsetType> loader(group,
                                                                    bankNames);
  //EventParser consumer(bankOffsets, eventLists);
  static_cast<void>(bankOffsets);
  static_cast<void>(eventLists);
  load<IndexType, TimeZeroType, TimeOffsetType>(chunker, loader);
}

template <class... T1, class... T2>
void load(const H5::Group &group, const std::vector<std::string> &bankNames,
          const std::vector<int32_t> &bankOffsets,
          std::vector<std::vector<TofEvent> *> eventLists,
          const H5::DataType &type, T2 &&... types) {
  if (type == H5::PredType::NATIVE_INT32)
    return load<T1..., int32_t>(group, bankNames, bankOffsets, eventLists,
                                types...);
  if (type == H5::PredType::NATIVE_INT64)
    return load<T1..., int64_t>(group, bankNames, bankOffsets, eventLists,
                                types...);
  if (type == H5::PredType::NATIVE_UINT32)
    return load<T1..., int32_t>(group, bankNames, bankOffsets, eventLists,
                                types...);
  if (type == H5::PredType::NATIVE_UINT64)
    return load<T1..., int64_t>(group, bankNames, bankOffsets, eventLists,
                                types...);
  if (type == H5::PredType::NATIVE_FLOAT)
    return load<T1..., float>(group, bankNames, bankOffsets, eventLists,
                              types...);
  if (type == H5::PredType::NATIVE_DOUBLE)
    return load<T1..., double>(group, bankNames, bankOffsets, eventLists,
                               types...);
  throw std::runtime_error(
      "Unsupported H5::DataType for entry in NXevent_data");
}
}

/*
EventLoader::EventLoader(const std::string &filename,
                         const std::string &groupName,
                         const std::vector<std::string> &bankNames,
                         const std::vector<int32_t> &bankOffsets,
                         std::vector<std::vector<TofEvent> *> eventLists)
    : m_file(Kernel::make_unique<H5::H5File>(filename, H5F_ACC_RDONLY)),
      m_groupName(groupName), m_bankNames(bankNames),
      m_bankOffsets(bankOffsets), m_eventLists(std::move(eventLists)) {}

EventLoader(const Chunker &chunker, const NXEventDataSource<IndexType, TimeZeroType, TimeOffsetType> &dataSource,
            const std::vector<int32_t> &bankOffsets,
            std::vector<std::vector<TofEvent> *> eventLists)
    : m_chunker(chunker), m_dataSource(dataSource), m_bankOffsets(bankOffsets),
      m_eventLists(std::move(eventLists)) {}

EventLoader::~EventLoader() = default;
*/

namespace EventLoader {
void load(const std::string &filename, const std::string &groupName,
          const std::vector<std::string> &bankNames,
          const std::vector<int32_t> &bankOffsets,
          std::vector<std::vector<TofEvent> *> eventLists) {
  H5::H5File file(filename, H5F_ACC_RDONLY);
  H5::Group group = file.openGroup(groupName);
  IO::load(group, bankNames, bankOffsets, eventLists,
       readDataType(group, bankNames, "event_index"),
       readDataType(group, bankNames, "event_time_zero"),
       readDataType(group, bankNames, "event_time_offset"));
}
}

/*
template MANTID_PARALLEL_DLL void
EventLoader::load<int64_t>(const H5::DataType &timeZeroType,
                           const H5::DataType &timeOffsetType);
template MANTID_PARALLEL_DLL void
EventLoader::load<int32_t>(const H5::DataType &timeZeroType,
                           const H5::DataType &timeOffsetType);

template MANTID_PARALLEL_DLL void
EventLoader::load<int64_t, int64_t>(const H5::DataType &timeOffsetType);
template MANTID_PARALLEL_DLL void
EventLoader::load<int64_t, double>(const H5::DataType &timeOffsetType);
template MANTID_PARALLEL_DLL void
EventLoader::load<int32_t, int64_t>(const H5::DataType &timeOffsetType);
template MANTID_PARALLEL_DLL void
EventLoader::load<int32_t, double>(const H5::DataType &timeOffsetType);

template MANTID_PARALLEL_DLL void
EventLoader::load<int64_t, int64_t, int32_t>();
template MANTID_PARALLEL_DLL void EventLoader::load<int64_t, int64_t, float>();
template MANTID_PARALLEL_DLL void EventLoader::load<int64_t, double, int32_t>();
template MANTID_PARALLEL_DLL void EventLoader::load<int64_t, double, float>();
template MANTID_PARALLEL_DLL void
EventLoader::load<int32_t, int64_t, int32_t>();
template MANTID_PARALLEL_DLL void EventLoader::load<int32_t, int64_t, float>();
template MANTID_PARALLEL_DLL void EventLoader::load<int32_t, double, int32_t>();
template MANTID_PARALLEL_DLL void EventLoader::load<int32_t, double, float>();
*/
} // namespace IO
} // namespace Parallel
} // namespace Mantid
