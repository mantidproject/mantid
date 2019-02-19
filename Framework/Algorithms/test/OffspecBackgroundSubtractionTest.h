// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_OFFSPECBACKGROUNDSUBTRACTIONTEST_H_
#define MANTID_ALGORITHMS_OFFSPECBACKGROUNDSUBTRACTIONTEST_H_

#include "MantidAPI/FrameworkManager.h"
#include "MantidAlgorithms/OffspecBackgroundSubtraction.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include <algorithm>

using Mantid::Algorithms::OffspecBackgroundSubtraction;

class OffspecBackgroundSubtractionTest : public CxxTest::TestSuite {
private:
  MatrixWorkspace_sptr m_workspaceWithValues;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static OffspecBackgroundSubtractionTest *createSuite() {
    return new OffspecBackgroundSubtractionTest();
  }
  static void destroySuite(OffspecBackgroundSubtractionTest *suite) {
    delete suite;
  }

  OffspecBackgroundSubtractionTest() {
    FrameworkManager::Instance();
    // A workspace with 6 spectra, 3 bins
    // All Y values are equal to 2.0
    m_workspaceWithValues =
        WorkspaceCreationHelper::create2DWorkspaceWhereYIsWorkspaceIndex(6, 3);
  }

  void test_Init() {
    OffspecBackgroundSubtraction alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_executionWithNoBackground() {
    // Test the algorithm returns an error when no background range is set
    auto alg = setupAlgorithm();
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("InputWorkspace", m_workspaceWithValues))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("OutputWorkspace", "outputWS"))
    TS_ASSERT_THROWS_ANYTHING(alg->execute())
  }

  void test_executionWithBackgroundSet() {
    // Test the algorithm is executed with both background ranges set
    auto alg = setupAlgorithm();
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("InputWorkspace", m_workspaceWithValues))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("OutputWorkspace", "outputWS"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("BottomBackgroundRanges", "0"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("TopBackgroundRanges", "5"))
    TS_ASSERT(alg->execute())
  }

  void test_outputWithBackground() {
    // Test the algorithm is executed with both background ranges set
    auto alg = setupAlgorithm();
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("InputWorkspace", m_workspaceWithValues))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("OutputWorkspace", "outputWS"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("BottomBackgroundRanges", "1"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("TopBackgroundRanges", "5"))
    TS_ASSERT(alg->execute())

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
    TS_ASSERT(outWS)
    auto nHist = outWS->getNumberHistograms();
    for (size_t histI = 0; histI < nHist; ++histI) {
      const auto &xs = m_workspaceWithValues->x(histI);
      const auto &ys = m_workspaceWithValues->y(histI);
      const auto &bkgYs = outWS->y(histI);
      const auto &bkgXs = outWS->x(histI);
      auto counts = outWS->counts(histI);
      for (size_t binI = 0; binI < counts.size(); ++binI) {
        // Y values are inputY - (5+1)/2
        TS_ASSERT_EQUALS(bkgYs[binI], ys[binI] - 3.0)
        TS_ASSERT_EQUALS(bkgXs[binI], xs[binI])
      }
    }
  }

  void test_outputWithMultipleSpectraInBackground() {
    // Test the algorithm is executed with both background ranges set
    auto alg = setupAlgorithm();
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("InputWorkspace", m_workspaceWithValues))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("OutputWorkspace", "outputWS"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("BottomBackgroundRanges", "0,1"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("TopBackgroundRanges", "4,5"))
    TS_ASSERT(alg->execute())

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
    TS_ASSERT(outWS)
    auto nHist = outWS->getNumberHistograms();
    for (size_t histI = 0; histI < nHist; ++histI) {
      const auto &xs = m_workspaceWithValues->x(histI);
      const auto &ys = m_workspaceWithValues->y(histI);
      const auto &bkgYs = outWS->y(histI);
      const auto &bkgXs = outWS->x(histI);
      auto counts = outWS->counts(histI);
      for (size_t binI = 0; binI < counts.size(); ++binI) {
        // Y values are inputY - (0+1+4+5)/4
        TS_ASSERT_EQUALS(bkgYs[binI], ys[binI] - 2.5)
        TS_ASSERT_EQUALS(bkgXs[binI], xs[binI])
      }
    }
  }

  void test_executionWithBottomBackgroundSet() {
    // Test the algorithm is executed with just the bottom background ranges set
    auto alg = setupAlgorithm();
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("InputWorkspace", m_workspaceWithValues))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("OutputWorkspace", "outputWS"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("BottomBackgroundRanges", "1"))
    TS_ASSERT(alg->execute())

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
    TS_ASSERT(outWS)
    auto nHist = outWS->getNumberHistograms();
    for (size_t histI = 0; histI < nHist; ++histI) {
      const auto &xs = m_workspaceWithValues->x(histI);
      const auto &ys = m_workspaceWithValues->y(histI);
      const auto &bkgYs = outWS->y(histI);
      const auto &bkgXs = outWS->x(histI);
      auto counts = outWS->counts(histI);
      for (size_t binI = 0; binI < counts.size(); ++binI) {
        // Y values are inputY - 1/1
        TS_ASSERT_EQUALS(bkgYs[binI], ys[binI] - 1.0)
        TS_ASSERT_EQUALS(bkgXs[binI], xs[binI])
      }
    }
  }

  void test_executionWithTopBackgroundSet() {
    // // Test the algorithm is executed with just the top background ranges set
    auto alg = setupAlgorithm();
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("InputWorkspace", m_workspaceWithValues))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("OutputWorkspace", "outputWS"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("TopBackgroundRanges", "4"))
    TS_ASSERT(alg->execute())

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
    TS_ASSERT(outWS)
    auto nHist = outWS->getNumberHistograms();
    for (size_t histI = 0; histI < nHist; ++histI) {
      const auto &xs = m_workspaceWithValues->x(histI);
      const auto &ys = m_workspaceWithValues->y(histI);
      const auto &bkgYs = outWS->y(histI);
      const auto &bkgXs = outWS->x(histI);
      auto counts = outWS->counts(histI);
      for (size_t binI = 0; binI < counts.size(); ++binI) {
        // Y values are inputY - 4/1
        TS_ASSERT_EQUALS(bkgYs[binI], ys[binI] - 4.0)
        TS_ASSERT_EQUALS(bkgXs[binI], xs[binI])
      }
    }
  }

  void test_executionWithRangeOutOfOrder() {
    // // Test the algorithm is executed with just the top background ranges
    // set
    auto alg = setupAlgorithm();
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("InputWorkspace", m_workspaceWithValues))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("OutputWorkspace", "outputWS"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("BottomBackgroundRanges", "1,0"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("TopBackgroundRanges", "5,4,3"))
    TS_ASSERT(alg->execute())

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
    TS_ASSERT(outWS)
    auto nHist = outWS->getNumberHistograms();
    for (size_t histI = 0; histI < nHist; ++histI) {
      const auto &xs = m_workspaceWithValues->x(histI);
      const auto &ys = m_workspaceWithValues->y(histI);
      const auto &bkgYs = outWS->y(histI);
      const auto &bkgXs = outWS->x(histI);
      auto counts = outWS->counts(histI);
      for (size_t binI = 0; binI < counts.size(); ++binI) {
        // Y values are inputY - (1+0+5+4+3)/5
        TS_ASSERT_EQUALS(bkgYs[binI], ys[binI] - 13.0/5.0)
        TS_ASSERT_EQUALS(bkgXs[binI], xs[binI])
      }
    }
  }

private:
  static boost::shared_ptr<OffspecBackgroundSubtraction>
  setupAlgorithm() {
    auto alg = boost::make_shared<OffspecBackgroundSubtraction>();
    alg->initialize();
    alg->setChild(true);
    alg->setRethrows(true);
    return alg;
  }
};

#endif /* MANTID_ALGORITHMS_OFFSPECBACKGROUNDSUBTRACTIONTEST_H_ */