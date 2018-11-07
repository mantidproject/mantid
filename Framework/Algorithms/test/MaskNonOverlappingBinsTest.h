// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MaskNonOverlappingBinsTEST_H_
#define MANTID_ALGORITHMS_MaskNonOverlappingBinsTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/MaskNonOverlappingBins.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/Counts.h"
#include "MantidHistogramData/LinearGenerator.h"

#include <array>

using namespace Mantid;

class MaskNonOverlappingBinsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaskNonOverlappingBinsTest *createSuite() {
    return new MaskNonOverlappingBinsTest();
  }
  static void destroySuite(MaskNonOverlappingBinsTest *suite) { delete suite; }

  void test_init() {
    Algorithms::MaskNonOverlappingBins alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_maskBegin() {
    HistogramData::BinEdges comparison{-1.1, -0.1};
    auto const expected = API::MatrixWorkspace::MaskList{{1, 1.}, {2, 1.}};
    runTestWithAlwaysSameExpectedOutcome(std::move(comparison), expected);
  }

  void test_maskCentre() {
    HistogramData::BinEdges comparison{-0.1, 0.9};
    auto const expected = API::MatrixWorkspace::MaskList{{0, 1.}, {2, 1.}};
    runTestWithAlwaysSameExpectedOutcome(std::move(comparison), expected);
  }

  void test_maskEnd() {
    HistogramData::BinEdges comparison{0.9, 1.8};
    auto const expected = API::MatrixWorkspace::MaskList{{0, 1.}, {1, 1.}};
    runTestWithAlwaysSameExpectedOutcome(std::move(comparison), expected);
  }

  void test_maskAll() {
    HistogramData::BinEdges comparison{-13., -1.1};
    auto const expected =
        API::MatrixWorkspace::MaskList{{0, 1.}, {1, 1.}, {2, 1.}};
    runTestWithAlwaysSameExpectedOutcome(std::move(comparison), expected);
    comparison = {1.8, 13.};
    runTestWithAlwaysSameExpectedOutcome(std::move(comparison), expected);
  }

  void test_partialOverlapMasking() {
    HistogramData::BinEdges const comparison{0., 0.1};
    auto expected = API::MatrixWorkspace::MaskList{{0, 1.}, {1, 1.}, {2, 1.}};
    runTestWithMatchingBins(comparison, expected, true);
    expected = {{0, 1.}, {2, 1.}};
    runTestWithMatchingBins(comparison, expected, false);
  }

  void test_maskNone() {
    HistogramData::BinEdges const comparison{-13., 13.};
    auto const expected = API::MatrixWorkspace::MaskList();
    runTestWithAlwaysSameExpectedOutcome(comparison, expected);
  }

  void test_unsortedXThrows() {
    API::MatrixWorkspace_sptr inputWS =
        makeWorkspace(HistogramData::BinEdges{-1.1, -0.1, 0.2, 1.8});
    inputWS->mutableX(0)[2] = -0.9;
    API::MatrixWorkspace_sptr comparisonWS =
        makeWorkspace(HistogramData::BinEdges{-1.1, 1.8});
    Algorithms::MaskNonOverlappingBins alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("ComparisonWorkspace", comparisonWS))
    TS_ASSERT_THROWS_EQUALS(alg.execute(), std::invalid_argument const &e,
                            e.what(),
                            std::string("InputWorkspace has unsorted X."))
    TS_ASSERT(!alg.isExecuted())
  }

