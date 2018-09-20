#ifndef MANTID_HISTOGRAMDATA_OFFSETABLETEST_H_
#define MANTID_HISTOGRAMDATA_OFFSETABLETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/FixedLengthVector.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/Iterable.h"
#include "MantidHistogramData/Offsetable.h"
#include "MantidHistogramData/VectorOf.h"

using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::detail::FixedLengthVector;
using Mantid::HistogramData::detail::Iterable;
using Mantid::HistogramData::detail::Offsetable;
using Mantid::HistogramData::detail::VectorOf;

class OffsetableTester : public VectorOf<OffsetableTester, HistogramX>,
                         public Iterable<OffsetableTester>,
                         public Offsetable<OffsetableTester> {
  using VectorOf<OffsetableTester, HistogramX>::VectorOf;
};

// Does Offsetable also work with FixedLengthVector instead of VectorOf?
struct OffsetableTester2 : public FixedLengthVector<OffsetableTester2>,
                           public Offsetable<OffsetableTester2> {
  using FixedLengthVector<OffsetableTester2>::FixedLengthVector;
};

class OffsetableTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static OffsetableTest *createSuite() { return new OffsetableTest(); }
  static void destroySuite(OffsetableTest *suite) { delete suite; }

  void test_plus_equals() {
    OffsetableTester lhs{0.1, 0.2};
    const double rhs = 0.01;
    lhs += rhs;
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], 0.11, 1e-14);
    TS_ASSERT_DELTA(lhs[1], 0.21, 1e-14);
  }

  void test_minus_equals() {
    OffsetableTester lhs{0.1, 0.2};
    const double rhs = 0.01;
    lhs -= rhs;
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], 0.09, 1e-14);
    TS_ASSERT_DELTA(lhs[1], 0.19, 1e-14);
  }

  void test_plus() {
    const OffsetableTester rhs1{0.1, 0.2};
    const double rhs2 = 0.01;
    const OffsetableTester lhs(rhs1 + rhs2);
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], 0.11, 1e-14);
    TS_ASSERT_DELTA(lhs[1], 0.21, 1e-14);
  }

  void test_minus() {
    const OffsetableTester rhs1{0.1, 0.2};
    const double rhs2 = 0.01;
    const OffsetableTester lhs(rhs1 - rhs2);
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], 0.09, 1e-14);
    TS_ASSERT_DELTA(lhs[1], 0.19, 1e-14);
  }

  void test_with_FixedLengthVector() {
    OffsetableTester2 lhs{0.1, 0.2};
    const double rhs = 0.01;
    lhs += rhs;
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], 0.11, 1e-14);
    TS_ASSERT_DELTA(lhs[1], 0.21, 1e-14);
  }
};

#endif /* MANTID_HISTOGRAMDATA_OFFSETABLETEST_H_ */
