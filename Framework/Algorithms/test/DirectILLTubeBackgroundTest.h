// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_DIRECTILLTUBEBACKGROUNDTEST_H_
#define MANTID_ALGORITHMS_DIRECTILLTUBEBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/DirectILLTubeBackground.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;

class DirectILLTubeBackgroundTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DirectILLTubeBackgroundTest *createSuite() {
    return new DirectILLTubeBackgroundTest();
  }
  static void destroySuite(DirectILLTubeBackgroundTest *suite) { delete suite; }

  DirectILLTubeBackgroundTest() : CxxTest::TestSuite() {
    API::FrameworkManager::Instance();
  }

  void test_Init() {
    Algorithms::DirectILLTubeBackground alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_Nondistribution() {
    API::MatrixWorkspace_sptr inWS =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            numBanks, numPixels, numBins);
    TS_ASSERT(inWS->isHistogramData())
    TS_ASSERT(!inWS->isDistribution())
    std::array<double, numBanks> bankBkgs{{2.33, 4.22}};
    constexpr double startX{9.};
    constexpr double deltaX{0.57};
    for (size_t i = 0; i < numBanks; ++i) {
      for (size_t j = 0; j < numSpectraPerBank; ++j) {
        auto const spectraIndex = i * numSpectraPerBank + j;
        auto &Xs = inWS->mutableX(spectraIndex);
        auto &Ys = inWS->mutableY(spectraIndex);
        Xs.front() = startX;
        for (size_t k = 1; k < Xs.size(); ++k) {
          Xs[k] = startX + static_cast<double>(k) * deltaX;
          Ys[k - 1] = bankBkgs[i];
        }
      }
    }
    auto eppWS = makeEPPWorkspace(*inWS);
    auto outWS = execAlgorithm(inWS, eppWS);
    TS_ASSERT(!outWS->isDistribution())
    for (size_t i = 0; i < numBanks; ++i) {
      for (size_t j = 0; j < numSpectraPerBank; ++j) {
        auto const &Ys = outWS->y(i * numSpectraPerBank + j);
        auto const &Es = outWS->e(i * numSpectraPerBank + j);
        for (size_t k = 0; k < Ys.size(); ++k) {
          TS_ASSERT_DELTA(Ys[k], bankBkgs[i], 1e-6)
          TS_ASSERT_EQUALS(Es[k], 0.)
        }
      }
    }
  }

  void test_NondistributionNonequidistandBinning() {
    API::MatrixWorkspace_sptr inWS =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            numBanks, numPixels, numBins);
    TS_ASSERT(inWS->isHistogramData())
    TS_ASSERT(!inWS->isDistribution())
    constexpr double startX{9.};
    constexpr double deltaX{0.57};
    std::array<double, numBanks> bankBkgs{{2.33, 4.22}};
    for (size_t i = 0; i < numBanks; ++i) {
      for (size_t j = 0; j < numSpectraPerBank; ++j) {
        auto &Ys = inWS->mutableY(i * numSpectraPerBank + j);
        auto &Xs = inWS->mutableX(i * numSpectraPerBank + j);
        Xs[0] = startX;
        for (size_t k = 1; k < Xs.size(); ++k) {
          auto const width = (static_cast<double>(k) * 0.1 + 1.) * deltaX;
          Xs[k] = Xs[k - 1] + width;
          Ys[k - 1] = bankBkgs[i] * width;
        }
        Ys[numBins / 2] = 1030.; // Peak.
      }
    }
    auto eppWS = makeEPPWorkspace(*inWS);
    auto outWS = execAlgorithm(inWS, eppWS);
    TS_ASSERT(!outWS->isDistribution())
    auto subtractedWS = inWS - outWS;
    for (size_t i = 0; i < subtractedWS->getNumberHistograms(); ++i) {
      auto const &Ys = subtractedWS->y(i);
      for (size_t j = 0; j < Ys.size(); ++j) {
        if (j != numBins / 2) {
          TS_ASSERT_DELTA(Ys[j], 0., 1e-12);
        }
      }
    }
  }

  void test_Distribution() {
    API::MatrixWorkspace_sptr inWS =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            numBanks, numPixels, numBins);
    TS_ASSERT(inWS->isHistogramData())
    TS_ASSERT(!inWS->isDistribution())
    std::array<double, numBanks> bankBkgs{{2.33, 4.22}};
    constexpr double startX{9.};
    constexpr double deltaX{0.57};
    for (size_t i = 0; i < numBanks; ++i) {
      for (size_t j = 0; j < numSpectraPerBank; ++j) {
        auto histogram = inWS->histogram(i * numSpectraPerBank + j);
        auto &Ys = histogram.mutableY();
        Ys = bankBkgs[i];
        Ys[numBins / 2] = 1090.; // Peak.
        // Non-equidistant binning.
        auto &Xs = inWS->mutableX(i * numSpectraPerBank + j);
        Xs[0] = startX;
        for (size_t k = 1; k < Xs.size(); ++k) {
          Xs[k] = Xs[k - 1] + static_cast<double>(k + 1) * deltaX;
        }
        histogram.convertToFrequencies();
      }
    }
    inWS->setDistribution(true);
    TS_ASSERT(inWS->isDistribution())
    auto eppWS = makeEPPWorkspace(*inWS);
    auto outWS = execAlgorithm(inWS, eppWS);
    TS_ASSERT(outWS->isDistribution())
    auto subtractedWS = inWS - outWS;
    for (size_t i = 0; i < subtractedWS->getNumberHistograms(); ++i) {
      auto const &Ys = subtractedWS->y(i);
      for (size_t j = 0; j < Ys.size(); ++j) {
        if (j != numBins / 2) {
          TS_ASSERT_EQUALS(Ys[j], 0);
        }
      }
    }
  }

  void test_HigherDegreePolynomial() {
    API::MatrixWorkspace_sptr inWS =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            numBanks, numPixels, numBins);
    for (size_t i = 0; i < inWS->getNumberHistograms(); ++i) {
      auto &Ys = inWS->mutableY(i);
      Ys = static_cast<double>(i);
      Ys[numBins / 2] = 1090.; // Peak.
    }
    auto eppWS = makeEPPWorkspace(*inWS);
    std::vector<std::string> const components{"bank1", "bank2"};
    Algorithms::DirectILLTubeBackground alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Components", components))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EPPWorkspace", eppWS))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Degree", 1))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    API::MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outWS)
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), inWS->getNumberHistograms())
    TS_ASSERT_EQUALS(outWS->blocksize(), inWS->blocksize())
    for (size_t i = 0; i < outWS->getNumberHistograms(); ++i) {
      auto const &Ys = outWS->y(i);
      auto const &Es = outWS->e(i);
      TS_ASSERT_DELTA(Ys[i], static_cast<double>(i), 1e-10)
      TS_ASSERT_EQUALS(Es[i], 0.)
    }
  }

  void test_DiagnosticsWorkspace() {
    API::MatrixWorkspace_sptr inWS =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            numBanks, numPixels, numBins);
    TS_ASSERT(inWS->isHistogramData())
    TS_ASSERT(!inWS->isDistribution())
    std::array<double, numBanks> bankBkgs{{2.33, 4.22}};
    for (size_t i = 0; i < numBanks; ++i) {
      for (size_t j = 0; j < numSpectraPerBank; ++j) {
        auto &Ys = inWS->mutableY(i * numSpectraPerBank + j);
        Ys = bankBkgs[i];
        Ys[numBins / 2] = 1090.; // Peak.
      }
    }
    auto maskWS =
        std::make_unique<DataObjects::MaskWorkspace>(inWS->getInstrument());
    maskWS->setMaskedIndex(1);
    inWS->mutableY(1) = -600;
    maskWS->setMaskedIndex(6);
    inWS->mutableY(6) = 900;
    API::MatrixWorkspace_sptr diagnosticsWS(maskWS.release());
    auto eppWS = makeEPPWorkspace(*inWS);
    std::vector<std::string> const components{"bank1", "bank2"};
    Algorithms::DirectILLTubeBackground alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Components", components))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EPPWorkspace", eppWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("DiagnosticsWorkspace", diagnosticsWS))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    API::MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outWS)
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), inWS->getNumberHistograms())
    TS_ASSERT_EQUALS(outWS->blocksize(), inWS->blocksize())
    for (size_t i = 0; i < numBanks; ++i) {
      for (size_t j = 0; j < numSpectraPerBank; ++j) {
        auto const &Ys = outWS->y(i * numSpectraPerBank + j);
        auto const &Es = outWS->e(i * numSpectraPerBank + j);
        for (size_t k = 0; k < Ys.size(); ++k) {
          TS_ASSERT_EQUALS(Ys[k], bankBkgs[i])
          TS_ASSERT_EQUALS(Es[k], 0.)
        }
      }
    }
  }

  void test_FailedEPPRowsAreIgnored() {
    API::MatrixWorkspace_sptr inWS =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            numBanks, numPixels, numBins);
    TS_ASSERT(inWS->isHistogramData())
    TS_ASSERT(!inWS->isDistribution())
    std::array<double, numBanks> bankBkgs{{2.33, 4.22}};
    for (size_t i = 0; i < numBanks; ++i) {
      for (size_t j = 0; j < numSpectraPerBank; ++j) {
        auto &Ys = inWS->mutableY(i * numSpectraPerBank + j);
        Ys = bankBkgs[i];
        Ys[numBins / 2] = 1090.; // Peak.
      }
    }
    inWS->mutableY(1) = -600;
    inWS->mutableY(6) = 900;
    std::vector<WorkspaceCreationHelper::EPPTableRow> eppRows(
        numBanks * numSpectraPerBank);
    for (auto &row : eppRows) {
      // Peak covers the middle bin of all histograms.
      row.peakCentre = static_cast<double>(numBins) / 2.;
      row.sigma = 1.1 / 6.;
    }
    // Fail the rows given special Y values above.
    eppRows[1].fitStatus =
        WorkspaceCreationHelper::EPPTableRow::FitStatus::FAILURE;
    eppRows[6].fitStatus =
        WorkspaceCreationHelper::EPPTableRow::FitStatus::FAILURE;
    auto eppWS = createEPPTableWorkspace(eppRows);
    auto outWS = execAlgorithm(inWS, eppWS);
    for (size_t i = 0; i < numBanks; ++i) {
      for (size_t j = 0; j < numSpectraPerBank; ++j) {
        auto const &Ys = outWS->y(i * numSpectraPerBank + j);
        auto const &Es = outWS->e(i * numSpectraPerBank + j);
        for (size_t k = 0; k < Ys.size(); ++k) {
          TS_ASSERT_EQUALS(Ys[k], bankBkgs[i])
          TS_ASSERT_EQUALS(Es[k], 0.)
        }
      }
    }
  }

