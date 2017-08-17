#ifndef MANTID_INDEXING_ROUNDROBINPARTITIONERTEST_H_
#define MANTID_INDEXING_ROUNDROBINPARTITIONERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/RoundRobinPartitioner.h"

using namespace Mantid::Indexing;

class RoundRobinPartitionerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RoundRobinPartitionerTest *createSuite() {
    return new RoundRobinPartitionerTest();
  }
  static void destroySuite(RoundRobinPartitionerTest *suite) { delete suite; }

  void test_1_rank() {
    RoundRobinPartitioner partitioner(
        1, PartitionIndex(0),
        Partitioner::MonitorStrategy::CloneOnEachPartition,
        std::vector<GlobalSpectrumIndex>{});
    TS_ASSERT_EQUALS(partitioner.numberOfPartitions(), 1);
    TS_ASSERT_EQUALS(partitioner.indexOf(GlobalSpectrumIndex(0)), 0);
    TS_ASSERT_EQUALS(partitioner.indexOf(GlobalSpectrumIndex(1)), 0);
    TS_ASSERT_EQUALS(partitioner.indexOf(GlobalSpectrumIndex(2)), 0);
  }

  void test_3_ranks() {
    RoundRobinPartitioner partitioner(
        3, PartitionIndex(0),
        Partitioner::MonitorStrategy::CloneOnEachPartition,
        std::vector<GlobalSpectrumIndex>{});
    TS_ASSERT_EQUALS(partitioner.numberOfPartitions(), 3);
    TS_ASSERT_EQUALS(partitioner.indexOf(GlobalSpectrumIndex(0)), 0);
    TS_ASSERT_EQUALS(partitioner.indexOf(GlobalSpectrumIndex(1)), 1);
    TS_ASSERT_EQUALS(partitioner.indexOf(GlobalSpectrumIndex(2)), 2);
    TS_ASSERT_EQUALS(partitioner.indexOf(GlobalSpectrumIndex(3)), 0);
  }
};

#endif /* MANTID_INDEXING_ROUNDROBINPARTITIONERTEST_H_ */
