// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MASKBINSIFTEST_H_
#define MANTID_ALGORITHMS_MASKBINSIFTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/CreateWorkspace.h"
#include "MantidAlgorithms/MaskBinsIf.h"

using Mantid::Algorithms::CreateSampleWorkspace;
using Mantid::Algorithms::CreateWorkspace;
using Mantid::Algorithms::MaskBinsIf;
using Mantid::API::MatrixWorkspace_sptr;

class MaskBinsIfTest : public CxxTest::TestSuite {
private:
  MatrixWorkspace_sptr createWorkspace() {
    CreateWorkspace creator;
    creator.initialize();
    creator.setChild(true);
    creator.setAlwaysStoreInADS(false);
    const std::vector<double> x = {1.1,  2.5,  3.2,  4.5,  6.7,  8.9,
                                   10.3, 12.4, 13.9, 14.1, 15.3, 16.8};
    const std::vector<double> y = {7,  23, 54, 34, 23, 64,
                                   34, 23, 58, 63, 34, 25};
    const std::vector<double> e = {3.2, 2.1, 8.4, 3.5, 6.3, 4.7,
                                   4.9, 3.6, 4.1, 6.7, 5.1, 3.2};
    const std::vector<double> dx = {0.1, 0.2, 0.4, 0.7, 0.9, 1.3,
                                    1.5, 1.7, 1.9, 1.2, 4.5, 2.3};
    const std::vector<std::string> spectrumAxis = {"3", "7", "11", "17"};
    creator.setProperty("DataX", x);
    creator.setProperty("DataY", y);
    creator.setProperty("DataE", e);
    creator.setProperty("Dx", dx);
    creator.setProperty("NSpec", 4);
    creator.setProperty("VerticalAxisValues", spectrumAxis);
    creator.setProperty("VerticalAxisUnit", "Label");
    creator.setProperty("OutputWorkspace", "__unused");
    creator.execute();
    MatrixWorkspace_sptr ws = creator.getProperty("OutputWorkspace");
    return ws;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaskBinsIfTest *createSuite() { return new MaskBinsIfTest(); }
  static void destroySuite(MaskBinsIfTest *suite) { delete suite; }

  void test_init() {
    MaskBinsIf alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    MaskBinsIf alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setAlwaysStoreInADS(false);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    const auto inputWS = createWorkspace();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "__unused_for_child"));
    const std::string criterion = "y>50 || e>6 || s<5 || dx>1.6";
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Criterion", criterion));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    Mantid::API::MatrixWorkspace_sptr outputWS =
        alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    const auto maskedBins1 = outputWS->maskedBins(0);
    TS_ASSERT(maskedBins1.find(0) != maskedBins1.end());
    TS_ASSERT(maskedBins1.find(1) != maskedBins1.end());
    TS_ASSERT(maskedBins1.find(2) != maskedBins1.end());
    const auto maskedBins2 = outputWS->maskedBins(1);
    TS_ASSERT(maskedBins2.find(0) == maskedBins2.end());
    TS_ASSERT(maskedBins2.find(1) != maskedBins2.end());
    TS_ASSERT(maskedBins2.find(2) != maskedBins2.end());
    const auto maskedBins3 = outputWS->maskedBins(2);
    TS_ASSERT(maskedBins3.find(0) == maskedBins3.end());
    TS_ASSERT(maskedBins3.find(1) != maskedBins3.end());
    TS_ASSERT(maskedBins3.find(2) != maskedBins3.end());
    const auto maskedBins4 = outputWS->maskedBins(3);
    TS_ASSERT(maskedBins4.find(0) != maskedBins4.end());
    TS_ASSERT(maskedBins4.find(1) != maskedBins4.end());
    TS_ASSERT(maskedBins4.find(2) != maskedBins4.end());
  }
};

class MaskBinsIfTestPerformance : public CxxTest::TestSuite {
public:
  static MaskBinsIfTestPerformance *createSuite() {
    return new MaskBinsIfTestPerformance();
  }
  static void destroySuite(MaskBinsIfTestPerformance *suite) { delete suite; }

  void setUp() override {
    Mantid::API::FrameworkManager::Instance();
    CreateSampleWorkspace creator;
    creator.initialize();
    creator.setChild(true);
    creator.setAlwaysStoreInADS(false);
    creator.setProperty("BankPixelWidth", 100);
    creator.setProperty("NumBanks", 20);
    creator.setProperty("BinWidth", 200.);
    creator.setProperty("Random", true);
    creator.setPropertyValue("OutputWorkspace", "__unused");
    creator.execute();
    Mantid::API::MatrixWorkspace_sptr ws =
        creator.getProperty("OutputWorkspace");
    m_alg.initialize();
    m_alg.setChild(true);
    m_alg.setAlwaysStoreInADS(false);
    m_alg.setProperty("InputWorkspace", ws);
    m_alg.setPropertyValue("Criterion", "y>100 || y<1");
    m_alg.setProperty("OutputWorkspace", "__out");
  }

  void test_performance() { m_alg.execute(); }

private:
  MaskBinsIf m_alg;
};

#endif /* MANTID_ALGORITHMS_MASKBINSIFTEST_H_ */
