// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PARALLEL_MULTIPROCESSEVENTLOADER_H_
#define MANTID_PARALLEL_MULTIPROCESSEVENTLOADER_H_

#include <atomic>
#include <boost/numeric/conversion/cast.hpp>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "MantidParallel/IO/EventLoaderHelpers.h"
#include "MantidParallel/IO/EventsListsShmemStorage.h"

#include "MantidParallel/DllConfig.h"

namespace Mantid {
namespace Parallel {
namespace IO {

/** MultiProcessEventLoader : Loades events from nexus file using
 * shared memory API from boost::interprocess library:
 * every child process reads own chunk of data from file and loads
 * to the own shared memory segment.
 *
 * The issue with shared memory: shared memory allocator is not smart enough,
 * so the dynamic allocation leads to memory fragmentation and unreasonable
 * memory consumption. For this reason we need to know the size of the data,
 * before loading to the shared memory. 2 strategies available for doing this:
 * 1. Load data from bank -> precalculate number events for each pixel
 *      -> sort by pixels directly in shared memory: LoadType::precalcEvents.
 * 2. Load data from bank -> sort by pixels in local memory
 *      -> copy from local to shared memory: LoadType::producerConsumer
 *      (uses dynamic allocation in local memory that is not so bad).
 *
 * There 3 main time consuming parts: reading from file, pushing to shared
 * memory, collecting from shared memory, the cost of sorting is small.

  @author Igor Gudich
  @date 2018
*/
class MANTID_PARALLEL_DLL MultiProcessEventLoader {
public:
  MultiProcessEventLoader(uint32_t numPixels, uint32_t numProcesses,
                          uint32_t numThreads, const std::string &binary,
                          bool precalc = true);
  void
  load(const std::string &filename, const std::string &groupname,
       const std::vector<std::string> &bankNames,
       const std::vector<int32_t> &bankOffsets,
       std::vector<std::vector<Types::Event::TofEvent> *> eventLists) const;

  static void fillFromFile(EventsListsShmemStorage &storage,
                           const std::string &filename,
                           const std::string &groupname,
                           const std::vector<std::string> &bankNames,
                           const std::vector<int32_t> &bankOffsets,
                           std::size_t from, std::size_t to, bool precalc);

  enum struct LoadType { preCalcEvents, producerConsumer };

private:
  static std::vector<std::string> generateSegmentsName(uint32_t procNum);
  static std::string generateStoragename();
  static std::string generateTimeBasedPrefix();

  template <typename MultiProcessEventLoader::LoadType LT =
                LoadType::preCalcEvents>
  struct GroupLoader {
    template <typename T>
    static void loadFromGroup(EventsListsShmemStorage &storage,
                              const H5::Group &group,
                              const std::vector<std::string> &bankNames,
                              const std::vector<int32_t> &bankOffsets,
                              std::size_t from, std::size_t to);

    static void loadFromGroupWrapper(const H5::DataType &type,
                                     EventsListsShmemStorage &storage,
                                     const H5::Group &group,
                                     const std::vector<std::string> &bankNames,
                                     const std::vector<int32_t> &bankOffsets,
                                     std::size_t from, std::size_t to);
  };

  void assembleFromShared(
      std::vector<std::vector<Mantid::Types::Event::TofEvent> *> &result) const;

