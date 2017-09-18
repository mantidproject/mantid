#ifndef MANTID_PARALLEL_EVENTLOADERTEST_H_
#define MANTID_PARALLEL_EVENTLOADERTEST_H_

#include <cxxtest/TestSuite.h>

#include <H5Cpp.h>

#include "MantidParallel/IO/EventLoader.h"

using namespace Mantid::Parallel::IO;

class EventLoaderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EventLoaderTest *createSuite() { return new EventLoaderTest(); }
  static void destroySuite(EventLoaderTest *suite) { delete suite; }

  void test_throws_if_file_does_not_exist() {
    TS_ASSERT_THROWS(EventLoader::load("abcdefg", "", {}, {}, {}),
                     H5::FileIException);
  }

  void test_tmp() {
    EventLoader::load("/home/simon/mantid/nexus/load-performance/sample-files/"
                      "PG3_4871_event.nxs",
                      "entry", {"bank102_events"}, {0}, {nullptr});
    //EventLoader::load("/mnt/extra/simon/neutron-data/realistic_NXEvent_data/events-100000_banks-7_pixels-10000_chunk-262144_compress-None.hdf5",
    //                  "entry/instrument", {"events-0"}, {0}, {nullptr});
  }
};

#endif /* MANTID_PARALLEL_EVENTLOADERTEST_H_ */
