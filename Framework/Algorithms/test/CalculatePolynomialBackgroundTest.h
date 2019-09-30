// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CALCULATEPOLYNOMIALBACKGROUNDTEST_H_
#define MANTID_ALGORITHMS_CALCULATEPOLYNOMIALBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CalculatePolynomialBackground.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;

class CalculatePolynomialBackgroundTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculatePolynomialBackgroundTest *createSuite() {
    return new CalculatePolynomialBackgroundTest();
  }
  static void destroySuite(CalculatePolynomialBackgroundTest *suite) {
    delete suite;
  }

  CalculatePolynomialBackgroundTest() { API::FrameworkManager::Instance(); }

  void test_Init() {
    Algorithms::CalculatePolynomialBackground alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_successfulExecutionWithDefaultParameters() {
    using namespace WorkspaceCreationHelper;
    const auto nHist = 2;
    const auto nBin = 2;
    auto ws = create2DWorkspaceWhereYIsWorkspaceIndex(nHist, nBin + 1);
    auto alg = makeAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("OutputWorkspace", "outputWS"))
    TS_ASSERT_THROWS_NOTHING(alg->execute())
    TS_ASSERT(alg->isExecuted())
  }

  void test_constantBackground() {
    using namespace WorkspaceCreationHelper;
    const size_t nHist{2};
    const size_t nBin{3};
    auto ws = create2DWorkspaceWhereYIsWorkspaceIndex(nHist, nBin);
    for (size_t histI = 0; histI < nHist; ++histI) {
      ws->setCountVariances(histI, nBin, static_cast<double>(histI + 1));
    }
    auto alg = makeAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("OutputWorkspace", "outputWS"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("Degree", 0))
    TS_ASSERT_THROWS_NOTHING(alg->execute())
    TS_ASSERT(alg->isExecuted())
    API::MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
    TS_ASSERT(outWS)
    for (size_t histI = 0; histI < nHist; ++histI) {
      const auto &ys = ws->y(histI);
      const auto &xs = ws->x(histI);
      const auto &bkgYs = outWS->y(histI);
      const auto &bkgEs = outWS->e(histI);
      const auto &bkgXs = outWS->x(histI);
      for (size_t binI = 0; binI < nBin; ++binI) {
        TS_ASSERT_DELTA(bkgYs[binI], ys[binI], 1e-12)
        TS_ASSERT_EQUALS(bkgEs[binI], 0)
        TS_ASSERT_EQUALS(bkgXs[binI], xs[binI])
      }
    }
  }

  void test_linearBackground() {
    using namespace WorkspaceCreationHelper;
    const size_t nHist{2};
    const size_t nBin{3};
    auto ws = create2DWorkspaceWhereYIsWorkspaceIndex(nHist, nBin);
    for (size_t histI = 0; histI < nHist; ++histI) {
      ws->setCountVariances(histI, nBin, static_cast<double>(histI + 1));
    }
    auto alg = makeAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("OutputWorkspace", "outputWS"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("Degree", 1))
    TS_ASSERT_THROWS_NOTHING(alg->execute())
    TS_ASSERT(alg->isExecuted())
    API::MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
    TS_ASSERT(outWS)
    for (size_t histI = 0; histI < nHist; ++histI) {
      const auto &ys = ws->y(histI);
      const auto &xs = ws->x(histI);
      const auto &bkgYs = outWS->y(histI);
      const auto &bkgEs = outWS->e(histI);
      const auto &bkgXs = outWS->x(histI);
      for (size_t binI = 0; binI < nBin; ++binI) {
        TS_ASSERT_DELTA(bkgYs[binI], ys[binI], 1e-10)
        TS_ASSERT_EQUALS(bkgEs[binI], 0)
        TS_ASSERT_EQUALS(bkgXs[binI], xs[binI])
      }
    }
  }

  void test_costFunctionLeastSquares() {
    using namespace WorkspaceCreationHelper;
    const size_t nHist{2};
    const HistogramData::Counts counts{0, 4, 0, 0};
    const HistogramData::CountStandardDeviations stdDevs{0, 0.001, 0, 0};
    const HistogramData::BinEdges edges{0, 1, 2, 3, 4};
    API::MatrixWorkspace_sptr ws(
        DataObjects::create<DataObjects::Workspace2D>(
            nHist, HistogramData::Histogram(edges, counts, stdDevs))
            .release());
    auto alg = makeAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("OutputWorkspace", "outputWS"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("Degree", 0))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("CostFunction", "Least squares"))
    TS_ASSERT_THROWS_NOTHING(alg->execute())
    TS_ASSERT(alg->isExecuted())
    API::MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
    TS_ASSERT(outWS)
    for (size_t histI = 0; histI < nHist; ++histI) {
      const auto &xs = ws->x(histI);
      const auto &bkgYs = outWS->y(histI);
      const auto &bkgEs = outWS->e(histI);
      const auto &bkgXs = outWS->x(histI);
      for (size_t binI = 0; binI < counts.size(); ++binI) {
        // Number 4 in counts is heavily weighted by the small error.
        TS_ASSERT_DELTA(bkgYs[binI], 4, 1e-4)
        TS_ASSERT_EQUALS(bkgEs[binI], 0)
        TS_ASSERT_EQUALS(bkgXs[binI], xs[binI])
      }
    }
  }

  void test_costFunctionUnweightedLeastSquares() {
    using namespace WorkspaceCreationHelper;
    const size_t nHist{2};
    const HistogramData::Counts counts{0, 4, 0, 0};
    const HistogramData::BinEdges edges{0, 1, 2, 3, 4};
    API::MatrixWorkspace_sptr ws(
        DataObjects::create<DataObjects::Workspace2D>(
            nHist, HistogramData::Histogram(edges, counts))
            .release());
    auto alg = makeAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("OutputWorkspace", "outputWS"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("Degree", 0))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("CostFunction", "Unweighted least squares"))
    TS_ASSERT_THROWS_NOTHING(alg->execute())
    TS_ASSERT(alg->isExecuted())
    API::MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
    TS_ASSERT(outWS)
    // Unweighted fitting of zeroth order polynomial is equivalent to the mean.
    const double result = std::accumulate(counts.cbegin(), counts.cend(), 0.0) /
                          static_cast<double>(counts.size());
    for (size_t histI = 0; histI < nHist; ++histI) {
      const auto &xs = ws->x(histI);
      const auto &bkgYs = outWS->y(histI);
      const auto &bkgEs = outWS->e(histI);
      const auto &bkgXs = outWS->x(histI);
      for (size_t binI = 0; binI < counts.size(); ++binI) {
        TS_ASSERT_DELTA(bkgYs[binI], result, 1e-5)
        TS_ASSERT_EQUALS(bkgEs[binI], 0)
        TS_ASSERT_EQUALS(bkgXs[binI], xs[binI])
      }
    }
  }

  void test_cubicBackgroundWithNoisyData() {
    const double xMin{1000.0};
    const double xMax{5000.0};
    const double xStep{10.0};
    const auto nBins = static_cast<size_t>((xMax - xMin) / xStep);
    HistogramData::BinEdges edges(nBins + 1);
    {
      auto &bins = edges.mutableRawData();
      for (size_t i = 0; i < bins.size(); ++i) {
        bins[i] = xMin + xStep * static_cast<double>(i);
      }
    }
    HistogramData::Counts counts(nBins);
    {
      auto &ys = counts.mutableRawData();
      for (size_t i = 0; i < ys.size(); ++i) {
        // The noise is not random, but a high frequency sinusoidal wave.
        const auto x = edges[i] + xStep / 2;
        ys[i] = 1000.0 + std::sin(x / 1000.0) + 0.5 * std::sin(x / 40.0);
      }
    }
    HistogramData::Histogram h{edges, counts};
    API::MatrixWorkspace_sptr ws =
        DataObjects::create<DataObjects::Workspace2D>(1, h);
    auto alg = makeAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("OutputWorkspace", "outputWS"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("Degree", 3))
    TS_ASSERT_THROWS_NOTHING(alg->execute())
    TS_ASSERT(alg->isExecuted())
    API::MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
    TS_ASSERT(outWS)
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 1)
    const auto &outH = outWS->histogram(0);
    TS_ASSERT_EQUALS(outH.size(), h.size())
    for (size_t i = 0; i < h.size(); ++i) {
      TS_ASSERT_EQUALS(outH.x()[i], h.x()[i])
      const auto x = h.x()[i] + xStep / 2;
      // Now without the "noise".
      const auto y = 1000.0 + std::sin(x / 1000.0);
      const auto diff = std::abs(outH.y()[i] - y);
      TS_ASSERT_LESS_THAN(diff, 0.08)
      TS_ASSERT_EQUALS(outH.e()[i], 0)
    }
  }

  void test_rangesWithGap() {
    using namespace WorkspaceCreationHelper;
    const size_t nHist{1};
    const HistogramData::BinEdges edges{0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5};
    const auto nBin = edges.size() - 1;
    const HistogramData::Counts counts{1.0, 2.0, 0.0, 0.0, 5.0, 6.0};
    const HistogramData::Histogram h{edges, counts};
    auto ws = API::MatrixWorkspace_sptr(
        DataObjects::create<DataObjects::Workspace2D>(nHist, h));
    auto alg = makeAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("OutputWorkspace", "outputWS"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("Degree", 1))
    const std::vector<double> ranges{0.0, 2.5, 4.5, 7.0};
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("XRanges", ranges))
    TS_ASSERT_THROWS_NOTHING(alg->execute())
    TS_ASSERT(alg->isExecuted())
    API::MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
    TS_ASSERT(outWS)
    const auto &xs = ws->x(0);
    const auto &bkgYs = outWS->y(0);
    const auto &bkgEs = outWS->e(0);
    const auto &bkgXs = outWS->x(0);
    const std::vector<double> expected{1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    for (size_t binI = 0; binI < nBin; ++binI) {
      TS_ASSERT_DELTA(bkgYs[binI], expected[binI], 1e-10)
      TS_ASSERT_EQUALS(bkgEs[binI], 0)
      TS_ASSERT_EQUALS(bkgXs[binI], xs[binI])
    }
  }

