// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_INDEXING_PARTITIONERTEST_H_
#define MANTID_INDEXING_PARTITIONERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/Partitioner.h"

using namespace Mantid;
using namespace Indexing;

class PartitionerHelper : public Partitioner {
public:
  explicit PartitionerHelper(int numberOfPartitions)
      : Partitioner(numberOfPartitions, PartitionIndex(0),
                    Partitioner::MonitorStrategy::CloneOnEachPartition,
                    std::vector<GlobalSpectrumIndex>{}) {}

  PartitionerHelper(int numberOfPartitions, const PartitionIndex partition,
                    const MonitorStrategy monitorStrategy,
                    std::vector<GlobalSpectrumIndex> monitors)
      : Partitioner(numberOfPartitions, partition, monitorStrategy, monitors) {}

private:
  PartitionIndex doIndexOf(const GlobalSpectrumIndex) const override {
    return PartitionIndex(0);
  }
};

class PartitionerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PartitionerTest *createSuite() { return new PartitionerTest(); }
  static void destroySuite(PartitionerTest *suite) { delete suite; }

  void test_constructor_failures() {
    TS_ASSERT_THROWS(PartitionerHelper(-1), const std::logic_error &);
    TS_ASSERT_THROWS(PartitionerHelper(0), const std::logic_error &);
    TS_ASSERT_THROWS_NOTHING(PartitionerHelper(
        1, PartitionIndex(13),
        Partitioner::MonitorStrategy::CloneOnEachPartition,
        std::vector<GlobalSpectrumIndex>{GlobalSpectrumIndex(3)}));
    TS_ASSERT_THROWS(
        PartitionerHelper(
            1, PartitionIndex(13),
            Partitioner::MonitorStrategy::DedicatedPartition,
            std::vector<GlobalSpectrumIndex>{GlobalSpectrumIndex(3)}),
        const std::logic_error &);
    TS_ASSERT_THROWS_NOTHING(PartitionerHelper(
        2, PartitionIndex(13), Partitioner::MonitorStrategy::DedicatedPartition,
        std::vector<GlobalSpectrumIndex>{GlobalSpectrumIndex(3)}));
  }

  void test_isValid() {
    PartitionerHelper p(42);
    TS_ASSERT(!p.isValid(PartitionIndex(-1)));
    TS_ASSERT(p.isValid(PartitionIndex(0)));
    TS_ASSERT(p.isValid(PartitionIndex(41)));
    TS_ASSERT(!p.isValid(PartitionIndex(42)));
  }

  void test_checkValid() {
    PartitionerHelper p(42);
    TS_ASSERT_THROWS(p.checkValid(PartitionIndex(-1)),
                     const std::out_of_range &);
    TS_ASSERT_THROWS_NOTHING(p.checkValid(PartitionIndex(0)));
    TS_ASSERT_THROWS_NOTHING(p.checkValid(PartitionIndex(41)));
    TS_ASSERT_THROWS(p.checkValid(PartitionIndex(42)),
                     const std::out_of_range &);
  }

  void test_numberOfPartitions() {
    TS_ASSERT_EQUALS(
        PartitionerHelper(42, PartitionIndex(13),
                          Partitioner::MonitorStrategy::CloneOnEachPartition,
                          std::vector<GlobalSpectrumIndex>{})
            .numberOfPartitions(),
        42);
    TS_ASSERT_EQUALS(
        PartitionerHelper(42, PartitionIndex(13),
                          Partitioner::MonitorStrategy::DedicatedPartition,
                          std::vector<GlobalSpectrumIndex>{})
            .numberOfPartitions(),
        42);
  }

  void test_indexOf_normal_monitors() {
    PartitionerHelper p(
        42, PartitionIndex(13),
        Partitioner::MonitorStrategy::TreatAsNormalSpectrum,
        std::vector<GlobalSpectrumIndex>{GlobalSpectrumIndex(3)});
    TS_ASSERT_EQUALS(p.indexOf(GlobalSpectrumIndex(3)), PartitionIndex(0));
  }

  void test_indexOf_cloned_monitors() {
    PartitionerHelper p(
        42, PartitionIndex(13),
        Partitioner::MonitorStrategy::CloneOnEachPartition,
        std::vector<GlobalSpectrumIndex>{GlobalSpectrumIndex(3)});
    TS_ASSERT_EQUALS(p.indexOf(GlobalSpectrumIndex(3)), PartitionIndex(13));
  }

  void test_indexOf_dedicated_monitors() {
    PartitionerHelper p(
        42, PartitionIndex(13),
        Partitioner::MonitorStrategy::DedicatedPartition,
        std::vector<GlobalSpectrumIndex>{GlobalSpectrumIndex(3)});
    TS_ASSERT_EQUALS(p.indexOf(GlobalSpectrumIndex(3)), PartitionIndex(41));
  }
};

#endif /* MANTID_INDEXING_PARTITIONERTEST_H_ */
