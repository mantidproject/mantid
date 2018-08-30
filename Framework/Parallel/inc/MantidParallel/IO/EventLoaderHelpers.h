#ifndef MANTID_PARALLEL_EVENTLOADERHELPERS_H_
#define MANTID_PARALLEL_EVENTLOADERHELPERS_H_

#include "MantidKernel/make_unique.h"
#include "MantidParallel/Communicator.h"
#include "MantidParallel/DllConfig.h"
#include "MantidParallel/IO/Chunker.h"
#include "MantidParallel/IO/EventParser.h"
#include "MantidParallel/IO/NXEventDataLoader.h"
#include "MantidParallel/IO/PulseTimeGenerator.h"

namespace Mantid {
namespace Parallel {
namespace IO {

/** Private parts of EventLoader.

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
namespace EventLoader {
/// Read number of events in given banks from file.
inline std::vector<size_t>
readBankSizes(const H5::Group &group,
              const std::vector<std::string> &bankNames) {
  std::vector<size_t> bankSizes;
  for (const auto &bankName : bankNames) {
    const H5::DataSet dataset = group.openDataSet(bankName + "/event_id");
    const H5::DataSpace dataSpace = dataset.getSpace();
    bankSizes.push_back(dataSpace.getSelectNpoints());
  }
  return bankSizes;
}

inline H5::DataType readDataType(const H5::Group &group,
                                 const std::vector<std::string> &bankNames,
                                 const std::string &name) {
  return group.openDataSet(bankNames.front() + "/" + name).getDataType();
}

template <class T> class ThreadWaiter {
public:
  ThreadWaiter(T &thread) : m_thread(thread) {}
  ~ThreadWaiter() { m_thread.wait(); }

private:
  T &m_thread;
};

template <class TimeOffsetType>
void load(const Chunker &chunker, NXEventDataSource<TimeOffsetType> &dataSource,
          EventParser<TimeOffsetType> &dataSink) {
  const size_t chunkSize = chunker.chunkSize();
  const auto &ranges = chunker.makeLoadRanges();
  std::vector<int32_t> event_id(2 * chunkSize);
  std::vector<TimeOffsetType> event_time_offset(2 * chunkSize);
  // Wait for thread completion before exit. Wrapped in struct in case of
  // exceptions.
  ThreadWaiter<EventParser<TimeOffsetType>> threadCleanup(dataSink);

  int64_t previousBank = -1;
  size_t bufferOffset{0};
  for (const auto &range : ranges) {
    std::unique_ptr<AbstractEventDataPartitioner<TimeOffsetType>> partitioner;
    if (static_cast<int64_t>(range.bankIndex) != previousBank) {
      partitioner = dataSource.setBankIndex(range.bankIndex);
    }
    dataSource.readEventID(event_id.data() + bufferOffset, range.eventOffset,
                           range.eventCount);
    dataSource.readEventTimeOffset(event_time_offset.data() + bufferOffset,
                                   range.eventOffset, range.eventCount);
    if (previousBank != -1)
      dataSink.wait();
    if (static_cast<int64_t>(range.bankIndex) != previousBank) {
      dataSink.setEventDataPartitioner(std::move(partitioner));
      dataSink.setEventTimeOffsetUnit(dataSource.readEventTimeOffsetUnit());
      previousBank = range.bankIndex;
    }
    dataSink.startAsync(event_id.data() + bufferOffset,
                        event_time_offset.data() + bufferOffset, range);
    bufferOffset = (bufferOffset + chunkSize) % (2 * chunkSize);
  }
}

template <class TimeOffsetType>
void load(const Communicator &comm, const H5::Group &group,
          const std::vector<std::string> &bankNames,
          const std::vector<int32_t> &bankOffsets,
          std::vector<std::vector<Types::Event::TofEvent> *> eventLists) {
  // In tests loading from a single SSD this chunk size seems close to the
  // optimum. May need to be adjusted in the future (potentially dynamically)
  // when loading from parallel file systems and running on a cluster.
  const size_t chunkSize = 1024 * 1024;
  // In tests loading from a single SSD there was no advantage using fewer
  // processes for loading than for processing. This may be different in larger
  // MPI runs on a cluster where limiting the number of IO processes may be
  // required when accessing the parallel file system.
  const Chunker chunker(comm.size(), comm.rank(),
                        readBankSizes(group, bankNames), chunkSize);
  NXEventDataLoader<TimeOffsetType> loader(comm.size(), group, bankNames);
  EventParser<TimeOffsetType> consumer(comm, chunker.makeWorkerGroups(),
                                       bankOffsets, eventLists);
  load<TimeOffsetType>(chunker, loader, consumer);
}

/// Translate from H5::DataType to actual type, forward to load implementation.
template <class... T> void load(const H5::DataType &type, T &&... args) {
  if (type == H5::PredType::NATIVE_INT32)
    return load<int32_t>(args...);
  if (type == H5::PredType::NATIVE_INT64)
    return load<int64_t>(args...);
  if (type == H5::PredType::NATIVE_UINT32)
    return load<uint32_t>(args...);
  if (type == H5::PredType::NATIVE_UINT64)
    return load<uint64_t>(args...);
  if (type == H5::PredType::NATIVE_FLOAT)
    return load<float>(args...);
  if (type == H5::PredType::NATIVE_DOUBLE)
    return load<double>(args...);
  throw std::runtime_error(
      "Unsupported H5::DataType for event_time_offset in NXevent_data");
}
} // namespace EventLoader

} // namespace IO
} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_EVENTLOADERHELPERS_H_ */
