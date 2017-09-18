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
  // EventParser consumer(bankOffsets, eventLists);
  static_cast<void>(bankOffsets);
  static_cast<void>(eventLists);
  load<IndexType, TimeZeroType, TimeOffsetType>(chunker, loader);
}

template <class... T1, class... T2>
void load(const H5::DataType &type, T2 &&... args) {
  // Translate from H5::DataType to actual type. Done step by step to avoid
  // combinatoric explosion. The T1 parameter pack holds the final template
  // arguments we want. The T2 parameter pack represents the remaining
  // H5::DataType arguments and any other arguments. In every call we peel off
  // the first entry from the T2 pack and append it to T1. This stops once the
  // next argument in args is not of type H5::DataType anymore, allowing us to
  // pass arbitrary extra arguments in the second part of args.
  if (type == H5::PredType::NATIVE_INT32)
    return load<T1..., int32_t>(args...);
  if (type == H5::PredType::NATIVE_INT64)
    return load<T1..., int64_t>(args...);
  if (type == H5::PredType::NATIVE_UINT32)
    return load<T1..., uint32_t>(args...);
  if (type == H5::PredType::NATIVE_UINT64)
    return load<T1..., uint64_t>(args...);
  if (type == H5::PredType::NATIVE_FLOAT)
    return load<T1..., float>(args...);
  if (type == H5::PredType::NATIVE_DOUBLE)
    return load<T1..., double>(args...);
  throw std::runtime_error(
      "Unsupported H5::DataType for entry in NXevent_data");
}
}

namespace EventLoader {
void load(const std::string &filename, const std::string &groupName,
          const std::vector<std::string> &bankNames,
          const std::vector<int32_t> &bankOffsets,
          std::vector<std::vector<TofEvent> *> eventLists) {
  H5::H5File file(filename, H5F_ACC_RDONLY);
  H5::Group group = file.openGroup(groupName);
  IO::load(readDataType(group, bankNames, "event_index"),
           readDataType(group, bankNames, "event_time_zero"),
           readDataType(group, bankNames, "event_time_offset"), group,
           bankNames, bankOffsets, eventLists);
}
}

} // namespace IO
} // namespace Parallel
} // namespace Mantid
