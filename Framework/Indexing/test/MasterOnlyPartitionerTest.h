#ifndef MANTID_INDEXING_MASTERONLYPARTITIONERTEST_H_
#define MANTID_INDEXING_MASTERONLYPARTITIONERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/MasterOnlyPartitioner.h"

using namespace Mantid::Indexing;

class MasterOnlyPartitionerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MasterOnlyPartitionerTest *createSuite() {
    return new MasterOnlyPartitionerTest();
  }
  static void destroySuite(MasterOnlyPartitionerTest *suite) { delete suite; }

  void test_1_rank() {
    MasterOnlyPartitioner partitioner(
        1, PartitionIndex(0),
        Partitioner::MonitorStrategy::TreatAsNormalSpectrum,
        std::vector<GlobalSpectrumIndex>{});
    TS_ASSERT_EQUALS(partitioner.numberOfPartitions(), 1);
    TS_ASSERT_EQUALS(partitioner.indexOf(GlobalSpectrumIndex(0)), 0);
    TS_ASSERT_EQUALS(partitioner.indexOf(GlobalSpectrumIndex(1)), 0);
    TS_ASSERT_EQUALS(partitioner.indexOf(GlobalSpectrumIndex(2)), 0);
  }

  void test_3_ranks() {
    MasterOnlyPartitioner partitioner(
        3, PartitionIndex(0),
        Partitioner::MonitorStrategy::TreatAsNormalSpectrum,
        std::vector<GlobalSpectrumIndex>{});
    TS_ASSERT_EQUALS(partitioner.numberOfPartitions(), 3);
    TS_ASSERT_EQUALS(partitioner.indexOf(GlobalSpectrumIndex(0)), 0);
    TS_ASSERT_EQUALS(partitioner.indexOf(GlobalSpectrumIndex(1)), 0);
    TS_ASSERT_EQUALS(partitioner.indexOf(GlobalSpectrumIndex(2)), 0);
    TS_ASSERT_EQUALS(partitioner.indexOf(GlobalSpectrumIndex(3)), 0);
  }
};

#endif /* MANTID_INDEXING_MASTERONLYPARTITIONERTEST_H_ */
