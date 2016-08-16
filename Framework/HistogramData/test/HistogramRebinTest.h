#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMREBINTEST_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMREBINTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramRebin.h"
#include "MantidHistogramData/LinearGenerator.h"

#include <algorithm>
#include <random>

using namespace Mantid::HistogramData;

class HistogramRebinTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramRebinTest *createSuite() { return new HistogramRebinTest(); }
  static void destroySuite(HistogramRebinTest *suite) { delete suite; }

  void testRebinCountsSmallerBins() {
    auto hist = getCountsHistogram();
    BinEdges smBins(20, LinearGenerator(0, 0.5));
    Histogram histSmaller(Histogram::XMode::BinEdges, Histogram::YMode::Counts);

    TS_ASSERT_THROWS_NOTHING(histSmaller = rebinCounts(hist, smBins));

    for (int i = 0; i < hist.y().size(); i++) {
      TS_ASSERT_EQUALS(hist.y()[i], histSmaller.y()[2 * i] * 2);
      TS_ASSERT_EQUALS(hist.y()[i], histSmaller.y()[(2 * i) + 1] * 2);
    }
  }

  void testRebinCountsLargerBins() {
    auto hist = getCountsHistogram();
    BinEdges lgBins(5, LinearGenerator(0, 2));
    Histogram histLarger(Histogram::XMode::BinEdges, Histogram::YMode::Counts);

    TS_ASSERT_THROWS_NOTHING(histLarger = rebinCounts(hist, lgBins));

    for (int i = 0; i < histLarger.y().size(); i++) {
      TS_ASSERT_EQUALS(histLarger.y()[i],
                       hist.y()[2 * i] + hist.y()[(2 * i) + 1]);
    }
  }

  void testRebinCountsFailModes() {
    auto hist = getFrequencyHistogram();
    BinEdges edges(10, LinearGenerator(0, 10));

    TS_ASSERT_THROWS(rebinCounts(hist, edges), std::runtime_error);

    hist = Histogram(Histogram::XMode::Points, Histogram::YMode::Counts);
    TS_ASSERT_THROWS(rebinCounts(hist, edges), std::runtime_error);

    hist = Histogram(BinEdges(10, LinearGenerator(0, 0.5)));
    TS_ASSERT_THROWS(rebinCounts(hist, edges), std::runtime_error);
  }

  void testRebinFrequenciesSmallerBins() {
    auto hist = getFrequencyHistogram();
    BinEdges smBins(20, LinearGenerator(0, 0.5));
    Histogram histSmaller(Histogram::XMode::BinEdges,
                          Histogram::YMode::Frequencies);

    TS_ASSERT_THROWS_NOTHING(histSmaller = rebinFrequencies(hist, smBins));

    for (int i = 0; i < hist.y().size(); i++) {
      TS_ASSERT_EQUALS(hist.y()[i], histSmaller.y()[2 * i] * 2);
      TS_ASSERT_EQUALS(hist.y()[i], histSmaller.y()[(2 * i) + 1] * 2);
    }
  }

  void testRebinFrequenciesLargerBins() {
    auto hist = getFrequencyHistogram();
    BinEdges lgBins(5, LinearGenerator(0, 2));
    Histogram histLarger(Histogram::XMode::BinEdges,
                         Histogram::YMode::Frequencies);

    TS_ASSERT_THROWS_NOTHING(histLarger = rebinFrequencies(hist, lgBins));

    for (int i = 0; i < histLarger.y().size(); i++) {
      TS_ASSERT_EQUALS(histLarger.y()[i],
                       hist.y()[2 * i] + hist.y()[(2 * i) + 1]);
    }
  }

  void testRebinFrequenciesFailModes() {
    auto hist = getCountsHistogram();
    BinEdges edges(10, LinearGenerator(0, 10));

    TS_ASSERT_THROWS(rebinFrequencies(hist, edges), std::runtime_error);

    hist = Histogram(Histogram::XMode::Points, Histogram::YMode::Frequencies);
    TS_ASSERT_THROWS(rebinFrequencies(hist, edges), std::runtime_error);

    hist = Histogram(BinEdges(10, LinearGenerator(0, 0.5)));
    TS_ASSERT_THROWS(rebinFrequencies(hist, edges), std::runtime_error);
  }

private:
  Histogram getCountsHistogram() {
    return Histogram(BinEdges(10, LinearGenerator(0, 1)),
                     Counts{10.5, 11.2, 19.3, 25.4, 36.8, 40.3, 17.7, 9.3, 4.6},
                     CountStandardDeviations{3.2404, 3.3466, 4.3932, 5.0398,
                                             6.0663, 6.3482, 4.2071, 3.0496,
                                             2.1448});
  }

  Histogram getFrequencyHistogram() {
    return Histogram(
        BinEdges(10, LinearGenerator(0, 1)),
        Frequencies{10.5, 11.2, 19.3, 25.4, 36.8, 40.3, 17.7, 9.3, 4.6},
        FrequencyStandardDeviations{3.2404, 3.3466, 4.3932, 5.0398, 6.0663,
                                    6.3482, 4.2071, 3.0496, 2.1448});
  }
};

class HistogramRebinTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramRebinTestPerformance *createSuite() {
    return new HistogramRebinTestPerformance();
  }
  static void destroySuite(HistogramRebinTestPerformance *suite) {
    delete suite;
  }

  HistogramRebinTestPerformance()
      : hist(BinEdges(binSize, LinearGenerator(0, 1))),
        histFreq(BinEdges(binSize, LinearGenerator(0, 1))),
        smBins(binSize * 2, LinearGenerator(0, 0.5)),
        lgBins(binSize / 2, LinearGenerator(0, 2)) {
    setupOutput();
  }

  void testRebinCountsSmallerBins() {
    for (int i = 0; i < nIters; i++)
      rebinCounts(hist, smBins);
  }

  void testRebinFrequenciesSmallerBins() {
    for (int i = 0; i < nIters; i++)
      rebinFrequencies(histFreq, smBins);
  }

  void testRebinCountsLargerBins() {
    for (int i = 0; i < nIters; i++)
      rebinCounts(hist, lgBins);
  }

  void testRebinFrequenciesLargerBins() {
    for (int i = 0; i < nIters; i++)
      rebinFrequencies(histFreq, lgBins);
  }

private:
  const size_t binSize = 10000;
  const size_t nIters = 10000;
  Histogram hist;
  Histogram histFreq;
  BinEdges smBins;
  BinEdges lgBins;

  void setupOutput() {
    Counts counts(binSize - 1);
    CountStandardDeviations countErrors(counts.size());
    Frequencies freqs(binSize - 1);
    FrequencyStandardDeviations freqErrors(freqs.size());

    std::default_random_engine eng;
    std::uniform_real_distribution<double> distr(100.0, 10000.0);

    std::generate(counts.begin(), counts.end(),
                  [distr, eng]() mutable { return distr(eng); });
    std::generate(freqs.begin(), freqs.end(),
                  [distr, eng]() mutable { return distr(eng); });

    std::transform(counts.cbegin(), counts.cend(), countErrors.begin(),
                   [](const double c) { return std::sqrt(c); });
    std::transform(freqs.cbegin(), freqs.cend(), freqErrors.begin(),
                   [](const double f) { return std::sqrt(f); });

    hist.setCounts(counts);
    hist.setCountStandardDeviations(countErrors);
    histFreq.setFrequencies(freqs);
    histFreq.setFrequencyStandardDeviations(freqErrors);
  }
};

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMREBINTEST_H_ */