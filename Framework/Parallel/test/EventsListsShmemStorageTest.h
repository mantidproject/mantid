#ifndef MANTID_PARALLEL_EVENTSLISTSSHMEMSTORAGETEST_H_
#define MANTID_PARALLEL_EVENTSLISTSSHMEMSTORAGETEST_H_

#include <string>
#include <chrono>
#include <thread>
#include <cxxtest/TestSuite.h>

#include "MantidParallel/IO/EventsListsShmemStorage.h"
#include "MantidParallel/IO/EventsListsShmemManager.h"
#include "MantidTypes/Event/TofEvent.h"

using Mantid::Parallel::EventsListsShmemStorage;

class EventsListsShmemStorageTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EventsListsShmemStorageTest *createSuite() { return new EventsListsShmemStorageTest(); }
  static void destroySuite(EventsListsShmemStorageTest *suite) { delete suite; }

  void test_storage_creating_and_filling() {
    std::cout << "Shmem storage test\n";
    std::string segmentName = "test_segment1";
    std::string storageName = "test_storage1";
    EventsListsShmemStorage storage(segmentName, storageName, 65536);
    for (unsigned i = 0; i < 10; ++i)
      storage.AppendEvent(i, Mantid::Types::Event::TofEvent(i));

    std::cout << storage << "\n";

    std::string command("./bin/MantidNexusParallelLoader test_segment1 test_storage1");
    std::system(command.c_str());
  }

};

#endif /* MANTID_PARALLEL_EVENTSLISTSSHMEMSTORAGETEST_H_ */