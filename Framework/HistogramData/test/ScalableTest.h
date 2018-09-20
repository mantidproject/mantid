#ifndef MANTID_HISTOGRAMDATA_SCALABLETEST_H_
#define MANTID_HISTOGRAMDATA_SCALABLETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/FixedLengthVector.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/Iterable.h"
#include "MantidHistogramData/Scalable.h"
#include "MantidHistogramData/VectorOf.h"

using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::detail::FixedLengthVector;
using Mantid::HistogramData::detail::Iterable;
using Mantid::HistogramData::detail::Scalable;
using Mantid::HistogramData::detail::VectorOf;

class ScalableTester : public VectorOf<ScalableTester, HistogramX>,
                       public Iterable<ScalableTester>,
                       public Scalable<ScalableTester> {
  using VectorOf<ScalableTester, HistogramX>::VectorOf;
};

// Does Scalable also work with FixedLengthVector instead of VectorOf?
struct ScalableTester2 : public FixedLengthVector<ScalableTester2>,
                         public Scalable<ScalableTester2> {
  using FixedLengthVector<ScalableTester2>::FixedLengthVector;
};

class ScalableTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ScalableTest *createSuite() { return new ScalableTest(); }
  static void destroySuite(ScalableTest *suite) { delete suite; }

  void test_times_equals() {
    ScalableTester lhs{0.1, 0.2};
    const double rhs = 3.0;
    lhs *= rhs;
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], 0.3, 1e-14);
    TS_ASSERT_DELTA(lhs[1], 0.6, 1e-14);
  }

  void test_divide_equals() {
    ScalableTester lhs{0.1, 0.2};
    const double rhs = 2.0;
    lhs /= rhs;
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], 0.05, 1e-14);
    TS_ASSERT_DELTA(lhs[1], 0.1, 1e-14);
  }

  void test_times() {
    const ScalableTester rhs1{0.1, 0.2};
    const double rhs2 = 3.0;
    const ScalableTester lhs(rhs1 * rhs2);
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], 0.3, 1e-14);
    TS_ASSERT_DELTA(lhs[1], 0.6, 1e-14);
  }

  void test_divide() {
    const ScalableTester rhs1{0.1, 0.2};
    const double rhs2 = 2.0;
    const ScalableTester lhs(rhs1 / rhs2);
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], 0.05, 1e-14);
    TS_ASSERT_DELTA(lhs[1], 0.1, 1e-14);
  }

  void test_times_reverse_order() {
    const ScalableTester rhs{0.1, 0.2};
    const ScalableTester lhs(3.0 * rhs);
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], 0.3, 1e-14);
    TS_ASSERT_DELTA(lhs[1], 0.6, 1e-14);
  }

  void test_with_FixedLengthVector() {
    ScalableTester2 lhs{0.1, 0.2};
    const double rhs = 3.0;
    lhs *= rhs;
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], 0.3, 1e-14);
    TS_ASSERT_DELTA(lhs[1], 0.6, 1e-14);
  }
};

#endif /* MANTID_HISTOGRAMDATA_SCALABLETEST_H_ */
