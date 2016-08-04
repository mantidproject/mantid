#ifndef MANTID_HISTOGRAMDATA_SQUAREADDABLETEST_H_
#define MANTID_HISTOGRAMDATA_SQUAREADDABLETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/SquareAddable.h"
#include "MantidHistogramData/FixedLengthVector.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/Iterable.h"
#include "MantidHistogramData/VectorOf.h"

using Mantid::HistogramData::detail::FixedLengthVector;
using Mantid::HistogramData::detail::Iterable;
using Mantid::HistogramData::detail::SquareAddable;
using Mantid::HistogramData::detail::VectorOf;
using Mantid::HistogramData::HistogramX;

class SquareAddableTester : public VectorOf<SquareAddableTester, HistogramX>,
                            public Iterable<SquareAddableTester>,
                            public SquareAddable<SquareAddableTester> {
  using VectorOf<SquareAddableTester, HistogramX>::VectorOf;
};

// Does SquareAddable also work with FixedLengthVector instead of VectorOf?
struct SquareAddableTester2 : public FixedLengthVector<SquareAddableTester2>,
                              public SquareAddable<SquareAddableTester2> {
  using FixedLengthVector<SquareAddableTester2>::FixedLengthVector;
};

class SquareAddableTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SquareAddableTest *createSuite() { return new SquareAddableTest(); }
  static void destroySuite(SquareAddableTest *suite) { delete suite; }

  void test_plus_equals() {
    SquareAddableTester lhs{1, 2};
    const SquareAddableTester rhs{3, 4};
    lhs += rhs;
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], sqrt(1 + 9), 1e-14);
    TS_ASSERT_DELTA(lhs[1], sqrt(4 + 16), 1e-14);
  }

  void test_plus() {
    const SquareAddableTester rhs1{1, 2};
    const SquareAddableTester rhs2{3, 4};
    const SquareAddableTester lhs(rhs1 + rhs2);
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], sqrt(1 + 9), 1e-14);
    TS_ASSERT_DELTA(lhs[1], sqrt(4 + 16), 1e-14);
  }

  void test_with_FixedLengthVector() {
    SquareAddableTester2 lhs{1, 2};
    const SquareAddableTester2 rhs{3, 4};
    lhs += rhs;
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], sqrt(1 + 9), 1e-14);
    TS_ASSERT_DELTA(lhs[1], sqrt(4 + 16), 1e-14);
  }
};

#endif /* MANTID_HISTOGRAMDATA_SQUAREADDABLETEST_H_ */
