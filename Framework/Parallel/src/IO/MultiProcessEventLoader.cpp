#include <chrono>
#include <iomanip>
#include <thread>

#include "MantidParallel/IO/MultiProcessEventLoader.h"
#include "MantidTypes/Event/TofEvent.h"

namespace Mantid {
namespace Parallel {

MultiProcessEventLoader::MultiProcessEventLoader(unsigned int numPixels,
                                                 unsigned int numProcesses,
                                                 unsigned int numThreads)
    : numPixels(numPixels), numProcesses(numProcesses), numThreads(numThreads),
      segmentNames(GenerateSegmentsName(numProcesses)), storageName(GenerateStoragename()) {}

std::vector<std::string> MultiProcessEventLoader::GenerateSegmentsName(unsigned procNum) {
  std::vector<std::string> res(procNum, GenerateTimeBasedPrefix() + "_mantid_multiprocess_NXloader_segment_");
  ushort i{0};
  for (auto &name: res)
    name += std::to_string(i++);
  return res;
}

std::string MultiProcessEventLoader::GenerateStoragename() {
  return GenerateTimeBasedPrefix() + "_mantid_multiprocess_NXloader_storage";
}

std::string MultiProcessEventLoader::GenerateTimeBasedPrefix() {
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);

  std::stringstream ss;
  ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d%X");
  return ss.str();
}

void MultiProcessEventLoader::load(const std::string &filename,
                                   const std::string &groupName,
                                   const std::vector<std::string> &bankNames,
                                   const std::vector<int32_t> &bankOffsets,
                                   std::vector<std::vector<Types::Event::TofEvent> *> eventLists) const {

}

void MultiProcessEventLoader::assembleFromShared(std::vector<std::vector<Mantid::Types::Event::TofEvent> *> &result) const {
  std::vector<std::thread> workers;
  std::atomic<int> cnt{0};

  for (unsigned i = 0; i < numThreads; ++i) {
    workers.emplace_back([&cnt, this, &result]() {
      std::vector<ip::managed_shared_memory> segments;
      std::vector<Mantid::Parallel::Chunks *> chunksPtrs;
      for (unsigned i = 0; i < numThreads; ++i) {
        segments.emplace_back(ip::open_read_only, segmentNames[i].c_str());
        chunksPtrs.emplace_back(
            segments[i]
                .find<Mantid::Parallel::Chunks>(storageName.c_str())
                .first);
      }

      for (unsigned pixel = atomic_fetch_add(&cnt, 1); pixel < numPixels;
           pixel = atomic_fetch_add(&cnt, 1)) {
        auto &res = result[pixel];
        for (unsigned i = 0; i < numThreads; ++i) {
          for (auto &chunk : *chunksPtrs[i]) {
            res->insert(res->end(), chunk[pixel].begin(), chunk[pixel].end());
          }
        }
      }
    });
  }

  for (auto &worker : workers)
    worker.join();
}

} // namespace Parallel
} // namespace Mantid