private:
  static boost::shared_ptr<Algorithms::CalculatePolynomialBackground>
  makeAlgorithm() {
    auto a = boost::make_shared<Algorithms::CalculatePolynomialBackground>();
    a->initialize();
    a->setChild(true);
    a->setRethrows(true);
    return a;
  }
};

class CalculatePolynomialBackgroundTestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    Mantid::API::FrameworkManager::Instance();
    const size_t nBins = 1000;
    const size_t nHisto = 50000;
    const std::vector<double> y(nBins, 1.0);
    Mantid::HistogramData::Counts counts(y);
    std::vector<double> x(nBins + 1);
    std::iota(x.begin(), x.end(), 0.0);
    const Mantid::HistogramData::BinEdges edges(x);
    const Mantid::HistogramData::Histogram h(edges, counts);
    m_ws = Mantid::DataObjects::create<Mantid::DataObjects::Workspace2D>(nHisto,
                                                                         h);
    // The histograms in m_ws share the same Y and E values. We want unshared
    // histograms to performance-test possible cache trashing issues.
    for (size_t i = 0; i < m_ws->getNumberHistograms(); ++i) {
      const std::vector<double> data(m_ws->y(i).size(), 1.0);
      const Mantid::HistogramData::Counts y{data};
      const Mantid::HistogramData::CountStandardDeviations e{data};
      m_ws->setHistogram(i, edges, y, e);
    }
  }

  void test_zerothDegreePolynomial() {
    Mantid::Algorithms::CalculatePolynomialBackground alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", m_ws);
    alg.setProperty("OutputWorkspace", "__unused_because_child");
    alg.setProperty("Degree", 0);
    alg.execute();
  }

  void test_thirdDegreePolynomial() {
    Mantid::Algorithms::CalculatePolynomialBackground alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", m_ws);
    alg.setProperty("OutputWorkspace", "__unused_because_child");
    alg.setProperty("Degree", 3);
    alg.execute();
  }

private:
  Mantid::API::MatrixWorkspace_sptr m_ws;
};

#endif /* MANTID_ALGORITHMS_CALCULATEPOLYNOMIALBACKGROUNDTEST_H_ */