private:
  constexpr static int numBanks{2};
  constexpr static int numPixels{2};
  constexpr static int numSpectraPerBank{numPixels * numPixels};
  constexpr static int numBins{12};

  static API::MatrixWorkspace_sptr
  execAlgorithm(API::MatrixWorkspace_sptr &inWS,
                API::ITableWorkspace_sptr &eppWS) {
    std::vector<std::string> const components{"bank1", "bank2"};
    Algorithms::DirectILLTubeBackground alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Components", components))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EPPWorkspace", eppWS))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    API::MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outWS)
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), inWS->getNumberHistograms())
    TS_ASSERT_EQUALS(outWS->blocksize(), inWS->blocksize())
    return outWS;
  }

  static API::ITableWorkspace_sptr
  makeEPPWorkspace(API::MatrixWorkspace const &ws) {
    std::vector<WorkspaceCreationHelper::EPPTableRow> eppRows(
        numBanks * numPixels * numPixels);
    auto const centreBin = numBins / 2;
    auto const &Xs = ws.x(0);
    for (auto &row : eppRows) {
      // Peak covers the middle bin of all histograms.
      row.peakCentre = (Xs[centreBin] + Xs[centreBin + 1]) / 2.;
      row.sigma = (Xs[centreBin + 1] - Xs[centreBin]) / 6.;
    }
    return createEPPTableWorkspace(eppRows);
  }
};

