#ifndef MANTID_INDEXING_ROUNDROBINPARTITIONINGTEST_H_
#define MANTID_INDEXING_ROUNDROBINPARTITIONINGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/RoundRobinPartitioning.h"

using Mantid::Indexing::RoundRobinPartitioning;
using Mantid::Indexing::SpectrumNumber;

class RoundRobinPartitioningTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RoundRobinPartitioningTest *createSuite() {
    return new RoundRobinPartitioningTest();
  }
  static void destroySuite(RoundRobinPartitioningTest *suite) { delete suite; }

  void test_constructor_failures() {
    TS_ASSERT_THROWS(RoundRobinPartitioning(-1, 0), std::logic_error);
    TS_ASSERT_THROWS(RoundRobinPartitioning(0, 0), std::logic_error);
    TS_ASSERT_THROWS(RoundRobinPartitioning(1, -1), std::logic_error);
    TS_ASSERT_THROWS(RoundRobinPartitioning(1, 1), std::logic_error);
  }

  void test_1_rank() {
    RoundRobinPartitioning partitioning(1, 0);
    TS_ASSERT_EQUALS(partitioning.numberOfPartitions(), 1);
    TS_ASSERT_EQUALS(partitioning.partitionIndex(), 0);
    TS_ASSERT_EQUALS(partitioning.partitionIndexOf(SpectrumNumber(0)), 0);
    TS_ASSERT_EQUALS(partitioning.partitionIndexOf(SpectrumNumber(1)), 0);
    TS_ASSERT_EQUALS(partitioning.partitionIndexOf(SpectrumNumber(2)), 0);
  }

  void test_3_ranks() {
    RoundRobinPartitioning partitioning(3, 1);
    TS_ASSERT_EQUALS(partitioning.numberOfPartitions(), 3);
    TS_ASSERT_EQUALS(partitioning.partitionIndex(), 1);
    TS_ASSERT_EQUALS(partitioning.partitionIndexOf(SpectrumNumber(0)), 0);
    TS_ASSERT_EQUALS(partitioning.partitionIndexOf(SpectrumNumber(1)), 1);
    TS_ASSERT_EQUALS(partitioning.partitionIndexOf(SpectrumNumber(2)), 2);
    TS_ASSERT_EQUALS(partitioning.partitionIndexOf(SpectrumNumber(3)), 0);
  }
};

#endif /* MANTID_INDEXING_ROUNDROBINPARTITIONINGTEST_H_ */
