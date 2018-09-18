#ifndef MANTID_PARALLEL_MULTIPROCESSEVENTLOADER_H_
#define MANTID_PARALLEL_MULTIPROCESSEVENTLOADER_H_

#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>

#include "MantidParallel/IO/EventLoaderHelpers.h"
#include "MantidParallel/IO/EventsListsShmemStorage.h"

#include "MantidParallel/DllConfig.h"

namespace Mantid {
namespace Parallel {
namespace IO {

/** MultiProcessEventLoader : TODO: DESCRIPTION

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
  MultiProcessEventLoader(unsigned int numPixels, unsigned int numProcesses,
                          unsigned int numThreads, const std::string &binary,
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
                           unsigned from, unsigned to, bool precalc);

  enum struct LoadType {
    preCalcEvents,
    producerConsumer
  };

private:
  static std::vector<std::string> GenerateSegmentsName(unsigned procNum);
  static std::string GenerateStoragename();
  static std::string GenerateTimeBasedPrefix();

  template<typename MultiProcessEventLoader::LoadType LT = LoadType::preCalcEvents>
  struct GroupLoader {
    template<typename T>
    static void loadFromGroup(EventsListsShmemStorage &storage,
                              const H5::Group &group,
                              const std::vector<std::string> &bankNames,
                              const std::vector<int32_t> &bankOffsets,
                              unsigned from, unsigned to);
  };

  void assembleFromShared(
      std::vector<std::vector<Mantid::Types::Event::TofEvent> *> &result) const;

  size_t estimateShmemAmount(size_t eventCount) const;

private:
  bool m_precalculateEvents;
  unsigned m_numPixels;
  unsigned m_numProcesses;
  unsigned m_numThreads;
  std::string m_binaryToLaunch;
  std::vector<std::string> m_segmentNames;
  std::string m_storageName;
};

template<>
template<typename T>
void MultiProcessEventLoader::GroupLoader<MultiProcessEventLoader::LoadType::preCalcEvents>::loadFromGroup(
    EventsListsShmemStorage &storage, const H5::Group &instrument,
    const std::vector<std::string> &bankNames,
    const std::vector<int32_t> &bankOffsets, unsigned from,
    unsigned to) {
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
      for (auto &pixId: eventId) {
        auto iter = eventsPerPixel.find(pixId);
        if (iter == eventsPerPixel.end())
          iter = eventsPerPixel.insert(std::make_pair(pixId, 0)).first;
        ++iter->second;
      }

      for (const auto &pair: eventsPerPixel)
        storage.reserve(0, pair.first, pair.second);

      part->setEventOffset(start);
      for (unsigned i = 0; i < eventId.size(); ++i) {
        try {
          TofEvent event{(double) eventTimeOffset[i], part->next()};
          storage.AppendEvent(0, eventId[i], event);
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

template<>
template<typename T>
void MultiProcessEventLoader::GroupLoader<MultiProcessEventLoader::LoadType::producerConsumer>::loadFromGroup(
    EventsListsShmemStorage &storage, const H5::Group &instrument,
    const std::vector<std::string> &bankNames,
    const std::vector<int32_t> &bankOffsets, unsigned from,
    unsigned to) {

  const std::size_t chLen{(from - to) / 100};
  auto bankSizes = EventLoader::readBankSizes(instrument, bankNames);

  struct Task {
    std::vector<int32_t> eventId;
    std::vector<T> eventTimeOffset;
    IO::NXEventDataLoader<T> loader;
    decltype(loader.setBankIndex(0)) partitioner;

    Task(const H5::Group &instrument, const std::vector<std::string> &bankNames) : loader(1, instrument, bankNames) {}
  };

  std::vector<std::vector<TofEvent> > pixels(storage.pixelCount());
  std::atomic<int> pixNum{0};
  std::queue<std::unique_ptr<Task>> queue;
  std::mutex guard;
  std::atomic<int> finished{0};

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

          std::unique_ptr<Task> task(new Task(instrument, bankNames));
          task->partitioner = task->loader.setBankIndex(bankIdx);
          task->eventTimeOffset.resize(cnt);
          task->loader.readEventTimeOffset(task->eventTimeOffset.data(), start, cnt);
          task->eventId.resize(cnt);
          task->loader.readEventID(task->eventId.data(), start, cnt);
          detail::eventIdToGlobalSpectrumIndex(task->eventId.data(), cnt, bankOffsets[bankIdx]);

          {
            std::lock_guard<std::mutex> lock(guard);
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

    for (unsigned pixel = atomic_fetch_add(&pixNum, 1); pixel < storage.pixelCount();
         pixel = atomic_fetch_add(&pixNum, 1))
      storage.AppendEvent(0, pixel, pixels[pixel].begin(), pixels[pixel].end());
  });

  std::thread consumer([&]() {
    while (finished < 1) { // producer whant to produce smth
      while (!queue.empty()) {
        std::unique_ptr<Task> task;
        {
          std::lock_guard<std::mutex> lock(guard);
          task = std::move(queue.front());
          queue.pop();
        }
        for (unsigned i = 0; i < task->eventId.size(); ++i)
          pixels.at(task->eventId[i]).emplace_back((double) task->eventTimeOffset[i], task->partitioner->next());
      }
    }

    ++finished; // all consumed

    for (unsigned pixel = atomic_fetch_add(&pixNum, 1); pixel < storage.pixelCount();
         pixel = atomic_fetch_add(&pixNum, 1))
      storage.AppendEvent(0, pixel, pixels[pixel].begin(), pixels[pixel].end());

  });

  producer.join();
  consumer.join();
}


} // namespace IO
} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_MULTIPROCESSEVENTLOADER_H_ */