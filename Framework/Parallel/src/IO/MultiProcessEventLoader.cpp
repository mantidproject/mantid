// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidParallel/IO/MultiProcessEventLoader.h"
#include <Poco/Process.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <thread>
#include <utility>

#include "MantidParallel/IO/MultiProcessEventLoader.h"
#include "MantidParallel/IO/NXEventDataLoader.h"
#include "MantidTypes/Event/TofEvent.h"

namespace Mantid::Parallel::IO {

/// Constructor
MultiProcessEventLoader::MultiProcessEventLoader(uint32_t numPixels, uint32_t numProcesses, uint32_t numThreads,
                                                 std::string binary, bool precalc)
    : m_precalculateEvents(precalc), m_numPixels(numPixels), m_numProcesses(numProcesses), m_numThreads(numThreads),
      m_binaryToLaunch(std::move(binary)), m_segmentNames(generateSegmentsName(numProcesses)),
      m_storageName(generateStoragename()) {}

/// Generates "unique" shared memory segment name
std::vector<std::string> MultiProcessEventLoader::generateSegmentsName(uint32_t procNum) {
  std::vector<std::string> res(procNum, generateTimeBasedPrefix() + "_mantid_multiprocess_NXloader_segment_");
  unsigned short i{0};
  std::for_each(res.begin(), res.end(), [&i](auto &name) { name += std::to_string(i++); });
  return res;
}

/// Generates "unique" shared memory storage structure name
std::string MultiProcessEventLoader::generateStoragename() {
  return generateTimeBasedPrefix() + "_mantid_multiprocess_NXloader_storage";
}

/// Generates "unique" prefix for shared memory stuff
std::string MultiProcessEventLoader::generateTimeBasedPrefix() {
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);

  std::stringstream ss;
  ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d%X");
  return ss.str();
}

/**Main API function for loading data from given file, group list of banks,
 * launches child processes for hdf5 parallel reading*/
void MultiProcessEventLoader::load(const std::string &filename, const std::string &groupname,
                                   const std::vector<std::string> &bankNames, const std::vector<int32_t> &bankOffsets,
                                   std::vector<std::vector<Types::Event::TofEvent> *> eventLists) const {

  try {
    H5::FileAccPropList access_plist;
    access_plist.setFcloseDegree(H5F_CLOSE_STRONG);
    H5::H5File file(filename.c_str(), H5F_ACC_RDONLY, access_plist);
    auto instrument = file.openGroup(groupname);

    auto bkSz = EventLoader::readBankSizes(instrument, bankNames);
    auto numEvents = std::accumulate(bkSz.begin(), bkSz.end(), std::size_t{0});

    std::size_t storageSize = estimateShmemAmount(numEvents);

    std::size_t evPerPr = numEvents / m_numProcesses;

    /*  boost::process implementation can be used
     * with proper boost version instead of Poco*/
    /*
        std::vector<bp::child> vChilds;

        // prepare command for launching of parallel processes
        for (unsigned i = 0; i < m_numProcesses; ++i) {
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
          command += m_precalculateEvents
                         ? "1 "
                         : "0 "; // variant of algorithm used for loading
          for (unsigned j = 0; j < bankNames.size(); ++j) {
            command += bankNames[j] + " ";                   // bank name
            command += std::to_string(bankOffsets[j]) + " "; // bank size
          }

          try {
            // launch child processes
            vChilds.emplace_back(command.c_str());
          } catch (std::exception const &ex) {
            std::rethrow_if_nested(ex);
          }
        }
    */

    // to cleanup shared memory in this function
    struct SharedMemoryDestroyer {
      const std::vector<std::string> &segments;
      explicit SharedMemoryDestroyer(const std::vector<std::string> &sm) : segments(sm) {}
      ~SharedMemoryDestroyer() {
        for (const auto &name : segments)
          ip::shared_memory_object::remove(name.c_str());
      }
    } shared_memory_destroyer(m_segmentNames);

    std::vector<Poco::ProcessHandle> vChilds;
    for (unsigned i = 0; i < m_numProcesses; ++i) {
      std::size_t upperBound = i < m_numProcesses - 1 ? evPerPr * (i + 1) : numEvents;
      std::vector<std::string> processArgs;

      processArgs.emplace_back(m_segmentNames[i]);                  // segment name
      processArgs.emplace_back(m_storageName);                      // storage name
      processArgs.emplace_back(std::to_string(i));                  // proc id
      processArgs.emplace_back(std::to_string(evPerPr * i));        // first event to load
      processArgs.emplace_back(std::to_string(upperBound));         // upper bound to load
      processArgs.emplace_back(std::to_string(m_numPixels));        // pixel count
      processArgs.emplace_back(std::to_string(storageSize));        // memory size
      processArgs.emplace_back(filename);                           // nexus file name
      processArgs.emplace_back(groupname);                          // instrument group name
      processArgs.emplace_back(m_precalculateEvents ? "1 " : "0 "); // variant of algorithm used for loading
      for (unsigned j = 0; j < bankNames.size(); ++j) {
        processArgs.emplace_back(bankNames[j]);                   // bank name
        processArgs.emplace_back(std::to_string(bankOffsets[j])); // bank size
      }

      try {
        // launch child processes
        vChilds.emplace_back(Poco::Process::launch(m_binaryToLaunch, processArgs));
      } catch (...) {
        std::throw_with_nested(std::runtime_error("MultiProcessEventLoader::load()"));
      }
    }

    /*  boost::process implementation can be used
     * with proper boost version instead of Poco*/
    /*
        // waiting for child processes
        for (auto &c : vChilds)
          c.wait();

        // check if oll childs are finished correctly
        for (auto &c : vChilds)
          if (c.exit_code())
            throw std::runtime_error("Error while multiprocess loading\n");
    */

    // waiting for child processes
    for (auto &c : vChilds)
      if (c.wait())
        throw std::runtime_error("Error while waiting processes in  multiprocess loading.");

    // Assemble multiprocess data from shared memory
    assembleFromShared(eventLists);
  } catch (...) {
    std::throw_with_nested(std::runtime_error("Something wrong in "
                                              "MultiprocessLoader."));
  }
}

