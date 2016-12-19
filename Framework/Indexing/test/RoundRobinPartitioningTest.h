#ifndef MANTID_INDEXING_ROUNDROBINPARTITIONINGTEST_H_
#define MANTID_INDEXING_ROUNDROBINPARTITIONINGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/RoundRobinPartitioning.h"

using namespace Mantid::Indexing;

class RoundRobinPartitioningTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RoundRobinPartitioningTest *createSuite() {
    return new RoundRobinPartitioningTest();
  }
  static void destroySuite(RoundRobinPartitioningTest *suite) { delete suite; }

  void test_1_rank() {
    RoundRobinPartitioning partitioning(
        1, PartitionIndex(0),
        Partitioning::MonitorStrategy::CloneOnEachPartition,
        std::vector<SpectrumNumber>{});
    TS_ASSERT_EQUALS(partitioning.numberOfPartitions(), 1);
    TS_ASSERT_EQUALS(partitioning.indexOf(SpectrumNumber(0)), 0);
    TS_ASSERT_EQUALS(partitioning.indexOf(SpectrumNumber(1)), 0);
    TS_ASSERT_EQUALS(partitioning.indexOf(SpectrumNumber(2)), 0);
  }

  void test_3_ranks() {
    RoundRobinPartitioning partitioning(3, PartitionIndex(0),
        Partitioning::MonitorStrategy::CloneOnEachPartition,
        std::vector<SpectrumNumber>{});
    TS_ASSERT_EQUALS(partitioning.numberOfPartitions(), 3);
    TS_ASSERT_EQUALS(partitioning.indexOf(SpectrumNumber(0)), 0);
    TS_ASSERT_EQUALS(partitioning.indexOf(SpectrumNumber(1)), 1);
    TS_ASSERT_EQUALS(partitioning.indexOf(SpectrumNumber(2)), 2);
    TS_ASSERT_EQUALS(partitioning.indexOf(SpectrumNumber(3)), 0);
  }
};

#endif /* MANTID_INDEXING_ROUNDROBINPARTITIONINGTEST_H_ */
