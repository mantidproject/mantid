#ifndef MANTID_PARALLEL_EVENTDATAPARTITIONERTEST_H_
#define MANTID_PARALLEL_EVENTDATAPARTITIONERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidParallel/IO/EventDataPartitioner.h"

using namespace Mantid::Parallel::IO;
using Mantid::Types::Core::DateAndTime;
namespace Mantid {
namespace Parallel {
namespace IO {
namespace detail {
bool operator==(const Event<double> &a, const Event<double> &b) {
  return a.index == b.index && a.tof == b.tof && a.pulseTime == b.pulseTime;
}
} // namespace detail
} // namespace IO
} // namespace Parallel
} // namespace Mantid
using Event = detail::Event<double>;

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

  void test_empty_range() {
    for (const auto workers : {1, 2, 3}) {
      EventDataPartitioner<int32_t, int64_t, double> partitioner(
          workers, PulseTimeGenerator<int32_t, int64_t>(
                       {0, 2, 2, 3}, {2, 4, 6, 8}, "nanosecond", 0));
      size_t count = 0;
      std::vector<std::vector<Event>> data;
      partitioner.partition(data, nullptr, nullptr, {0, 1, count});
      TS_ASSERT_EQUALS(data.size(), workers);
      for (int worker = 0; worker < workers; ++worker) {
        TS_ASSERT_EQUALS(data[worker].size(), 0);
      }
    }
  }

  void test_partition_1_worker() {
    EventDataPartitioner<int32_t, int64_t, double> partitioner(
        1, PulseTimeGenerator<int32_t, int64_t>({0, 2, 2, 3}, {2, 4, 6, 8},
                                                "nanosecond", 0));
    std::vector<std::vector<Event>> data;
    std::vector<int32_t> index{5, 1, 4};
    std::vector<double> tof{1.1, 2.2, 3.3};
    // Starting at beginning, length 3
    partitioner.partition(data, index.data(), tof.data(), {0, 0, 3});
    TS_ASSERT_EQUALS(data.size(), 1);
    TS_ASSERT_EQUALS(data[0].size(), 3);
    TS_ASSERT_EQUALS(data[0][0], (Event{5, 1.1, DateAndTime(2)}));
    TS_ASSERT_EQUALS(data[0][1], (Event{1, 2.2, DateAndTime(2)}));
    TS_ASSERT_EQUALS(data[0][2], (Event{4, 3.3, DateAndTime(6)}));
    // Starting at offset 1, length 3
    partitioner.partition(data, index.data(), tof.data(), {0, 1, 3});
    TS_ASSERT_EQUALS(data.size(), 1);
    TS_ASSERT_EQUALS(data[0].size(), 3);
    TS_ASSERT_EQUALS(data[0][0], (Event{5, 1.1, DateAndTime(2)}));
    TS_ASSERT_EQUALS(data[0][1], (Event{1, 2.2, DateAndTime(6)}));
    TS_ASSERT_EQUALS(data[0][2], (Event{4, 3.3, DateAndTime(8)}));
  }

  void test_partition_2_workers() {
    EventDataPartitioner<int32_t, int64_t, double> partitioner(
        2, PulseTimeGenerator<int32_t, int64_t>({0, 2, 2, 3}, {2, 4, 6, 8},
                                                "nanosecond", 0));
    std::vector<std::vector<Event>> data;
    std::vector<int32_t> index{5, 1, 4, 1};
    std::vector<double> tof{1.1, 2.2, 3.3, 4.4};
    // Starting at beginning, length 4
    partitioner.partition(data, index.data(), tof.data(), {0, 0, 4});
    TS_ASSERT_EQUALS(data.size(), 2);
    // Worker is given by index%workers
    TS_ASSERT_EQUALS(data[0].size(), 1);
    TS_ASSERT_EQUALS(data[1].size(), 3);
    // Index is translated to local index = index/workers
    TS_ASSERT_EQUALS(data[1][0], (Event{2, 1.1, DateAndTime(2)}));
    TS_ASSERT_EQUALS(data[1][1], (Event{0, 2.2, DateAndTime(2)}));
    TS_ASSERT_EQUALS(data[0][0], (Event{2, 3.3, DateAndTime(6)}));
    TS_ASSERT_EQUALS(data[1][2], (Event{0, 4.4, DateAndTime(8)}));
    // Starting at offset 1, length 4
    partitioner.partition(data, index.data(), tof.data(), {0, 1, 4});
    TS_ASSERT_EQUALS(data.size(), 2);
    TS_ASSERT_EQUALS(data[0].size(), 1);
    TS_ASSERT_EQUALS(data[1].size(), 3);
    TS_ASSERT_EQUALS(data[1][0], (Event{2, 1.1, DateAndTime(2)}));
    TS_ASSERT_EQUALS(data[1][1], (Event{0, 2.2, DateAndTime(6)}));
    TS_ASSERT_EQUALS(data[0][0], (Event{2, 3.3, DateAndTime(8)}));
    TS_ASSERT_EQUALS(data[1][2], (Event{0, 4.4, DateAndTime(8)}));
  }

  void test_partition_3_workers() {
    EventDataPartitioner<int32_t, int64_t, double> partitioner(
        3, PulseTimeGenerator<int32_t, int64_t>({0, 2, 2, 3}, {2, 4, 6, 8},
                                                "nanosecond", 0));
    std::vector<std::vector<Event>> data;
    std::vector<int32_t> index{5, 1, 4, 1};
    std::vector<double> tof{1.1, 2.2, 3.3, 4.4};
    // Starting at beginning, length 4
    partitioner.partition(data, index.data(), tof.data(), {0, 0, 4});
    TS_ASSERT_EQUALS(data.size(), 3);
    TS_ASSERT_EQUALS(data[0].size(), 0); // no index with %3 == 0
    TS_ASSERT_EQUALS(data[1].size(), 3);
    TS_ASSERT_EQUALS(data[2].size(), 1);
    TS_ASSERT_EQUALS(data[2][0], (Event{1, 1.1, DateAndTime(2)}));
    TS_ASSERT_EQUALS(data[1][0], (Event{0, 2.2, DateAndTime(2)}));
    TS_ASSERT_EQUALS(data[1][1], (Event{1, 3.3, DateAndTime(6)}));
    TS_ASSERT_EQUALS(data[1][2], (Event{0, 4.4, DateAndTime(8)}));
    // Starting at offset 1, length 4
    partitioner.partition(data, index.data(), tof.data(), {0, 1, 4});
    TS_ASSERT_EQUALS(data.size(), 3);
    TS_ASSERT_EQUALS(data[0].size(), 0); // no index with %3 == 0
    TS_ASSERT_EQUALS(data[1].size(), 3);
    TS_ASSERT_EQUALS(data[2].size(), 1);
    TS_ASSERT_EQUALS(data[2][0], (Event{1, 1.1, DateAndTime(2)}));
    TS_ASSERT_EQUALS(data[1][0], (Event{0, 2.2, DateAndTime(6)}));
    TS_ASSERT_EQUALS(data[1][1], (Event{1, 3.3, DateAndTime(8)}));
    TS_ASSERT_EQUALS(data[1][2], (Event{0, 4.4, DateAndTime(8)}));
  }
};

#endif /* MANTID_PARALLEL_EVENTDATAPARTITIONERTEST_H_ */
