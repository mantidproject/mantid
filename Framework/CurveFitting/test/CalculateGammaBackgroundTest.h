#ifndef MANTID_CURVEFITTING_CALCULATEGAMMABACKGROUNDTEST_H_
#define MANTID_CURVEFITTING_CALCULATEGAMMABACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/CalculateGammaBackground.h"
#include "ComptonProfileTestHelpers.h"

using Mantid::CurveFitting::CalculateGammaBackground;

class CalculateGammaBackgroundTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateGammaBackgroundTest *createSuite() {
    return new CalculateGammaBackgroundTest();
  }
  static void destroySuite(CalculateGammaBackgroundTest *suite) {
    delete suite;
  }

  //------------------------------------ Success cases
  //---------------------------------------
  void
  test_Input_With_Spectrum_Number_Inside_Forward_Scatter_Range_Gives_Expected_Correction() {
    using namespace Mantid::API;
    auto inputWS = createTestWorkspaceWithFoilChanger(); // specNo=1
    // Put spectrum in forward scatter range
    inputWS->getSpectrum(0)->setSpectrumNo(135);
    auto alg = runSuccessTestCase(inputWS);

    MatrixWorkspace_sptr backgroundWS = alg->getProperty("BackgroundWorkspace");
    MatrixWorkspace_sptr correctedWS = alg->getProperty("CorrectedWorkspace");
    TS_ASSERT(backgroundWS != 0);
    TS_ASSERT(correctedWS != 0);
    TS_ASSERT(backgroundWS != correctedWS);

    // Test some values in the range
    const size_t npts(inputWS->blocksize());
    const auto &inX(inputWS->readX(0));
    const auto &backX(backgroundWS->readX(0));
    const auto &corrX(backgroundWS->readX(0));

    // X values are just straight copy
    TS_ASSERT_DELTA(backX.front(), inX.front(), 1e-08);
    TS_ASSERT_DELTA(backX[npts / 2], inX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(backX.back(), inX.back(), 1e-08);
    TS_ASSERT_DELTA(corrX.front(), inX.front(), 1e-08);
    TS_ASSERT_DELTA(corrX[npts / 2], inX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(corrX.back(), inX.back(), 1e-08);

    // E values are zero for background & copy for corrected
    const auto &backE(backgroundWS->readE(0));
    TS_ASSERT_DELTA(backE.front(), 0.0, 1e-08);
    TS_ASSERT_DELTA(backE[npts / 2], 0.0, 1e-08);
    TS_ASSERT_DELTA(backE.back(), 0.0, 1e-08);
    const auto &corrE(correctedWS->readE(0));
    const auto &inE(inputWS->readE(0));
    TS_ASSERT_DELTA(corrE.front(), inE.front(), 1e-08);
    TS_ASSERT_DELTA(corrE[npts / 2], inE[npts / 2], 1e-08);
    TS_ASSERT_DELTA(corrE.back(), inE.back(), 1e-08);

    const auto &corrY(correctedWS->readY(0));
    TS_ASSERT_DELTA(corrY.front(), -0.00253802, 1e-08);
    TS_ASSERT_DELTA(corrY[npts / 2], 0.15060372, 1e-08);
    TS_ASSERT_DELTA(corrY.back(), -0.01696477, 1e-08);

    // Background Y values = 0.0
    const auto &backY(backgroundWS->readY(0));
    TS_ASSERT_DELTA(backY.front(), -0.00000138, 1e-08);
    TS_ASSERT_DELTA(backY[npts / 2], -0.00015056, 1e-08);
    TS_ASSERT_DELTA(backY.back(), 0.01650629, 1e-08);
  }

  void
  test_Input_With_Spectrum_Number_Outside_Range_Leaves_Data_Uncorrected_And_Background_Zeroed() {
    using namespace Mantid::API;
    auto inputWS = createTestWorkspaceWithFoilChanger(); // specNo=1
    auto alg = runSuccessTestCase(inputWS);

    MatrixWorkspace_sptr backgroundWS = alg->getProperty("BackgroundWorkspace");
    MatrixWorkspace_sptr correctedWS = alg->getProperty("CorrectedWorkspace");
    TS_ASSERT(backgroundWS != 0);
    TS_ASSERT(correctedWS != 0);
    TS_ASSERT(backgroundWS != correctedWS);

    // Test some values in the range
    const size_t npts(inputWS->blocksize());
    const auto &inX(inputWS->readX(0));
    const auto &backX(backgroundWS->readX(0));
    const auto &corrX(backgroundWS->readX(0));

    // X values are just straight copy
    TS_ASSERT_DELTA(backX.front(), inX.front(), 1e-08);
    TS_ASSERT_DELTA(backX[npts / 2], inX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(backX.back(), inX.back(), 1e-08);
    TS_ASSERT_DELTA(corrX.front(), inX.front(), 1e-08);
    TS_ASSERT_DELTA(corrX[npts / 2], inX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(corrX.back(), inX.back(), 1e-08);

    // Corrected matches input the detector is not defined as forward scatter
    // range - Currently hardcoded in algorithm
    const auto &inY(inputWS->readY(0));
    const auto &corrY(correctedWS->readY(0));
    TS_ASSERT_DELTA(corrY.front(), inY.front(), 1e-08);
    TS_ASSERT_DELTA(corrY[npts / 2], inY[npts / 2], 1e-08);
    TS_ASSERT_DELTA(corrY.back(), inY.back(), 1e-08);

    // Background Y values = 0.0
    const auto &backY(backgroundWS->readY(0));
    TS_ASSERT_DELTA(backY.front(), 0.0, 1e-08);
    TS_ASSERT_DELTA(backY[npts / 2], 0.0, 1e-08);
    TS_ASSERT_DELTA(backY.back(), 0.0, 1e-08);
  }

  void test_Restricting_Correction_Range_Only_Gives_Output_For_Those_Spectra() {
    using namespace Mantid::API;
    auto inputWS = createTwoSpectrumWorkspaceWithFoilChanger();
    inputWS->getSpectrum(0)->setSpectrumNo(135);
    inputWS->getSpectrum(1)->setSpectrumNo(135);
    inputWS->getSpectrum(1)->clearDetectorIDs();
    inputWS->getSpectrum(1)->addDetectorID(1);
    auto alg = runSuccessTestCase(inputWS, "1");

    MatrixWorkspace_sptr backgroundWS = alg->getProperty("BackgroundWorkspace");
    MatrixWorkspace_sptr correctedWS = alg->getProperty("CorrectedWorkspace");
    TS_ASSERT(backgroundWS != 0);
    TS_ASSERT(correctedWS != 0);
    TS_ASSERT(backgroundWS != correctedWS);

    TS_ASSERT_EQUALS(1, backgroundWS->getNumberHistograms());
    TS_ASSERT_EQUALS(1, correctedWS->getNumberHistograms());

    // Test some values in the range
    const size_t npts(inputWS->blocksize());
    const auto &inX(inputWS->readX(0));
    const auto &backX(backgroundWS->readX(0));
    const auto &corrX(backgroundWS->readX(0));

    // X values are just straight copy
    TS_ASSERT_DELTA(backX.front(), inX.front(), 1e-08);
    TS_ASSERT_DELTA(backX[npts / 2], inX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(backX.back(), inX.back(), 1e-08);
    TS_ASSERT_DELTA(corrX.front(), inX.front(), 1e-08);
    TS_ASSERT_DELTA(corrX[npts / 2], inX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(corrX.back(), inX.back(), 1e-08);

    // E values are zero for background & copy for corrected
    const auto &backE(backgroundWS->readE(0));
    TS_ASSERT_DELTA(backE.front(), 0.0, 1e-08);
    TS_ASSERT_DELTA(backE[npts / 2], 0.0, 1e-08);
    TS_ASSERT_DELTA(backE.back(), 0.0, 1e-08);
    const auto &corrE(correctedWS->readE(0));
    const auto &inE(inputWS->readE(0));
    TS_ASSERT_DELTA(corrE.front(), inE.front(), 1e-08);
    TS_ASSERT_DELTA(corrE[npts / 2], inE[npts / 2], 1e-08);
    TS_ASSERT_DELTA(corrE.back(), inE.back(), 1e-08);

    // Corrected values
    const auto &corrY(correctedWS->readY(0));
    TS_ASSERT_DELTA(corrY.front(), -0.00253802, 1e-08);
    TS_ASSERT_DELTA(corrY[npts / 2], 0.15060372, 1e-08);
    TS_ASSERT_DELTA(corrY.back(), -0.01696477, 1e-08);

    // Background Y values = 0.0
    const auto &backY(backgroundWS->readY(0));
    TS_ASSERT_DELTA(backY.front(), -0.00000138, 1e-08);
    TS_ASSERT_DELTA(backY[npts / 2], -0.00015056, 1e-08);
    TS_ASSERT_DELTA(backY.back(), 0.01650629, 1e-08);
  }

  //------------------------------------ Error cases
  //---------------------------------------

  void test_Empty_Function_Property_Throws_Error() {
    auto alg = createAlgorithm();
    alg->setRethrows(true);

    alg->setProperty("InputWorkspace", createTestWorkspaceWithFoilChanger());
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
    TS_ASSERT(!alg->isExecuted());
  }

  void test_Function_Property_With_Single_Non_ComptonProfile_Throws_Error() {
    auto alg = createAlgorithm();
    alg->setRethrows(true);

    alg->setProperty("InputWorkspace", createTestWorkspaceWithFoilChanger());
    alg->setPropertyValue("ComptonFunction", "name=Gaussian");
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
    TS_ASSERT(!alg->isExecuted());
  }

  void test_Function_Property_With_Composite_Non_ComptonProfile_Throws_Error() {
    auto alg = createAlgorithm();
    alg->setRethrows(true);

    alg->setProperty("InputWorkspace", createTestWorkspaceWithFoilChanger());
    alg->setPropertyValue("ComptonFunction",
                          "name=GaussianComptonProfile;name=Gaussian");
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
    TS_ASSERT(!alg->isExecuted());
  }

private:
  Mantid::API::IAlgorithm_sptr
  runSuccessTestCase(const Mantid::API::MatrixWorkspace_sptr &inputWS,
                     const std::string &index = "") {
    auto alg = createAlgorithm();
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", inputWS);
    alg->setPropertyValue(
        "ComptonFunction",
        "name=GaussianComptonProfile,Mass=1.0079,Width=2.9e-2,Intensity=4.29");
    if (!index.empty())
      alg->setPropertyValue("WorkspaceIndexList", index);

    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    return alg;
  }

  Mantid::API::IAlgorithm_sptr createAlgorithm() {
    Mantid::API::IAlgorithm_sptr alg =
        boost::make_shared<CalculateGammaBackground>();
    alg->initialize();
    alg->setChild(true);
    alg->setPropertyValue("CorrectedWorkspace", "__UNUSED__");
    alg->setPropertyValue("BackgroundWorkspace", "__UNUSED__");
    return alg;
  }

  Mantid::API::MatrixWorkspace_sptr createTestWorkspaceWithFoilChanger() {
    double x0(50.0), x1(300.0), dx(0.5);
    return ComptonProfileTestHelpers::createTestWorkspace(1, x0, x1, dx, true,
                                                          true);
  }

  Mantid::API::MatrixWorkspace_sptr createTestWorkspaceWithNoFoilChanger() {
    double x0(165.0), x1(166.0), dx(0.5);
    return ComptonProfileTestHelpers::createTestWorkspace(1, x0, x1, dx, false);
  }

  Mantid::API::MatrixWorkspace_sptr
  createTwoSpectrumWorkspaceWithFoilChanger() {
    double x0(50.0), x1(300.0), dx(0.5);
    auto singleSpectrum = ComptonProfileTestHelpers::createTestWorkspace(
        1, x0, x1, dx, true, true);
    const size_t nhist = 2;
    auto twoSpectrum =
        Mantid::API::WorkspaceFactory::Instance().create(singleSpectrum, nhist);
    // Copy data
    for (size_t i = 0; i < nhist; ++i) {
      twoSpectrum->setX(i, singleSpectrum->refX(0));
      twoSpectrum->dataY(i) = singleSpectrum->readY(0);
      twoSpectrum->dataE(i) = singleSpectrum->readE(0);
    }
    return twoSpectrum;
  }
};

#endif /* MANTID_ALGORITHMS_CalculateGammaBackgroundTEST_H_ */
