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

  void testExecrebin() {
    TS_ASSERT_THROWS_NOTHING(
        rebin(getCountsHistogram(), BinEdges(10, LinearGenerator(0, 0.5))));
  }

  void testExecRebinFrequency() {
    TS_ASSERT_THROWS_NOTHING(
        rebin(getFrequencyHistogram(), BinEdges(10, LinearGenerator(0, 0.5))));
  }

  void testRebinNoYModeDefined() {
    BinEdges edges(5, LinearGenerator(0, 2));
    // X Mode Counts
    TS_ASSERT_THROWS(
        rebin(Histogram(Histogram::XMode::Points, Histogram::YMode::Counts),
              edges),
        std::runtime_error);
    // No YMode set
    TS_ASSERT_THROWS(
        rebin(Histogram(BinEdges(10, LinearGenerator(0, 0.5))), edges),
        std::runtime_error);
  }

  void testRebinFailsBinEdgesInvalid() {
    std::vector<double> binEdges{1, 2, 3, 3, 5, 7};
    BinEdges edges(binEdges);

    TS_ASSERT_THROWS(rebin(getCountsHistogram(), edges), std::runtime_error);
  }

  void testRebinFailsInputBinEdgesInvalid() {
    std::vector<double> binEdges{1, 2, 3, 3, 5, 7};
    Histogram hist(BinEdges(std::move(binEdges)), Counts(5, 10));
    BinEdges edges{1, 2, 3, 4, 5, 6};

    TS_ASSERT_THROWS(rebin(hist, edges), std::runtime_error);
  }

  void testRebinIdenticalBins() {
    auto histCounts = getCountsHistogram();
    auto histFreq = getFrequencyHistogram();

    auto outCounts = rebin(histCounts, histCounts.binEdges());
    auto outFreq = rebin(histFreq, histFreq.binEdges());

    TS_ASSERT_EQUALS(outCounts.x().rawData(), histCounts.x().rawData());
    TS_ASSERT_EQUALS(outCounts.y().rawData(), histCounts.y().rawData());
    TS_ASSERT_EQUALS(outCounts.e().rawData(), histCounts.e().rawData());

    TS_ASSERT_EQUALS(outFreq.x().rawData(), histFreq.x().rawData());
    TS_ASSERT_EQUALS(outFreq.y().rawData(), histFreq.y().rawData());
    TS_ASSERT_EQUALS(outFreq.e().rawData(), histFreq.e().rawData());
  }

  void testBinEdgesOutsideInputBins() {
    auto histCounts = getCountsHistogram();
    auto histFreq = getFrequencyHistogram();

    auto outCounts = rebin(histCounts, BinEdges(10, LinearGenerator(30, 1)));
    auto outFreq = rebin(histFreq, BinEdges(5, LinearGenerator(100, 2)));

    TS_ASSERT(std::all_of(outCounts.y().cbegin(), outCounts.y().cend(),
                          [](const double i) { return i == 0; }));
    TS_ASSERT(std::all_of(outFreq.y().cbegin(), outFreq.y().cend(),
                          [](const double i) { return i == 0; }));
    TS_ASSERT(std::all_of(outFreq.e().cbegin(), outFreq.e().cend(),
                          [](const double i) { return i == 0; }));
  }

  void testSplitBinSymmetric() {
    // Handles the case where
    // | | |   becomes:
    // |||||
    Histogram hist(BinEdges{0, 1, 2}, Counts{10, 10});
    Histogram histFreq(BinEdges{0, 1, 2}, Frequencies{12, 12});
    BinEdges edges{0, 0.5, 1, 1.5, 2};

    auto outCounts = rebin(hist, edges);
    auto outFreq = rebin(histFreq, edges);

    for (size_t i = 0; i < outCounts.y().size(); i++) {
      TS_ASSERT_EQUALS(outCounts.y()[i], 5.0);
      TS_ASSERT_EQUALS(outFreq.y()[i], 12.0);
    }
  }

  void testCombineMultipleBinsSymmetric() {
    // Handles the case where
    // |||||   becomes:
    // | | |
    Histogram hist(BinEdges(5, LinearGenerator(0, 1)), Counts{5, 7, 10, 6});
    Histogram histFreq(BinEdges(5, LinearGenerator(0, 1)),
                       Frequencies{3, 9, 8, 12});
    BinEdges edges(3, LinearGenerator(0, 2));

    auto outCounts = rebin(hist, edges);
    auto outFreq = rebin(histFreq, edges);

    for (size_t i = 0; i < outCounts.y().size(); i++) {
      TS_ASSERT_EQUALS(outCounts.y()[i],
                       hist.y()[2 * i] + hist.y()[(2 * i) + 1]);
      TS_ASSERT_EQUALS(outFreq.y()[i],
                       (histFreq.y()[2 * i] + histFreq.y()[(2 * i) + 1]) / 2);
    }
  }

  void testSplitBinsAssymetric() {
    // Handles the case where
    // |  |  |   becomes:
    // ||   ||
    Histogram hist(BinEdges(3, LinearGenerator(0, 1)), Counts{15, 7});
    Histogram histFreq(BinEdges(3, LinearGenerator(0, 1)), Frequencies{12, 20});
    BinEdges edges{0, 0.5, 1.5, 2};

    auto outCounts = rebin(hist, edges);
    auto outFreq = rebin(histFreq, edges);

    TS_ASSERT_EQUALS(outCounts.y()[0], hist.y()[0] / 2);
    TS_ASSERT_EQUALS(outCounts.y()[1], (hist.y()[0] + hist.y()[1]) / 2);
    TS_ASSERT_EQUALS(outCounts.y()[2], hist.y()[1] / 2);

    TS_ASSERT_EQUALS(outFreq.y()[0], histFreq.y()[0]);
    TS_ASSERT_EQUALS(outFreq.y()[1], (histFreq.y()[0] + histFreq.y()[1]) / 2);
    TS_ASSERT_EQUALS(outFreq.y()[2], histFreq.y()[1]);
  }

  void testCombineBinsAssymetric() {
    // Handles the case where
    // ||   ||   becomes:
    // |  |  |
    Histogram hist(BinEdges{0, 0.5, 1.5, 2}, Counts{10, 18, 7});
    Histogram histFreq(BinEdges{0, 0.5, 1.5, 2}, Frequencies{16, 32, 8});
    BinEdges edges{0, 1, 2};

    auto outCounts = rebin(hist, edges);
    auto outFreq = rebin(histFreq, edges);

    TS_ASSERT_EQUALS(outCounts.y()[0], hist.y()[0] + (hist.y()[1] / 2));
    TS_ASSERT_EQUALS(outCounts.y()[1], (hist.y()[1] / 2) + hist.y()[2]);

    TS_ASSERT_EQUALS(outFreq.y()[0], (histFreq.y()[0] + histFreq.y()[1]) / 2);
    TS_ASSERT_EQUALS(outFreq.y()[1], (histFreq.y()[1] + histFreq.y()[2]) / 2);
  }

  void testSplitCombineBinsAssymetric() {
    // Handles the case where
    // | | | |   becomes:
    // ||   ||
    Histogram hist(BinEdges{0, 1, 2, 3}, Counts{100, 50, 216});
    Histogram histFreq(BinEdges{0, 1, 2, 3}, Frequencies{210, 19, 80});
    BinEdges edges{0, 0.5, 2.5, 3};

    auto outCounts = rebin(hist, edges);
    auto outFreq = rebin(histFreq, edges);

    TS_ASSERT_EQUALS(outCounts.y()[0], hist.y()[0] / 2);
    TS_ASSERT_EQUALS(outCounts.y()[1],
                     ((hist.y()[0] + hist.y()[2]) / 2) + hist.y()[1]);
    TS_ASSERT_EQUALS(outCounts.y()[2], hist.y()[2] / 2);

    TS_ASSERT_EQUALS(outFreq.y()[0], histFreq.y()[0]);
    TS_ASSERT_EQUALS(
        outFreq.y()[1],
        ((histFreq.y()[0] / 2) + histFreq.y()[1] + (histFreq.y()[2] / 2)) / 2);
    TS_ASSERT_EQUALS(outFreq.y()[2], histFreq.y()[2]);
  }

  void testSplitCombineBinsAssymetric2() {
    // Handles the case where
    // ||   ||   becomes:
    // | | | |
    Histogram hist(BinEdges{0, 0.5, 2.5, 3}, Counts{10, 100, 30});
    Histogram histFreq(BinEdges{0, 0.5, 2.5, 3}, Frequencies{17, 8, 15});
    BinEdges edges{0, 1, 2, 3};

    auto outCounts = rebin(hist, edges);
    auto outFreq = rebin(histFreq, edges);

    TS_ASSERT_EQUALS(outCounts.y()[0], hist.y()[0] + (hist.y()[1] / 4));
    TS_ASSERT_EQUALS(outCounts.y()[1], hist.y()[1] / 2);
    TS_ASSERT_EQUALS(outCounts.y()[2], (hist.y()[1] / 4) + hist.y()[2]);

    TS_ASSERT_EQUALS(outFreq.y()[0], (histFreq.y()[0] + histFreq.y()[1]) / 2);
    TS_ASSERT_EQUALS(outFreq.y()[1], histFreq.y()[1]);
    TS_ASSERT_EQUALS(outFreq.y()[2], (histFreq.y()[1] + histFreq.y()[2]) / 2);
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

  void testrebinSmallerBins() {
    for (int i = 0; i < nIters; i++)
      rebin(hist, smBins);
  }

  void testrebinSmallerBins() {
    for (int i = 0; i < nIters; i++)
      rebin(histFreq, smBins);
  }

  void testrebinLargerBins() {
    for (int i = 0; i < nIters; i++)
      rebin(hist, lgBins);
  }

  void testrebinLargerBins() {
    for (int i = 0; i < nIters; i++)
      rebin(histFreq, lgBins);
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