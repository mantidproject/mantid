#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMITERATORTEST_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMITERATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramItem.h"
#include "MantidHistogramData/HistogramIterator.h"
#include "MantidHistogramData/LinearGenerator.h"

#include <iostream>
using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::HistogramItem;
using Mantid::HistogramData::HistogramIterator;
using Mantid::HistogramData::Points;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::Frequencies;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::CountVariances;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::FrequencyVariances;
using Mantid::HistogramData::FrequencyStandardDeviations;
using Mantid::HistogramData::LinearGenerator;

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
    TS_ASSERT_THROWS_NOTHING(HistogramIterator iter(hist));
  }

  void test_iterator_begin() {
    Histogram hist(Points{1, 2, 3}, Frequencies{2, 3, 4});
    auto iter = hist.begin();
    TS_ASSERT(iter != hist.end());
    TS_ASSERT_EQUALS(iter->frequency(), 2);
  }

  void test_iterator_end() {
    Histogram hist(Points{1, 2, 3}, Frequencies{2, 3, 4});
    auto iter = hist.end();
    TS_ASSERT(iter != hist.begin());
  }

  void test_iterator_increment() {
    Histogram hist(Points{1, 2, 3}, Frequencies{2, 3, 4});
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
    Histogram hist(Points{1, 2, 3}, Frequencies{2, 3, 4});
    auto iter = hist.end();
    --iter;
    TS_ASSERT_DIFFERS(iter, hist.begin());
    TS_ASSERT_EQUALS(iter->frequency(), 4);
    --iter;
    TS_ASSERT_DIFFERS(iter, hist.begin());
    TS_ASSERT_EQUALS(iter->frequency(), 3);
    --iter;
    TS_ASSERT_EQUALS(iter,  hist.begin());
    TS_ASSERT_EQUALS(iter->frequency(), 2);
  }

  void test_iterator_advance() {
    Histogram hist(Points{1, 2, 3}, Frequencies{2, 3, 4});
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
    Histogram hist(Points{1, 2, 3}, Frequencies{2, 3, 4});
    auto begin = hist.begin();
    auto end = hist.end();

    TS_ASSERT_DIFFERS(begin, end);
    TS_ASSERT_EQUALS(std::distance(begin, end), 3);
    ++begin;
    TS_ASSERT_DIFFERS(begin, end);
    TS_ASSERT_EQUALS(std::distance(begin, end), 2);
    --begin;
    std::advance(begin, std::distance(begin, end)/2);
    TS_ASSERT_DIFFERS(begin, end);
    TS_ASSERT_EQUALS(std::distance(begin, end), 2);
  }

  void test_iterate_over_histogram_counts() {
    // Arrange
    Counts expectedCounts{2, 3, 4};
    Histogram hist(Points{1, 2, 3}, expectedCounts);

    // Act
    auto result =
        std::equal(hist.begin(), hist.end(), expectedCounts.begin(),
                   [](const HistogramItem &item, const double &counts) {
                     return item.counts() == counts;
                   });

    // Assert
    TSM_ASSERT("Counts did not match", result);
  }

  void test_iterate_over_histogram_counts_when_histogram_has_frequencies() {
    // Arrange
    Histogram hist(Points{1, 2, 3}, Frequencies{2, 3, 4});
    Counts expectedCounts = hist.counts();

    // Act
    auto result =
        std::equal(hist.begin(), hist.end(), expectedCounts.begin(),
                   [](const HistogramItem &item, const double &counts) {
                     return item.counts() == counts;
                   });

    // Assert
    TSM_ASSERT("Counts did not match", result);
  }

  void test_iterate_over_histogram_frequencies() {
    // Arrange
    Frequencies expectedFrequencies{2, 3, 4};
    Histogram hist(Points{1, 2, 3}, expectedFrequencies);

    // Act
    auto result =
        std::equal(hist.begin(), hist.end(), expectedFrequencies.begin(),
                   [](const HistogramItem &item, const double &frequency) {
                     return item.frequency() == frequency;
                   });

    // Assert
    TSM_ASSERT("Frequencies did not match", result);
  }

  void test_iterate_over_histogram_frequencies_when_histogram_has_counts() {
    // Arrange
    Histogram hist(Points{1, 2, 3}, Counts{2, 3, 4});
    Frequencies expectedFrequencies = hist.frequencies();

    // Act
    auto result =
        std::equal(hist.begin(), hist.end(), expectedFrequencies.begin(),
                   [](const HistogramItem &item, const double &frequency) {
                     return item.frequency() == frequency;
                   });

    // Assert
    TSM_ASSERT("Frequencies did not match", result);
  }

  void test_iterate_over_histogram_center_when_histogram_has_bins() {
    // Arrange
    Histogram hist(BinEdges{1, 2, 3, 4}, Counts{2, 3, 4});
    Points expectedPoints = hist.points();

    // Act
    auto result =
        std::equal(hist.begin(), hist.end(), expectedPoints.begin(),
                   [](const HistogramItem &item, const double &point) {
                     return item.center() == point;
                   });

    // Assert
    TSM_ASSERT("Bin centers did not match", result);
  }

  void test_iterate_over_histogram_center_when_histogram_has_points() {
    // Arrange
    Histogram hist(Points{1, 2, 3}, Counts{2, 3, 4});
    Points expectedPoints = hist.points();

    // Act
    auto result =
        std::equal(hist.begin(), hist.end(), expectedPoints.begin(),
                   [](const HistogramItem &item, const double &point) {
                     return item.center() == point;
                   });

    // Assert
    TSM_ASSERT("Bin centers did not match", result);
  }

  void test_iterate_over_histogram_width_when_histogram_has_bins() {
    // Arrange
    Histogram hist(BinEdges{1, 2, 3, 5}, Counts{2, 3, 4});
    std::vector<double> expectedWidths = {1, 1, 2};

    // Act
    auto result =
        std::equal(hist.begin(), hist.end(), expectedWidths.begin(),
                   [](const HistogramItem &item, const double &width) {
                     return item.width() == width;
                   });

    // Assert
    TSM_ASSERT("Bin widths did not match", result);
  }
      
  void test_iterate_over_histogram_width_when_histogram_has_points() {
    // Arrange
    Histogram hist(Points{1, 3, 5}, Counts{2, 3, 4});
    std::vector<double> expectedWidths = {2, 2, 2};

    // Act
    auto result =
        std::equal(hist.begin(), hist.end(), expectedWidths.begin(),
                   [](const HistogramItem &item, const double &width) {
                     return item.width() == width;
                   });

    // Assert
    TSM_ASSERT("Bin widths did not match", result);
  }

  void test_iterate_over_histogram_count_variances_when_histogram_has_counts() {
    // Arrange
    Histogram hist(BinEdges{1, 2, 3, 5}, Counts{2, 3, 4}, CountVariances{3, 2, 1});
    auto expectedCountVariances = hist.countVariances();

    // Act
    auto result =
        std::equal(hist.begin(), hist.end(), expectedCountVariances.begin(),
                   [](const HistogramItem &item, const double &variance) {
                     return item.countVariance() == variance;
                   });

    // Assert
    TSM_ASSERT("Count variances did not match", result);
  }

  void test_iterate_over_histogram_count_variances_when_histogram_has_frequencies() {
    // Arrange
    Histogram hist(BinEdges{1, 2, 3, 5}, Frequencies{2, 3, 4}, FrequencyVariances{3, 2, 1});
    auto expectedCountVariances = hist.countVariances();

    // Act
    auto result =
        std::equal(hist.begin(), hist.end(), expectedCountVariances.begin(),
                   [](const HistogramItem &item, const double &variance) {
                     return item.countVariance() == variance;
                   });

    // Assert
    TSM_ASSERT("Count variances did not match", result);
  }

  void test_iterate_over_histogram_count_std_when_histogram_has_counts() {
    // Arrange
    Histogram hist(BinEdges{1, 2, 3, 5}, Counts{2, 3, 4}, CountVariances{3, 2, 1});
    auto expectedCountStd = hist.countStandardDeviations();

    // Act
    auto result =
        std::equal(hist.begin(), hist.end(), expectedCountStd.begin(),
                   [](const HistogramItem &item, const double &sigma) {
                     return item.countStandardDeviation() == sigma;
                   });

    // Assert
    TSM_ASSERT("Count standard deviations did not match", result);
  }

  void test_iterate_over_histogram_count_std_when_histogram_has_frequencies() {
    // Arrange
    Histogram hist(BinEdges{1, 2, 3, 5}, Frequencies{2, 3, 4}, FrequencyVariances{3, 2, 1});
    auto expectedCountStd = hist.countStandardDeviations();

    // Act
    auto result =
        std::equal(hist.begin(), hist.end(), expectedCountStd.begin(),
                   [](const HistogramItem &item, const double &sigma) {
                     return item.countStandardDeviation() == sigma;
                   });

    // Assert
    TSM_ASSERT("Count standard deviations did not match", result);
  }

  void test_iterate_over_histogram_frequency_variances_when_histogram_has_frequencys() {
    // Arrange
    Histogram hist(BinEdges{1, 2, 3, 5}, Counts{2, 3, 4}, CountVariances{3, 2, 1});
    auto expectedFrequencyVariances = hist.frequencyVariances();

    // Act
    auto result =
        std::equal(hist.begin(), hist.end(), expectedFrequencyVariances.begin(),
                   [](const HistogramItem &item, const double &variance) {
                     return item.frequencyVariance() == variance;
                   });

    // Assert
    TSM_ASSERT("Frequency variances did not match", result);
  }

  void test_iterate_over_histogram_frequency_variances_when_histogram_has_frequencies() {
    // Arrange
    Histogram hist(BinEdges{1, 2, 3, 5}, Frequencies{2, 3, 4}, FrequencyVariances{3, 2, 1});
    auto expectedFrequencyVariances = hist.frequencyVariances();

    // Act
    auto result =
        std::equal(hist.begin(), hist.end(), expectedFrequencyVariances.begin(),
                   [](const HistogramItem &item, const double &variance) {
                     return item.frequencyVariance() == variance;
                   });

    // Assert
    TSM_ASSERT("Frequency variances did not match", result);
  }

  void test_iterate_over_histogram_frequency_std_when_histogram_has_frequencys() {
    // Arrange
    Histogram hist(BinEdges{1, 2, 3, 5}, Counts{2, 3, 4}, CountVariances{3, 2, 1});
    auto expectedFrequencyStd = hist.frequencyStandardDeviations();

    // Act
    auto result =
        std::equal(hist.begin(), hist.end(), expectedFrequencyStd.begin(),
                   [](const HistogramItem &item, const double &sigma) {
                     return item.frequencyStandardDeviation() == sigma;
                   });

    // Assert
    TSM_ASSERT("Frequency standard deviations did not match", result);
  }

  void test_iterate_over_histogram_frequency_std_when_histogram_has_frequencies() {
    // Arrange
    Histogram hist(BinEdges{1, 2, 3, 5}, Frequencies{2, 3, 4}, FrequencyVariances{3, 2, 1});
    auto expectedFrequencyStd = hist.frequencyStandardDeviations();

    // Act
    auto result =
        std::equal(hist.begin(), hist.end(), expectedFrequencyStd.begin(),
                   [](const HistogramItem &item, const double &sigma) {
                     return item.frequencyStandardDeviation() == sigma;
                   });

    // Assert
    TSM_ASSERT("Frequency standard deviations did not match", result);
  }
};

class HistogramIteratorTestPerformance : public CxxTest::TestSuite {
public:
  static HistogramIteratorTestPerformance *createSuite() {
    return new HistogramIteratorTestPerformance;
  }
  static void destroySuite(HistogramIteratorTestPerformance *suite) { delete suite; }

  HistogramIteratorTestPerformance() {
    BinEdges edges(histSize, LinearGenerator(0, 2));
    Counts counts(histSize-1, LinearGenerator(0, 2));
    for (size_t i = 0; i < nHists; i++)
      hists.push_back(Histogram(edges, counts));
  }

  void test_iterate_and_access_each_item() {
    double total = 0;
    for (auto &hist : hists)
        for (auto &item : hist)
            total += item.frequency();
  }

private:
  const size_t nHists = 500;
  const size_t histSize = 10000;
  std::vector<Histogram> hists;
};

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMITERATORTEST_H_ */
