#ifndef FFT_TEST_H_
#define FFT_TEST_H_

#include <cmath>
#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FFT.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"

using namespace Mantid;
using namespace Mantid::API;

/**
 * This is a test class that exists to test the method validateInputs()
 */
class TestFFT : public Mantid::Algorithms::FFT {
public:
  std::map<std::string, std::string> wrapValidateInputs() {
    return this->validateInputs();
  }
};

class FFTTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FFTTest *createSuite() { return new FFTTest(); }
  static void destroySuite(FFTTest *suite) { delete suite; }

  FFTTest()
      : dX(0.2), h(sqrt(M_PI / 3)), a(M_PI * M_PI / 3), tolerance(0.001) {}
  ~FFTTest() override {}

  void testForward() {
    const int N = 100;
    const double XX = dX * N;
    double dx = 1 / (XX);

    const auto inputWS = createWS(N, 0);

    IAlgorithm *fft =
        Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setChild(true);
    fft->setProperty("InputWorkspace", inputWS);
    fft->setPropertyValue("OutputWorkspace", "__NotUsed");
    fft->setPropertyValue("Real", "0");
    fft->execute();

    MatrixWorkspace_sptr fWS = fft->getProperty("OutputWorkspace");
    TS_ASSERT(fWS);

    const MantidVec &X = fWS->readX(3);
    const MantidVec &Yr = fWS->readY(3);
    const MantidVec &Yi = fWS->readY(4);

    const MantidVec::const_iterator it = std::find(X.begin(), X.end(), 0.);

    TS_ASSERT(it != X.end());
    int i0 = static_cast<int>(it - X.begin());

    for (int i = 0; i < N / 4; i++) {
      int j = i0 + i;
      double x = X[j];
      TS_ASSERT_DELTA(x, dx * i, 0.00001);
      TS_ASSERT_DELTA(Yr[i0 + i] / (h * exp(-a * x * x)), 1., 0.001);
      TS_ASSERT_DELTA(Yi[i0 + i], 0.0, 0.00001);
      TS_ASSERT_DELTA(Yr[i0 - i] / (h * exp(-a * x * x)), 1., 0.001);
      TS_ASSERT_DELTA(Yi[i0 - i], 0.0, 0.00001);
    }
  }

  void testBackward() {
    const int N = 100;

    MatrixWorkspace_sptr inputWS = createWS(N, 0);

    IAlgorithm *fft =
        Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setChild(true);
    fft->setProperty("InputWorkspace", inputWS);
    fft->setPropertyValue("OutputWorkspace", "__NotUsed");
    fft->setPropertyValue("Real", "0");
    fft->execute();

    MatrixWorkspace_sptr intermediate = fft->getProperty("OutputWorkspace");
    TS_ASSERT(intermediate);

    fft = Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setChild(true);
    fft->setProperty("InputWorkspace", intermediate);
    fft->setPropertyValue("OutputWorkspace", "__NotUsed");
    fft->setPropertyValue("Real", "3");
    fft->setPropertyValue("Imaginary", "4");
    fft->setPropertyValue("Transform", "Backward");
    fft->execute();

    MatrixWorkspace_sptr outWS = fft->getProperty("OutputWorkspace");
    TS_ASSERT(outWS);

    const MantidVec &Y0 = inputWS->readY(0);

    const MantidVec &X = outWS->readX(0);
    const MantidVec &Y = outWS->readY(0);

    for (int i = 0; i < N; i++) {
      TS_ASSERT_DELTA(X[i], dX * (i - N / 2), 0.00001);
      TS_ASSERT_DELTA(Y[i], Y0[i], 0.00001);
    }
  }

  void testForwardHist() {
    const int N = 100;
    const double XX = dX * N;
    double dx = 1 / (XX);

    auto inputWS = createWS(N, 1);

    IAlgorithm *fft =
        Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setChild(true);
    fft->setProperty("InputWorkspace", inputWS);
    fft->setPropertyValue("OutputWorkspace", "__NotUsed");
    fft->setPropertyValue("Real", "0");
    fft->execute();

    MatrixWorkspace_sptr fWS = fft->getProperty("OutputWorkspace");
    TS_ASSERT(fWS);

    const MantidVec &X = fWS->readX(3);
    const MantidVec &Yr = fWS->readY(3);
    const MantidVec &Yi = fWS->readY(4);

    const MantidVec::const_iterator it = std::find(X.begin(), X.end(), 0.);

    TS_ASSERT(it != X.end());
    int i0 = static_cast<int>(it - X.begin());

    for (int i = 0; i < N / 4; i++) {
      int j = i0 + i;
      double x = X[j];
      TS_ASSERT_DELTA(x, dx * i, 0.00001);
      TS_ASSERT_DELTA(Yr[i0 + i] / (h * exp(-a * x * x)), 1., 0.001);
      TS_ASSERT_DELTA(Yi[i0 + i], 0.0, 0.00001);
      TS_ASSERT_DELTA(Yr[i0 - i] / (h * exp(-a * x * x)), 1., 0.001);
      TS_ASSERT_DELTA(Yi[i0 - i], 0.0, 0.00001);
    }
  }

  void testBackwardHist() {
    const int N = 100;

    MatrixWorkspace_sptr inWS = createWS(N, 1);

    IAlgorithm *fft =
        Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setChild(true);
    fft->setProperty("InputWorkspace", inWS);
    fft->setPropertyValue("OutputWorkspace", "__NotUsed");
    fft->setPropertyValue("Real", "0");
    fft->execute();

    MatrixWorkspace_sptr intermediate = fft->getProperty("OutputWorkspace");
    TS_ASSERT(intermediate);

    fft = Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setChild(true);
    fft->setProperty("InputWorkspace", intermediate);
    fft->setPropertyValue("OutputWorkspace", "__NotUsed");
    fft->setPropertyValue("Real", "3");
    fft->setPropertyValue("Imaginary", "4");
    fft->setPropertyValue("Transform", "Backward");
    fft->execute();

    MatrixWorkspace_sptr fWS = fft->getProperty("OutputWorkspace");
    TS_ASSERT(fWS);

    const MantidVec &Y0 = inWS->readY(0);

    const MantidVec &X = fWS->readX(0);
    const MantidVec &Y = fWS->readY(0);

    for (int i = 0; i < N; i++) {
      TS_ASSERT_DELTA(X[i], dX * (i - N / 2), 0.00001);
      TS_ASSERT_DELTA(Y[i], Y0[i], 0.00001);
    }
  }

  void testOddForward() {
    const int N = 101;
    const double XX = dX * N;
    double dx = 1 / (XX);

    const auto inWS = createWS(N, 0);

    IAlgorithm *fft =
        Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setChild(true);
    fft->setProperty("InputWorkspace", inWS);
    fft->setPropertyValue("OutputWorkspace", "__NotUsed");
    fft->setPropertyValue("Real", "0");
    fft->execute();

    MatrixWorkspace_sptr fWS = fft->getProperty("OutputWorkspace");
    TS_ASSERT(fWS);

    const MantidVec &X = fWS->readX(3);
    const MantidVec &Yr = fWS->readY(3);
    const MantidVec &Yi = fWS->readY(4);

    const MantidVec::const_iterator it = std::find(X.begin(), X.end(), 0.);

    TS_ASSERT(it != X.end());
    int i0 = static_cast<int>(it - X.begin());

    for (int i = 0; i < N / 4; i++) {
      int j = i0 + i;
      double x = X[j];
      TS_ASSERT_DELTA(x, dx * i, 0.00001);
      TS_ASSERT_DELTA(Yr[i0 + i] / (h * exp(-a * x * x)), 1., 0.001);
      TS_ASSERT_DELTA(Yi[i0 + i], 0.0, 0.00001);
      TS_ASSERT_DELTA(Yr[i0 - i] / (h * exp(-a * x * x)), 1., 0.001);
      TS_ASSERT_DELTA(Yi[i0 - i], 0.0, 0.00001);
    }
  }

  void testOddBackward() {
    const int N = 101;

    const MatrixWorkspace_sptr inWS = createWS(N, 0);

    IAlgorithm *fft =
        Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setChild(true);
    fft->setProperty("InputWorkspace", inWS);
    fft->setPropertyValue("OutputWorkspace", "__NotUsed");
    fft->setPropertyValue("Real", "0");
    fft->execute();

    const MatrixWorkspace_sptr intermediate =
        fft->getProperty("OutputWorkspace");
    TS_ASSERT(intermediate);

    fft = Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setChild(true);
    fft->setProperty("InputWorkspace", intermediate);
    fft->setPropertyValue("OutputWorkspace", "__NotUsed");
    fft->setPropertyValue("Real", "3");
    fft->setPropertyValue("Imaginary", "4");
    fft->setPropertyValue("Transform", "Backward");
    fft->execute();

    MatrixWorkspace_sptr fWS = fft->getProperty("OutputWorkspace");
    TS_ASSERT(fWS);

    const MantidVec &Y0 = inWS->readY(0);

    const MantidVec &X = fWS->readX(0);
    const MantidVec &Y = fWS->readY(0);

    for (int i = 0; i < N; i++) {
      TS_ASSERT_DELTA(X[i], dX * (i - N / 2), 0.00001);
      TS_ASSERT_DELTA(Y[i], Y0[i], 0.00001);
    }
  }

  void testOddForwardHist() {
    const int N = 101;
    const double XX = dX * N;
    double dx = 1 / (XX);

    const auto inWS = createWS(N, 1);

    IAlgorithm *fft =
        Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setChild(true);
    fft->setProperty("InputWorkspace", inWS);
    fft->setPropertyValue("OutputWorkspace", "__NotUsed");
    fft->setPropertyValue("Real", "0");
    fft->execute();

    MatrixWorkspace_sptr fWS = fft->getProperty("OutputWorkspace");
    TS_ASSERT(fWS);

    const MantidVec &X = fWS->readX(3);
    const MantidVec &Yr = fWS->readY(3);
    const MantidVec &Yi = fWS->readY(4);

    const MantidVec::const_iterator it = std::find(X.begin(), X.end(), 0.);

    TS_ASSERT(it != X.end());
    int i0 = static_cast<int>(it - X.begin());

    for (int i = 0; i < N / 4; i++) {
      int j = i0 + i;
      double x = X[j];
      TS_ASSERT_DELTA(x, dx * i, 0.00001);
      TS_ASSERT_DELTA(Yr[i0 + i] / (h * exp(-a * x * x)), 1., 0.001);
      TS_ASSERT_DELTA(Yi[i0 + i], 0.0, 0.00001);
      TS_ASSERT_DELTA(Yr[i0 - i] / (h * exp(-a * x * x)), 1., 0.001);
      TS_ASSERT_DELTA(Yi[i0 - i], 0.0, 0.00001);
    }
  }

  void testOddBackwardHist() {
    const int N = 101;

    const MatrixWorkspace_sptr inWS = createWS(N, 1);

    IAlgorithm *fft =
        Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setChild(true);
    fft->setProperty("InputWorkspace", inWS);
    fft->setPropertyValue("OutputWorkspace", "__NotUsed");
    fft->setPropertyValue("Real", "0");
    fft->execute();

    const MatrixWorkspace_sptr intermediate =
        fft->getProperty("OutputWorkspace");
    TS_ASSERT(intermediate);

    fft = Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setChild(true);
    fft->setProperty("InputWorkspace", intermediate);
    fft->setPropertyValue("OutputWorkspace", "__NotUsed");
    fft->setPropertyValue("Real", "3");
    fft->setPropertyValue("Imaginary", "4");
    fft->setPropertyValue("Transform", "Backward");
    fft->execute();

    MatrixWorkspace_sptr fWS = fft->getProperty("OutputWorkspace");

    const MantidVec &Y0 = inWS->readY(0);

    const MantidVec &X = fWS->readX(0);
    const MantidVec &Y = fWS->readY(0);

    for (int i = 0; i < N; i++) {
      TS_ASSERT_DELTA(X[i], dX * (i - N / 2), 0.00001);
      TS_ASSERT_DELTA(Y[i], Y0[i], 0.00001);
    }
  }

  void testInputImaginary() {
    const int N = 100;
    const double XX = dX * N;
    double dx = 1 / (XX);

    const auto realWS = createWS(N, 0);
    const auto imagWS = createWS(N, 0);

    IAlgorithm *fft =
        Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setChild(true);
    fft->setProperty("InputWorkspace", realWS);
    fft->setProperty("InputImagWorkspace", imagWS);
    fft->setPropertyValue("OutputWorkspace", "__NotUsed");
    fft->setPropertyValue("Real", "0");
    fft->setPropertyValue("Imaginary", "0");
    fft->execute();

    MatrixWorkspace_sptr fWS = fft->getProperty("OutputWorkspace");
    TS_ASSERT(fWS);

    // Test the output label
    TS_ASSERT_EQUALS(fWS->getAxis(0)->unit()->caption(), "Quantity");
    TS_ASSERT_EQUALS(fWS->getAxis(0)->unit()->label(), "");

    const MantidVec &X = fWS->readX(0);
    const MantidVec &Yr = fWS->readY(0);
    const MantidVec &Yi = fWS->readY(1);

    const MantidVec::const_iterator it = std::find(X.begin(), X.end(), 0.);

    TS_ASSERT(it != X.end());
    int i0 = static_cast<int>(it - X.begin());

    for (int i = 0; i < N / 4; i++) {
      int j = i0 + i;
      double x = X[j];
      TS_ASSERT_DELTA(x, dx * i, 0.00001);
      TS_ASSERT_DELTA(Yr[i0 + i] / (h * exp(-a * x * x)), 1., 0.001);
      TS_ASSERT_DELTA(Yi[i0 + i] / (h * exp(-a * x * x)), 1., 0.001);
      TS_ASSERT_DELTA(Yr[i0 - i] / (h * exp(-a * x * x)), 1., 0.001);
      TS_ASSERT_DELTA(Yi[i0 - i] / (h * exp(-a * x * x)), 1., 0.001);
    }
  }

  void testUnitsEnergy() {

    const int N = 100;
    const double XX = dX * N;
    double dx = 1 / (XX);

    MatrixWorkspace_sptr inWS = createWS(N, 1);

    // Set a label
    inWS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("Energy");

    IAlgorithm *fft =
        Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setChild(true);
    fft->setProperty("InputWorkspace", inWS);
    fft->setPropertyValue("OutputWorkspace", "__NotUsed");
    fft->setPropertyValue("Real", "0");
    fft->execute();

    MatrixWorkspace_sptr fWS = fft->getProperty("OutputWorkspace");
    TS_ASSERT(fWS);

    // Test X values
    // When the input unit is 'Energy' in 'meV'
    // there is a factor of 1/2.418e2 in X
    const MantidVec &X = fWS->readX(0);

    const MantidVec::const_iterator it = std::find(X.begin(), X.end(), 0.);
    int i0 = static_cast<int>(it - X.begin());

    for (int i = 0; i < N / 4; i++) {
      int j = i0 + i;
      double x = X[j];
      TS_ASSERT_DELTA(x, dx * i / 2.418e2, 0.00001);
    }

    // Test the output label
    TS_ASSERT_EQUALS(fWS->getAxis(0)->unit()->caption(), "Time");
    TS_ASSERT_EQUALS(fWS->getAxis(0)->unit()->label(), "ns");
  }

  // Test that unevenly spaced X values are rejected by default
  void testUnequalBinWidths_Throws() {
    const int N = 100;
    auto inputWS = createWS(N, 0);
    Mantid::MantidVec &X = inputWS->dataX(0);
    double aveX = (X[51] + X[49]) / 2.0;
    X[50] = aveX + 0.01;

    auto fft = FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setChild(true);
    fft->setProperty("InputWorkspace", inputWS);
    fft->setPropertyValue("OutputWorkspace", "__NotUsed");
    fft->setPropertyValue("Real", "0");
    TS_ASSERT_THROWS(fft->execute(), std::runtime_error);
  }

  // Test that unevenly spaced X values are accepted if the property is set to
  // do so
  void testUnequalBinWidths_acceptRoundingErrors() {
    const int N = 100;
    auto inputWS = createWS(N, 0);
    Mantid::MantidVec &X = inputWS->dataX(0);
    double aveX = (X[51] + X[49]) / 2.0;
    X[50] = aveX + 0.01;

    auto fft = FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setChild(true);
    fft->setProperty("InputWorkspace", inputWS);
    fft->setPropertyValue("OutputWorkspace", "__NotUsed");
    fft->setPropertyValue("Real", "0");
    fft->setProperty("AcceptXRoundingErrors", true);
    TS_ASSERT_THROWS_NOTHING(fft->execute());
  }

  // Test that algorithm will not accept an empty input workspace
  void testEmptyInputWorkspace_Throws() {
    auto inputWS = createWS(1, 0);
    auto fft = FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setChild(true);
    fft->setProperty("InputWorkspace", inputWS);
    fft->setPropertyValue("OutputWorkspace", "__NotUsed");
    fft->setPropertyValue("Real", "0");
    TS_ASSERT_THROWS(fft->execute(), std::runtime_error);
  }

  void testRealOutOfRange_Throws() {
    auto inputWS = createWS(100, 0);
    auto fft = FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setChild(true);
    fft->setProperty("InputWorkspace", inputWS);
    fft->setPropertyValue("OutputWorkspace", "__NotUsed");
    fft->setPropertyValue("Real", "100");
    TS_ASSERT_THROWS(fft->execute(), std::runtime_error);
  }

  void testImaginaryOutOfRange_Throws() {
    auto inputWS = createWS(100, 0);
    auto fft = FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setChild(true);
    fft->setProperty("InputWorkspace", inputWS);
    fft->setPropertyValue("OutputWorkspace", "__NotUsed");
    fft->setPropertyValue("Real", "0");
    fft->setPropertyValue("Imaginary", "100");
    TS_ASSERT_THROWS(fft->execute(), std::runtime_error);
  }

  void testRealImaginarySizeMismatch_Throws() {
    auto inputWS = createWS(100, 0);
    auto inImagWS = createWS(99, 0);
    auto fft = FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setChild(true);
    fft->setProperty("InputWorkspace", inputWS);
    fft->setPropertyValue("OutputWorkspace", "__NotUsed");
    fft->setPropertyValue("Real", "0");
    fft->setPropertyValue("Imaginary", "0");
    fft->setProperty("InputImagWorkspace", inImagWS);
    TS_ASSERT_THROWS(fft->execute(), std::runtime_error);
  }

  /**
 * Test that the algorithm can handle a WorkspaceGroup as input without
 * crashing
 * We have to use the ADS to test WorkspaceGroups
 */
  void testValidateInputsWithWSGroup() {
    auto ws1 =
        boost::static_pointer_cast<Workspace>(createWS(100, 0, "real_1"));
    auto ws2 =
        boost::static_pointer_cast<Workspace>(createWS(100, 0, "real_2"));
    auto group = boost::make_shared<WorkspaceGroup>();
    AnalysisDataService::Instance().add("group", group);
    group->addWorkspace(ws1);
    group->addWorkspace(ws2);
    TestFFT fft;
    fft.initialize();
    fft.setChild(true);
    TS_ASSERT_THROWS_NOTHING(fft.setPropertyValue("InputWorkspace", "group"));
    fft.setPropertyValue("OutputWorkspace", "__NotUsed");
    fft.setPropertyValue("Real", "0");
    fft.setPropertyValue("Imaginary", "0");
    TS_ASSERT_THROWS_NOTHING(fft.wrapValidateInputs());
    AnalysisDataService::Instance().clear();
  }

  void test_AutoShift() {
    // create two workspaces offset from each other
    const auto inputWS = createWS(100, 0);
    auto offsetWS = offsetWorkspace(createWS(100, 0));

    // perform transforms for no shift and auto shift
    auto fftNoShiftNoOffset = doFFT(inputWS, false, false);
    auto fftNoShiftWithOffset = doFFT(offsetWS, false, false);
    auto fftAutoShiftNoOffset = doFFT(inputWS, false, true);
    auto fftAutoShiftWithOffset = doFFT(offsetWS, false, true);

    // perform transforms for manual shift
    auto fft = FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setChild(true);
    fft->setPropertyValue("OutputWorkspace", "__NotUsed");
    fft->setPropertyValue("Real", "0");
    const auto &noOffsetX = inputWS->readX(0);
    const auto &offsetX = offsetWS->readX(0);
    const double offsetShift = -offsetX[offsetX.size() / 2];
    const double noOffsetShift = -noOffsetX[noOffsetX.size() / 2];
    fft->setProperty("InputWorkspace", inputWS);
    fft->setProperty("AutoShift", false);
    fft->setProperty("Shift", noOffsetShift);
    fft->execute();
    MatrixWorkspace_sptr fftManualShiftNoOffset =
        fft->getProperty("OutputWorkspace");
    fft->setProperty("InputWorkspace", offsetWS);
    fft->setProperty("Shift", offsetShift);
    fft->execute();
    MatrixWorkspace_sptr fftManualShiftWithOffset =
        fft->getProperty("OutputWorkspace");

    // No shift - should match
    TS_ASSERT(Mantid::API::equals(fftNoShiftNoOffset, fftNoShiftWithOffset,
                                  tolerance));
    // Shift - should have a phase difference (correct)
    TS_ASSERT(!Mantid::API::equals(fftAutoShiftNoOffset, fftAutoShiftWithOffset,
                                   tolerance));
    TS_ASSERT(!Mantid::API::equals(fftManualShiftNoOffset,
                                   fftManualShiftWithOffset, tolerance));
    // Manual shift by -X[N/2] should do the same as auto shift
    TS_ASSERT(Mantid::API::equals(fftAutoShiftNoOffset, fftManualShiftNoOffset,
                                  tolerance));
    TS_ASSERT(Mantid::API::equals(fftAutoShiftWithOffset,
                                  fftManualShiftWithOffset, tolerance));
  }

  void test_complexWorkspace_phase() {
    const auto inputWS = createComplexWorkspace(100, 10);
    doPhaseTest(inputWS, true);
  }

  /**
   * This test suggested by instrument scientists
   * The transform of a symmetric pulse centered at t=0 should have phase=0 (or
   * pi) for all frequency components
   */
  void test_GaussianBurst_phase() {
    // These Gaussian bursts are not the same
    const auto inputWSOne = createPulseWS(1000, 41.76, 0.0, -2.32, 2.51);
    const auto inputWSTwo = createPulseWS(1000, 41.76, 0.0, -3.123, 2.51);
    TS_ASSERT(!Mantid::API::equals(inputWSOne, inputWSTwo, tolerance));

    // but their transforms should be the same
    const auto fftOne = doFFT(inputWSOne, true, true);
    const auto fftTwo = doFFT(inputWSTwo, true, true);
    TS_ASSERT(Mantid::API::equals(fftOne, fftTwo, tolerance));
  }

  /**
   * Test suggested by instrument scientists
   * Generate the same function with different X point spacing
   * Transforms should match (although with different point spacing)
   */
  void test_GaussianBurst_xSpacing() {
    // Same function, different number of points
    const auto inputWSOne = createPulseWS(1000, 41.76, 0.0, -2.32, 2.51);
    const auto inputWSTwo = createPulseWS(500, 41.76, 0.0, -2.32, 2.51);
    const auto fftOne = doFFT(inputWSOne, true, true);
    const auto fftTwo = doFFT(inputWSOne, true, true);
    // Rebin due to different point spacing
    const auto rebinOne = doRebin(fftOne, "-20, 0.1, 20");
    const auto rebinTwo = doRebin(fftTwo, "-20, 0.1, 20");
    TS_ASSERT(Mantid::API::equals(rebinOne, rebinTwo, tolerance));
  }

