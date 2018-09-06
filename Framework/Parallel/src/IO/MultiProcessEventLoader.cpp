#include <MantidParallel/IO/MultiProcessEventLoader.h>
#include <boost/process/child.hpp>
#include <chrono>
#include <iomanip>
#include <numeric>
#include <thread>

#include "MantidParallel/IO/MultiProcessEventLoader.h"
#include "MantidParallel/IO/NXEventDataLoader.h"
#include "MantidTypes/Event/TofEvent.h"

namespace bp = boost::process;

namespace Mantid {
namespace Parallel {
namespace IO {

MultiProcessEventLoader::MultiProcessEventLoader(unsigned int numPixels,
                                                 unsigned int numProcesses,
                                                 unsigned int numThreads,
                                                 const std::string &binary)
    : m_numPixels(numPixels), m_numProcesses(numProcesses),
      m_numThreads(numThreads), m_binaryToLaunch(binary),
      m_segmentNames(GenerateSegmentsName(numProcesses)),
      m_storageName(GenerateStoragename()) {}

std::vector<std::string>
MultiProcessEventLoader::GenerateSegmentsName(unsigned procNum) {
  std::vector<std::string> res(procNum,
                               GenerateTimeBasedPrefix() +
                                   "_mantid_multiprocess_NXloader_segment_");
  ushort i{0};
  for (auto &name : res)
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

void MultiProcessEventLoader::load(
    const std::string &filename, const std::string &groupname,
    const std::vector<std::string> &bankNames,
    const std::vector<int32_t> &bankOffsets,
    std::vector<std::vector<Types::Event::TofEvent> *> eventLists) const {

  try {
    H5::H5File file(filename.c_str(), H5F_ACC_RDONLY);
    auto instrument = file.openGroup(groupname);

    auto bkSz = EventLoader::readBankSizes(instrument, bankNames);
    auto numEvents = std::accumulate(bkSz.begin(), bkSz.end(), 0);

    std::size_t storageSize = estimateShmemAmount(numEvents);

    std::size_t evPerPr = numEvents / m_numProcesses;

    std::vector<bp::child> vChilds;

    for (unsigned i = 1; i < m_numProcesses; ++i) {
      std::size_t upperBound =
          i < m_numProcesses - 1 ? evPerPr * (i + 1) : numEvents;

      std::string command;
      command += m_binaryToLaunch + " ";
      command += m_segmentNames[i] + " ";           // segment name
      command += m_storageName + " ";               // storage name
      command += std::to_string(i) + " ";           // proc id
      command += std::to_string(evPerPr * i) + " "; // first event to load
      command += std::to_string(upperBound) + " ";  // upper bound to load
      command += std::to_string(m_numPixels) + " "; // pixel count
      command += std::to_string(storageSize) + " "; // memory size
      command += filename + " ";                    // nexus file name
      command += groupname + " ";                   // instrument group name
      for (unsigned j = 0; j < bankNames.size(); ++j) {
        command += bankNames[j] + " ";                   // bank name
        command += std::to_string(bankOffsets[j]) + " "; // bank size
      }

      try {
        vChilds.emplace_back(command.c_str());
      } catch (std::exception const &ex) {
        std::rethrow_if_nested(ex);
      }
    }

    EventsListsShmemStorage storage(m_segmentNames[0], m_storageName,
                                    storageSize, 1, m_numPixels, false);
    fillFromFile(storage, filename, groupname, bankNames, bankOffsets, 0,
                 numEvents / m_numProcesses);

    for (auto &c : vChilds)
      c.wait();

    assembleFromShared(eventLists);

    for (const auto &name : m_segmentNames)
      ip::shared_memory_object::remove(name.c_str());
  } catch (std::exception const &ex) {
    for (const auto &name : m_segmentNames)
      ip::shared_memory_object::remove(name.c_str());
    std::rethrow_if_nested(ex);
  }
}

void MultiProcessEventLoader::assembleFromShared(
    std::vector<std::vector<Mantid::Types::Event::TofEvent> *> &result) const {
  std::vector<std::thread> workers;
  std::atomic<int> cnt{0};

  for (unsigned i = 0; i < m_numThreads; ++i) {
    workers.emplace_back([&cnt, this, &result]() {
      std::vector<ip::managed_shared_memory> segments;
      std::vector<Mantid::Parallel::IO::Chunks *> chunksPtrs;
      for (unsigned pid = 0; pid < m_numProcesses; ++pid) {
        segments.emplace_back(ip::open_read_only, m_segmentNames[pid].c_str());
        chunksPtrs.emplace_back(
            segments[pid]
                .find<Mantid::Parallel::IO::Chunks>(m_storageName.c_str())
                .first);
      }

      for (unsigned pixel = atomic_fetch_add(&cnt, 1); pixel < m_numPixels;
           pixel = atomic_fetch_add(&cnt, 1)) {
        auto &res = result[pixel];
        for (unsigned i = 0; i < m_numProcesses; ++i) {
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

void MultiProcessEventLoader::fillFromFile(
    EventsListsShmemStorage &storage, const std::string &filename,
    const std::string &groupname, const std::vector<std::string> &bankNames,
    const std::vector<int32_t> &bankOffsets, unsigned from, unsigned to) {
  H5::H5File file(filename.c_str(), H5F_ACC_RDONLY);
  auto instrument = file.openGroup(groupname);

  auto type =
      EventLoader::readDataType(instrument, bankNames, "event_time_offset");

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

size_t MultiProcessEventLoader::estimateShmemAmount(size_t eventCount) const {
  std::size_t eventBased{eventCount / m_numProcesses * sizeof(TofEvent) +
      m_numPixels *
          sizeof(std::vector<std::vector<TofEvent>>)};
  return eventBased * 100; // TODO we have a big shared memory overhead because
  // of reallocation memory for eventlists
}

} // namespace IO
} // namespace Parallel
} // namespace Mantid
