// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_HISTOGRAMDATA_FREQUENCIESTEST_H_
#define MANTID_HISTOGRAMDATA_FREQUENCIESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/Counts.h"
#include "MantidHistogramData/Frequencies.h"

using namespace Mantid;
using namespace HistogramData;

class FrequenciesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FrequenciesTest *createSuite() { return new FrequenciesTest(); }
  static void destroySuite(FrequenciesTest *suite) { delete suite; }

  void test_has_correct_mixins() {
    Frequencies data;
    TS_ASSERT_THROWS_NOTHING(UNUSED_ARG(
        (dynamic_cast<detail::VectorOf<Frequencies, HistogramY> &>(data))));
    TS_ASSERT_THROWS_NOTHING(
        UNUSED_ARG(dynamic_cast<detail::Addable<Frequencies> &>(data)));
    TS_ASSERT_THROWS_NOTHING(
        UNUSED_ARG(dynamic_cast<detail::Iterable<Frequencies> &>(data)));
    TS_ASSERT_THROWS_NOTHING(
        UNUSED_ARG(dynamic_cast<detail::Offsetable<Frequencies> &>(data)));
    TS_ASSERT_THROWS_NOTHING(
        UNUSED_ARG(dynamic_cast<detail::Scalable<Frequencies> &>(data)));
  }

  void test_construct_default() {
    const Counts counts{};
    TS_ASSERT(!counts);
  }

  void test_construct_from_null_Counts() {
    const Counts counts{};
    const BinEdges edges{};
    const Frequencies frequencies(counts, edges);
    TS_ASSERT(!counts);
  }

  void test_construct_from_empty_Counts() {
    const Counts counts(0);
    const BinEdges edges{0};
    const Frequencies frequencies(counts, edges);
    TS_ASSERT_EQUALS(counts.size(), 0);
  }

  void test_construct_from_empty_Counts_null_BinEdges() {
    const Counts counts(0);
    const BinEdges edges{};
    TS_ASSERT_THROWS(const Frequencies frequencies(counts, edges),
                     const std::logic_error &);
  }

  void test_construct_from_empty_Counts_size_mismatch() {
    const Counts counts(0);
    const BinEdges edges{1.0, 2.0};
    TS_ASSERT_THROWS(const Frequencies frequencies(counts, edges),
                     const std::logic_error &);
  }

  void test_construct_from_Counts_null_BinEdges() {
    const Counts counts(1);
    const BinEdges edges{};
    TS_ASSERT_THROWS(const Frequencies frequencies(counts, edges),
                     const std::logic_error &);
  }

  void test_construct_from_Counts_size_mismatch() {
    const Counts counts(2);
    const BinEdges edges{1.0, 2.0};
    TS_ASSERT_THROWS(const Frequencies frequencies(counts, edges),
                     const std::logic_error &);
  }

  void test_construct_from_Counts() {
    const Counts counts{1.0, 2.0};
    const BinEdges edges{0.1, 0.2, 0.4};
    const Frequencies frequencies(counts, edges);
    TS_ASSERT_EQUALS(frequencies.size(), 2);
    TS_ASSERT_DELTA(frequencies[0], 10.0, 1e-14);
    TS_ASSERT_DELTA(frequencies[1], 10.0, 1e-14);
  }

  void test_move_construct_from_Counts() {
    Counts counts(1);
    const BinEdges edges{1.0, 2.0};
    auto old_ptr = &counts[0];
    const Frequencies frequencies(std::move(counts), edges);
    TS_ASSERT(!counts);
    TS_ASSERT_EQUALS(&frequencies[0], old_ptr);
  }

  void test_move_construct_from_Counts_and_cow() {
    Counts counts(1);
    const Counts copy(counts);
    const BinEdges edges{1.0, 2.0};
    auto old_ptr = &counts[0];
    const Frequencies frequencies(std::move(counts), edges);
    // Moved from counts...
    TS_ASSERT(!counts);
    // ... but made a copy of data, since "copy" also held a reference.
    TS_ASSERT_DIFFERS(&frequencies[0], old_ptr);
  }
};

#endif /* MANTID_HISTOGRAMDATA_FREQUENCIESTEST_H_ */
