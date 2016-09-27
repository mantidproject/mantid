#ifndef MANTID_CURVEFITTING_NORMALISEBYPEAKAREATEST_H_
#define MANTID_CURVEFITTING_NORMALISEBYPEAKAREATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Algorithms/NormaliseByPeakArea.h"
#include "../Functions/ComptonProfileTestHelpers.h"

using Mantid::CurveFitting::Algorithms::NormaliseByPeakArea;

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
    TS_ASSERT(outputWS != 0);
    TS_ASSERT(yspaceWS != 0);
    TS_ASSERT(fittedWS != 0);
    TS_ASSERT(symmetrisedWS != 0);

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
    const auto &outX = outputWS->readX(0);
    const auto &outY = outputWS->readY(0);
    const auto &outE = outputWS->readE(0);
    const size_t npts = outputWS->blocksize();

    // X
    TS_ASSERT_DELTA(50.0, outX.front(), 1e-08);
    TS_ASSERT_DELTA(175.0, outX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(300.0, outX.back(), 1e-08);
    // Y
    TS_ASSERT_DELTA(0.00020684, outY.front(), 1e-08);
    TS_ASSERT_DELTA(0.00346369, outY[npts / 2], 1e-08);
    TS_ASSERT_DELTA(0.00038601, outY.back(), 1e-08);
    // E
    TS_ASSERT_DELTA(0.02008497, outE.front(), 1e-08);
    TS_ASSERT_DELTA(0.02008497, outE[npts / 2], 1e-08);
    TS_ASSERT_DELTA(0.02008497, outE.back(), 1e-08);

    // ====== Y-space =====
    const auto &ysX = yspaceWS->readX(0);
    const auto &ysY = yspaceWS->readY(0);
    const auto &ysE = yspaceWS->readE(0);
    // X
    TS_ASSERT_DELTA(-18.71348856, ysX.front(), 1e-08);
    TS_ASSERT_DELTA(-1.670937938, ysX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(17.99449408, ysX.back(), 1e-08);
    // Y
    TS_ASSERT_DELTA(0.48319776, ysY.front(), 1e-08);
    TS_ASSERT_DELTA(6.38061442, ysY[npts / 2], 1e-08);
    TS_ASSERT_DELTA(1.4251361, ysY.back(), 1e-08);
    // E
    TS_ASSERT_DELTA(25.14204252, ysE.front(), 1e-08);
    TS_ASSERT_DELTA(36.99940026, ysE[npts / 2], 1e-08);
    TS_ASSERT_DELTA(138.38603736, ysE.back(), 1e-08);

    // ====== Fitted ======
    const auto &fitX = fittedWS->readX(0);
    const auto &fitY = fittedWS->readY(0);
    const auto &fitE = fittedWS->readE(0);

    // X
    TS_ASSERT_DELTA(-18.71348856, fitX.front(), 1e-08);
    TS_ASSERT_DELTA(-1.670937938, fitX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(17.99449408, fitX.back(), 1e-08);
    // Y
    TS_ASSERT_DELTA(-0.00548538, fitY.front(), 1e-08);
    TS_ASSERT_DELTA(6.06951398, fitY[npts / 2], 1e-08);
    TS_ASSERT_DELTA(-0.00645323, fitY.back(), 1e-08);
    // E
    TS_ASSERT_DELTA(25.14204252, fitE.front(), 1e-08);
    TS_ASSERT_DELTA(36.99940026, fitE[npts / 2], 1e-08);
    TS_ASSERT_DELTA(138.38603736, fitE.back(), 1e-08);

    // ====== Symmetrised ======
    const auto &symX = symmetrisedWS->readX(0);
    const auto &symY = symmetrisedWS->readY(0);
    const auto &symE = symmetrisedWS->readE(0);

    // X
    TS_ASSERT_DELTA(-18.71348856, symX.front(), 1e-08);
    TS_ASSERT_DELTA(-1.670937938, symX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(17.99449408, symX.back(), 1e-08);
    // Y
    TS_ASSERT_DELTA(0.38871577, symY.front(), 1e-08);
    TS_ASSERT_DELTA(6.56372265, symY[npts / 2], 1e-08);
    TS_ASSERT_DELTA(0.21596637, symY.back(), 1e-08);
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
    TS_ASSERT(outputWS != 0);
    TS_ASSERT(yspaceWS != 0);
    TS_ASSERT(fittedWS != 0);
    TS_ASSERT(symmetrisedWS != 0);

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
    const auto &outX = outputWS->readX(0);
    const auto &outY = outputWS->readY(0);
    const auto &outE = outputWS->readE(0);
    size_t npts = outputWS->blocksize();

    // X
    TS_ASSERT_DELTA(50.0, outX.front(), 1e-08);
    TS_ASSERT_DELTA(175.0, outX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(300.0, outX.back(), 1e-08);
    // Y
    TS_ASSERT_DELTA(3.124e-05, outY.front(), 1e-08);
    TS_ASSERT_DELTA(0.00052321, outY[npts / 2], 1e-08);
    TS_ASSERT_DELTA(5.831e-05, outY.back(), 1e-08);
    // E
    TS_ASSERT_DELTA(0.00303394, outE.front(), 1e-08);
    TS_ASSERT_DELTA(0.00303394, outE[npts / 2], 1e-08);
    TS_ASSERT_DELTA(0.00303394, outE.back(), 1e-08);

    // ====== Y-space =====
    const auto &ysX = yspaceWS->readX(0);
    const auto &ysY = yspaceWS->readY(0);
    const auto &ysE = yspaceWS->readE(0);
    npts = yspaceWS->blocksize();

    // X
    TS_ASSERT_DELTA(-18.46348856, ysX.front(), 1e-08);
    TS_ASSERT_DELTA(0.03651144, ysX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(17.89050276, ysX.back(), 1e-08);
    // Y
    TS_ASSERT_DELTA(1.06684471, ysY.front(), 1e-08);
    TS_ASSERT_DELTA(53.17060919, ysY[npts / 2], 1e-08);
    TS_ASSERT_DELTA(2.12962024, ysY.back(), 1e-08);
    // E
    TS_ASSERT_DELTA(52.17644100, ysE.front(), 1e-08);
    TS_ASSERT_DELTA(71.30383310, ysE[npts / 2], 1e-08);
    TS_ASSERT_DELTA(137.96461559, ysE.back(), 1e-08);

    // ====== Fitted ======
    const auto &fitX = fittedWS->readX(0);
    const auto &fitY = fittedWS->readY(0);
    const auto &fitE = fittedWS->readE(0);

    // X
    TS_ASSERT_DELTA(-18.46348856, fitX.front(), 1e-08);
    TS_ASSERT_DELTA(0.03651144, fitX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(17.89050276, fitX.back(), 1e-08);
    // Y
    std::cerr << std::fixed << std::setprecision(8) << fitY.front() << " "
              << fitY[npts / 2] << "  " << fitY.back() << "\n";
    TS_ASSERT_DELTA(-0.03810116, fitY.front(), 1e-08);
    TS_ASSERT_DELTA(52.54741763, fitY[npts / 2], 1e-08);
    TS_ASSERT_DELTA(-0.04398723, fitY.back(), 1e-08);
    // E
    std::cerr << std::fixed << std::setprecision(8) << fitE.front() << " "
              << fitE[npts / 2] << "  " << fitE.back() << "\n";
    TS_ASSERT_DELTA(52.17644100, fitE.front(), 1e-08);
    TS_ASSERT_DELTA(71.30383310, fitE[npts / 2], 1e-08);
    TS_ASSERT_DELTA(137.96461559, fitE.back(), 1e-08);

    // ====== Symmetrised ======
    const auto &symX = symmetrisedWS->readX(0);
    const auto &symY = symmetrisedWS->readY(0);
    const auto &symE = symmetrisedWS->readE(0);

    // X
    TS_ASSERT_DELTA(-18.46348856, symX.front(), 1e-08);
    TS_ASSERT_DELTA(0.03651144, symX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(17.89050276, symX.back(), 1e-08);
    // Y
    std::cerr << std::fixed << std::setprecision(8) << symY.front() << " "
              << symY[npts / 2] << "  " << symY.back() << "\n";
    TS_ASSERT_DELTA(1.06684471, symY.front(), 1e-08);
    TS_ASSERT_DELTA(53.17060919, symY[npts / 2], 1e-08);
    TS_ASSERT_DELTA(0.82096809, symY.back(), 1e-08);
    // E
    std::cerr << std::fixed << std::setprecision(8) << symE.front() << " "
              << symE[npts / 2] << "  " << symE.back() << "\n";
    TS_ASSERT_DELTA(52.17644100, symE.front(), 1e-08);
    TS_ASSERT_DELTA(71.30383310, symE[npts / 2], 1e-08);
    TS_ASSERT_DELTA(48.83869866, symE.back(), 1e-08);
  }

private:
  Mantid::API::IAlgorithm_sptr createAlgorithm() {
    Mantid::API::IAlgorithm_sptr alg =
        boost::make_shared<NormaliseByPeakArea>();
    alg->initialize();
    alg->setChild(true);
    alg->setPropertyValue("OutputWorkspace", "__UNUSED__");
    alg->setPropertyValue("YSpaceDataWorkspace", "__UNUSED__");
    alg->setPropertyValue("FittedWorkspace", "__UNUSED__");
    alg->setPropertyValue("SymmetrisedWorkspace", "__UNUSED__");
    return alg;
  }

  Mantid::API::MatrixWorkspace_sptr createTwoSpectrumWorkspace() {
    double x0(50.0), x1(300.0), dx(0.5);
    auto twoSpectrum = ComptonProfileTestHelpers::createTestWorkspace(
        2, x0, x1, dx, true, true);
    return twoSpectrum;
  }
};

#endif /* MANTID_CURVEFITTING_NORMALISEBYPEAKAREATEST_H_ */
