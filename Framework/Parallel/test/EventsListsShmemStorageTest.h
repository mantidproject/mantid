#ifndef MANTID_PARALLEL_EVENTSLISTSSHMEMSTORAGETEST_H_
#define MANTID_PARALLEL_EVENTSLISTSSHMEMSTORAGETEST_H_

#include <atomic>
#include <boost/process/child.hpp>
#include <boost/process/spawn.hpp>
#include <chrono>
#include <cxxtest/TestSuite.h>
#include <string>
#include <thread>

#include "MantidParallel/IO/EventsListsShmemManager.h"
#include "MantidParallel/IO/EventsListsShmemStorage.h"
#include "MantidTypes/Event/TofEvent.h"

#define NUM_PROCESSES 12
#define NUM_PIXELS 10000
#define NUM_EVENTS 22000000
#define STORAGE_CNT 1

using Mantid::Parallel::EventsListsShmemStorage;
namespace bp = boost::process;

class EventsListsShmemStorageTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EventsListsShmemStorageTest *createSuite() {
    return new EventsListsShmemStorageTest();
  }
  static void destroySuite(EventsListsShmemStorageTest *suite) { delete suite; }

  void test_storage_creating_and_filling() {
    auto start = std::chrono::high_resolution_clock::now();

    std::cout << "Shmem storage test\n";
    std::string segmentName = "test_segment";
    std::string storageName = "test_storage";
    std::size_t storageSize =
        std::max(NUM_PIXELS * sizeof(Mantid::Types::Event::TofEvent) * 10000,
                 sizeof(Mantid::Types::Event::TofEvent) * NUM_EVENTS * 100) *
            2;

    std::cout << "Storage size: " << storageSize << "\n";
    std::vector<std::string> segmentNames(NUM_PROCESSES, segmentName);
    for (unsigned i = 0; i < NUM_PROCESSES; ++i)
      segmentNames[i] += std::to_string(i);

    std::vector<bp::child> vChilds;

    for (unsigned i = 1; i < NUM_PROCESSES; ++i) {

      std::string command;
      command += "./bin/MantidNexusParallelLoader ";
      command += segmentName + std::to_string(i) + " "; // segment name
      command += storageName + " ";                     // storage name
      command += std::to_string(i) + " ";               // proc id
      command += std::to_string(NUM_EVENTS / NUM_PROCESSES) +
          " ";                              // events per process
      command += std::to_string(NUM_PIXELS) + " "; // pixel count
      command += std::to_string(storageSize);      // memory size

      vChilds.emplace_back(command.c_str());
    }

    EventsListsShmemStorage storage(segmentNames[0], storageName, storageSize,
                                    1, NUM_PIXELS, false);
    Mantid::Parallel::EventsListsShmemManager::appendEventsDeterm(
        NUM_EVENTS / NUM_PROCESSES, NUM_PIXELS, 0, storage);

    for (auto &c : vChilds)
      c.wait();

    auto end = std::chrono::high_resolution_clock::now();
    auto dur =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Measured time multiprocess: " << dur.count() << "ms"
              << std::endl;

    start = std::chrono::high_resolution_clock::now();

    std::vector<std::vector<Mantid::Types::Event::TofEvent>> result(NUM_PIXELS);
    for (auto &res : result)
      res.reserve(2 * NUM_EVENTS / NUM_PIXELS);
    std::vector<std::thread> workers;
    std::atomic<int> cnt{0};

    for (unsigned i = 0; i < NUM_PROCESSES; ++i)
      workers.emplace_back([&cnt, &segmentNames, &storageName, &result]() {
        std::vector<ip::managed_shared_memory> segments;
        std::vector<Mantid::Parallel::Chunks *> chunksPtrs;
        for (unsigned i = 0; i < NUM_PROCESSES; ++i) {
          segments.emplace_back(ip::open_read_only, segmentNames[i].c_str());
          chunksPtrs.emplace_back(
              segments[i]
                  .find<Mantid::Parallel::Chunks>(storageName.c_str())
                  .first);
        }

        for (unsigned pixel = atomic_fetch_add(&cnt, 1); pixel < NUM_PIXELS;
             pixel = atomic_fetch_add(&cnt, 1)) {
          auto &res = result[pixel];
          for (unsigned i = 0; i < NUM_PROCESSES; ++i) {
            res.insert(res.end(), chunksPtrs[i]->at(0)[pixel].begin(),
                       chunksPtrs[i]->at(0)[pixel].end());
          }
        }
      });

    for (auto &worker : workers)
      worker.join();

    for (const auto &name : segmentNames)
      ip::shared_memory_object::remove(name.c_str());

    end = std::chrono::high_resolution_clock::now();
    auto dur1 =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Measured time treads: " << dur1.count() << "ms" << std::endl;
    std::cout << "Related: " << (float) dur.count() / dur1.count() << std::endl;

    /*    for(auto& res: result){
          std::cout << "[";
          for(auto& event: res){
            std::cout << event.tof() << " ";
          }
          std::cout << "]\n";
        }
    */
  }
};

#endif /* MANTID_PARALLEL_EVENTSLISTSSHMEMSTORAGETEST_H_ */