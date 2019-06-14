// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_HISTOGRAMDATA_MULTIPLIABLETEST_H_
#define MANTID_HISTOGRAMDATA_MULTIPLIABLETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/FixedLengthVector.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/Iterable.h"
#include "MantidHistogramData/Multipliable.h"
#include "MantidHistogramData/VectorOf.h"

using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::detail::FixedLengthVector;
using Mantid::HistogramData::detail::Iterable;
using Mantid::HistogramData::detail::Multipliable;
using Mantid::HistogramData::detail::VectorOf;

class MultipliableTester : public VectorOf<MultipliableTester, HistogramX>,
                           public Iterable<MultipliableTester>,
                           public Multipliable<MultipliableTester> {
  using VectorOf<MultipliableTester, HistogramX>::VectorOf;
};

// Does Multipliable also work with FixedLengthVector instead of VectorOf?
struct MultipliableTester2 : public FixedLengthVector<MultipliableTester2>,
                             public Multipliable<MultipliableTester2> {
  using FixedLengthVector<MultipliableTester2>::FixedLengthVector;
};

class MultipliableTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MultipliableTest *createSuite() { return new MultipliableTest(); }
  static void destroySuite(MultipliableTest *suite) { delete suite; }

  void test_times_equals() {
    MultipliableTester lhs{0.1, 0.2};
    const MultipliableTester rhs{2, 4};
    lhs *= rhs;
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], 0.2, 1e-14);
    TS_ASSERT_DELTA(lhs[1], 0.8, 1e-14);
  }

  void test_divide_equals() {
    MultipliableTester lhs{0.1, 0.2};
    const MultipliableTester rhs{2, 4};
    lhs /= rhs;
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], 0.05, 1e-14);
    TS_ASSERT_DELTA(lhs[1], 0.05, 1e-14);
  }

  void test_times() {
    const MultipliableTester rhs1{0.1, 0.2};
    const MultipliableTester rhs2{2, 4};
    const MultipliableTester lhs(rhs1 * rhs2);
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], 0.2, 1e-14);
    TS_ASSERT_DELTA(lhs[1], 0.8, 1e-14);
  }

  void test_divide() {
    const MultipliableTester rhs1{0.1, 0.2};
    const MultipliableTester rhs2{2, 4};
    const MultipliableTester lhs(rhs1 / rhs2);
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], 0.05, 1e-14);
    TS_ASSERT_DELTA(lhs[1], 0.05, 1e-14);
  }

  void test_length_mismatch() {
    MultipliableTester rhs1{1, 2};
    const MultipliableTester rhs2{1, 2, 3};
    TS_ASSERT_THROWS(rhs1 * rhs2, const std::runtime_error &);
    TS_ASSERT_THROWS(rhs1 / rhs2, const std::runtime_error &);
    TS_ASSERT_THROWS(rhs1 *= rhs2, const std::runtime_error &);
    TS_ASSERT_THROWS(rhs1 /= rhs2, const std::runtime_error &);
  }

  void test_with_FixedLengthVector() {
    MultipliableTester2 lhs{0.1, 0.2};
    const MultipliableTester2 rhs{2, 4};
    lhs *= rhs;
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], 0.2, 1e-14);
    TS_ASSERT_DELTA(lhs[1], 0.8, 1e-14);
  }
};

#endif /* MANTID_HISTOGRAMDATA_MULTIPLIABLETEST_H_ */
