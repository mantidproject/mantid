#ifndef MANTID_HISTOGRAMDATA_SLICETEST_H_
#define MANTID_HISTOGRAMDATA_SLICETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/Slice.h"

using namespace Mantid::HistogramData;

class SliceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SliceTest *createSuite() { return new SliceTest(); }
  static void destroySuite(SliceTest *suite) { delete suite; }

  void test_out_of_range() {
    const Histogram histogram(BinEdges{1, 2, 3});
    TS_ASSERT_THROWS_EQUALS(
        slice(histogram, 1, 0), const std::out_of_range &e,
        std::string(e.what()),
        "Histogram slice: begin must not be greater than end");
    TS_ASSERT_THROWS_EQUALS(
        slice(histogram, 0, 3), const std::out_of_range &e,
        std::string(e.what()),
        "Histogram slice: end may not be larger than the histogram size");
  }

  void test_empty_slice() {
    const Histogram histogram(BinEdges{1, 2, 3});
    auto sliced = slice(histogram, 1, 1);
    TS_ASSERT_EQUALS(sliced.size(), 0);
    TS_ASSERT_EQUALS(sliced.x().size(), 0);
  }

  void test_empty_slice_point_data() {
    const Histogram histogram(Points{1, 2});
    auto sliced = slice(histogram, 1, 1);
    TS_ASSERT_EQUALS(sliced.size(), 0);
    TS_ASSERT_EQUALS(sliced.x().size(), 0);
  }

  void test_full_range_sharing_maintained() {
    const Histogram histogram(BinEdges{1, 2, 3}, Counts{4, 9});
    auto sliced = slice(histogram, 0, 2);
    TS_ASSERT_EQUALS(sliced.sharedX(), histogram.sharedX());
    TS_ASSERT_EQUALS(sliced.sharedY(), histogram.sharedY());
    TS_ASSERT_EQUALS(sliced.sharedE(), histogram.sharedE());
    TS_ASSERT(!sliced.sharedDx());
  }

  void test_slices_dx() {
    Histogram histogram(BinEdges{1, 2, 3}, Counts{4, 9});
    histogram.setPointStandardDeviations(2);
    auto sliced = slice(histogram, 0, 2);
    TS_ASSERT_EQUALS(sliced.dx(), histogram.dx());
  }

  void test_slice_single_bin_at_start() {
    const Histogram histogram(BinEdges{1, 2, 3, 4}, Counts{4, 9, 16});
    auto sliced = slice(histogram, 0, 1);
    TS_ASSERT_EQUALS(sliced.x(), HistogramX({1, 2}));
    TS_ASSERT_EQUALS(sliced.y(), HistogramY({4}));
    TS_ASSERT_EQUALS(sliced.e(), HistogramE({2}));
  }

  void test_slice_single_bin() {
    const Histogram histogram(BinEdges{1, 2, 3, 4}, Counts{4, 9, 16});
    auto sliced = slice(histogram, 1, 2);
    TS_ASSERT_EQUALS(sliced.x(), HistogramX({2, 3}));
    TS_ASSERT_EQUALS(sliced.y(), HistogramY({9}));
    TS_ASSERT_EQUALS(sliced.e(), HistogramE({3}));
  }

  void test_slice_single_bin_at_end() {
    const Histogram histogram(BinEdges{1, 2, 3, 4}, Counts{4, 9, 16});
    auto sliced = slice(histogram, 2, 3);
    TS_ASSERT_EQUALS(sliced.x(), HistogramX({3, 4}));
    TS_ASSERT_EQUALS(sliced.y(), HistogramY({16}));
    TS_ASSERT_EQUALS(sliced.e(), HistogramE({4}));
  }

  void test_points_slice_single_bin_at_start() {
    const Histogram histogram(Points{1, 2, 3}, Counts{4, 9, 16});
    auto sliced = slice(histogram, 0, 1);
    TS_ASSERT_EQUALS(sliced.x(), HistogramX({1}));
    TS_ASSERT_EQUALS(sliced.y(), HistogramY({4}));
    TS_ASSERT_EQUALS(sliced.e(), HistogramE({2}));
  }

  void test_points_slice_single_bin() {
    const Histogram histogram(Points{1, 2, 3}, Counts{4, 9, 16});
    auto sliced = slice(histogram, 1, 2);
    TS_ASSERT_EQUALS(sliced.x(), HistogramX({2}));
    TS_ASSERT_EQUALS(sliced.y(), HistogramY({9}));
    TS_ASSERT_EQUALS(sliced.e(), HistogramE({3}));
  }

  void test_points_slice_single_bin_at_end() {
    const Histogram histogram(Points{1, 2, 3}, Counts{4, 9, 16});
    auto sliced = slice(histogram, 2, 3);
    TS_ASSERT_EQUALS(sliced.x(), HistogramX({3}));
    TS_ASSERT_EQUALS(sliced.y(), HistogramY({16}));
    TS_ASSERT_EQUALS(sliced.e(), HistogramE({4}));
  }

  void test_slice_two_bins_at_start() {
    const Histogram histogram(BinEdges{1, 2, 3, 4, 5}, Counts{1, 4, 9, 16});
    auto sliced = slice(histogram, 0, 2);
    TS_ASSERT_EQUALS(sliced.x(), HistogramX({1, 2, 3}));
    TS_ASSERT_EQUALS(sliced.y(), HistogramY({1, 4}));
    TS_ASSERT_EQUALS(sliced.e(), HistogramE({1, 2}));
  }

  void test_slice_two_bins() {
    const Histogram histogram(BinEdges{1, 2, 3, 4, 5}, Counts{1, 4, 9, 16});
    auto sliced = slice(histogram, 1, 3);
    TS_ASSERT_EQUALS(sliced.x(), HistogramX({2, 3, 4}));
    TS_ASSERT_EQUALS(sliced.y(), HistogramY({4, 9}));
    TS_ASSERT_EQUALS(sliced.e(), HistogramE({2, 3}));
  }

  void test_slice_two_bins_at_end() {
    const Histogram histogram(BinEdges{1, 2, 3, 4, 5}, Counts{1, 4, 9, 16});
    auto sliced = slice(histogram, 2, 4);
    TS_ASSERT_EQUALS(sliced.x(), HistogramX({3, 4, 5}));
    TS_ASSERT_EQUALS(sliced.y(), HistogramY({9, 16}));
    TS_ASSERT_EQUALS(sliced.e(), HistogramE({3, 4}));
  }

  void test_points_slice_two_bins_at_start() {
    const Histogram histogram(Points{1, 2, 3, 4}, Counts{1, 4, 9, 16});
    auto sliced = slice(histogram, 0, 2);
    TS_ASSERT_EQUALS(sliced.x(), HistogramX({1, 2}));
    TS_ASSERT_EQUALS(sliced.y(), HistogramY({1, 4}));
    TS_ASSERT_EQUALS(sliced.e(), HistogramE({1, 2}));
  }

  void test_points_slice_two_bins() {
    const Histogram histogram(Points{1, 2, 3, 4}, Counts{1, 4, 9, 16});
    auto sliced = slice(histogram, 1, 3);
    TS_ASSERT_EQUALS(sliced.x(), HistogramX({2, 3}));
    TS_ASSERT_EQUALS(sliced.y(), HistogramY({4, 9}));
    TS_ASSERT_EQUALS(sliced.e(), HistogramE({2, 3}));
  }

  void test_points_slice_two_bins_at_end() {
    const Histogram histogram(Points{1, 2, 3, 4}, Counts{1, 4, 9, 16});
    auto sliced = slice(histogram, 2, 4);
    TS_ASSERT_EQUALS(sliced.x(), HistogramX({3, 4}));
    TS_ASSERT_EQUALS(sliced.y(), HistogramY({9, 16}));
    TS_ASSERT_EQUALS(sliced.e(), HistogramE({3, 4}));
  }
};

#endif /* MANTID_HISTOGRAMDATA_SLICETEST_H_ */
