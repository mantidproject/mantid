#ifndef MANTID_INDEXING_PARTITIONINDEXTEST_H_
#define MANTID_INDEXING_PARTITIONINDEXTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/PartitionIndex.h"

using namespace Mantid;
using namespace Indexing;

class PartitionIndexTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PartitionIndexTest *createSuite() { return new PartitionIndexTest(); }
  static void destroySuite(PartitionIndexTest *suite) { delete suite; }

  void test_has_correct_mixins() {
    PartitionIndex data(0);
    TS_ASSERT_THROWS_NOTHING(UNUSED_ARG(
        (dynamic_cast<detail::IndexType<PartitionIndex, int> &>(data))));
  }
};

#endif /* MANTID_INDEXING_PARTITIONINDEXTEST_H_ */