/**Collects data from the chunks in shared memory to the final structure*/
void MultiProcessEventLoader::assembleFromShared(
    std::vector<std::vector<Mantid::Types::Event::TofEvent> *> &result) const {
  std::vector<std::thread> workers;

  std::vector<std::atomic<uint32_t>> cnts(m_segmentNames.size());
  std::fill(cnts.begin(), cnts.end(), 0);
  std::vector<std::atomic<uint32_t>> processCounter(m_segmentNames.size());
  std::fill(processCounter.begin(), processCounter.end(), 0);

  const unsigned portion{std::max<unsigned>(m_numPixels / m_numThreads / 3, 1)};

  for (unsigned i = 0; i < m_numThreads; ++i) {
    workers.emplace_back([&]() {
      for (std::size_t segId = 0; segId < m_segmentNames.size(); ++segId) {
        ip::managed_shared_memory segment{ip::open_read_only, m_segmentNames[segId].c_str()};
        auto chunks = segment.find<Mantid::Parallel::IO::Chunks>(m_storageName.c_str()).first;
        auto &cnt = cnts[segId];
        for (uint32_t startPixel = cnt.fetch_add(portion); startPixel < m_numPixels;
             startPixel = cnt.fetch_add(portion)) {
          auto toPixel = std::min(startPixel + portion, m_numPixels);
          for (uint32_t pixel = startPixel; pixel < toPixel; ++pixel) {
            auto &res = result[pixel];
            for (auto &ch : *chunks) {
              res->insert(res->end(), ch[pixel].begin(), ch[pixel].end());
            }
          }
        }
        ++processCounter[segId];
        if (processCounter[segId] == m_numThreads)
          ip::shared_memory_object::remove(m_segmentNames[segId].c_str());

        while (processCounter[segId] != m_numThreads)
          ;
      }
    });
  }

  for (auto &worker : workers)
    worker.join();
}

/**Wrapper for loading the PART of ("from" event "to" event) data
 * from nexus file with different strategies*/
void MultiProcessEventLoader::fillFromFile(EventsListsShmemStorage &storage, const std::string &filename,
                                           const std::string &groupname, const std::vector<std::string> &bankNames,
                                           const std::vector<int32_t> &bankOffsets, const std::size_t from,
                                           const std::size_t to, bool precalc) {
  H5::FileAccPropList access_plist;
  access_plist.setFcloseDegree(H5F_CLOSE_STRONG);
  H5::H5File file(filename.c_str(), H5F_ACC_RDONLY, access_plist);
  auto instrument = file.openGroup(groupname);

  auto type = EventLoader::readDataType(instrument, bankNames, "event_time_offset");

  if (precalc)
    return GroupLoader<LoadType::preCalcEvents>::loadFromGroupWrapper(type, storage, instrument, bankNames, bankOffsets,
                                                                      from, to);
  else
    return GroupLoader<LoadType::producerConsumer>::loadFromGroupWrapper(type, storage, instrument, bankNames,
                                                                         bankOffsets, from, to);
}

// Estimates the memory amount for shared memory segments
// vector representing each pixel allocated only once, so we have allocationFee
// bytes extra overhead
size_t MultiProcessEventLoader::estimateShmemAmount(size_t eventCount) const {
  // 8 bytes pointer to allocator + 8 bytes pointer to metadata
  auto allocationFee = 8 + 8 + generateStoragename().length();
  std::size_t len{(eventCount / m_numProcesses + eventCount % m_numProcesses) * sizeof(TofEvent) +
                  m_numPixels * (sizeof(EventLists) + allocationFee) + sizeof(Chunks) + allocationFee};
  return len;
}

} // namespace Mantid::Parallel::IO
