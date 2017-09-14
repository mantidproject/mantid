#ifndef MANTID_PARALLEL_EVENTLOADERTEST_H_
#define MANTID_PARALLEL_EVENTLOADERTEST_H_

#include <cxxtest/TestSuite.h>

#include <H5Cpp.h>

#include "MantidParallel/IO/EventLoader.h"

using Mantid::Parallel::IO::EventLoader;

class EventLoaderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EventLoaderTest *createSuite() { return new EventLoaderTest(); }
  static void destroySuite(EventLoaderTest *suite) { delete suite; }

  void test_throws_if_file_does_not_exist() {
    TS_ASSERT_THROWS(EventLoader("abcdefg", "", {}, {}, {}),
                     H5::FileIException);
  }
};

#endif /* MANTID_PARALLEL_EVENTLOADERTEST_H_ */
