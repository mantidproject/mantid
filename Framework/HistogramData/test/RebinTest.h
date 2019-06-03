// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_HISTOGRAMDATA_REBINTEST_H_
#define MANTID_HISTOGRAMDATA_REBINTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/Exception.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidHistogramData/Rebin.h"

#include <algorithm>
#include <random>

using namespace Mantid::HistogramData;
using namespace Mantid::HistogramData::Exception;

class RebinTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RebinTest *createSuite() { return new RebinTest(); }
  static void destroySuite(RebinTest *suite) { delete suite; }

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
    Points points(5, LinearGenerator(0, 1));
    Counts counts{10, 1, 3, 4, 7};
    // X Mode Points
    TS_ASSERT_THROWS(rebin(Histogram(points, counts), edges),
                     const std::runtime_error &);
    // No YMode set
    TS_ASSERT_THROWS(
        rebin(Histogram(BinEdges(10, LinearGenerator(0, 0.5))), edges),
        const std::runtime_error &);
  }

  void testRebinFailsCentralBinEdgesInvalid() {
    std::vector<double> binEdges{1, 2, 3, 3, 5, 7};
    BinEdges edges(binEdges);

    TS_ASSERT_THROWS(rebin(getCountsHistogram(), edges),
                     const InvalidBinEdgesError &);
  }

  void testRebinFailsStartBinEdgesInvalid() {
    std::vector<double> binEdges{1, 1, 3, 4, 5, 7};
    BinEdges edges(binEdges);

    TS_ASSERT_THROWS(rebin(getCountsHistogram(), edges),
                     const InvalidBinEdgesError &);
  }

  void testRebinEndCentralBinEdgesInvalid() {
    std::vector<double> binEdges{1, 2, 3, 4, 5, 5};
    BinEdges edges(binEdges);

    TS_ASSERT_THROWS(rebin(getCountsHistogram(), edges),
                     const InvalidBinEdgesError &);
  }

  void testNegativeBinEdges() {
    auto hist = Histogram(BinEdges(3, LinearGenerator(-3, 3)), Counts{20, 10},
                          CountStandardDeviations{4.4721, 3.1622});
    std::vector<double> binEdges{-3, -2, -1, 0, 1, 2, 3};
    BinEdges edges(std::move(binEdges));

    TS_ASSERT_THROWS_NOTHING(rebin(getCountsHistogram(), edges));
  }

  void testRebinFailsInputBinEdgesInvalid() {
    std::vector<double> binEdges{1, 2, 3, 3, 5, 7};
    Histogram hist(BinEdges(std::move(binEdges)), Counts(5, 10));
    BinEdges edges{1, 2, 3, 4, 5, 6};

    TS_ASSERT_THROWS(rebin(hist, edges), const InvalidBinEdgesError &);
  }

  void testRebinFailsInputAndOutputBinEdgesInvalid() {
    std::vector<double> binEdges{1, 2, 3, 3, 5, 7};
    Histogram hist(BinEdges(binEdges), Counts(5, 10));
    BinEdges edges(std::move(binEdges));

    TS_ASSERT_THROWS(rebin(hist, edges), const InvalidBinEdgesError &);
  }

  void testRebinIdenticalBins() {
    auto histCounts = getCountsHistogram();
    auto histFreq = getFrequencyHistogram();

    auto outCounts = rebin(histCounts, histCounts.binEdges());
    auto outFreq = rebin(histFreq, histFreq.binEdges());

    TS_ASSERT_EQUALS(outCounts.x(), histCounts.x());
    TS_ASSERT_EQUALS(outCounts.y(), histCounts.y());
    TS_ASSERT_EQUALS(outCounts.e(), histCounts.e());

    TS_ASSERT_EQUALS(outFreq.x(), histFreq.x());
    TS_ASSERT_EQUALS(outFreq.y(), histFreq.y());
    TS_ASSERT_EQUALS(outFreq.e(), histFreq.e());
  }

  void testBinEdgesOutsideInputBins() {
    auto histCounts = getCountsHistogram();
    auto histFreq = getFrequencyHistogram();

    auto outCounts = rebin(histCounts, BinEdges(10, LinearGenerator(30, 1)));
    auto outFreq = rebin(histFreq, BinEdges(10, LinearGenerator(30, 2)));

    TS_ASSERT(std::all_of(outCounts.y().cbegin(), outCounts.y().cend(),
                          [](const double i) { return i == 0; }));
    TS_ASSERT(std::all_of(outCounts.e().cbegin(), outCounts.e().cend(),
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
      TS_ASSERT_DELTA(outCounts.e()[i], std::sqrt(5), 1e-14);
      TS_ASSERT_EQUALS(outFreq.y()[i], 12.0);
      TS_ASSERT_DELTA(outFreq.e()[i],
                      std::sqrt(outFreq.y()[i] / (edges[i + 1] - edges[i])),
                      1e-14);
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
      TS_ASSERT_DELTA(outCounts.e()[i], std::sqrt(outCounts.y()[i]), 1e-14);
      TS_ASSERT_EQUALS(outFreq.y()[i],
                       (histFreq.y()[2 * i] + histFreq.y()[(2 * i) + 1]) / 2);
      TS_ASSERT_DELTA(outFreq.e()[i], std::sqrt(outFreq.y()[i] / 2), 1e-14);
    }
  }

  void testSplitBinsAsymmetric() {
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

    for (size_t i = 0; i < outCounts.y().size(); i++) {
      TS_ASSERT_DELTA(outCounts.e()[i], std::sqrt(outCounts.y()[i]), 1e-14);
      TS_ASSERT_DELTA(outFreq.e()[i],
                      std::sqrt(outFreq.y()[i] / (edges[i + 1] - edges[i])),
                      1e-14);
    }
  }

  void testCombineBinsAsymmetric() {
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

    TS_ASSERT_DELTA(outCounts.e()[0], std::sqrt(outCounts.y()[0]), 1e-14);
    TS_ASSERT_DELTA(outCounts.e()[1], std::sqrt(outCounts.y()[1]), 1e-14);

    TS_ASSERT_DELTA(outFreq.e()[0],
                    std::sqrt(((histFreq.y()[0] / 2) + histFreq.y()[1]) / 2),
                    1e-14);
    TS_ASSERT_DELTA(outFreq.e()[1],
                    std::sqrt(((histFreq.y()[2] / 2) + histFreq.y()[1]) / 2),
                    1e-14);
  }

  void testSplitCombineBinsAsymmetric() {
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

    for (size_t i = 0; i < outCounts.y().size(); i++) {
      TS_ASSERT_DELTA(outCounts.e()[i], std::sqrt(outCounts.y()[i]), 1e-14);
      TS_ASSERT_DELTA(outFreq.e()[i],
                      std::sqrt(outFreq.y()[i] / (edges[i + 1] - edges[i])),
                      1e-14);
    }
  }

  void testSplitCombineBinsAsymmetric2() {
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

    TS_ASSERT_DELTA(outCounts.e()[0], std::sqrt(outCounts.y()[0]), 1e-14);
    TS_ASSERT_DELTA(outCounts.e()[1], std::sqrt(outCounts.y()[1]), 1e-14);
    TS_ASSERT_DELTA(outCounts.e()[2], std::sqrt(outCounts.y()[2]), 1e-14);

    TS_ASSERT_DELTA(
        outFreq.e()[0],
        std::sqrt(((histFreq.y()[0] / 2) + (histFreq.y()[1] * 2)) / 2), 1e-14);
    TS_ASSERT_DELTA(outFreq.e()[1], std::sqrt(histFreq.y()[1] * 2), 1e-14);
    TS_ASSERT_DELTA(
        outFreq.e()[2],
        std::sqrt(((histFreq.y()[2] / 2) + (histFreq.y()[1] * 2)) / 2), 1e-14);
  }

  void testSmallerBinsAsymmetric() {
    // Handles the case where
    // | | | |   becomes:
    //  | | |
    Histogram hist(BinEdges{0, 1, 2, 3}, Counts{15, 35, 9});
    Histogram histFreq(BinEdges{0, 1, 2, 3}, Frequencies{17, 8, 15});
    BinEdges edges{0.5, 1.5, 2.5};

    auto outCounts = rebin(hist, edges);
    auto outFreq = rebin(histFreq, edges);

    TS_ASSERT_EQUALS(outCounts.y()[0], (hist.y()[0] + hist.y()[1]) / 2);
    TS_ASSERT_EQUALS(outCounts.y()[1], (hist.y()[1] + hist.y()[2]) / 2);

    TS_ASSERT_EQUALS(outFreq.y()[0], (histFreq.y()[0] + histFreq.y()[1]) / 2);
    TS_ASSERT_EQUALS(outFreq.y()[1], (histFreq.y()[1] + histFreq.y()[2]) / 2);

    for (size_t i = 0; i < outCounts.y().size(); i++) {
      TS_ASSERT_DELTA(outCounts.e()[i], std::sqrt(outCounts.y()[i]), 1e-14);
      TS_ASSERT_DELTA(outFreq.e()[i], std::sqrt(outFreq.y()[i]), 1e-14);
    }
  }

  void testLargerRangeAsymmetric() {
    // Handles the case where
    //  | | |    becomes:
    // | | | |
    Histogram hist(BinEdges{0.5, 1.5, 2.5}, Counts{11, 23});
    Histogram histFreq(BinEdges{0.5, 1.5, 2.5}, Frequencies{100, 14});
    BinEdges edges{0, 1, 2, 3};

    auto outCounts = rebin(hist, edges);
    auto outFreq = rebin(histFreq, edges);

    TS_ASSERT_EQUALS(outCounts.y()[0], hist.y()[0] / 2);
    TS_ASSERT_EQUALS(outCounts.y()[1], (hist.y()[0] + hist.y()[1]) / 2);
    TS_ASSERT_EQUALS(outCounts.y()[2], hist.y()[1] / 2);

    TS_ASSERT_EQUALS(outFreq.y()[0], histFreq.y()[0] / 2);
    TS_ASSERT_EQUALS(outFreq.y()[1], (histFreq.y()[0] + histFreq.y()[1]) / 2);
    TS_ASSERT_EQUALS(outFreq.y()[2], histFreq.y()[1] / 2);

    TS_ASSERT_DELTA(outCounts.e()[0], std::sqrt(outCounts.y()[0]), 1e-14);
    TS_ASSERT_DELTA(outCounts.e()[1], std::sqrt(outCounts.y()[1]), 1e-14);
    TS_ASSERT_DELTA(outCounts.e()[2], std::sqrt(outCounts.y()[2]), 1e-14);

    TS_ASSERT_DELTA(outFreq.e()[0], std::sqrt(outFreq.y()[0]), 1e-14);
    TS_ASSERT_DELTA(outFreq.e()[1], std::sqrt(outFreq.y()[1]), 1e-14);
    TS_ASSERT_DELTA(outFreq.e()[2], std::sqrt(outFreq.y()[2]), 1e-14);
  }

  void testSmallerBinsSymmetric() {
    // Handles the case where
    //  | | | | becomes:
    //	  | |
    Histogram hist(BinEdges{0, 1, 2, 3}, Counts{15, 35, 9});
    Histogram histFreq(BinEdges{0, 1, 2, 3}, Frequencies{17, 8, 15});
    BinEdges edges{1, 2};

    auto outCounts = rebin(hist, edges);
    auto outFreq = rebin(histFreq, edges);

    TS_ASSERT_EQUALS(outCounts.y()[0], hist.y()[1]);
    TS_ASSERT_EQUALS(outCounts.e()[0], hist.e()[1]);

    TS_ASSERT_EQUALS(outFreq.y()[0], histFreq.y()[1]);
    TS_ASSERT_EQUALS(outFreq.e()[0], histFreq.e()[1]);
  }

  void testLargerBinsSymmetric() {
    // Handles the case where
    //  | |    becomes:
    //| | | |
    Histogram hist(BinEdges{1, 2}, Counts{20});
    Histogram histFreq(BinEdges{1, 2}, Frequencies{13});
    BinEdges edges{0, 1, 2, 3};

    auto outCounts = rebin(hist, edges);
    auto outFreq = rebin(histFreq, edges);

    TS_ASSERT_EQUALS(outCounts.y()[0], 0);
    TS_ASSERT_EQUALS(outCounts.y()[1], hist.y()[0]);
    TS_ASSERT_EQUALS(outCounts.y()[2], 0);

    TS_ASSERT_EQUALS(outCounts.e()[0], 0);
    TS_ASSERT_DELTA(outCounts.e()[1], std::sqrt(outCounts.y()[1]), 1e-14);
    TS_ASSERT_EQUALS(outCounts.e()[2], 0);

    TS_ASSERT_EQUALS(outFreq.y()[0], 0);
    TS_ASSERT_EQUALS(outFreq.y()[1], histFreq.y()[0]);
    TS_ASSERT_EQUALS(outFreq.y()[2], 0);

    TS_ASSERT_EQUALS(outFreq.e()[0], 0);
    TS_ASSERT_DELTA(outFreq.e()[1], std::sqrt(outFreq.y()[1]), 1e-14);
    TS_ASSERT_EQUALS(outFreq.e()[2], 0);
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

class RebinTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RebinTestPerformance *createSuite() {
    return new RebinTestPerformance();
  }
  static void destroySuite(RebinTestPerformance *suite) { delete suite; }

  RebinTestPerformance()
      : hist(BinEdges(binSize, LinearGenerator(0, 1))),
        histFreq(BinEdges(binSize, LinearGenerator(0, 1))),
        smBins(binSize * 2, LinearGenerator(0, 0.5)),
        lgBins(binSize / 2, LinearGenerator(0, 2)) {
    setupOutput();
  }

  void testRebinCountsSmallerBins() {
    for (size_t i = 0; i < nIters; i++)
      rebin(hist, smBins);
  }

  void testRebinFrequenciesSmallerBins() {
    for (size_t i = 0; i < nIters; i++)
      rebin(histFreq, smBins);
  }

  void testRebinCountsLargerBins() {
    for (size_t i = 0; i < nIters; i++)
      rebin(hist, lgBins);
  }

  void testRebinFrequenciesLargerBins() {
    for (size_t i = 0; i < nIters; i++)
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
    Counts counts(binSize - 1, 0);
    CountStandardDeviations countErrors(counts.size(), 0);
    Frequencies freqs(binSize - 1, 0);
    FrequencyStandardDeviations freqErrors(freqs.size(), 0);

    hist.setCounts(counts);
    hist.setCountStandardDeviations(countErrors);
    histFreq.setFrequencies(freqs);
    histFreq.setFrequencyStandardDeviations(freqErrors);
  }
};

#endif /* MANTID_HISTOGRAMDATA_REBINTEST_H_ */