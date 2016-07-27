#ifndef MANTID_INDEXING_PARTITIONINGTEST_H_
#define MANTID_INDEXING_PARTITIONINGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/Partitioning.h"

using namespace Mantid;
using namespace Indexing;

class PartitioningHelper : public Partitioning {
public:
  explicit PartitioningHelper(int numberOfPartitions)
      : Partitioning(numberOfPartitions, PartitionIndex(0),
                     Partitioning::MonitorStrategy::CloneOnEachPartition,
                     std::vector<SpectrumNumber>{}) {}

  PartitioningHelper(int numberOfPartitions, const PartitionIndex partition,
                     const MonitorStrategy monitorStrategy,
                     std::vector<SpectrumNumber> monitors)
      : Partitioning(numberOfPartitions, partition, monitorStrategy, monitors) {
  }

private:
  PartitionIndex doIndexOf(const SpectrumNumber) const override {
    return PartitionIndex(0);
  }
};

class PartitioningTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PartitioningTest *createSuite() { return new PartitioningTest(); }
  static void destroySuite(PartitioningTest *suite) { delete suite; }

  void test_constructor_failures() {
    TS_ASSERT_THROWS(PartitioningHelper(-1), std::logic_error);
    TS_ASSERT_THROWS(PartitioningHelper(0), std::logic_error);
    TS_ASSERT_THROWS_NOTHING(
        PartitioningHelper(1, PartitionIndex(13),
                           Partitioning::MonitorStrategy::CloneOnEachPartition,
                           std::vector<SpectrumNumber>{SpectrumNumber(3)}));
    TS_ASSERT_THROWS(
        PartitioningHelper(1, PartitionIndex(13),
                           Partitioning::MonitorStrategy::DedicatedPartition,
                           std::vector<SpectrumNumber>{SpectrumNumber(3)}),
        std::logic_error);
    TS_ASSERT_THROWS_NOTHING(
        PartitioningHelper(2, PartitionIndex(13),
                           Partitioning::MonitorStrategy::DedicatedPartition,
                           std::vector<SpectrumNumber>{SpectrumNumber(3)}));
  }

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

  void test_numberOfPartitions() {
    TS_ASSERT_EQUALS(
        PartitioningHelper(42, PartitionIndex(13),
                           Partitioning::MonitorStrategy::CloneOnEachPartition,
                           std::vector<SpectrumNumber>{}).numberOfPartitions(),
        42);
    TS_ASSERT_EQUALS(
        PartitioningHelper(42, PartitionIndex(13),
                           Partitioning::MonitorStrategy::DedicatedPartition,
                           std::vector<SpectrumNumber>{}).numberOfPartitions(),
        42);
  }

  void test_indexOf_cloned_monitors() {
    PartitioningHelper p(42, PartitionIndex(13),
                         Partitioning::MonitorStrategy::CloneOnEachPartition,
                         std::vector<SpectrumNumber>{SpectrumNumber(3)});
    TS_ASSERT_EQUALS(p.indexOf(SpectrumNumber(3)), PartitionIndex(13));
  }

  void test_indexOf_dedicated_monitors() {
    PartitioningHelper p(42, PartitionIndex(13),
                         Partitioning::MonitorStrategy::DedicatedPartition,
                         std::vector<SpectrumNumber>{SpectrumNumber(3)});
    TS_ASSERT_EQUALS(p.indexOf(SpectrumNumber(3)), PartitionIndex(41));
  }
};

#endif /* MANTID_INDEXING_PARTITIONINGTEST_H_ */
