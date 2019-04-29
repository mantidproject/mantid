// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_INDEXING_INDEXSETTEST_H_
#define MANTID_INDEXING_INDEXSETTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/IndexSet.h"

using Mantid::Indexing::detail::IndexSet;

class IndexSetTester : public IndexSet<IndexSetTester> {
public:
  using IndexSet<IndexSetTester>::IndexSet;
};

class IndexSetTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IndexSetTest *createSuite() { return new IndexSetTest(); }
  static void destroySuite(IndexSetTest *suite) { delete suite; }

  void test_fullRangeConstructor() {
    TS_ASSERT_THROWS_NOTHING(IndexSetTester set(3));
    // Also empty set is supported
    TS_ASSERT_THROWS_NOTHING(IndexSetTester set(0));
    TS_ASSERT_THROWS_NOTHING(IndexSetTester set);
  }

  void test_rangeConstructor() {
    // maximal possible range: 0...N-1
    TS_ASSERT_THROWS_NOTHING(IndexSetTester set(0, 2, 3));
    // smaller range works?
    TS_ASSERT_THROWS_NOTHING(IndexSetTester set(1, 2, 3));
    // min == max should work as well
    TS_ASSERT_THROWS_NOTHING(IndexSetTester set(2, 2, 3));
  }

  void test_rangeConstructorErrorCases() {
    // min negative
    TS_ASSERT_THROWS(IndexSetTester set(-1, 2, 3), std::logic_error);
    // min > max
    TS_ASSERT_THROWS(IndexSetTester set(2, 1, 3), std::logic_error);
    // max above count
    TS_ASSERT_THROWS(IndexSetTester set(1, 3, 3), std::out_of_range);
    // does it still fail if both are wrong?
    TS_ASSERT_THROWS(IndexSetTester set(3, 3, 3), std::logic_error);
  }

  void test_indexListConstructor() {
    TS_ASSERT_THROWS_NOTHING(IndexSetTester set({1, 2}, 3));
    // Empty set is supported
    TS_ASSERT_THROWS_NOTHING(IndexSetTester set({}, 3));
  }

  void test_indexListConstructorErrorCases() {
    TS_ASSERT_THROWS(IndexSetTester set({3}, 3), std::out_of_range);
  }

  void test_size() {
    size_t fullRange = 5;
    IndexSetTester set1(fullRange);
    TS_ASSERT_EQUALS(set1.size(), fullRange);
    IndexSetTester set2(1, 2, fullRange);
    TS_ASSERT_EQUALS(set2.size(), 2);
  }

  void test_empty() {
    TS_ASSERT(IndexSetTester(0).empty());
    TS_ASSERT(!IndexSetTester(1).empty());
  }

  void test_fullRange() {
    IndexSetTester set(3);
    TS_ASSERT_EQUALS(set.size(), 3);
    TS_ASSERT_EQUALS(set[0], 0);
    TS_ASSERT_EQUALS(set[1], 1);
    TS_ASSERT_EQUALS(set[2], 2);
  }

  void test_range() {
    IndexSetTester set(1, 2, 3);
    TS_ASSERT_EQUALS(set.size(), 2);
    TS_ASSERT_EQUALS(set[0], 1);
    TS_ASSERT_EQUALS(set[1], 2);
  }

  void test_indexList_order_preserved() {
    IndexSetTester set({2, 1, 3}, 4);
    TS_ASSERT_EQUALS(set.size(), 3);
    TS_ASSERT_EQUALS(set[0], 2);
    TS_ASSERT_EQUALS(set[1], 1);
    TS_ASSERT_EQUALS(set[2], 3);
  }

  void test_indexList_duplicate_throws() {
    TS_ASSERT_THROWS_EQUALS(IndexSetTester({2, 1, 2}, 3),
                            const std::runtime_error &e, std::string(e.what()),
                            "IndexSet: duplicate indices are not allowed");
  }

  void test_iterator_basics() {
    const IndexSetTester set(3);
    auto it = set.begin();
    TS_ASSERT_DIFFERS(it, set.end());
    TS_ASSERT_EQUALS(*it, 0);
    TS_ASSERT_THROWS_NOTHING(++it);
    TS_ASSERT_EQUALS(*it, 1);
    TS_ASSERT_THROWS_NOTHING(++it);
    TS_ASSERT_EQUALS(*it, 2);
    TS_ASSERT_THROWS_NOTHING(++it);
    TS_ASSERT_EQUALS(it, set.end());
    TS_ASSERT_THROWS_NOTHING(++it);
    TS_ASSERT_EQUALS(it, set.end());
  }

  void test_iterator_random_access() {
    const IndexSetTester set(3);
    auto it = set.begin();
    TS_ASSERT_EQUALS(it + 3, set.end());
    TS_ASSERT_EQUALS(it[0], 0);
    TS_ASSERT_EQUALS(it[1], 1);
    TS_ASSERT_EQUALS(it[2], 2);
    TS_ASSERT(it < set.end());
    TS_ASSERT_THROWS_NOTHING(--it);
    TS_ASSERT_EQUALS(*it, 0);
    auto it2 = set.end();
    TS_ASSERT_THROWS_NOTHING(--it2);
    TS_ASSERT_EQUALS(*it2, 2);
    TS_ASSERT_THROWS_NOTHING(--it2);
    TS_ASSERT_EQUALS(*it2, 1);
    TS_ASSERT_THROWS_NOTHING(--it2);
    TS_ASSERT_EQUALS(*it2, 0);
  }

  void test_isContiguous() {
    const IndexSetTester empty{};
    TS_ASSERT(empty.isContiguous())
    const IndexSetTester range(3);
    TS_ASSERT(range.isContiguous())
    const IndexSetTester manualRange({3, 4, 5}, 6);
    TS_ASSERT(manualRange.isContiguous())
    IndexSetTester nonContiguous({2, 1, 3}, 4);
    TS_ASSERT(!nonContiguous.isContiguous())
  }
};

#endif /* MANTID_INDEXING_INDEXSETTEST_H_ */
