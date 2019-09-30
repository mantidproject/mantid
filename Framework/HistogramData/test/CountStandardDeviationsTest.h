// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_HISTOGRAMDATA_COUNTSTANDARDDEVIATIONSTEST_H_
#define MANTID_HISTOGRAMDATA_COUNTSTANDARDDEVIATIONSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/CountStandardDeviations.h"
#include "MantidHistogramData/CountVariances.h"
#include "MantidHistogramData/FrequencyStandardDeviations.h"
#include "MantidHistogramData/FrequencyVariances.h"

using namespace Mantid;
using namespace HistogramData;

class CountStandardDeviationsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CountStandardDeviationsTest *createSuite() {
    return new CountStandardDeviationsTest();
  }
  static void destroySuite(CountStandardDeviationsTest *suite) { delete suite; }

  void test_has_correct_mixins() {
    CountStandardDeviations data;
    TS_ASSERT_THROWS_NOTHING(UNUSED_ARG(
        (dynamic_cast<detail::StandardDeviationVectorOf<
             CountStandardDeviations, HistogramE, CountVariances> &>(data))));
  }

  void test_construct_default() {
    const CountStandardDeviations counts{};
    TS_ASSERT(!counts);
  }

  void test_construct_from_null_FrequencyStandardDeviations() {
    const FrequencyStandardDeviations frequencies{};
    const BinEdges edges{};
    const CountStandardDeviations counts(frequencies, edges);
    TS_ASSERT(!counts);
  }

  void test_construct_from_empty_FrequencyStandardDeviations() {
    const FrequencyStandardDeviations frequencies(0);
    const BinEdges edges{0};
    const CountStandardDeviations counts(frequencies, edges);
    TS_ASSERT_EQUALS(counts.size(), 0);
  }

  void test_construct_from_empty_FrequencyStandardDeviations_null_BinEdges() {
    const FrequencyStandardDeviations frequencies(0);
    const BinEdges edges{};
    TS_ASSERT_THROWS(const CountStandardDeviations counts(frequencies, edges),
                     const std::logic_error &);
  }

  void test_construct_from_empty_FrequencyStandardDeviations_size_mismatch() {
    const FrequencyStandardDeviations frequencies(0);
    const BinEdges edges{1.0, 2.0};
    TS_ASSERT_THROWS(const CountStandardDeviations counts(frequencies, edges),
                     const std::logic_error &);
  }

  void test_construct_from_FrequencyStandardDeviations_null_BinEdges() {
    const FrequencyStandardDeviations frequencies(1);
    const BinEdges edges{};
    TS_ASSERT_THROWS(const CountStandardDeviations counts(frequencies, edges),
                     const std::logic_error &);
  }

  void test_construct_from_FrequencyStandardDeviations_size_mismatch() {
    const FrequencyStandardDeviations frequencies(2);
    const BinEdges edges{1.0, 2.0};
    TS_ASSERT_THROWS(const CountStandardDeviations counts(frequencies, edges),
                     const std::logic_error &);
  }

  void test_construct_from_FrequencyStandardDeviations() {
    const FrequencyStandardDeviations frequencies{1.0, 2.0};
    const BinEdges edges{0.1, 0.2, 0.4};
    const CountStandardDeviations counts(frequencies, edges);
    TS_ASSERT_EQUALS(counts.size(), 2);
    TS_ASSERT_DELTA(counts[0], 0.1, 1e-14);
    TS_ASSERT_DELTA(counts[1], 0.4, 1e-14);
  }

  void test_move_construct_from_FrequencyStandardDeviations() {
    FrequencyStandardDeviations frequencies(1);
    const BinEdges edges{1.0, 2.0};
    auto old_ptr = &frequencies[0];
    const CountStandardDeviations counts(std::move(frequencies), edges);
    TS_ASSERT(!frequencies);
    TS_ASSERT_EQUALS(&counts[0], old_ptr);
  }

  void test_move_construct_from_FrequencyStandardDeviations_and_cow() {
    FrequencyStandardDeviations frequencies(1);
    const FrequencyStandardDeviations copy(frequencies);
    const BinEdges edges{1.0, 2.0};
    auto old_ptr = &frequencies[0];
    const CountStandardDeviations counts(std::move(frequencies), edges);
    // Moved from frequencies...
    TS_ASSERT(!frequencies);
    // ... but made a copy of data, since "copy" also held a reference.
    TS_ASSERT_DIFFERS(&counts[0], old_ptr);
  }

  void test_construct_from_FrequencyVariances() {
    const FrequencyVariances frequencies{1.0, 2.0};
    const BinEdges edges{0.1, 0.2, 0.4};
    // This implicitly constructs FrequencyStandardDeviations first.
    const CountStandardDeviations counts(frequencies, edges);
    TS_ASSERT_EQUALS(counts.size(), 2);
    TS_ASSERT_DELTA(counts[0], 0.1, 1e-14);
    TS_ASSERT_DELTA(counts[1], M_SQRT2 * 0.2, 1e-14);
  }

  void test_move_construct_from_FrequencyVariances() {
    FrequencyVariances frequencies(1);
    const BinEdges edges{1.0, 2.0};
    auto old_ptr = &frequencies[0];
    // This implicitly constructs FrequencyStandardDeviations first, so there is
    // a two-step move going on!
    const CountStandardDeviations counts(std::move(frequencies), edges);
    TS_ASSERT(!frequencies);
    TS_ASSERT_EQUALS(&counts[0], old_ptr);
  }
};

#endif /* MANTID_HISTOGRAMDATA_COUNTSTANDARDDEVIATIONSTEST_H_ */
