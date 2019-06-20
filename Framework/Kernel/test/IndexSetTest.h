// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_INDEXSETTEST_H_
#define MANTID_KERNEL_INDEXSETTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Exception.h"
#include "MantidKernel/IndexSet.h"

using namespace Mantid;
using namespace Mantid::Kernel;

class IndexSetTest : public CxxTest::TestSuite {
public:
  void test_fullRangeConstructor() {
    TS_ASSERT_THROWS_NOTHING(IndexSet set(3));
    // Also empty set is supported
    TS_ASSERT_THROWS_NOTHING(IndexSet set(0));
  }

  void test_rangeConstructor() {
    // maximal possible range: 0...N-1
    TS_ASSERT_THROWS_NOTHING(IndexSet set(0, 2, 3));
    // smaller range works?
    TS_ASSERT_THROWS_NOTHING(IndexSet set(1, 2, 3));
    // min == max should work as well
    TS_ASSERT_THROWS_NOTHING(IndexSet set(2, 2, 3));
  }

  void test_rangeConstructorErrorCases() {
    // min negative
    TS_ASSERT_THROWS(IndexSet set(-1, 2, 3), const Exception::IndexError &);
    // min > max
    TS_ASSERT_THROWS(IndexSet set(2, 1, 3), const Exception::IndexError &);
    // max above count
    TS_ASSERT_THROWS(IndexSet set(1, 3, 3), const Exception::IndexError &);
    // does it still fail if both are wrong?
    TS_ASSERT_THROWS(IndexSet set(3, 3, 3), const Exception::IndexError &);
  }

  void test_indexListConstructor() {
    TS_ASSERT_THROWS_NOTHING(IndexSet set({1, 2}, 3));
    // Empty set is supported
    TS_ASSERT_THROWS_NOTHING(IndexSet set({}, 3));
  }

  void test_indexListConstructorErrorCases() {
    TS_ASSERT_THROWS(IndexSet set({3}, 3), const Exception::IndexError &);
  }

  void test_size() {
    size_t fullRange = 5;
    IndexSet set1(fullRange);
    TS_ASSERT_EQUALS(set1.size(), fullRange);
    IndexSet set2(1, 2, fullRange);
    TS_ASSERT_EQUALS(set2.size(), 2);
  }

  void test_fullRange() {
    IndexSet set(3);
    TS_ASSERT_EQUALS(set.size(), 3);
    TS_ASSERT_EQUALS(set[0], 0);
    TS_ASSERT_EQUALS(set[1], 1);
    TS_ASSERT_EQUALS(set[2], 2);
  }

  void test_range() {
    IndexSet set(1, 2, 3);
    TS_ASSERT_EQUALS(set.size(), 2);
    TS_ASSERT_EQUALS(set[0], 1);
    TS_ASSERT_EQUALS(set[1], 2);
  }

  void test_indexList() {
    // Note duplicate index
    IndexSet set({2, 1, 2}, 3);
    TS_ASSERT_EQUALS(set.size(), 2);
    TS_ASSERT_EQUALS(set[0], 1);
    TS_ASSERT_EQUALS(set[1], 2);
  }
};

#endif /* MANTID_KERNEL_INDEXSETTEST_H_ */
