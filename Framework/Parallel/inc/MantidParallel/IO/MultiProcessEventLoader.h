#ifndef MANTID_PARALLEL_MULTIPROCESSEVENTLOADER_H_
#define MANTID_PARALLEL_MULTIPROCESSEVENTLOADER_H_

#include <atomic>
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

  @author Igor Gudich
  @date 2018

  Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
                           uint32_t from, uint32_t to, bool precalc);

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
                              uint32_t from, uint32_t to);
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

/**Loades the portion of data from hdf5 given group and banks list to shared
 * memory, 'from' and 'to' represent the range in global number of events.
 * Implements the 'precalculation of events' strategy*/

template <>
template <typename T>
void MultiProcessEventLoader::GroupLoader<
    MultiProcessEventLoader::LoadType::preCalcEvents>::
    loadFromGroup(EventsListsShmemStorage &storage, const H5::Group &instrument,
                  const std::vector<std::string> &bankNames,
                  const std::vector<int32_t> &bankOffsets, const uint32_t from,
                  const uint32_t to) {
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
      for (unsigned i = 0; i < eventId.size(); ++i) {
        try {
          TofEvent event{(double)eventTimeOffset[i], part->next()};
          storage.appendEvent(0, eventId[i], event);
        } catch (std::exception const &ex) {
          std::rethrow_if_nested(ex);
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
                  const std::vector<int32_t> &bankOffsets, const uint32_t from,
                  const uint32_t to) {
  constexpr std::size_t chunksPerBank{100};
  const std::size_t chLen{
      std::max<std::size_t>((to - from) / chunksPerBank, 1)};
  auto bankSizes = EventLoader::readBankSizes(instrument, bankNames);

  struct Task {
    std::vector<int32_t> eventId;
    std::vector<T> eventTimeOffset;
    IO::NXEventDataLoader<T> loader;
    decltype(loader.setBankIndex(0)) partitioner;

    Task(const H5::Group &instrument, const std::vector<std::string> &bankNames)
        : loader(1, instrument, bankNames) {}
  };

  std::vector<std::vector<TofEvent>> pixels(storage.pixelCount());
  std::atomic<std::size_t> pixNum{0};
  std::queue<std::unique_ptr<Task>> queue;
  std::mutex mutex;
  std::atomic<int> finished{0};
  std::size_t portion{std::max<std::size_t>(pixels.size() / 16, 1)};

  std::thread producer([&]() {
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

          auto task{std::make_unique<Task>(instrument, bankNames)};
          task->partitioner = task->loader.setBankIndex(bankIdx);
          task->partitioner->setEventOffset(cur);
          task->eventTimeOffset.resize(cnt);
          task->loader.readEventTimeOffset(task->eventTimeOffset.data(), start,
                                           cnt);
          task->eventId.resize(cnt);
          task->loader.readEventID(task->eventId.data(), start, cnt);
          detail::eventIdToGlobalSpectrumIndex(task->eventId.data(), cnt,
                                               bankOffsets[bankIdx]);

          {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push(std::move(task));
          }
          cur += cnt;
        }
      }
      eventCounter += count;
      if (eventCounter >= to) {
        ++finished;
        break;
      }
    }

    while (finished != 2) {
    }; // consumer have finished his work ready to transfer to shmem

    for (auto startPixel = pixNum.fetch_add(portion);
         startPixel < storage.pixelCount();
         startPixel = pixNum.fetch_add(portion)) {
      for (auto pixel = startPixel;
           pixel < std::min(startPixel + portion, pixels.size()); ++pixel)
        storage.appendEvent(0, pixel, pixels[pixel].begin(),
                            pixels[pixel].end());
    }
  });

  std::thread consumer([&]() {
    while (finished < 1) { // producer wants to produce smth
      while (!queue.empty()) {
        std::unique_ptr<Task> task;
        {
          std::lock_guard<std::mutex> lock(mutex);
          task = std::move(queue.front());
          queue.pop();
        }
        for (unsigned i = 0; i < task->eventId.size(); ++i)
          pixels.at(task->eventId[i])
              .emplace_back((double)task->eventTimeOffset[i],
                            task->partitioner->next());
      }
    }

    ++finished; // all is consumed

    for (auto startPixel = pixNum.fetch_add(portion);
         startPixel < storage.pixelCount();
         startPixel = pixNum.fetch_add(portion)) {
      for (auto pixel = startPixel;
           pixel < std::min(startPixel + portion, pixels.size()); ++pixel)
        storage.appendEvent(0, pixel, pixels[pixel].begin(),
                            pixels[pixel].end());
    }

  });

  producer.join();
  consumer.join();
}

} // namespace IO
} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_MULTIPROCESSEVENTLOADER_H_ */