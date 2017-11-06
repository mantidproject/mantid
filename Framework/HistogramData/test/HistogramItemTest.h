#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMITEMTEST_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMITEMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramItem.h"

using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::HistogramItem;
using Mantid::HistogramData::Points;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::Frequencies;
using Mantid::HistogramData::BinEdges;

class HistogramItemTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramItemTest *createSuite() { return new HistogramItemTest(); }
  static void destroySuite(HistogramItemTest *suite) { delete suite; }

  static constexpr double tolerance = 1e-6;

  void test_construction() {
    Histogram hist(Histogram::XMode::BinEdges, Histogram::YMode::Counts);
    TS_ASSERT_THROWS_NOTHING(HistogramItem item(hist, 0));
  }

  void test_get_counts_from_histogram_with_counts() {
    Histogram hist(Points{0.1, 0.2, 0.4}, Counts{1, 2, 4});
    HistogramItem item(hist, 1);
    TS_ASSERT_EQUALS(item.counts(), 2.0)
  }

  void test_get_counts_from_histogram_with_frequencies() {
    Histogram hist(Points{0.1, 0.2, 0.4}, Frequencies{1, 2, 4});
    HistogramItem item(hist, 1);
    TS_ASSERT_DELTA(item.counts(), 0.3, tolerance)
  }

  void test_get_countVariance_from_histogram_with_counts() {
    Histogram hist(Points{0.1, 0.2, 0.4}, Counts{1, 2, 4});
    HistogramItem item(hist, 1);
    TS_ASSERT_DELTA(item.countVariance(), 2.0, tolerance)
  }

  void test_get_countVariance_from_histogram_with_frequencies() {
    Histogram hist(Points{0.1, 0.2, 0.4}, Frequencies{1, 2, 4});
    HistogramItem item(hist, 1);
    TS_ASSERT_DELTA(item.countVariance(), 0.045, 1e-12)
  }

  void test_get_countStandardDeviation_from_histogram_with_counts() {
    Histogram hist(Points{0.1, 0.2, 0.4}, Counts{1, 2, 4});
    HistogramItem item(hist, 1);
    TS_ASSERT_DELTA(item.countStandardDeviation(), std::sqrt(2.0), tolerance)
  }

  void test_get_countStandardDeviation_from_histogram_with_frequencies() {
    Histogram hist(Points{0.1, 0.2, 0.4}, Frequencies{1, 2, 4});
    HistogramItem item(hist, 1);
    TS_ASSERT_DELTA(item.countStandardDeviation(), std::sqrt(0.045), 1e-12)
  }

  void test_get_frequency_from_histogram_with_counts() {
    Histogram hist(Points{0.1, 0.2, 0.4}, Counts{1, 2, 4});
    HistogramItem item(hist, 1);
    TS_ASSERT_DELTA(item.frequency(), 13.3333333517, tolerance)
  }

  void test_get_frequency_from_histogram_with_frequencies() {
    Histogram hist(Points{0.1, 0.2, 0.4}, Frequencies{1, 2, 4});
    HistogramItem item(hist, 1);
    TS_ASSERT_DELTA(item.frequency(), 2.0, 1e-12)
  }

  void test_get_frequencyVariance_from_histogram_with_counts() {
    Histogram hist(Points{0.1, 0.2, 0.4}, Counts{1, 2, 4});
    HistogramItem item(hist, 1);
    TS_ASSERT_DELTA(item.frequencyVariance(), 88.8888888, tolerance)
  }

  void test_get_frequencyVariance_from_histogram_with_frequencies() {
    Histogram hist(Points{0.1, 0.2, 0.4}, Frequencies{1, 2, 4});
    HistogramItem item(hist, 1);
    TS_ASSERT_DELTA(item.frequencyVariance(), 2.0, 1e-12)
  }

  void test_get_frequencyStandardDeviation_from_histogram_with_counts() {
    Histogram hist(Points{0.1, 0.2, 0.4}, Counts{1, 2, 4});
    HistogramItem item(hist, 1);
    TS_ASSERT_DELTA(item.frequencyStandardDeviation(), std::sqrt(88.8888888), tolerance)
  }

  void test_get_frequencyStandardDeviation_from_histogram_with_frequencies() {
    Histogram hist(Points{0.1, 0.2, 0.4}, Frequencies{1, 2, 4});
    HistogramItem item(hist, 1);
    TS_ASSERT_DELTA(item.frequencyStandardDeviation(), std::sqrt(2.0), 1e-12)
  }

  void test_get_center_from_histogram_with_bins() {
    Histogram hist(BinEdges{0.1, 0.2, 0.4, 0.5}, Frequencies{1, 2, 4});
    HistogramItem item(hist, 1);
    TS_ASSERT_DELTA(item.center(), 0.3, tolerance)
  }

  void test_get_center_from_histogram_with_points() {
    Histogram hist(Points{0.1, 0.2, 0.4}, Frequencies{1, 2, 4});
    HistogramItem item(hist, 1);
    TS_ASSERT_EQUALS(item.center(), 0.2)
  }

  void test_get_width_from_histogram_with_bins() {
    Histogram hist(BinEdges{0.1, 0.2, 0.4, 0.5}, Frequencies{1, 2, 4});
    HistogramItem item(hist, 1);
    TS_ASSERT_EQUALS(item.width(), 0.2)
  }

  void test_get_width_from_histogram_with_points() {
    Histogram hist(Points{0.1, 0.2, 0.4}, Frequencies{1, 2, 4});
    HistogramItem item(hist, 1);
    TS_ASSERT_DELTA(item.width(), 0.15, tolerance)
  }

  void test_get_binEdges_from_histogram_with_bins() {
    Histogram hist(BinEdges{0.1, 0.2, 0.4, 0.5}, Frequencies{1, 2, 4});
    HistogramItem item(hist, 1);
    compare(item.binEdges(), BinEdges{0.2, 0.4});
  }

  void test_get_binEdges_from_histogram_with_points() {
    Histogram hist(Points{0.1, 0.2, 0.4}, Frequencies{1, 2, 4});
    HistogramItem item(hist, 1);
    compare(item.binEdges(), BinEdges{0.15, 0.3});
  }

  void test_get_point_from_histogram_with_bins() {
    Histogram hist(BinEdges{0.1, 0.2, 0.4, 0.5}, Frequencies{1, 2, 4});
    HistogramItem item(hist, 1);
    compare(item.point(), Points{0.3});
  }

  void test_get_point_from_histogram_with_points() {
    Histogram hist(Points{0.1, 0.2, 0.4}, Frequencies{1, 2, 4});
    HistogramItem item(hist, 1);
    compare(item.point(), Points{0.2});
  }

  template<typename T>
  void compare(const T& lhs, const T& rhs) {
      TS_ASSERT_EQUALS(lhs.size(), rhs.size())

      for (size_t i = 0; i <  lhs.size(); ++i) {
        TS_ASSERT_DELTA(lhs[i], rhs[i], tolerance)
      }

  }
};

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMITEMTEST_H_ */

