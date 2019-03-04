// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_AVERAGESPECTRUMBACKGROUNDTEST_H_
#define MANTID_ALGORITHMS_AVERAGESPECTRUMBACKGROUNDTEST_H_

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAlgorithms/AverageSpectrumBackground.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <algorithm>
#include <cxxtest/TestSuite.h>

using Mantid::Algorithms::AverageSpectrumBackground;
using namespace Mantid::API;

class AverageSpectrumBackgroundTest : public CxxTest::TestSuite {
private:
  MatrixWorkspace_sptr m_workspaceWithValues;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AverageSpectrumBackgroundTest *createSuite() {
    return new AverageSpectrumBackgroundTest();
  }
  static void destroySuite(AverageSpectrumBackgroundTest *suite) {
    delete suite;
  }

  AverageSpectrumBackgroundTest() {
    FrameworkManager::Instance();
    // A workspace with 6 spectra, 3 bins
    // All Y values are equal to 2.0
    m_workspaceWithValues =
        WorkspaceCreationHelper::create2DWorkspaceWhereYIsWorkspaceIndex(6, 3);
  }

  void test_Init() {
    AverageSpectrumBackground alg;
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
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("BottomBackgroundRange", "1,2"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("TopBackgroundRange", "4,5"))
    TS_ASSERT(alg->execute())
  }

  void test_outputWithBackground() {
    // Test the algorithm is executed with both background ranges set
    auto alg = setupAlgorithm();
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("InputWorkspace", m_workspaceWithValues))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("OutputWorkspace", "outputWS"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("BottomBackgroundRange", "1,2"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("TopBackgroundRange", "4,5"))
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
        // Y values are inputY - (4+1)/2
        TS_ASSERT_EQUALS(bkgYs[binI], ys[binI] - 2.5)
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
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("BottomBackgroundRange", "0,2"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("TopBackgroundRange", "3,5"))
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
        // Y values are inputY - (0+1+3+4)/4
        TS_ASSERT_EQUALS(bkgYs[binI], ys[binI] - 2.0)
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
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("BottomBackgroundRange", "1,2"))
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
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("TopBackgroundRange", "4,5"))
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

  void test_executionBadBottomRanges() {
    // Test the algorithm returns an error when more than 2 numbers are given for a range
    auto alg = setupAlgorithm();
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("InputWorkspace", m_workspaceWithValues))
	TS_ASSERT_THROWS_NOTHING(alg->setProperty("OutputWorkspace", "outputWS"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("BottomBackgroundRange", "1,2,3"))
    TS_ASSERT_THROWS_ANYTHING(alg->execute())
  }


   void test_executionBadTopRanges() {
    // Test the algorithm returns an error when more than 2 numbers are given
    // for a range
    auto alg = setupAlgorithm();
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("InputWorkspace", m_workspaceWithValues))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("OutputWorkspace", "outputWS"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("TopBackgroundRange", "1,2,3"))
    TS_ASSERT_THROWS_ANYTHING(alg->execute())
  }


private:
  static boost::shared_ptr<AverageSpectrumBackground> setupAlgorithm() {
    auto alg = boost::make_shared<AverageSpectrumBackground>();
    alg->initialize();
    alg->setChild(true);
    alg->setRethrows(true);
    return alg;
  }
};

#endif /* MANTID_ALGORITHMS_AVERAGESPECTRUMBACKGROUNDTEST_H_ */