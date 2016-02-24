#ifndef MANTID_KERNEL_SPECTRUMINDEXSETTEST_H_
#define MANTID_KERNEL_SPECTRUMINDEXSETTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/SpectrumIndexSet.h"
#include "MantidKernel/Exception.h"

using namespace Mantid;
using namespace Mantid::Kernel;

class SpectrumIndexSetTest : public CxxTest::TestSuite {
public:
  void test_fullRangeConstructor() {
    TS_ASSERT_THROWS_NOTHING(SpectrumIndexSet set(3));
    // Also empty set is supported
    TS_ASSERT_THROWS_NOTHING(SpectrumIndexSet set(0));
  }

  void test_rangeConstructor() {
    // maximal possible range: 0...N-1
    TS_ASSERT_THROWS_NOTHING(SpectrumIndexSet set(0, 2, 3));
    // smaller range works?
    TS_ASSERT_THROWS_NOTHING(SpectrumIndexSet set(1, 2, 3));
    // min == max should work as well
    TS_ASSERT_THROWS_NOTHING(SpectrumIndexSet set(2, 2, 3));
  }

  void test_rangeConstructorErrorCases() {
    // min negative
    TS_ASSERT_THROWS(SpectrumIndexSet set(-1, 2, 3), Exception::IndexError);
    // min > max
    TS_ASSERT_THROWS(SpectrumIndexSet set(2, 1, 3), Exception::IndexError);
    // max above count
    TS_ASSERT_THROWS(SpectrumIndexSet set(1, 3, 3), Exception::IndexError);
    // does it still fail if both are wrong?
    TS_ASSERT_THROWS(SpectrumIndexSet set(3, 3, 3), Exception::IndexError);
  }

  void test_indexListConstructor() {
    TS_ASSERT_THROWS_NOTHING(SpectrumIndexSet set({1, 2}, 3));
    // Empty set is supported
    TS_ASSERT_THROWS_NOTHING(SpectrumIndexSet set({}, 3));
  }

  void test_indexListConstructorErrorCases() {
    TS_ASSERT_THROWS(SpectrumIndexSet set({3}, 3), Exception::IndexError);
  }

  void test_size() {
    size_t numberOfHistograms = 5;
    SpectrumIndexSet set1(numberOfHistograms);
    TS_ASSERT_EQUALS(set1.size(), numberOfHistograms);
    SpectrumIndexSet set2(1, 2, numberOfHistograms);
    TS_ASSERT_EQUALS(set2.size(), 2);
  }

  void test_fullRange() {
    SpectrumIndexSet set(3);
    TS_ASSERT_EQUALS(set.size(), 3);
    TS_ASSERT_EQUALS(set[0], 0);
    TS_ASSERT_EQUALS(set[1], 1);
    TS_ASSERT_EQUALS(set[2], 2);
  }

  void test_range() {
    SpectrumIndexSet set(1, 2, 3);
    TS_ASSERT_EQUALS(set.size(), 2);
    TS_ASSERT_EQUALS(set[0], 1);
    TS_ASSERT_EQUALS(set[1], 2);
  }

  void test_indexList() {
    // Note duplicate index
    SpectrumIndexSet set({2, 1, 2}, 3);
    TS_ASSERT_EQUALS(set.size(), 2);
    TS_ASSERT_EQUALS(set[0], 1);
    TS_ASSERT_EQUALS(set[1], 2);
  }

};

#endif /* MANTID_KERNEL_SPECTRUMINDEXSETTEST_H_ */
