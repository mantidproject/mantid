#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMITERATORTEST_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMITERATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramItem.h"
#include "MantidHistogramData/HistogramIterator.h"

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

  void test_iterate_over_empty_histogram() {
    Histogram hist(Histogram::XMode::BinEdges, Histogram::YMode::Counts);
    /* assert(hist.size() == 0); */

    double total = 0;
    /* for (const auto &item : hist) { */
    /*   total += item.counts(); */
    /* } */

    TS_ASSERT_EQUALS(total, 0);
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

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMITERATORTEST_H_ */
