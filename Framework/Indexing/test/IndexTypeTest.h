#ifndef MANTID_INDEXING_INDEXTYPETEST_H_
#define MANTID_INDEXING_INDEXTYPETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/IndexType.h"

using Mantid::Indexing::detail::IndexType;

struct MyIndex : public IndexType<MyIndex, int64_t> {
  using IndexType<MyIndex, int64_t>::IndexType;
  using IndexType<MyIndex, int64_t>::operator=;
};

class IndexTypeTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IndexTypeTest *createSuite() { return new IndexTypeTest(); }
  static void destroySuite(IndexTypeTest *suite) { delete suite; }

  void test_construct() {
    MyIndex index(42);
    TS_ASSERT_EQUALS(index, 42);
  }

  void test_copy() {
    MyIndex index(42);
    MyIndex copy(index);
    TS_ASSERT_EQUALS(copy, 42);
  }

  void test_assignment() {
    MyIndex index(42);
    MyIndex assignee(0);
    assignee = index;
    TS_ASSERT_EQUALS(assignee, 42);
    assignee = 0;
    TS_ASSERT_EQUALS(assignee, 0);
  }

  void test_operator_equals() {
    MyIndex data(42);
    MyIndex same(42);
    MyIndex different(100);
    TS_ASSERT(data == data);
    TS_ASSERT(data == same);
    TS_ASSERT(!(data == different));
    TS_ASSERT(data == 42);
    TS_ASSERT(!(data == 100));
  }

  void test_operator_not_equals() {
    MyIndex data(42);
    MyIndex same(42);
    MyIndex different(100);
    TS_ASSERT(!(data != data));
    TS_ASSERT(!(data != same));
    TS_ASSERT(data != different);
    TS_ASSERT(!(data != 42));
    TS_ASSERT(data != 100);
  }

  void test_operator_greater() {
    MyIndex data(42);
    TS_ASSERT(data > 41);
    TS_ASSERT(!(data > 42));
  }

  void test_operator_greater_equal() {
    MyIndex data(42);
    TS_ASSERT(data >= 42);
    TS_ASSERT(!(data >= 43));
  }

  void test_operator_less() {
    MyIndex data(42);
    TS_ASSERT(data < 43);
    TS_ASSERT(!(data < 42));
  }

  void test_operator_less_equal() {
    MyIndex data(42);
    TS_ASSERT(data <= 42);
    TS_ASSERT(!(data <= 41));
  }
};

#endif /* MANTID_INDEXING_INDEXTYPETEST_H_ */
