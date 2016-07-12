#ifndef MANTID_INDEXING_PARTITIONINGTEST_H_
#define MANTID_INDEXING_PARTITIONINGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/Partitioning.h"

using namespace Mantid;
using namespace Indexing;

class PartitioningHelper : public Partitioning {
public:
  explicit PartitioningHelper(int numberOfPartitions)
      : m_partitions(numberOfPartitions) {}

  int numberOfPartitions() const override { return m_partitions; }
  PartitionIndex indexOf(const SpectrumNumber &) const override {
    return PartitionIndex(0);
  }

private:
  int m_partitions;
};

class PartitioningTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PartitioningTest *createSuite() { return new PartitioningTest(); }
  static void destroySuite(PartitioningTest *suite) { delete suite; }

  void test_isValid() {
    PartitioningHelper p(42);
    TS_ASSERT(!p.isValid(PartitionIndex(-1)));
    TS_ASSERT(p.isValid(PartitionIndex(0)));
    TS_ASSERT(p.isValid(PartitionIndex(41)));
    TS_ASSERT(!p.isValid(PartitionIndex(42)));
  }

  void test_checkValid() {
    PartitioningHelper p(42);
    TS_ASSERT_THROWS(p.checkValid(PartitionIndex(-1)), std::out_of_range);
    TS_ASSERT_THROWS_NOTHING(p.checkValid(PartitionIndex(0)));
    TS_ASSERT_THROWS_NOTHING(p.checkValid(PartitionIndex(41)));
    TS_ASSERT_THROWS(p.checkValid(PartitionIndex(42)), std::out_of_range);
  }
};

#endif /* MANTID_INDEXING_PARTITIONINGTEST_H_ */