  size_t estimateShmemAmount(size_t eventCount) const;

private:
  bool m_precalculateEvents;
  uint32_t m_numPixels;
  uint32_t m_numProcesses;
  uint32_t m_numThreads;
  std::string m_binaryToLaunch;
  std::vector<std::string> m_segmentNames;
  std::string m_storageName;
};

/// Wrapper to avoid manual processing of all cases of 2 template arguments
template <typename MultiProcessEventLoader::LoadType LT>
void MultiProcessEventLoader::GroupLoader<LT>::loadFromGroupWrapper(
    const H5::DataType &type, EventsListsShmemStorage &storage,
    const H5::Group &instrument, const std::vector<std::string> &bankNames,
    const std::vector<int32_t> &bankOffsets, std::size_t from, std::size_t to) {
  if (type == H5::PredType::NATIVE_INT32)
    return loadFromGroup<int32_t>(storage, instrument, bankNames, bankOffsets,
                                  from, to);
  if (type == H5::PredType::NATIVE_INT64)
    return loadFromGroup<int64_t>(storage, instrument, bankNames, bankOffsets,
                                  from, to);
  if (type == H5::PredType::NATIVE_UINT32)
    return loadFromGroup<uint32_t>(storage, instrument, bankNames, bankOffsets,
                                   from, to);
  if (type == H5::PredType::NATIVE_UINT64)
    return loadFromGroup<uint64_t>(storage, instrument, bankNames, bankOffsets,
                                   from, to);
  if (type == H5::PredType::NATIVE_FLOAT)
    return loadFromGroup<float>(storage, instrument, bankNames, bankOffsets,
                                from, to);
  if (type == H5::PredType::NATIVE_DOUBLE)
    return loadFromGroup<double>(storage, instrument, bankNames, bankOffsets,
                                 from, to);
  throw std::runtime_error(
      "Unsupported H5::DataType for event_time_offset in NXevent_data");
}

// aliase  for ToF type used in TofEvent
using ToFType = decltype(std::declval<TofEvent>().tof());

/**Loades the portion of data from hdf5 given group and banks list to shared
 * memory, 'from' and 'to' represent the range in global number of events.
 * Implements the 'precalculation of events' strategy*/

template <>
template <typename T>
void MultiProcessEventLoader::GroupLoader<
    MultiProcessEventLoader::LoadType::preCalcEvents>::
    loadFromGroup(EventsListsShmemStorage &storage, const H5::Group &instrument,
                  const std::vector<std::string> &bankNames,
                  const std::vector<int32_t> &bankOffsets,
                  const std::size_t from, const std::size_t to) {
  std::vector<int32_t> eventId;
  std::vector<T> eventTimeOffset;

  std::size_t eventCounter{0};
  auto bankSizes = EventLoader::readBankSizes(instrument, bankNames);
  IO::NXEventDataLoader<T> loader(1, instrument, bankNames);
  for (unsigned bankIdx = 0; bankIdx < bankNames.size(); ++bankIdx) {
    auto count = bankSizes[bankIdx];

    bool isFirstBank = (eventCounter < from);

    if (eventCounter + count > from) {
      size_t start{0};
      size_t finish{count};
      if (isFirstBank)
        start = from - eventCounter;
      if (eventCounter + count > to)
        finish = to - eventCounter;

      std::size_t cnt = finish - start;

      auto part = loader.setBankIndex(bankIdx);
      eventTimeOffset.resize(cnt);
      loader.readEventTimeOffset(eventTimeOffset.data(), start, cnt);
      eventId.resize(cnt);
      loader.readEventID(eventId.data(), start, cnt);

      detail::eventIdToGlobalSpectrumIndex(eventId.data(), cnt,
                                           bankOffsets[bankIdx]);

      std::unordered_map<int32_t, std::size_t> eventsPerPixel;
      for (auto &pixId : eventId) {
        auto iter = eventsPerPixel.find(pixId);
        if (iter == eventsPerPixel.end())
          iter = eventsPerPixel.insert(std::make_pair(pixId, 0)).first;
        ++iter->second;
      }

      for (const auto &pair : eventsPerPixel)
        storage.reserve(0, pair.first, pair.second);

      part->setEventOffset(start);
      for (std::size_t i = 0; i < eventId.size(); ++i) {
        try {
          storage.appendEvent(
              0, eventId[i],
              TofEvent{boost::numeric_cast<ToFType>(eventTimeOffset[i]),
                       part->next()});
        } catch (...) {
          std::throw_with_nested(
              std::runtime_error("Something wrong in multiprocess "
                                 "LoadFromGroup precountEvent mode."));
        }
      }
    }
    eventCounter += count;
    if (eventCounter >= to)
      break;
  }
}

/**Loades the portion of data from hdf5 given group and banks list to shared
 * memory, 'from' and 'to' represent the range in global number of events.
 * Implements the 'producer-consumer' strategy*/
template <>
template <typename T>
void MultiProcessEventLoader::GroupLoader<
    MultiProcessEventLoader::LoadType::producerConsumer>::
    loadFromGroup(EventsListsShmemStorage &storage, const H5::Group &instrument,
                  const std::vector<std::string> &bankNames,
                  const std::vector<int32_t> &bankOffsets,
                  const std::size_t from, const std::size_t to) {
  constexpr std::size_t chunksPerBank{10};
  const std::size_t chLen{
      std::max<std::size_t>((to - from) / chunksPerBank, 1)};
  auto bankSizes = EventLoader::readBankSizes(instrument, bankNames);

  struct Task {
    unsigned bankIdx;
    std::size_t from;
    std::vector<int32_t> eventId;
    std::vector<T> eventTimeOffset;
    IO::NXEventDataLoader<T> loader;
    decltype(loader.setBankIndex(0)) partitioner;

    Task(const H5::Group &instrument, const std::vector<std::string> &bankNames,
         unsigned bank, std::size_t cur, std::size_t cnt)
        : bankIdx(bank), from(cur), eventId(cnt), eventTimeOffset(cnt),
          loader(1, instrument, bankNames) {}
  };

  std::vector<std::vector<TofEvent>> pixels(storage.pixelCount());
  std::atomic<std::size_t> pixNum{0};
  std::atomic<std::size_t> taskCount{0};
  std::atomic<int> finished{0};
  std::size_t portion{std::max<std::size_t>(pixels.size() / 16, 1)};

  std::vector<Task> tasks;

  // prepare tasks: determine chunks of data to be loaded and sorted by pixel
  // number
  std::size_t eventCounter{0};
  for (unsigned bankIdx = 0; bankIdx < bankNames.size(); ++bankIdx) {
    auto count = bankSizes[bankIdx];
    bool isFirstBank = (eventCounter < from);

    if (eventCounter + count > from) {
      size_t start{0};
      size_t finish{count};
      if (isFirstBank)
        start = from - eventCounter;
      if (eventCounter + count > to)
        finish = to - eventCounter;

      std::size_t cur = start;
      while (cur < finish) {
        auto cnt(chLen);
        if (cur + chLen >= finish)
          cnt = finish - cur;

        tasks.emplace_back(instrument, bankNames, bankIdx, cur, cnt);
        cur += cnt;
      }
    }
    eventCounter += count;
    if (eventCounter >= to) {
      break;
    }
  }

  auto loadToshmem = [&]() {
    for (auto startPixel = pixNum.fetch_add(portion);
         startPixel < storage.pixelCount();
         startPixel = pixNum.fetch_add(portion)) {
      for (auto pixel = startPixel;
           pixel < std::min(startPixel + portion, pixels.size()); ++pixel)
        storage.appendEvent(0, pixel, pixels[pixel].begin(),
                            pixels[pixel].end());
    }
  };

  std::exception_ptr shmemLoaderException;
  std::thread fileLoader([&]() {
    try {
      for (auto &task : tasks) {
        task.partitioner = task.loader.setBankIndex(task.bankIdx);
        task.loader.readEventTimeOffset(task.eventTimeOffset.data(), task.from,
                                        task.eventTimeOffset.size());
        task.loader.readEventID(task.eventId.data(), task.from,
                                task.eventId.size());
        detail::eventIdToGlobalSpectrumIndex(task.eventId.data(),
                                             task.eventId.size(),
                                             bankOffsets[task.bankIdx]);
        ++taskCount;
      }
      ++finished;

      while (finished != 2) {
      }; // consumer have finished his work ready to transfer to shmem

      loadToshmem();
    } catch (...) {
      finished = 1; // prevent the infinite loop;
      shmemLoaderException = std::current_exception();
    }

  });

  std::exception_ptr sorterException;
  std::thread sorter([&]() {
    std::size_t tasksDone{0};

    auto processTask = [&]() {
      std::size_t tasksLoaded{taskCount};
      if (tasksDone < tasksLoaded) {
        for (auto tn = tasksDone; tn < tasksLoaded; ++tn) {
          auto &task = tasks[tn];
          task.partitioner->setEventOffset(task.from);
          for (unsigned i = 0; i < task.eventId.size(); ++i) {
            pixels.at(task.eventId[i])
                .emplace_back(
                    boost::numeric_cast<ToFType>(task.eventTimeOffset[i]),
                    task.partitioner->next());
          }
          task.eventId.resize(0);
          task.eventId.shrink_to_fit();
          task.eventTimeOffset.resize(0);
          task.eventTimeOffset.shrink_to_fit();
          task.partitioner.release();
        }
        tasksDone = tasksLoaded;
      }
    };

    try {
      while (finished < 1) // producer wants to produce smth
        processTask();
      // Clean up the queue
      processTask();

      ++finished; // all is consumed

      loadToshmem();
    } catch (...) {
      finished = 2; // prevent the infinite loop
      sorterException = std::current_exception();
    }
  });

  fileLoader.join();
  sorter.join();

  try {
    if (shmemLoaderException)
      std::rethrow_exception(shmemLoaderException);
    if (sorterException)
      std::rethrow_exception(sorterException);
  } catch (...) {
    std::throw_with_nested(
        std::runtime_error("Something wrong in multiprocess "
                           "LoadFromGroup producerConsumer mode."));
  }
}

} // namespace IO
} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_MULTIPROCESSEVENTLOADER_H_ */