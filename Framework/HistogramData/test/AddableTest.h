// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_HISTOGRAMDATA_ADDABLETEST_H_
#define MANTID_HISTOGRAMDATA_ADDABLETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/Addable.h"
#include "MantidHistogramData/FixedLengthVector.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/Iterable.h"
#include "MantidHistogramData/VectorOf.h"

using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::detail::Addable;
using Mantid::HistogramData::detail::FixedLengthVector;
using Mantid::HistogramData::detail::Iterable;
using Mantid::HistogramData::detail::VectorOf;

class AddableTester : public VectorOf<AddableTester, HistogramX>,
                      public Iterable<AddableTester>,
                      public Addable<AddableTester> {
  using VectorOf<AddableTester, HistogramX>::VectorOf;
};

// Does Addable also work with FixedLengthVector instead of VectorOf?
struct AddableTester2 : public FixedLengthVector<AddableTester2>,
                        public Addable<AddableTester2> {
  using FixedLengthVector<AddableTester2>::FixedLengthVector;
};

class AddableTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AddableTest *createSuite() { return new AddableTest(); }
  static void destroySuite(AddableTest *suite) { delete suite; }

  void test_plus_equals() {
    AddableTester lhs{0.1, 0.2};
    const AddableTester rhs{0.01, 0.02};
    lhs += rhs;
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], 0.11, 1e-14);
    TS_ASSERT_DELTA(lhs[1], 0.22, 1e-14);
  }

  void test_minus_equals() {
    AddableTester lhs{0.1, 0.2};
    const AddableTester rhs{0.01, 0.02};
    lhs -= rhs;
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], 0.09, 1e-14);
    TS_ASSERT_DELTA(lhs[1], 0.18, 1e-14);
  }

  void test_plus() {
    const AddableTester rhs1{0.1, 0.2};
    const AddableTester rhs2{0.01, 0.02};
    const AddableTester lhs(rhs1 + rhs2);
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], 0.11, 1e-14);
    TS_ASSERT_DELTA(lhs[1], 0.22, 1e-14);
  }

  void test_minus() {
    const AddableTester rhs1{0.1, 0.2};
    const AddableTester rhs2{0.01, 0.02};
    const AddableTester lhs(rhs1 - rhs2);
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], 0.09, 1e-14);
    TS_ASSERT_DELTA(lhs[1], 0.18, 1e-14);
  }

  void test_length_mismatch() {
    AddableTester rhs1{1, 2};
    const AddableTester rhs2{1, 2, 3};
    TS_ASSERT_THROWS(rhs1 + rhs2, const std::runtime_error &);
    TS_ASSERT_THROWS(rhs1 - rhs2, const std::runtime_error &);
    TS_ASSERT_THROWS(rhs1 += rhs2, const std::runtime_error &);
    TS_ASSERT_THROWS(rhs1 -= rhs2, const std::runtime_error &);
  }

  void test_with_FixedLengthVector() {
    AddableTester2 lhs{0.1, 0.2};
    const AddableTester2 rhs{0.01, 0.02};
    lhs += rhs;
    TS_ASSERT_EQUALS(lhs.size(), 2);
    TS_ASSERT_DELTA(lhs[0], 0.11, 1e-14);
    TS_ASSERT_DELTA(lhs[1], 0.22, 1e-14);
  }
};

#endif /* MANTID_HISTOGRAMDATA_ADDABLETEST_H_ */