class DirectILLTubeBackgroundTestPerformance : public CxxTest::TestSuite {
public:
  static DirectILLTubeBackgroundTestPerformance *createSuite() {
    return new DirectILLTubeBackgroundTestPerformance();
  }
  static void destroySuite(DirectILLTubeBackgroundTestPerformance *suite) {
    delete suite;
  }

  DirectILLTubeBackgroundTestPerformance() : CxxTest::TestSuite() {
    API::FrameworkManager::Instance();
  }

  void testPerformance() {
    constexpr int numBanks{256};
    constexpr int numPixels{20};
    constexpr int numBins{512};
    API::MatrixWorkspace_sptr inWS =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            numBanks, numPixels, numBins);
    std::vector<WorkspaceCreationHelper::EPPTableRow> eppRows(
        numBanks * numPixels * numPixels);
    for (auto &row : eppRows) {
      row.peakCentre = static_cast<double>(numBins) / 2.;
      row.sigma = 5;
    }
    auto eppWS = createEPPTableWorkspace(eppRows);
    std::vector<std::string> components(numBanks);
    for (size_t i = 0; i < components.size(); ++i) {
      components[i] = "bank" + std::to_string(i + 1);
    }
    Algorithms::DirectILLTubeBackground alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setPropertyValue("OutputWorkspace", "_unused");
    alg.setProperty("Components", components);
    alg.setProperty("EPPWorkspace", eppWS);
    alg.execute();
    TS_ASSERT(alg.isExecuted())
  }
};

#endif /* MANTID_ALGORITHMS_DIRECTILLTUBEBACKGROUNDTEST_H_ */
