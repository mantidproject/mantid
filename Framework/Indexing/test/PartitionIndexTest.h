#ifndef MANTID_INDEXING_PARTITIONINDEXTEST_H_
#define MANTID_INDEXING_PARTITIONINDEXTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/PartitionIndex.h"

using Mantid::Indexing::PartitionIndex;

class PartitionIndexTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PartitionIndexTest *createSuite() { return new PartitionIndexTest(); }
  static void destroySuite(PartitionIndexTest *suite) { delete suite; }

  void test_construct() {
    PartitionIndex number(42);
    TS_ASSERT_EQUALS(number, 42);
  }

  void test_operator_equals() {
    PartitionIndex data(42);
    PartitionIndex same(42);
    PartitionIndex different(100);
    TS_ASSERT(data == data);
    TS_ASSERT(data == same);
    TS_ASSERT(!(data == different));
    TS_ASSERT(data == 42);
    TS_ASSERT(!(data == 100));
  }

  void test_operator_not_equals() {
    PartitionIndex data(42);
    PartitionIndex same(42);
    PartitionIndex different(100);
    TS_ASSERT(!(data != data));
    TS_ASSERT(!(data != same));
    TS_ASSERT(data != different);
    TS_ASSERT(!(data != 42));
    TS_ASSERT(data != 100);
  }
};

#endif /* MANTID_INDEXING_PARTITIONINDEXTEST_H_ */
