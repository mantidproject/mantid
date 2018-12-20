#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMITERATORTEST_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMITERATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramItem.h"
#include "MantidHistogramData/HistogramIterator.h"
#include "MantidHistogramData/LinearGenerator.h"

using namespace Mantid::HistogramData;

class HistogramIteratorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramIteratorTest *createSuite() {
    return new HistogramIteratorTest();
  }
  static void destroySuite(HistogramIteratorTest *suite) { delete suite; }

  void test_construction() {
    Histogram hist(Histogram::XMode::BinEdges, Histogram::YMode::Counts);
    TS_ASSERT_THROWS_NOTHING(HistogramIterator iter(hist, 0));
  }

  void test_iterator_begin() {
    Histogram hist(Points{1.1, 1.2, 1.4}, Frequencies{2, 3, 4});
    auto iter = hist.begin();
    TS_ASSERT(iter != hist.end());
    TS_ASSERT_EQUALS(iter->frequency(), 2);
  }

  void test_iterator_end() {
    Histogram hist(Points{1.1, 1.2, 1.4}, Frequencies{2, 3, 4});
    auto iter = hist.end();
    TS_ASSERT(iter != hist.begin());
  }

  void test_iterator_increment() {
    Histogram hist(Points{1.1, 1.2, 1.4}, Frequencies{2, 3, 4});
    auto iter = hist.begin();
    TS_ASSERT(iter != hist.end());
    TS_ASSERT_EQUALS(iter->frequency(), 2);
    ++iter;
    TS_ASSERT(iter != hist.end());
    TS_ASSERT_EQUALS(iter->frequency(), 3);
    ++iter;
    TS_ASSERT(iter != hist.end());
    TS_ASSERT_EQUALS(iter->frequency(), 4);
    ++iter;
    TS_ASSERT(iter == hist.end());
  }

  void test_iterator_decrement() {
    Histogram hist(Points{1.1, 1.2, 1.4}, Frequencies{2, 3, 4});
    auto iter = hist.end();
    --iter;
    TS_ASSERT_DIFFERS(iter, hist.begin());
    TS_ASSERT_EQUALS(iter->frequency(), 4);
    --iter;
    TS_ASSERT_DIFFERS(iter, hist.begin());
    TS_ASSERT_EQUALS(iter->frequency(), 3);
    --iter;
    TS_ASSERT_EQUALS(iter, hist.begin());
    TS_ASSERT_EQUALS(iter->frequency(), 2);
  }

  void test_iterator_advance() {
    Histogram hist(Points{1.1, 1.2, 1.4}, Frequencies{2, 3, 4});
    auto iter = hist.begin();

    std::advance(iter, 2);
    TS_ASSERT_EQUALS(iter->frequency(), 4);
    // check past end of valid range
    std::advance(iter, 2);
    TS_ASSERT_EQUALS(iter, hist.end());
    std::advance(iter, -3);
    TS_ASSERT_EQUALS(iter->frequency(), 2);
    // check before start of valid range
    std::advance(iter, -2);
    TS_ASSERT_EQUALS(iter, hist.begin());
  }

  void test_iterator_distance() {
    Histogram hist(Points{1.1, 1.2, 1.4}, Frequencies{2, 3, 4});
    auto begin = hist.begin();
    auto end = hist.end();

    TS_ASSERT_DIFFERS(begin, end);
    TS_ASSERT_EQUALS(std::distance(begin, end), 3);
    ++begin;
    TS_ASSERT_DIFFERS(begin, end);
    TS_ASSERT_EQUALS(std::distance(begin, end), 2);
    --begin;
    std::advance(begin, std::distance(begin, end) / 2);
    TS_ASSERT_DIFFERS(begin, end);
    TS_ASSERT_EQUALS(std::distance(begin, end), 2);
  }

  void test_iterate_over_histogram_counts() {
    Counts expectedCounts{2, 3, 4};
    Histogram hist(Points{1.1, 1.2, 1.4}, expectedCounts);

    TS_ASSERT(std::equal(hist.begin(), hist.end(), expectedCounts.begin(),
                         [](const HistogramItem &item, const double &counts) {
                           return item.counts() == counts;
                         }));
  }

  void test_iterate_over_histogram_counts_when_histogram_has_frequencies() {
    Histogram hist(BinEdges{1.0, 1.1, 1.2, 1.5}, Frequencies{2, 3, 4});
    Counts expectedCounts = hist.counts();

    TS_ASSERT(std::equal(hist.begin(), hist.end(), expectedCounts.begin(),
                         [](const HistogramItem &item, const double &counts) {
                           return item.counts() == counts;
                         }));
  }

  void test_iterate_over_histogram_frequencies() {
    Frequencies expectedFrequencies{2, 3, 4};
    Histogram hist(Points{1.1, 1.2, 1.4}, expectedFrequencies);

    TS_ASSERT(
        std::equal(hist.begin(), hist.end(), expectedFrequencies.begin(),
                   [](const HistogramItem &item, const double &frequency) {
                     return item.frequency() == frequency;
                   }));
  }

  void test_iterate_over_histogram_frequencies_when_histogram_has_counts() {
    Histogram hist(BinEdges{1.1, 1.2, 1.3, 1.5}, Counts{2, 3, 4});
    Frequencies expectedFrequencies = hist.frequencies();

    TS_ASSERT(
        std::equal(hist.begin(), hist.end(), expectedFrequencies.begin(),
                   [](const HistogramItem &item, const double &frequency) {
                     return item.frequency() == frequency;
                   }));
  }

  void test_iterate_over_histogram_center_when_histogram_has_bins() {
    Histogram hist(BinEdges{1.1, 1.2, 1.3, 1.4}, Counts{2, 3, 4});
    Points expectedPoints = hist.points();

    TS_ASSERT(std::equal(hist.begin(), hist.end(), expectedPoints.begin(),
                         [](const HistogramItem &item, const double &point) {
                           return item.center() == point;
                         }));
  }

  void test_iterate_over_histogram_center_when_histogram_has_points() {
    Histogram hist(Points{1.1, 1.2, 1.4}, Counts{2, 3, 4});
    Points expectedPoints = hist.points();

    TS_ASSERT(std::equal(hist.begin(), hist.end(), expectedPoints.begin(),
                         [](const HistogramItem &item, const double &point) {
                           return item.center() == point;
                         }));
  }

  void test_iterate_over_histogram_width_when_histogram_has_bins() {
    Histogram hist(BinEdges{1, 2, 3, 5}, Counts{2, 3, 4});
    std::vector<double> expectedWidths = {1, 1, 2};

    TS_ASSERT(std::equal(hist.begin(), hist.end(), expectedWidths.begin(),
                         [](const HistogramItem &item, const double &width) {
                           return item.binWidth() == width;
                         }));
  }

  void test_iterate_over_histogram_width_when_histogram_has_points() {
    Histogram hist(Points{1, 3, 5}, Counts{2, 3, 4});
    std::vector<double> expectedWidths = {2, 2, 2};

    TS_ASSERT(std::equal(hist.begin(), hist.end(), expectedWidths.begin(),
                         [](const HistogramItem &item, const double &width) {
                           return item.binWidth() == width;
                         }));
  }

  void test_iterate_over_histogram_count_variances_when_histogram_has_counts() {
    Histogram hist(BinEdges{1, 2, 3, 5}, Counts{2, 3, 4});
    auto expectedCountVariances = hist.countVariances();

    TS_ASSERT(std::equal(hist.begin(), hist.end(),
                         expectedCountVariances.begin(),
                         [](const HistogramItem &item, const double &variance) {
                           return item.countVariance() == variance;
                         }));
  }

  void
  test_iterate_over_histogram_count_variances_when_histogram_has_frequencies() {
    Histogram hist(BinEdges{1, 2, 3, 5}, Frequencies{2, 3, 4});
    auto expectedCountVariances = hist.countVariances();

    TS_ASSERT(std::equal(hist.begin(), hist.end(),
                         expectedCountVariances.begin(),
                         [](const HistogramItem &item, const double &variance) {
                           return item.countVariance() == variance;
                         }));
  }

  void test_iterate_over_histogram_count_std_when_histogram_has_counts() {
    Histogram hist(BinEdges{1, 2, 3, 5}, Counts{2, 3, 4});
    auto expectedCountStd = hist.countStandardDeviations();

    TS_ASSERT(std::equal(hist.begin(), hist.end(), expectedCountStd.begin(),
                         [](const HistogramItem &item, const double &sigma) {
                           return item.countStandardDeviation() == sigma;
                         }));
  }

  void test_iterate_over_histogram_count_std_when_histogram_has_frequencies() {
    Histogram hist(BinEdges{1, 2, 3, 5}, Frequencies{2, 3, 4});
    auto expectedCountStd = hist.countStandardDeviations();

    TS_ASSERT(std::equal(hist.begin(), hist.end(), expectedCountStd.begin(),
                         [](const HistogramItem &item, const double &sigma) {
                           return item.countStandardDeviation() == sigma;
                         }));
  }

  void
  test_iterate_over_histogram_frequency_variances_when_histogram_has_frequencys() {
    Histogram hist(BinEdges{1, 2, 3, 5}, Counts{2, 3, 4});
    auto expectedFrequencyVariances = hist.frequencyVariances();

    TS_ASSERT(std::equal(hist.begin(), hist.end(),
                         expectedFrequencyVariances.begin(),
                         [](const HistogramItem &item, const double &variance) {
                           return item.frequencyVariance() == variance;
                         }));
  }

  void
  test_iterate_over_histogram_frequency_variances_when_histogram_has_frequencies() {
    Histogram hist(BinEdges{1, 2, 3, 5}, Frequencies{2, 3, 4});
    auto expectedFrequencyVariances = hist.frequencyVariances();

    TS_ASSERT(std::equal(hist.begin(), hist.end(),
                         expectedFrequencyVariances.begin(),
                         [](const HistogramItem &item, const double &variance) {
                           return item.frequencyVariance() == variance;
                         }));
  }

  void
  test_iterate_over_histogram_frequency_std_when_histogram_has_frequencys() {
    Histogram hist(BinEdges{1, 2, 3, 5}, Counts{2, 3, 4});
    auto expectedFrequencyStd = hist.frequencyStandardDeviations();

    TS_ASSERT(std::equal(hist.begin(), hist.end(), expectedFrequencyStd.begin(),
                         [](const HistogramItem &item, const double &sigma) {
                           return item.frequencyStandardDeviation() == sigma;
                         }));
  }

  void
  test_iterate_over_histogram_frequency_std_when_histogram_has_frequencies() {
    Histogram hist(BinEdges{1, 2, 3, 5}, Frequencies{2, 3, 4});
    auto expectedFrequencyStd = hist.frequencyStandardDeviations();

    TS_ASSERT(std::equal(hist.begin(), hist.end(), expectedFrequencyStd.begin(),
                         [](const HistogramItem &item, const double &sigma) {
                           return item.frequencyStandardDeviation() == sigma;
                         }));
  }
};

