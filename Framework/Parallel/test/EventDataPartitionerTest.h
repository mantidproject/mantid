#ifndef MANTID_PARALLEL_EVENTDATAPARTITIONERTEST_H_
#define MANTID_PARALLEL_EVENTDATAPARTITIONERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidParallel/IO/EventDataPartitioner.h"

using namespace Mantid::Parallel::IO;

class EventDataPartitionerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EventDataPartitionerTest *createSuite() {
    return new EventDataPartitionerTest();
  }
  static void destroySuite(EventDataPartitionerTest *suite) { delete suite; }

  void test_construct() {
    TS_ASSERT_THROWS_NOTHING((EventDataPartitioner<int32_t, int64_t, double>(
        7, PulseTimeGenerator<int32_t, int64_t>{})));
  }
};

#endif /* MANTID_PARALLEL_EVENTDATAPARTITIONERTEST_H_ */
