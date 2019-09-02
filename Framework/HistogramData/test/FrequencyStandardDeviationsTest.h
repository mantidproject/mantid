// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_HISTOGRAMDATA_FREQUENCYSTANDARDDEVIATIONSTEST_H_
#define MANTID_HISTOGRAMDATA_FREQUENCYSTANDARDDEVIATIONSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/CountStandardDeviations.h"
#include "MantidHistogramData/CountVariances.h"
#include "MantidHistogramData/FrequencyStandardDeviations.h"
#include "MantidHistogramData/FrequencyVariances.h"

using namespace Mantid;
using namespace HistogramData;

class FrequencyStandardDeviationsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FrequencyStandardDeviationsTest *createSuite() {
    return new FrequencyStandardDeviationsTest();
  }
  static void destroySuite(FrequencyStandardDeviationsTest *suite) {
    delete suite;
  }

  void test_has_correct_mixins() {
    FrequencyStandardDeviations data;
    TS_ASSERT_THROWS_NOTHING(UNUSED_ARG(
        (dynamic_cast<detail::StandardDeviationVectorOf<
             FrequencyStandardDeviations, HistogramE, FrequencyVariances> &>(
            data))));
  }

  void test_construct_default() {
    const FrequencyStandardDeviations points{};
    TS_ASSERT(!points);
  }

  void test_construct_from_null_CountStandardDeviations() {
    const CountStandardDeviations counts{};
    const BinEdges edges{};
    const FrequencyStandardDeviations frequencies(counts, edges);
    TS_ASSERT(!frequencies);
  }

  void test_construct_from_empty_CountStandardDeviations() {
    const CountStandardDeviations counts(0);
    const BinEdges edges{0};
    const FrequencyStandardDeviations frequencies(counts, edges);
    TS_ASSERT_EQUALS(frequencies.size(), 0);
  }

  void test_construct_from_empty_CountStandardDeviations_null_BinEdges() {
    const CountStandardDeviations counts(0);
    const BinEdges edges{};
    TS_ASSERT_THROWS(
        const FrequencyStandardDeviations frequencies(counts, edges),
        const std::logic_error &);
  }

  void test_construct_from_empty_CountStandardDeviations_size_mismatch() {
    const CountStandardDeviations counts(0);
    const BinEdges edges{1.0, 2.0};
    TS_ASSERT_THROWS(
        const FrequencyStandardDeviations frequencies(counts, edges),
        const std::logic_error &);
  }

  void test_construct_from_CountStandardDeviations_null_BinEdges() {
    const CountStandardDeviations counts(1);
    const BinEdges edges{};
    TS_ASSERT_THROWS(
        const FrequencyStandardDeviations frequencies(counts, edges),
        const std::logic_error &);
  }

  void test_construct_from_CountStandardDeviations_size_mismatch() {
    const CountStandardDeviations counts(2);
    const BinEdges edges{1.0, 2.0};
    TS_ASSERT_THROWS(
        const FrequencyStandardDeviations frequencies(counts, edges),
        const std::logic_error &);
  }

  void test_construct_from_CountStandardDeviations() {
    const CountStandardDeviations counts{1.0, 2.0};
    const BinEdges edges{0.1, 0.2, 0.4};
    const FrequencyStandardDeviations frequencies(counts, edges);
    TS_ASSERT_EQUALS(frequencies.size(), 2);
    TS_ASSERT_DELTA(frequencies[0], 10.0, 1e-14);
    TS_ASSERT_DELTA(frequencies[1], 10.0, 1e-14);
  }

  void test_move_construct_from_CountStandardDeviations() {
    CountStandardDeviations counts(1);
    const BinEdges edges{1.0, 2.0};
    auto old_ptr = &counts[0];
    const FrequencyStandardDeviations frequencies(std::move(counts), edges);
    TS_ASSERT(!counts);
    TS_ASSERT_EQUALS(&frequencies[0], old_ptr);
  }

  void test_move_construct_from_CountStandardDeviations_and_cow() {
    CountStandardDeviations counts(1);
    const CountStandardDeviations copy(counts);
    const BinEdges edges{1.0, 2.0};
    auto old_ptr = &counts[0];
    const FrequencyStandardDeviations frequencies(std::move(counts), edges);
    // Moved from counts...
    TS_ASSERT(!counts);
    // ... but made a copy of data, since "copy" also held a reference.
    TS_ASSERT_DIFFERS(&frequencies[0], old_ptr);
  }

  void test_construct_from_CountVariances() {
    const CountVariances counts{1.0, 2.0};
    const BinEdges edges{0.1, 0.2, 0.4};
    // This implicitly constructs CountStandardDeviations first.
    const FrequencyStandardDeviations frequencies(counts, edges);
    TS_ASSERT_EQUALS(frequencies.size(), 2);
    TS_ASSERT_DELTA(frequencies[0], 10.0, 1e-14);
    TS_ASSERT_DELTA(frequencies[1], M_SQRT2 * 5.0, 1e-14);
  }

  void test_move_construct_from_CountVariances() {
    CountVariances counts(1);
    const BinEdges edges{1.0, 2.0};
    auto old_ptr = &counts[0];
    // This implicitly constructs CountStandardDeviations first, so there is a
    // two-step move going on!
    const FrequencyStandardDeviations frequencies(std::move(counts), edges);
    TS_ASSERT(!counts);
    TS_ASSERT_EQUALS(&frequencies[0], old_ptr);
  }
};

#endif /* MANTID_HISTOGRAMDATA_FREQUENCYSTANDARDDEVIATIONSTEST_H_ */