private:
  template <typename BinEdges>
  static API::MatrixWorkspace_sptr makeWorkspace(BinEdges &&binEdges) {
    HistogramData::BinEdges edges(std::forward<BinEdges>(binEdges));
    HistogramData::Counts counts(edges.size() - 1, 2);
    return DataObjects::create<DataObjects::Workspace2D>(
        1, HistogramData::Histogram(edges, counts));
  }

  static void runTestWithAlwaysSameExpectedOutcome(
      HistogramData::BinEdges comparisonBinEdges,
      API::MatrixWorkspace::MaskList const &expected) {
    runTestWithMatchingBins(comparisonBinEdges, expected, true);
    runTestWithMatchingBins(std::move(comparisonBinEdges), expected, false);
  }

  template <typename BinEdges>
  static void
  runTestWithMatchingBins(BinEdges &&comparisonBinEdges,
                          API::MatrixWorkspace::MaskList const &expected,
                          bool const maskPartial) {
    API::MatrixWorkspace_sptr inputWS =
        makeWorkspace(HistogramData::BinEdges{-1.1, -0.1, 0.9, 1.8});
    API::MatrixWorkspace_sptr comparisonWS =
        makeWorkspace(std::forward<BinEdges>(comparisonBinEdges));
    std::array<std::string, 3> raggedOptions{
        {"Check", "Ragged", "Common Bins"}};
    for (auto const &raggedness : raggedOptions) {
      Algorithms::MaskNonOverlappingBins alg;
      alg.setChild(true);
      alg.setRethrows(true);
      TS_ASSERT_THROWS_NOTHING(alg.initialize())
      TS_ASSERT(alg.isInitialized())
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
      TS_ASSERT_THROWS_NOTHING(
          alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
      TS_ASSERT_THROWS_NOTHING(
          alg.setProperty("ComparisonWorkspace", comparisonWS))
      TS_ASSERT_THROWS_NOTHING(
          alg.setProperty("MaskPartiallyOverlapping", maskPartial))
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("RaggedInputs", raggedness))
      TS_ASSERT_THROWS_NOTHING(alg.execute())
      TS_ASSERT(alg.isExecuted())
      API::MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
      TS_ASSERT(outputWS);
      TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1)
      if (!expected.empty()) {
        auto const mask = outputWS->maskedBins(0);
        TS_ASSERT_EQUALS(mask, expected)
      } else {
        TS_ASSERT(!outputWS->hasMaskedBins(0))
      }
    }
  }
};

class MaskNonOverlappingBinsTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaskNonOverlappingBinsTestPerformance *createSuite() {
    return new MaskNonOverlappingBinsTestPerformance();
  }
  static void destroySuite(MaskNonOverlappingBinsTestPerformance *suite) {
    delete suite;
  }

  void setUp() override {
    HistogramData::BinEdges edges(1000,
                                  HistogramData::LinearGenerator(-100., 23.));
    HistogramData::Counts counts(edges.size() - 1, 2);
    m_ws = DataObjects::create<DataObjects::Workspace2D>(
        10000, HistogramData::Histogram(edges, counts));
    edges =
        HistogramData::BinEdges(200, HistogramData::LinearGenerator(-10., 2.3));
    counts = HistogramData::Counts(edges.size() - 1, 2);
    m_compWS = DataObjects::create<DataObjects::Workspace2D>(
        10000, HistogramData::Histogram(edges, counts));
    m_alg.initialize();
    m_alg.setChild(true);
    m_alg.setRethrows(true);
    m_alg.setProperty("InputWorkspace", m_ws);
    m_alg.setPropertyValue("OutputWorkspace", "_unused_for_child");
    m_alg.setProperty("ComparisonWorkspace", m_compWS);
    m_alg.setProperty("MaskPartiallyOverlapping", true);
  }

  void test_default() { TS_ASSERT_THROWS_NOTHING(m_alg.execute()) }

  void test_nonragged() {
    m_alg.setProperty("CheckSortedX", false);
    m_alg.setProperty("RaggedInputs", "Common Bins");
    TS_ASSERT_THROWS_NOTHING(m_alg.execute())
  }

  void test_ragged() {
    m_alg.setProperty("CheckSortedX", false);
    m_alg.setProperty("RaggedInputs", "Ragged");
    TS_ASSERT_THROWS_NOTHING(m_alg.execute())
  }

private:
  API::MatrixWorkspace_sptr m_ws;
  API::MatrixWorkspace_sptr m_compWS;
  Algorithms::MaskNonOverlappingBins m_alg;
};

#endif /* MANTID_ALGORITHMS_MaskNonOverlappingBinsTEST_H_ */
