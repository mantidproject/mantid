#ifndef MANTID_CURVEFITTING_NORMALISEBYPEAKAREATEST_H_
#define MANTID_CURVEFITTING_NORMALISEBYPEAKAREATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Algorithms/NormaliseByPeakArea.h"
#include "../Functions/ComptonProfileTestHelpers.h"

using Mantid::CurveFitting::Algorithms::NormaliseByPeakArea;

namespace {

Mantid::API::MatrixWorkspace_sptr
createTwoSpectrumWorkspace(double x0 = 50, double x1 = 300, double dx = 0.5) {
  auto twoSpectrum = ComptonProfileTestHelpers::createTestWorkspace(
      2, x0, x1, dx, ComptonProfileTestHelpers::NoiseType::Full, true, true);
  return twoSpectrum;
}

Mantid::API::IAlgorithm_sptr createAlgorithm() {
  Mantid::API::IAlgorithm_sptr alg = boost::make_shared<NormaliseByPeakArea>();
  alg->initialize();
  alg->setChild(true);
  alg->setPropertyValue("OutputWorkspace", "__UNUSED__");
  alg->setPropertyValue("YSpaceDataWorkspace", "__UNUSED__");
  alg->setPropertyValue("FittedWorkspace", "__UNUSED__");
  alg->setPropertyValue("SymmetrisedWorkspace", "__UNUSED__");
  return alg;
}
}

class NormaliseByPeakAreaTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NormaliseByPeakAreaTest *createSuite() {
    return new NormaliseByPeakAreaTest();
  }
  static void destroySuite(NormaliseByPeakAreaTest *suite) { delete suite; }

  void test_Init() {
    NormaliseByPeakArea alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec_nosum_spectrum_gives_correct_values() {
    using namespace Mantid::API;

    auto alg = createAlgorithm();
    auto testWS = createTwoSpectrumWorkspace();
    alg->setProperty("InputWorkspace", testWS);
    alg->setProperty("Mass", 1.0097);
    alg->setProperty("Sum", false);
    alg->execute();
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outputWS = alg->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr yspaceWS = alg->getProperty("YSpaceDataWorkspace");
    MatrixWorkspace_sptr fittedWS = alg->getProperty("FittedWorkspace");
    MatrixWorkspace_sptr symmetrisedWS =
        alg->getProperty("SymmetrisedWorkspace");
    TS_ASSERT(outputWS != nullptr);
    TS_ASSERT(yspaceWS != nullptr);
    TS_ASSERT(fittedWS != nullptr);
    TS_ASSERT(symmetrisedWS != nullptr);

    // Dimensions
    TS_ASSERT_EQUALS(testWS->getNumberHistograms(),
                     outputWS->getNumberHistograms());
    TS_ASSERT_EQUALS(testWS->getNumberHistograms(),
                     yspaceWS->getNumberHistograms());
    TS_ASSERT_EQUALS(testWS->getNumberHistograms(),
                     fittedWS->getNumberHistograms());
    TS_ASSERT_EQUALS(testWS->getNumberHistograms(),
                     symmetrisedWS->getNumberHistograms());

    TS_ASSERT_EQUALS(testWS->blocksize(), outputWS->blocksize());
    TS_ASSERT_EQUALS(testWS->blocksize(), yspaceWS->blocksize());
    TS_ASSERT_EQUALS(testWS->blocksize(), fittedWS->blocksize());
    TS_ASSERT_EQUALS(testWS->blocksize(), symmetrisedWS->blocksize());

    // Test a few values
    // ====== TOF data ======
    const auto &outX = outputWS->x(0);
    const auto &outY = outputWS->y(0);
    const auto &outE = outputWS->e(0);
    const size_t npts = outputWS->blocksize();

    // X
    TS_ASSERT_DELTA(50.0, outX.front(), 1e-08);
    TS_ASSERT_DELTA(175.0, outX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(300.0, outX.back(), 1e-08);
    // Y
    TS_ASSERT_DELTA(0.00040504, outY.front(), 1e-08);
    TS_ASSERT_DELTA(0.00355931, outY[npts / 2], 1e-08);
    TS_ASSERT_DELTA(-0.0001652, outY.back(), 1e-08);
    // E
    TS_ASSERT_DELTA(0.02030939, outE.front(), 1e-08);
    TS_ASSERT_DELTA(0.02030939, outE[npts / 2], 1e-08);
    TS_ASSERT_DELTA(0.02030939, outE.back(), 1e-08);

    // ====== Y-space =====
    const auto &ysX = yspaceWS->x(0);
    const auto &ysY = yspaceWS->y(0);
    const auto &ysE = yspaceWS->e(0);
    // X
    TS_ASSERT_DELTA(-18.71348856, ysX.front(), 1e-08);
    TS_ASSERT_DELTA(-1.670937938, ysX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(17.99449408, ysX.back(), 1e-08);
    // Y
    TS_ASSERT_DELTA(-0.20450682, ysY.front(), 1e-08);
    TS_ASSERT_DELTA(6.48431743, ysY[npts / 2], 1e-08);
    TS_ASSERT_DELTA(2.75992908, ysY.back(), 1e-08);
    // E
    TS_ASSERT_DELTA(25.14204252, ysE.front(), 1e-08);
    TS_ASSERT_DELTA(36.99940026, ysE[npts / 2], 1e-08);
    TS_ASSERT_DELTA(138.38603736, ysE.back(), 1e-08);

    // ====== Fitted ======
    const auto &fitX = fittedWS->x(0);
    const auto &fitY = fittedWS->y(0);
    const auto &fitE = fittedWS->e(0);

    // X
    TS_ASSERT_DELTA(-18.71348856, fitX.front(), 1e-08);
    TS_ASSERT_DELTA(-1.670937938, fitX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(17.99449408, fitX.back(), 1e-08);
    // Y
    TS_ASSERT_DELTA(-0.00540713, fitY.front(), 1e-08);
    TS_ASSERT_DELTA(5.94335449, fitY[npts / 2], 1e-08);
    TS_ASSERT_DELTA(-0.00641808, fitY.back(), 1e-08);
    // E
    TS_ASSERT_DELTA(25.14204252, fitE.front(), 1e-08);
    TS_ASSERT_DELTA(36.99940026, fitE[npts / 2], 1e-08);
    TS_ASSERT_DELTA(138.38603736, fitE.back(), 1e-08);

    // ====== Symmetrised ======
    const auto &symX = symmetrisedWS->x(0);
    const auto &symY = symmetrisedWS->y(0);
    const auto &symE = symmetrisedWS->e(0);

    // X
    TS_ASSERT_DELTA(-18.71348856, symX.front(), 1e-08);
    TS_ASSERT_DELTA(-1.670937938, symX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(17.99449408, symX.back(), 1e-08);
    // Y
    TS_ASSERT_DELTA(-0.13447607, symY.front(), 1e-08);
    TS_ASSERT_DELTA(6.39791398, symY[npts / 2], 1e-08);
    TS_ASSERT_DELTA(0.28362702, symY.back(), 1e-08);
    // E
    TS_ASSERT_DELTA(17.78587720, symE.front(), 1e-08);
    TS_ASSERT_DELTA(15.98016067, symE[npts / 2], 1e-08);
    TS_ASSERT_DELTA(14.59086103, symE.back(), 1e-08);
  }

  void
  test_exec_sum_spectrum_gives_original_TOF_plus_single_spectrum_yspace_values() {
    using namespace Mantid::API;

    auto alg = createAlgorithm();
    auto testWS = createTwoSpectrumWorkspace();
    alg->setProperty("InputWorkspace", testWS);
    alg->setProperty("Mass", 1.0097);
    alg->setProperty("Sum", true);
    alg->execute();
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outputWS = alg->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr yspaceWS = alg->getProperty("YSpaceDataWorkspace");
    MatrixWorkspace_sptr fittedWS = alg->getProperty("FittedWorkspace");
    MatrixWorkspace_sptr symmetrisedWS =
        alg->getProperty("SymmetrisedWorkspace");
    TS_ASSERT(outputWS != nullptr);
    TS_ASSERT(yspaceWS != nullptr);
    TS_ASSERT(fittedWS != nullptr);
    TS_ASSERT(symmetrisedWS != nullptr);

    // Dimensions
    TS_ASSERT_EQUALS(testWS->getNumberHistograms(),
                     outputWS->getNumberHistograms()); // TOF is original size
    TS_ASSERT_EQUALS(1, yspaceWS->getNumberHistograms());
    TS_ASSERT_EQUALS(1, fittedWS->getNumberHistograms());
    TS_ASSERT_EQUALS(1, symmetrisedWS->getNumberHistograms());

    TS_ASSERT_EQUALS(testWS->blocksize(), outputWS->blocksize());
    TS_ASSERT_EQUALS(74, yspaceWS->blocksize());
    TS_ASSERT_EQUALS(74, fittedWS->blocksize());
    TS_ASSERT_EQUALS(74, symmetrisedWS->blocksize());

    // Test a few values
    // ====== TOF data ======
    const auto &outX = outputWS->x(0);
    const auto &outY = outputWS->y(0);
    const auto &outE = outputWS->e(0);
    size_t npts = outputWS->blocksize();

    // X
    TS_ASSERT_DELTA(50.0, outX.front(), 1e-08);
    TS_ASSERT_DELTA(175.0, outX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(300.0, outX.back(), 1e-08);
    // Y

    TS_ASSERT_DELTA(0.00006119, outY.front(), 1e-08);
    TS_ASSERT_DELTA(0.00053774, outY[npts / 2], 1e-08);
    TS_ASSERT_DELTA(-0.00002496, outY.back(), 1e-08);
    // E
    TS_ASSERT_DELTA(0.00306834, outE.front(), 1e-08);
    TS_ASSERT_DELTA(0.00306834, outE[npts / 2], 1e-08);
    TS_ASSERT_DELTA(0.00306834, outE.back(), 1e-08);

    // ====== Y-space =====
    const auto &ysX = yspaceWS->x(0);
    const auto &ysY = yspaceWS->y(0);
    const auto &ysE = yspaceWS->e(0);
    npts = yspaceWS->blocksize();

    // X
    TS_ASSERT_DELTA(-18.46348856, ysX.front(), 1e-08);
    TS_ASSERT_DELTA(0.03651144, ysX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(17.89050276, ysX.back(), 1e-08);
    // Y
    TS_ASSERT_DELTA(1.04988426, ysY.front(), 1e-08);
    TS_ASSERT_DELTA(52.62476059, ysY[npts / 2], 1e-08);
    TS_ASSERT_DELTA(2.04137251, ysY.back(), 1e-08);
    // E
    TS_ASSERT_DELTA(52.17644100, ysE.front(), 1e-08);
    TS_ASSERT_DELTA(71.30383310, ysE[npts / 2], 1e-08);
    TS_ASSERT_DELTA(137.96461559, ysE.back(), 1e-08);

    // ====== Fitted ======
    const auto &fitX = fittedWS->x(0);
    const auto &fitY = fittedWS->y(0);
    const auto &fitE = fittedWS->e(0);

    // X
    TS_ASSERT_DELTA(-18.46348856, fitX.front(), 1e-08);
    TS_ASSERT_DELTA(0.03651144, fitX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(17.89050276, fitX.back(), 1e-08);
    // Y
    TS_ASSERT_DELTA(-0.0375677957, fitY.front(), 1e-08);
    TS_ASSERT_DELTA(52.483339, fitY[npts / 2], 1e-06);
    TS_ASSERT_DELTA(-0.04348319, fitY.back(), 1e-08);
    // E
    TS_ASSERT_DELTA(52.17644100, fitE.front(), 1e-08);
    TS_ASSERT_DELTA(71.30383310, fitE[npts / 2], 1e-08);
    TS_ASSERT_DELTA(137.96461559, fitE.back(), 1e-08);

    // ====== Symmetrised ======
    const auto &symX = symmetrisedWS->x(0);
    const auto &symY = symmetrisedWS->y(0);
    const auto &symE = symmetrisedWS->e(0);

    // X
    TS_ASSERT_DELTA(-18.46348856, symX.front(), 1e-08);
    TS_ASSERT_DELTA(0.03651144, symX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(17.89050276, symX.back(), 1e-08);
    // Y
    TS_ASSERT_DELTA(1.04988426, symY.front(), 1e-08);
    TS_ASSERT_DELTA(52.624761, symY[npts / 2], 1e-06);
    TS_ASSERT_DELTA(1.00228928, symY.back(), 1e-08);
    // E
    TS_ASSERT_DELTA(52.17644100, symE.front(), 1e-08);
    TS_ASSERT_DELTA(71.30383310, symE[npts / 2], 1e-08);
    TS_ASSERT_DELTA(48.83869866, symE.back(), 1e-08);
  }
};

class NormaliseByPeakAreaTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NormaliseByPeakAreaTestPerformance *createSuite() {
    return new NormaliseByPeakAreaTestPerformance();
  }
  static void destroySuite(NormaliseByPeakAreaTestPerformance *suite) {
    delete suite;
  }

  void setUp() override { testWS = createTwoSpectrumWorkspace(); }

  void test_sum_false() {
    using namespace Mantid::API;

    auto alg = createAlgorithm();
    auto testWS = createTwoSpectrumWorkspace(50, 2000, 0.1);
    alg->setProperty("InputWorkspace", testWS);
    alg->setProperty("Mass", 1.0097);
    alg->setProperty("Sum", false);
    alg->execute();
  }

  void test_sum_true() {
    using namespace Mantid::API;

    auto alg = createAlgorithm();
    auto testWS = createTwoSpectrumWorkspace(50, 5000, 0.0005);
    alg->setProperty("InputWorkspace", testWS);
    alg->setProperty("Mass", 1.0097);
    alg->setProperty("Sum", true);
    alg->execute();
  }

private:
  Mantid::API::MatrixWorkspace_sptr testWS;
};

#endif /* MANTID_CURVEFITTING_NORMALISEBYPEAKAREATEST_H_ */