private:
  MatrixWorkspace_sptr doRebin(MatrixWorkspace_sptr inputWS,
                               const std::string &params) {
    auto rebin = FrameworkManager::Instance().createAlgorithm("Rebin");
    rebin->initialize();
    rebin->setChild(true);
    rebin->setProperty("InputWorkspace", inputWS);
    rebin->setPropertyValue("OutputWorkspace", "__NotUsed");
    rebin->setPropertyValue("Params", params);
    rebin->execute();
    return rebin->getProperty("OutputWorkspace");
  }

  MatrixWorkspace_sptr doFFT(MatrixWorkspace_sptr inputWS, const bool complex,
                             const bool phaseShift) {
    auto fft = FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setChild(true);
    fft->setProperty("InputWorkspace", inputWS);
    fft->setPropertyValue("OutputWorkspace", "__NotUsed");
    fft->setPropertyValue("Real", "0");
    if (complex) {
      fft->setPropertyValue("Imaginary", "1");
    }
    if (phaseShift) {
      fft->setProperty("AutoShift", true);
    }
    fft->execute();
    return fft->getProperty("OutputWorkspace");
  }

  void doPhaseTest(MatrixWorkspace_sptr inputWS, bool complex) {
    // Offset the input workspace
    const auto offsetWS = offsetWorkspace(inputWS);

    // perform transforms
    auto fftNoShiftNoOffset = doFFT(inputWS, complex, false);
    auto fftNoShiftWithOffset = doFFT(offsetWS, complex, false);
    auto fftAutoShiftNoOffset = doFFT(inputWS, complex, true);
    auto fftAutoShiftWithOffset = doFFT(offsetWS, complex, true);

    // No shift - should match
    TS_ASSERT(Mantid::API::equals(fftNoShiftNoOffset, fftNoShiftWithOffset,
                                  tolerance));
    // Shift - should have a phase difference (correct)
    TS_ASSERT(!Mantid::API::equals(fftAutoShiftNoOffset, fftAutoShiftWithOffset,
                                   tolerance));
  }

  MatrixWorkspace_sptr createWS(int n, int dn) {
    Mantid::DataObjects::Workspace2D_sptr ws =
        boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
            WorkspaceFactory::Instance().create("Workspace2D", 1, n + dn, n));

    Mantid::MantidVec &X = ws->dataX(0);
    Mantid::MantidVec &Y = ws->dataY(0);
    Mantid::MantidVec &E = ws->dataE(0);

    int n2 = n / 2;
    for (int k = 0; k <= n2; k++) {
      int i = n2 - k;
      if (i >= 0) {
        X[i] = -dX * (k);
        Y[i] = exp(-X[i] * X[i] * 3.);
        E[i] = 1.;
      }
      i = n2 + k;
      if (i < n) {
        X[i] = dX * (k);
        Y[i] = exp(-X[i] * X[i] * 3.);
        E[i] = 1.;
      }
    }

    if (dn > 0)
      X[n] = X[n - 1] + dX;

    return ws;
  }

  MatrixWorkspace_sptr createWS(int n, int dn, const std::string &name) {
    FrameworkManager::Instance();
    MatrixWorkspace_sptr ws = createWS(n, dn);
    AnalysisDataService::Instance().add("FFT_WS_" + name, ws);
    return ws;
  }

  MatrixWorkspace_sptr offsetWorkspace(MatrixWorkspace_sptr workspace) {
    auto scaleX = FrameworkManager::Instance().createAlgorithm("ScaleX");
    scaleX->initialize();
    scaleX->setChild(true);
    scaleX->setProperty("InputWorkspace", workspace);
    scaleX->setPropertyValue("OutputWorkspace", "__NotUsed");
    scaleX->setProperty("Factor", "1"); // 1 us offset is 62.5 bins
    scaleX->setProperty("Operation", "Add");
    scaleX->execute();
    return scaleX->getProperty("OutputWorkspace");
  }

  MatrixWorkspace_sptr createComplexWorkspace(const size_t n,
                                              const double omega) {
    Mantid::MantidVec X, Y, E;
    X.reserve(2 * n);
    Y.reserve(2 * n);
    E.reserve(2 * n);
    // real spectrum
    for (size_t i = 0; i < n; i++) {
      double x = 2.0 * M_PI * double(i) / double(n);
      X.push_back(x);
      Y.push_back(cos(omega * x));
      E.push_back(0.1);
    }
    // imaginary spectrum
    for (size_t i = n; i < 2 * n; i++) {
      double j = (double)(i - n);
      double x = 2.0 * M_PI * j / double(n);
      X.push_back(x);
      Y.push_back(sin(omega * x));
      E.push_back(0.1);
    }
    auto create =
        FrameworkManager::Instance().createAlgorithm("CreateWorkspace");
    create->initialize();
    create->setChild(true);
    create->setProperty("DataX", X);
    create->setProperty("DataY", Y);
    create->setProperty("DataE", E);
    create->setProperty("NSpec", 2);
    create->setPropertyValue("OutputWorkspace", "__NotUsed");
    create->execute();
    return create->getProperty("OutputWorkspace");
  }

  MatrixWorkspace_sptr createPulseWS(const size_t n, const double omega,
                                     const double x0, const double factor,
                                     const double sigma) {
    Mantid::MantidVec X, Y, E;
    X.reserve(2 * n);
    Y.reserve(2 * n);
    E.reserve(2 * n);
    // real spectrum
    for (size_t i = 0; i < n; i++) {
      double x = (2.0 * M_PI * double(i) / double(n)) + factor;
      X.push_back(x);
      double y = cos(omega * x);
      y *= exp(-1.0 * (pow((x - x0) * sigma, 2.0)));
      Y.push_back(y);
      E.push_back(0.1);
    }
    // imaginary spectrum
    for (size_t i = n; i < 2 * n; i++) {
      double j = (double)(i - n);
      double x = (2.0 * M_PI * j / double(n)) + factor;
      X.push_back(x);
      double y = sin(omega * x);
      y *= exp(-1.0 * (pow((x - x0) * sigma, 2.0)));
      Y.push_back(y);
      E.push_back(0.1);
    }
    auto create =
        FrameworkManager::Instance().createAlgorithm("CreateWorkspace");
    create->initialize();
    create->setChild(true);
    create->setProperty("DataX", X);
    create->setProperty("DataY", Y);
    create->setProperty("DataE", E);
    create->setProperty("NSpec", 2);
    create->setPropertyValue("OutputWorkspace", "__NotUsed");
    create->execute();
    return create->getProperty("OutputWorkspace");
  }

  const double dX;
  const double h;
  const double a;
  const double tolerance;
};

#endif /*FFT_TEST_H_*/
