// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_HISTOGRAMDATA_FREQUENCYVARIANCESTEST_H_
#define MANTID_HISTOGRAMDATA_FREQUENCYVARIANCESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/CountStandardDeviations.h"
#include "MantidHistogramData/CountVariances.h"
#include "MantidHistogramData/FrequencyStandardDeviations.h"
#include "MantidHistogramData/FrequencyVariances.h"

using namespace Mantid;
using namespace HistogramData;

class FrequencyVariancesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FrequencyVariancesTest *createSuite() {
    return new FrequencyVariancesTest();
  }
  static void destroySuite(FrequencyVariancesTest *suite) { delete suite; }

  void test_has_correct_mixins() {
    FrequencyVariances data;
    TS_ASSERT_THROWS_NOTHING(UNUSED_ARG(
        (dynamic_cast<detail::VarianceVectorOf<FrequencyVariances, HistogramE,
                                               FrequencyStandardDeviations> &>(
            data))));
  }

  void test_construct_default() {
    const FrequencyVariances frequencies{};
    TS_ASSERT(!frequencies);
  }

  void test_conversion_identity() {
    const FrequencyVariances variances{1.0, 4.0, 9.0};
    const FrequencyStandardDeviations sigmas(variances);
    const FrequencyVariances result(sigmas);
    TS_ASSERT_EQUALS(result[0], variances[0]);
    TS_ASSERT_EQUALS(result[1], variances[1]);
    TS_ASSERT_EQUALS(result[2], variances[2]);
  }

  void test_construct_from_null_CountVariances() {
    const CountVariances counts{};
    const BinEdges edges{};
    const FrequencyVariances frequencies(counts, edges);
    TS_ASSERT(!frequencies);
  }

  void test_construct_from_empty_CountVariances() {
    const CountVariances counts(0);
    const BinEdges edges{0};
    const FrequencyVariances frequencies(counts, edges);
    TS_ASSERT_EQUALS(frequencies.size(), 0);
  }

  void test_construct_from_empty_CountVariances_null_BinEdges() {
    const CountVariances counts(0);
    const BinEdges edges{};
    TS_ASSERT_THROWS(const FrequencyVariances frequencies(counts, edges),
                     const std::logic_error &);
  }

  void test_construct_from_empty_CountVariances_size_mismatch() {
    const CountVariances counts(0);
    const BinEdges edges{1.0, 2.0};
    TS_ASSERT_THROWS(const FrequencyVariances frequencies(counts, edges),
                     const std::logic_error &);
  }

  void test_construct_from_CountVariances_null_BinEdges() {
    const CountVariances counts(1);
    const BinEdges edges{};
    TS_ASSERT_THROWS(const FrequencyVariances frequencies(counts, edges),
                     const std::logic_error &);
  }

  void test_construct_from_CountVariances_size_mismatch() {
    const CountVariances counts(2);
    const BinEdges edges{1.0, 2.0};
    TS_ASSERT_THROWS(const FrequencyVariances frequencies(counts, edges),
                     const std::logic_error &);
  }

  void test_construct_from_CountVariances() {
    const CountVariances counts{1.0, 2.0};
    const BinEdges edges{0.1, 0.2, 0.4};
    const FrequencyVariances frequencies(counts, edges);
    TS_ASSERT_EQUALS(frequencies.size(), 2);
    TS_ASSERT_DELTA(frequencies[0], 100.0, 1e-14);
    TS_ASSERT_DELTA(frequencies[1], 50.0, 1e-14);
  }

  void test_move_construct_from_CountVariances() {
    CountVariances counts(1);
    const BinEdges edges{1.0, 2.0};
    auto old_ptr = &counts[0];
    const FrequencyVariances frequencies(std::move(counts), edges);
    TS_ASSERT(!counts);
    TS_ASSERT_EQUALS(&frequencies[0], old_ptr);
  }

  void test_move_construct_from_CountVariances_and_cow() {
    CountVariances counts(1);
    const CountVariances copy(counts);
    const BinEdges edges{1.0, 2.0};
    auto old_ptr = &counts[0];
    const FrequencyVariances frequencies(std::move(counts), edges);
    // Moved from counts...
    TS_ASSERT(!counts);
    // ... but made a copy of data, since "copy" also held a reference.
    TS_ASSERT_DIFFERS(&frequencies[0], old_ptr);
  }

  void test_construct_from_CountStandardDeviations() {
    const CountStandardDeviations counts{1.0, M_SQRT2};
    const BinEdges edges{0.1, 0.2, 0.4};
    // This implicitly constructs CountVariances first.
    const FrequencyVariances frequencies(counts, edges);
    TS_ASSERT_EQUALS(frequencies.size(), 2);
    TS_ASSERT_DELTA(frequencies[0], 100.0, 1e-14);
    TS_ASSERT_DELTA(frequencies[1], 50.0, 1e-14);
  }

  void test_move_construct_from_CountStandardDeviations() {
    CountStandardDeviations counts(1);
    const BinEdges edges{1.0, 2.0};
    auto old_ptr = &counts[0];
    // This implicitly constructs CountVariances first, so there is a
    // two-step move going on!
    const FrequencyVariances frequencies(std::move(counts), edges);
    TS_ASSERT(!counts);
    TS_ASSERT_EQUALS(&frequencies[0], old_ptr);
  }
};

#endif /* MANTID_HISTOGRAMDATA_FREQUENCYVARIANCESTEST_H_ */