class HistogramIteratorTestPerformance : public CxxTest::TestSuite {
public:
  static HistogramIteratorTestPerformance *createSuite() {
    return new HistogramIteratorTestPerformance;
  }
  static void destroySuite(HistogramIteratorTestPerformance *suite) {
    delete suite;
  }

  HistogramIteratorTestPerformance()
      : m_hist(BinEdges(histSize, LinearGenerator(0, 1)),
               Counts(histSize - 1, LinearGenerator(0, 1))) {}

  void test_convert_counts_to_frequency_for_each_item() {
    double total = 0;
    for (size_t i = 0; i < nHists; i++)
      for (auto &item : m_hist)
        total += item.frequency();
  }

  void test_convert_counts_to_frequency_once_per_histogram() {
    double total = 0;
    for (size_t i = 0; i < nHists; i++) {
      const auto &frequencies = m_hist.frequencies();
      for (auto &frequency : frequencies)
        total += frequency;
    }
  }

  void test_convert_counts_to_frequency_for_each_item_sparse() {
    double total = 0;
    double floor = static_cast<double>(histSize) - 5.0;
    for (size_t i = 0; i < nHists; i++) {
      for (auto &item : m_hist) {
        if (item.counts() > floor)
          total += item.frequency();
      }
    }
  }

  void test_convert_counts_to_frequency_once_per_histogram_sparse() {
    double total = 0;
    double floor = static_cast<double>(histSize) - 5.0;
    for (size_t i = 0; i < nHists; i++) {
      const auto &counts = m_hist.counts();
      const auto &frequencies = m_hist.frequencies();
      for (size_t j = 0; j < histSize; ++j)
        if (counts[j] > floor)
          total += frequencies[j];
    }
  }

private:
  const size_t nHists = 1000;
  const size_t histSize = 1000000;
  Histogram m_hist;
};

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMITERATORTEST_H_ */
