// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_DEADTIMECORRECTIONTEST_H_
#define MANTID_ALGORITHMS_DEADTIMECORRECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/DeadTimeCorrection.h"

using Mantid::API::FrameworkManager;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::Algorithms::CreateSampleWorkspace;
using Mantid::Algorithms::DeadTimeCorrection;

namespace {
MatrixWorkspace_sptr createWorkspace(const int nPixelsPerBank = 3,
                                     const int nBins = 2,
                                     const int numBanks = 2) {
  CreateSampleWorkspace creator;
  creator.initialize();
  creator.setChild(true);
  creator.setAlwaysStoreInADS(false);
  creator.setProperty("NumBanks", numBanks);
  creator.setProperty("XMin", 1.);
  creator.setProperty("XMax", 2.);
  creator.setProperty("BinWidth", 1. / nBins);
  creator.setProperty("BankPixelWidth", nPixelsPerBank);
  creator.setPropertyValue("OutputWorkspace", "__unused");
  creator.execute();
  MatrixWorkspace_sptr in = creator.getProperty("OutputWorkspace");
  return in;
}
} // namespace

class DeadTimeCorrectionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DeadTimeCorrectionTest *createSuite() {
    return new DeadTimeCorrectionTest();
  }
  static void destroySuite(DeadTimeCorrectionTest *suite) { delete suite; }

  DeadTimeCorrectionTest() { FrameworkManager::Instance(); }

  void test_init() {
    DeadTimeCorrection alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    const double tau = 0.001;
    MatrixWorkspace_sptr in = createWorkspace();
    // we have 2 TOF bins, and will be grouping 9 pixels
    const double countrate = 9 * (in->readY(0)[0] + in->readY(0)[1]);
    const double expectation = 1. / (1. - tau * countrate);
    DeadTimeCorrection alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", in))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Tau", tau))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupingPattern", "0-8,9-17"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_sptr out = alg.getProperty("OutputWorkspace");
    TS_ASSERT(out)
    TS_ASSERT_EQUALS(out->getNumberHistograms(), in->getNumberHistograms())
    for (size_t index = 0; index < in->getNumberHistograms(); ++index) {
      const auto &yIn = in->readY(index);
      const auto &eIn = in->readE(index);
      const auto &yOut = out->readY(index);
      const auto &eOut = out->readE(index);
      TS_ASSERT_EQUALS(yIn.size(), yOut.size())
      TS_ASSERT_EQUALS(eIn.size(), eOut.size())
      for (size_t bin = 0; bin < yIn.size(); ++bin) {
        const double corrY = yOut[bin] / yIn[bin];
        const double corrE = eOut[bin] / eIn[bin];
        TS_ASSERT_DELTA(corrY, expectation, 1E-10)
        TS_ASSERT_DELTA(corrE, expectation, 1E-10)
      }
    }
  }
};

class DeadTimeCorrectionTestPerformance : public CxxTest::TestSuite {
public:
  static DeadTimeCorrectionTestPerformance *createSuite() {
    return new DeadTimeCorrectionTestPerformance();
  }
  static void destroySuite(DeadTimeCorrectionTestPerformance *suite) {
    delete suite;
  }

  DeadTimeCorrectionTestPerformance() { FrameworkManager::Instance(); }

  void setUp() override {
    MatrixWorkspace_sptr in = createWorkspace(100, 1000, 10);
    m_alg.initialize();
    m_alg.setChild(true);
    m_alg.setRethrows(true);
    m_alg.setProperty("InputWorkspace", in);
    m_alg.setPropertyValue("OutputWorkspace", "__unused");
    m_alg.setProperty("Tau", 0.0000001);
    m_alg.setPropertyValue("GroupingPattern",
                           "0-9999,10000-19999,20000-29999,30000-39999,40000-"
                           "49999,50000-59999,60000-"
                           "69999,70000-79999,80000-89999,90000-99999");
  }

  void test_performance() {
    for (size_t i = 0; i < 5; ++i) {
      TS_ASSERT_THROWS_NOTHING(m_alg.execute())
    }
  }

private:
  DeadTimeCorrection m_alg;
  MatrixWorkspace_sptr in;
};

#endif /* MANTID_ALGORITHMS_DEADTIMECORRECTIONTEST_H_ */
