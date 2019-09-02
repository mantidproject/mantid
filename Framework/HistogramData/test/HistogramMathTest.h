// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMMATHTEST_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMMATHTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramMath.h"
#include "MantidKernel/WarningSuppressions.h"

using namespace Mantid::HistogramData;

class HistogramMathTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramMathTest *createSuite() { return new HistogramMathTest(); }
  static void destroySuite(HistogramMathTest *suite) { delete suite; }

  void test_times_equals() {
    Histogram hist(BinEdges{1, 2, 3}, Counts{4, 9});
    hist *= 3.0;
    TS_ASSERT_EQUALS(hist.x()[0], 1.0);
    TS_ASSERT_EQUALS(hist.x()[1], 2.0);
    TS_ASSERT_EQUALS(hist.x()[2], 3.0);
    TS_ASSERT_EQUALS(hist.y()[0], 12.0);
    TS_ASSERT_EQUALS(hist.y()[1], 27.0);
    TS_ASSERT_EQUALS(hist.e()[0], 6.0);
    TS_ASSERT_EQUALS(hist.e()[1], 9.0);
  }

  void test_divide_equals() {
    Histogram hist(BinEdges{1, 2, 3}, Counts{4, 9});
    hist /= 0.5;
    TS_ASSERT_EQUALS(hist.x()[0], 1.0);
    TS_ASSERT_EQUALS(hist.x()[1], 2.0);
    TS_ASSERT_EQUALS(hist.x()[2], 3.0);
    TS_ASSERT_EQUALS(hist.y()[0], 8.0);
    TS_ASSERT_EQUALS(hist.y()[1], 18.0);
    TS_ASSERT_EQUALS(hist.e()[0], 4.0);
    TS_ASSERT_EQUALS(hist.e()[1], 6.0);
  }

  void test_times() {
    const Histogram hist(BinEdges{1, 2, 3}, Counts{4, 9});
    auto result = hist * 3.0;
    TS_ASSERT_EQUALS(result.x()[0], 1.0);
    TS_ASSERT_EQUALS(result.x()[1], 2.0);
    TS_ASSERT_EQUALS(result.x()[2], 3.0);
    TS_ASSERT_EQUALS(result.y()[0], 12.0);
    TS_ASSERT_EQUALS(result.y()[1], 27.0);
    TS_ASSERT_EQUALS(result.e()[0], 6.0);
    TS_ASSERT_EQUALS(result.e()[1], 9.0);
  }

  void test_times_reverse_order() {
    const Histogram hist(BinEdges{1, 2, 3}, Counts{4, 9});
    auto result = 3.0 * hist;
    TS_ASSERT_EQUALS(result.x()[0], 1.0);
    TS_ASSERT_EQUALS(result.x()[1], 2.0);
    TS_ASSERT_EQUALS(result.x()[2], 3.0);
    TS_ASSERT_EQUALS(result.y()[0], 12.0);
    TS_ASSERT_EQUALS(result.y()[1], 27.0);
    TS_ASSERT_EQUALS(result.e()[0], 6.0);
    TS_ASSERT_EQUALS(result.e()[1], 9.0);
  }

  void test_divide() {
    const Histogram hist(BinEdges{1, 2, 3}, Counts{4, 9});
    auto result = hist / 0.5;
    TS_ASSERT_EQUALS(result.x()[0], 1.0);
    TS_ASSERT_EQUALS(result.x()[1], 2.0);
    TS_ASSERT_EQUALS(result.x()[2], 3.0);
    TS_ASSERT_EQUALS(result.y()[0], 8.0);
    TS_ASSERT_EQUALS(result.y()[1], 18.0);
    TS_ASSERT_EQUALS(result.e()[0], 4.0);
    TS_ASSERT_EQUALS(result.e()[1], 6.0);
  }

  void test_bad_factors() {
    Histogram hist(BinEdges{1, 2, 3}, Counts{4, 9});
    TS_ASSERT_THROWS(hist *= (-1.0), const std::runtime_error &);
    TS_ASSERT_THROWS(hist * (-1.0), const std::runtime_error &);
    TS_ASSERT_THROWS((-1.0) * hist, const std::runtime_error &);
    TS_ASSERT_THROWS(hist /= (-1.0), const std::runtime_error &);
    TS_ASSERT_THROWS(hist / (-1.0), const std::runtime_error &);
    TS_ASSERT_THROWS(hist /= 0.0, const std::runtime_error &);
    TS_ASSERT_THROWS(hist / 0.0, const std::runtime_error &);
  }

  void test_plus_histogram() {
    const Histogram hist1(BinEdges{1, 2, 3}, Counts{4, 9});
    const Histogram hist2(BinEdges{1, 2, 3}, Counts{1, 2});
    auto hist = hist1 + hist2;
    TS_ASSERT_EQUALS(hist.xMode(), hist1.xMode());
    TS_ASSERT_EQUALS(hist.sharedX(), hist1.sharedX());
    TS_ASSERT_EQUALS(hist.y()[0], 5.0);
    TS_ASSERT_EQUALS(hist.y()[1], 11.0);
    TS_ASSERT_DELTA(hist.e()[0], sqrt(5.0), 1e-14);
    TS_ASSERT_DELTA(hist.e()[1], sqrt(11.0), 1e-14);
  }

  void test_plus_histogram_self() {
    BinEdges edges{1, 2, 3};
    Histogram hist(edges, Counts{4, 9});
    hist += hist;
    TS_ASSERT_EQUALS(hist.sharedX(), edges.cowData());
    TS_ASSERT_EQUALS(hist.y()[0], 8.0);
    TS_ASSERT_EQUALS(hist.y()[1], 18.0);
    TS_ASSERT_DELTA(hist.e()[0], sqrt(8.0), 1e-14);
    TS_ASSERT_DELTA(hist.e()[1], sqrt(18.0), 1e-14);
  }

  void test_plus_histogram_fail_xMode() {
    const Histogram hist1(BinEdges{1, 2, 3}, Counts{4, 9});
    const Histogram hist2(Points{1, 2, 3}, Counts{1, 2, 3});
    TS_ASSERT_THROWS(hist1 + hist2, const std::runtime_error &);
  }

  void test_plus_histogram_fail_yMode() {
    const Histogram hist1(BinEdges{1, 2, 3}, Counts{4, 9});
    const Histogram hist2(BinEdges{1, 2, 3}, Frequencies{4, 9});
    TS_ASSERT_THROWS(hist1 + hist2, const std::runtime_error &);
  }

  void test_plus_histogram_fail_x_length_mismatch() {
    const Histogram hist1(BinEdges{1, 2, 3}, Counts{4, 9});
    const Histogram hist2(BinEdges{1, 2}, Counts{1});
    TS_ASSERT_THROWS(hist1 + hist2, const std::runtime_error &);
  }

  void test_plus_histogram_fail_x_value_mismatch() {
    const Histogram hist1(BinEdges{1, 2.0, 3}, Counts{4, 9});
    const Histogram hist2(BinEdges{1, 2.1, 3}, Counts{1, 2});
    TS_ASSERT_THROWS(hist1 + hist2, const std::runtime_error &);
  }

  void test_minus_histogram() {
    const Histogram hist1(BinEdges{1, 2, 3}, Counts{4, 9});
    const Histogram hist2(BinEdges{1, 2, 3}, Counts{1, 2});
    auto hist = hist1 - hist2;
    TS_ASSERT_EQUALS(hist.xMode(), hist1.xMode());
    TS_ASSERT_EQUALS(hist.sharedX(), hist1.sharedX());
    TS_ASSERT_EQUALS(hist.y()[0], 3.0);
    TS_ASSERT_EQUALS(hist.y()[1], 7.0);
    TS_ASSERT_DELTA(hist.e()[0], sqrt(5.0), 1e-14);
    TS_ASSERT_DELTA(hist.e()[1], sqrt(11.0), 1e-14);
  }

  void test_minus_histogram_self() {
    BinEdges edges{1, 2, 3};
    Histogram hist(edges, Counts{4, 9});
    GNU_DIAG_OFF("self-assign-overloaded")
    hist -= hist;
    GNU_DIAG_ON("self-assign-overloaded")
    TS_ASSERT_EQUALS(hist.sharedX(), edges.cowData());
    TS_ASSERT_EQUALS(hist.y()[0], 0.0);
    TS_ASSERT_EQUALS(hist.y()[1], 0.0);
    TS_ASSERT_DELTA(hist.e()[0], sqrt(8.0), 1e-14);
    TS_ASSERT_DELTA(hist.e()[1], sqrt(18.0), 1e-14);
  }

  void test_minus_histogram_fail_xMode() {
    const Histogram hist1(BinEdges{1, 2, 3}, Counts{4, 9});
    const Histogram hist2(Points{1, 2, 3}, Counts{1, 2, 3});
    TS_ASSERT_THROWS(hist1 - hist2, const std::runtime_error &);
  }

  void test_minus_histogram_fail_yMode() {
    const Histogram hist1(BinEdges{1, 2, 3}, Counts{4, 9});
    const Histogram hist2(BinEdges{1, 2, 3}, Frequencies{4, 9});
    TS_ASSERT_THROWS(hist1 - hist2, const std::runtime_error &);
  }

  void test_minus_histogram_fail_x_length_mismatch() {
    const Histogram hist1(BinEdges{1, 2, 3}, Counts{4, 9});
    const Histogram hist2(BinEdges{1, 2}, Counts{1});
    TS_ASSERT_THROWS(hist1 - hist2, const std::runtime_error &);
  }

  void test_minus_histogram_fail_x_value_mismatch() {
    const Histogram hist1(BinEdges{1, 2.0, 3}, Counts{4, 9});
    const Histogram hist2(BinEdges{1, 2.1, 3}, Counts{1, 2});
    TS_ASSERT_THROWS(hist1 - hist2, const std::runtime_error &);
  }

  void test_times_histogram() {
    const Histogram hist1(BinEdges{1, 2, 3}, Frequencies{4, 9});
    const Histogram hist2(BinEdges{1, 2, 3}, Frequencies{1, 4});
    auto hist = hist1 * hist2;
    TS_ASSERT_EQUALS(hist.xMode(), hist1.xMode());
    TS_ASSERT_EQUALS(hist.sharedX(), hist1.sharedX());
    TS_ASSERT_EQUALS(hist.y()[0], 4.0);
    TS_ASSERT_EQUALS(hist.y()[1], 36.0);
    TS_ASSERT_DELTA(hist.e()[0], sqrt(4.0 + 16.0), 1e-14);
    TS_ASSERT_DELTA(hist.e()[1], sqrt(12.0 * 12.0 + 18.0 * 18.0), 1e-14);
  }

  void test_times_equals_histogram() {
    Histogram hist(BinEdges{1, 2, 3}, Frequencies{4, 9});
    const Histogram hist2(BinEdges{1, 2, 3}, Frequencies{1, 4});
    hist *= hist2;
    TS_ASSERT_EQUALS(hist.y()[0], 4.0);
    TS_ASSERT_EQUALS(hist.y()[1], 36.0);
    TS_ASSERT_DELTA(hist.e()[0], sqrt(4.0 + 16.0), 1e-14);
    TS_ASSERT_DELTA(hist.e()[1], sqrt(12.0 * 12.0 + 18.0 * 18.0), 1e-14);
  }

  void test_times_histogram_output_yMode() {
    const Histogram histC(BinEdges{1, 2, 3}, Counts{4, 9});
    const Histogram histF(BinEdges{1, 2, 3}, Frequencies{4, 9});
    TS_ASSERT_EQUALS((histC * histF).yMode(), Histogram::YMode::Counts);
    TS_ASSERT_EQUALS((histF * histC).yMode(), Histogram::YMode::Counts);
    TS_ASSERT_EQUALS((histF * histF).yMode(), Histogram::YMode::Frequencies);
  }

  void test_times_histogram_self() {
    BinEdges edges{1, 2, 3};
    Histogram hist(edges, Frequencies{4, 9});
    hist *= hist;
    TS_ASSERT_EQUALS(hist.sharedX(), edges.cowData());
    TS_ASSERT_EQUALS(hist.y()[0], 16.0);
    TS_ASSERT_EQUALS(hist.y()[1], 81.0);
    TS_ASSERT_DELTA(hist.e()[0], sqrt(8 * 8 + 8 * 8), 1e-14);
    TS_ASSERT_DELTA(hist.e()[1], sqrt(27 * 27 + 27 * 27), 1e-14);
  }

  void test_times_histogram_fail_xMode() {
    const Histogram hist1(BinEdges{1, 2, 3}, Frequencies{4, 9});
    const Histogram hist2(Points{1, 2, 3}, Frequencies{1, 2, 3});
    TS_ASSERT_THROWS(hist1 * hist2, const std::runtime_error &);
  }

  void test_times_histogram_fail_yMode() {
    const Histogram histC(BinEdges{1, 2, 3}, Counts{4, 9});
    const Histogram histF(BinEdges{1, 2, 3}, Frequencies{4, 9});
    TS_ASSERT_THROWS(histC * histC, const std::runtime_error &);
    TS_ASSERT_THROWS_NOTHING(histC * histF);
    TS_ASSERT_THROWS_NOTHING(histF * histC);
    TS_ASSERT_THROWS_NOTHING(histF * histF);
  }

  void test_times_histogram_fail_x_length_mismatch() {
    const Histogram hist1(BinEdges{1, 2, 3}, Frequencies{4, 9});
    const Histogram hist2(BinEdges{1, 2}, Frequencies{1});
    TS_ASSERT_THROWS(hist1 * hist2, const std::runtime_error &);
  }

  void test_times_histogram_fail_x_value_mismatch() {
    const Histogram hist1(BinEdges{1, 2.0, 3}, Frequencies{4, 9});
    const Histogram hist2(BinEdges{1, 2.1, 3}, Frequencies{1, 2});
    TS_ASSERT_THROWS(hist1 * hist2, const std::runtime_error &);
  }

  void test_divide_histogram() {
    const Histogram hist1(BinEdges{1, 2, 3}, Frequencies{4, 16});
    const Histogram hist2(BinEdges{1, 2, 3}, Frequencies{1, 4});
    auto hist = hist1 / hist2;
    TS_ASSERT_EQUALS(hist.xMode(), hist1.xMode());
    TS_ASSERT_EQUALS(hist.sharedX(), hist1.sharedX());
    TS_ASSERT_EQUALS(hist.y()[0], 4.0);
    TS_ASSERT_EQUALS(hist.y()[1], 4.0);
    TS_ASSERT_DELTA(hist.e()[0], sqrt(4 + 4 * 4 * 1) / 1, 1e-14);
    TS_ASSERT_DELTA(hist.e()[1], sqrt(16 + 4 * 4 * 4) / 4.0, 1e-14);
  }

  void test_divide_equals_histogram() {
    Histogram hist(BinEdges{1, 2, 3}, Frequencies{4, 16});
    const Histogram hist2(BinEdges{1, 2, 3}, Frequencies{1, 4});
    hist /= hist2;
    TS_ASSERT_EQUALS(hist.y()[0], 4.0);
    TS_ASSERT_EQUALS(hist.y()[1], 4.0);
    TS_ASSERT_DELTA(hist.e()[0], sqrt(4 + 4 * 4 * 1) / 1, 1e-14);
    TS_ASSERT_DELTA(hist.e()[1], sqrt(16 + 4 * 4 * 4) / 4.0, 1e-14);
  }

  void test_divide_histogram_output_yMode() {
    const Histogram histC(BinEdges{1, 2, 3}, Counts{4, 9});
    const Histogram histF(BinEdges{1, 2, 3}, Frequencies{4, 9});
    TS_ASSERT_EQUALS((histC / histC).yMode(), Histogram::YMode::Frequencies);
    TS_ASSERT_EQUALS((histC / histF).yMode(), Histogram::YMode::Counts);
    TS_ASSERT_EQUALS((histF / histF).yMode(), Histogram::YMode::Frequencies);
  }

  void test_divide_histogram_self() {
    BinEdges edges{1, 2, 3};
    Histogram hist(edges, Frequencies{4, 9});
    GNU_DIAG_OFF("self-assign-overloaded")
    hist /= hist;
    GNU_DIAG_ON("self-assign-overloaded")
    TS_ASSERT_EQUALS(hist.sharedX(), edges.cowData());
    TS_ASSERT_EQUALS(hist.y()[0], 1.0);
    TS_ASSERT_EQUALS(hist.y()[1], 1.0);
    TS_ASSERT_DELTA(hist.e()[0], sqrt(4 + 1 * 1 * 4) / 4, 1e-14);
    TS_ASSERT_DELTA(hist.e()[1], sqrt(9 + 1 * 1 * 9) / 9, 1e-14);
  }

  void test_divide_histogram_fail_xMode() {
    const Histogram hist1(BinEdges{1, 2, 3}, Frequencies{4, 9});
    const Histogram hist2(Points{1, 2, 3}, Frequencies{1, 2, 3});
    TS_ASSERT_THROWS(hist1 / hist2, const std::runtime_error &);
  }

  void test_divide_histogram_fail_yMode() {
    const Histogram histC(BinEdges{1, 2, 3}, Counts{4, 9});
    const Histogram histF(BinEdges{1, 2, 3}, Frequencies{4, 9});
    TS_ASSERT_THROWS_NOTHING(histC / histC);
    TS_ASSERT_THROWS_NOTHING(histC / histF);
    TS_ASSERT_THROWS(histF / histC, const std::runtime_error &);
    TS_ASSERT_THROWS_NOTHING(histF / histF);
  }

  void test_divide_histogram_fail_x_length_mismatch() {
    const Histogram hist1(BinEdges{1, 2, 3}, Frequencies{4, 9});
    const Histogram hist2(BinEdges{1, 2}, Frequencies{1});
    TS_ASSERT_THROWS(hist1 / hist2, const std::runtime_error &);
  }

  void test_divide_histogram_fail_x_value_mismatch() {
    const Histogram hist1(BinEdges{1, 2.0, 3}, Frequencies{4, 9});
    const Histogram hist2(BinEdges{1, 2.1, 3}, Frequencies{1, 2});
    TS_ASSERT_THROWS(hist1 / hist2, const std::runtime_error &);
  }
};

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMMATHTEST_H_ */
