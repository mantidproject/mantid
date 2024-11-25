// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAlgorithms/CropWorkspace.h"
#include "MantidAlgorithms/Q1D2.h"
#include "MantidAlgorithms/Rebin.h"
#include "MantidDataHandling/LoadRKH.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataHandling/MaskDetectors.h"
#include <cxxtest/TestSuite.h>

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

#include <algorithm>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

/// data for the pixel (flood file) correction
static double flat_cell061Ys[] = {1.002863E+00, 1.032594E+00, 1.017332E+00, 1.062325E+00, 1.004316E+00, 1.041249E+00,
                                  1.023080E+00, 1.011716E+00, 1.012046E+00, 1.066157E+00, 9.934810E-01, 1.087497E+00,
                                  9.593894E-01, 1.060872E+00, 1.022618E+00, 1.054595E+00, 1.042901E+00, 1.064241E+00,
                                  1.035699E+00, 1.048186E+00, 1.020834E+00, 1.063712E+00, 1.034774E+00, 1.025458E+00,
                                  9.860153E-01, 1.044222E+00, 9.872045E-01, 1.046006E+00, 9.772280E-01, 1.011782E+00};
static double flat_cell061Es[] = {8.140295E-03, 8.260089E-03, 8.198814E-03, 8.378171E-03, 8.146192E-03, 8.294637E-03,
                                  8.221945E-03, 8.176151E-03, 8.177485E-03, 8.393270E-03, 8.102125E-03, 8.476863E-03,
                                  7.961886E-03, 8.372437E-03, 8.220086E-03, 8.347631E-03, 8.301214E-03, 8.385724E-03,
                                  8.272501E-03, 8.322226E-03, 8.212913E-03, 8.383642E-03, 8.268805E-03, 8.231497E-03,
                                  8.071622E-03, 8.306473E-03, 8.076489E-03, 8.313566E-03, 8.035571E-03, 8.176417E-03};

/// defined below this creates some input data
void createInputWorkspaces(int start, int end, Mantid::API::MatrixWorkspace_sptr &input,
                           Mantid::API::MatrixWorkspace_sptr &wave, Mantid::API::MatrixWorkspace_sptr &pixels);
void createInputWorkSpacesForMasking(Mantid::API::MatrixWorkspace_sptr &input, Mantid::API::MatrixWorkspace_sptr &wave,
                                     Mantid::API::MatrixWorkspace_sptr &pixels);
void createQResolutionWorkspace(Mantid::API::MatrixWorkspace_sptr &qResolution,
                                Mantid::API::MatrixWorkspace_sptr &alteredInput,
                                Mantid::API::MatrixWorkspace_sptr &input, double value1, double value2);

class Q1D2Test : public CxxTest::TestSuite {
public:
  void testStatics() {
    Mantid::Algorithms::Q1D2 Q1D2;
    TS_ASSERT_EQUALS(Q1D2.name(), "Q1D")
    TS_ASSERT_EQUALS(Q1D2.version(), 2)
    TS_ASSERT_EQUALS(Q1D2.category(), "SANS")
  }

  /// Test that we can run without the an optional workspace
  void testNoPixelAdj() {
    Mantid::Algorithms::Q1D2 Q1D2;
    Q1D2.initialize();

    const std::string outputWS("Q1D2Test_result");
    TS_ASSERT_THROWS_NOTHING(
        Q1D2.setProperty("DetBankWorkspace", m_inputWS); Q1D2.setProperty("WavelengthAdj", m_wavNorm);
        Q1D2.setPropertyValue("OutputWorkspace", outputWS); Q1D2.setPropertyValue("OutputBinning", "0,0.02,0.5");
        // The property PixelAdj is undefined but that shouldn't cause this to
        // throw
        Q1D2.execute())

    TS_ASSERT(Q1D2.isExecuted())

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(result = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)))
    TS_ASSERT_EQUALS(result->isDistribution(), true)
    TS_ASSERT_EQUALS(result->getAxis(0)->unit()->unitID(), "MomentumTransfer")
    TS_ASSERT_EQUALS(result->getNumberHistograms(), 1)

    TS_ASSERT_EQUALS(result->x(0).size(), 26)
    TS_ASSERT_DELTA(result->x(0).front(), 0, 1e-5)
    TS_ASSERT_DELTA(result->x(0)[6], 0.12, 1e-5)
    TS_ASSERT_DELTA(result->x(0).back(), 0.5, 1e-5)

    // values below taken from running the algorithm in the state it was
    // excepted by the ISIS SANS in
    // empty bins are 0/0
    TS_ASSERT_DELTA(result->y(0).front(), 2226533, 1)
    TS_ASSERT_DELTA(result->y(0)[4], 946570.8, 0.1)
    TS_ASSERT(std::isnan(result->y(0)[18]))
    TS_ASSERT(std::isnan(result->y(0).back()))

    TS_ASSERT_DELTA(result->e(0)[1], 57964.04, 0.01)
    TS_ASSERT_DELTA(result->e(0)[5], 166712.6, 0.1)
    TS_ASSERT(std::isnan(result->e(0).back()))

    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

  void testOutputParts() {
    Mantid::Algorithms::Q1D2 Q1D2;
    Q1D2.initialize();

    const std::string outputWS("Q1D2Test_OutputParts");
    TS_ASSERT_THROWS_NOTHING(
        Q1D2.setProperty("DetBankWorkspace", m_inputWS); Q1D2.setProperty("WavelengthAdj", m_wavNorm);
        Q1D2.setPropertyValue("OutputWorkspace", outputWS); Q1D2.setPropertyValue("OutputBinning", "0,0.02,0.5");
        Q1D2.setProperty("OutputParts", true);

        // The property PixelAdj is undefined but that shouldn't cause this to
        // throw
        Q1D2.execute())

    TS_ASSERT(Q1D2.isExecuted())

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(result = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)))

    Mantid::API::MatrixWorkspace_sptr sumOfCounts;
    TS_ASSERT_THROWS_NOTHING(sumOfCounts = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 Mantid::API::AnalysisDataService::Instance().retrieve(outputWS + "_sumOfCounts")))

    Mantid::API::MatrixWorkspace_sptr sumOfNormFactors;
    TS_ASSERT_THROWS_NOTHING(sumOfNormFactors = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 Mantid::API::AnalysisDataService::Instance().retrieve(outputWS + "_sumOfNormFactors")))

    TS_ASSERT_DELTA(result->x(0)[10], 0.1999, 0.0001)
    TS_ASSERT_DELTA(sumOfCounts->x(0)[10], 0.1999, 0.0001)
    TS_ASSERT_DELTA(sumOfNormFactors->x(0)[10], 0.1999, 0.0001)

    TS_ASSERT_DELTA(result->y(0)[1], 1131778.3299, 0.01)
    TS_ASSERT_DELTA(sumOfCounts->y(0)[1], 1016.8990, 0.01)
    TS_ASSERT_DELTA(sumOfNormFactors->y(0)[1], 0.00089849, 0.01)

    TS_ASSERT_DELTA(result->e(0)[1], 57964.04, 0.01)
    TS_ASSERT_DELTA(sumOfCounts->e(0)[1], 31.888, 0.01)
    TS_ASSERT_DELTA(sumOfNormFactors->e(0)[1], 3.6381851288154988e-005, 0.01)

    TS_ASSERT_EQUALS(result->getNumberHistograms(), 1)
    TS_ASSERT_EQUALS(sumOfCounts->getNumberHistograms(), 1)
    TS_ASSERT_EQUALS(sumOfNormFactors->getNumberHistograms(), 1)

    TS_ASSERT_EQUALS(result->y(0).size(), 25)
    TS_ASSERT_EQUALS(sumOfCounts->y(0).size(), 25)

    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS + "_sumOfCounts");
    Mantid::API::AnalysisDataService::Instance().remove(outputWS + "_sumOfNormFactors");
  }

  void testPixelAdj() {
    Mantid::Algorithms::Q1D2 Q1D;
    Q1D.initialize();

    TS_ASSERT_THROWS_NOTHING(Q1D.setProperty("DetBankWorkspace", m_inputWS);
                             Q1D.setProperty("WavelengthAdj", m_wavNorm); Q1D.setProperty("PixelAdj", m_pixel);
                             Q1D.setPropertyValue("OutputWorkspace", m_noGrav);
                             Q1D.setPropertyValue("OutputBinning", "0.1,-0.02,0.5");)
    // #default is don't correct for gravity
    TS_ASSERT_THROWS_NOTHING(Q1D.execute())
    TS_ASSERT(Q1D.isExecuted())

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(result = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 Mantid::API::AnalysisDataService::Instance().retrieve(m_noGrav)))
    TS_ASSERT(result)
    TS_ASSERT_EQUALS(result->getNumberHistograms(), 1)

    TS_ASSERT_EQUALS(result->x(0).size(), 83)
    TS_ASSERT_EQUALS(result->x(0).front(), 0.1)
    TS_ASSERT_DELTA(result->x(0)[3], 0.1061208, 1e-6)
    TS_ASSERT_DELTA(result->x(0)[56], 0.3031165, 1e-5)
    TS_ASSERT_EQUALS(result->x(0).back(), 0.5)

    TS_ASSERT_DELTA(result->y(0).front(), 944237.8, 0.1)
    TS_ASSERT_DELTA(result->y(0)[3], 1009296, 1)
    TS_ASSERT_DELTA(result->y(0)[12], 620952.6, 0.1)
    TS_ASSERT(std::isnan(result->y(0).back()))

    // empty bins are 0/0
    TS_ASSERT_DELTA(result->e(0)[2], 404981, 10)
    TS_ASSERT_DELTA(result->e(0)[10], 489710.39, 100)
    TS_ASSERT(std::isnan(result->e(0)[7]))

    TSM_ASSERT("Should not have a DX value", !result->hasDx(0))
  }

  void testInvalidPixelAdj() {
    Mantid::API::MatrixWorkspace_sptr mask_input, mask_wave, mask_pixels;
    createInputWorkSpacesForMasking(mask_input, mask_wave, mask_pixels);

    Mantid::DataHandling::MaskDetectors maskDetectors;
    Mantid::Algorithms::Q1D2 Q1D;
    Q1D.initialize();

    // First run algorithm on workspaces for masking without masking any pixels.
    // It should fail because two pixels have non-positive PixelAdj values
    const std::string outputWS("Q1D2Test_invalid_Pixel_Adj");
    TS_ASSERT_THROWS_NOTHING(Q1D.setProperty("DetBankWorkspace", mask_input);
                             Q1D.setProperty("WavelengthAdj", mask_wave); Q1D.setProperty("PixelAdj", mask_pixels);
                             Q1D.setPropertyValue("OutputWorkspace", outputWS);
                             Q1D.setPropertyValue("OutputBinning", "0.1,-0.02,0.5");)
    Q1D.execute();

    TS_ASSERT(!Q1D.isExecuted())

    // Secondly we mask the detectors for these two pixels
    maskDetectors.initialize();
    maskDetectors.setPropertyValue("Workspace", "Q1D2Test_inputworkspace_for_masking");
    ;
    maskDetectors.setPropertyValue("WorkspaceIndexList", "5,6");
    maskDetectors.execute();

    // Now it should work
    TS_ASSERT_THROWS_NOTHING(Q1D.execute())
    TS_ASSERT(Q1D.isExecuted())
  }

  void testGravity() {
    Mantid::Algorithms::Q1D2 Q1D;
    TS_ASSERT_THROWS_NOTHING(Q1D.initialize());
    TS_ASSERT(Q1D.isInitialized())

    const std::string outputWS("Q1D2Test_result");
    TS_ASSERT_THROWS_NOTHING(
        Q1D.setProperty("DetBankWorkspace", m_inputWS); Q1D.setProperty("WavelengthAdj", m_wavNorm);
        Q1D.setProperty("PixelAdj", m_pixel); Q1D.setPropertyValue("OutputWorkspace", outputWS);
        Q1D.setPropertyValue("OutputBinning", "0.1,-0.02,0.5"); Q1D.setPropertyValue("AccountForGravity", "1");

        Q1D.execute())
    TS_ASSERT(Q1D.isExecuted())

    Mantid::API::MatrixWorkspace_sptr gravity, refNoGrav = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                                   Mantid::API::AnalysisDataService::Instance().retrieve(m_noGrav));
    TS_ASSERT_THROWS_NOTHING(gravity = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)))

    TS_ASSERT(refNoGrav)

    // TODO: Re-enable this line. Disabled by Janik Zikovsky Jul 1, 2011 because
    // I didn't know what the test was.
    // TS_ASSERT_EQUALS( (*(gravity->getAxis(1)))(0),
    // (*(refNoGrav->getAxis(1)))(0) )

    TS_ASSERT_EQUALS(gravity->x(0).size(), refNoGrav->x(0).size())
    TS_ASSERT_EQUALS(gravity->x(0)[55], refNoGrav->x(0)[55])

    TS_ASSERT_DELTA(gravity->y(0)[3], 1009296.4, 0.8)
    TS_ASSERT_DELTA(gravity->y(0)[10], 891346.9, 0.1)
    TS_ASSERT(std::isnan(gravity->y(0)[78]))

    TS_ASSERT_DELTA(gravity->e(0).front(), 329383, 1)
    TS_ASSERT_DELTA(gravity->e(0)[10], 489708, 1) // 489710
    TS_ASSERT(std::isnan(gravity->e(0)[77]))

    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

  void testCuts() {
    Mantid::Algorithms::Q1D2 Q1D;
    Q1D.initialize();

    const std::string outputWS("Q1D2Test_result");
    TS_ASSERT_THROWS_NOTHING(Q1D.setProperty("DetBankWorkspace", m_inputWS);
                             Q1D.setProperty("WavelengthAdj", m_wavNorm); Q1D.setProperty("PixelAdj", m_pixel);
                             Q1D.setPropertyValue("OutputWorkspace", outputWS);
                             Q1D.setPropertyValue("OutputBinning", "0.1,-0.02,0.5");
                             Q1D.setProperty("RadiusCut", 220.0); Q1D.setProperty("WaveCut", 8.0);)
    // #default is don't correct for gravity
    TS_ASSERT_THROWS_NOTHING(Q1D.execute())
    TS_ASSERT(Q1D.isExecuted())

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(result = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)))
    TS_ASSERT(result)
    TS_ASSERT_EQUALS(result->getNumberHistograms(), 1)

    TS_ASSERT_EQUALS(result->x(0).size(), 83)
    TS_ASSERT_EQUALS(result->x(0).front(), 0.1)
    TS_ASSERT_DELTA(result->x(0)[3], 0.1061208, 1e-6)
    TS_ASSERT_DELTA(result->x(0)[56], 0.3031165, 1e-5)
    TS_ASSERT_EQUALS(result->x(0).back(), 0.5)

    TS_ASSERT_DELTA(result->y(0).front(), 1192471.95, 0.1)
    TS_ASSERT(std::isnan(result->y(0)[3]))
    TS_ASSERT_DELTA(result->y(0)[12], 503242.79, 0.1)
    TS_ASSERT(std::isnan(result->y(0).back()))

    TS_ASSERT_DELTA(result->e(0)[2], 404980, 1)
    TS_ASSERT_DELTA(result->e(0)[10], 489708, 100)
    TS_ASSERT(std::isnan(result->e(0)[7]))
  }

  // here the cut parameters are set but should only affect detectors with lower
  // R
  void testNoCuts() {
    Mantid::Algorithms::Q1D2 Q1D;
    Q1D.initialize();

    const std::string outputWS("Q1D2Test_result");
    TS_ASSERT_THROWS_NOTHING(Q1D.setProperty("DetBankWorkspace", m_inputWS);
                             Q1D.setProperty("WavelengthAdj", m_wavNorm); Q1D.setProperty("PixelAdj", m_pixel);
                             Q1D.setPropertyValue("OutputWorkspace", outputWS);
                             Q1D.setPropertyValue("OutputBinning", "0.1,-0.02,0.5");
                             // this raduis is too small to exclude anything
                             Q1D.setProperty("RadiusCut", 50.0);
                             // this is the entire wavelength range
                             Q1D.setProperty("WaveCut", 30.0);)
    TS_ASSERT_THROWS_NOTHING(Q1D.execute())
    TS_ASSERT(Q1D.isExecuted())

    Mantid::API::MatrixWorkspace_sptr nocuts, noGrav = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                                  Mantid::API::AnalysisDataService::Instance().retrieve(m_noGrav));
    TS_ASSERT_THROWS_NOTHING(nocuts = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)))

    TS_ASSERT(nocuts)
    TS_ASSERT_EQUALS(nocuts->getNumberHistograms(), 1)

    for (size_t i = 0; i < nocuts->y(0).size(); ++i) {
      TS_ASSERT_EQUALS(nocuts->x(0)[i], noGrav->x(0)[i])
      if (!std::isnan(nocuts->y(0)[i]) && !std::isnan(nocuts->e(0)[i])) {
        TS_ASSERT_EQUALS(nocuts->y(0)[i], noGrav->y(0)[i])

        TS_ASSERT_EQUALS(nocuts->e(0)[i], noGrav->e(0)[i])
      }
    }

    Mantid::API::AnalysisDataService::Instance().remove(m_noGrav);
  }

  void testInvalidInput() {
    Mantid::Algorithms::Q1D2 Q1D;
    Q1D.initialize();

    // this is a small change to the normalization workspace that should be
    // enough to stop progress
    auto &xData = m_wavNorm->mutableX(0);
    xData[15] += 0.001;

    const std::string outputWS("Q1D2Test_invalid_result");
    TS_ASSERT_THROWS_NOTHING(
        Q1D.setProperty("DetBankWorkspace", m_inputWS); Q1D.setProperty("WavelengthAdj", m_wavNorm);
        Q1D.setPropertyValue("OutputWorkspace", outputWS); Q1D.setPropertyValue("OutputBinning", "0.1,-0.02,0.5");
        Q1D.setPropertyValue("AccountForGravity", "1");)
    Q1D.execute();

    TS_ASSERT(!Q1D.isExecuted())

    // Restore the old binning
    xData[15] -= 0.001;
  }

  void testRunsWithQResolution() {
    // Arrange
    Mantid::API::MatrixWorkspace_sptr qResolution, alteredInput;
    const double value1 = 1;
    const double value2 = 1;
    createQResolutionWorkspace(qResolution, alteredInput, m_inputWS, value1, value2);

    // Act
    TSM_ASSERT("Resolution workspace should not be NULL", qResolution)
    Mantid::Algorithms::Q1D2 Q1D;
    TS_ASSERT_THROWS_NOTHING(Q1D.initialize());
    TS_ASSERT(Q1D.isInitialized())

    const std::string outputWS("Q1D2Test_result");
    Q1D.setProperty("DetBankWorkspace", alteredInput);
    Q1D.setProperty("WavelengthAdj", m_wavNorm);
    Q1D.setProperty("PixelAdj", m_pixel);
    Q1D.setPropertyValue("OutputWorkspace", outputWS);
    Q1D.setPropertyValue("OutputBinning", "0.5,0.5,10.0");
    Q1D.setProperty("QResolution", qResolution);
    Q1D.setProperty("OutputParts", true);

    // Assert
    TS_ASSERT_THROWS_NOTHING(Q1D.execute());

    TS_ASSERT(Q1D.isExecuted())

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(result = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)))

    Mantid::API::MatrixWorkspace_sptr sumOfCounts;
    TS_ASSERT_THROWS_NOTHING(sumOfCounts = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 Mantid::API::AnalysisDataService::Instance().retrieve(outputWS + "_sumOfCounts")))

    TSM_ASSERT("Should have the x error flag set", result->hasDx(0));
    // That output will be
    // SUM_i(Yin_i*sqrt(QRES_in_i^2 + *QBinWidth^2/sqrt(12)^2))/(SUM_i(Y_in_i))
    // for each q
    // value
    // In our test workspace we set QRes_in_1 to 1, this means that all DX
    // values should be SUM_i(Y_in_i*sqrt((1+ QBinWidth^2/12)/(SUM_i(Y_in_i))
    // which is either sqrt(1 + 0.5^2/12) or 0. It can be 0 if no data falls
    // into this
    // bin. We make sure that there is at least one bin with a count
    // of sqrt(1 + 0.5^2/12) ~ 1.01036297108
    unsigned int counter = 0;
    for (const auto &dx : result->dx(0)) {
      // Since we are dealing with a float it can be difficult to compare
      // our value with sqrt(1 + 0.5^2/12). Hence it is enough for us to confirm
      // that the values lie in an interval around this value
      auto isZeroValue = dx == 0.0;
      auto isCloseToZeroPoint1DividedByRootTwelve = (dx > 1.01035) && (dx < 1.01037);
      if (isCloseToZeroPoint1DividedByRootTwelve) {
        counter++;
      }

      // Make sure that the value is either sqrt(1 + 0.5^2/12) or 0
      TSM_ASSERT("The DX value should be either sqrt(1 + 0.5^2/12) or 0",
                 isCloseToZeroPoint1DividedByRootTwelve || isZeroValue);
    }
    TSM_ASSERT("There should be at least one bin which is not 0", counter > 0);

    // Clean Up
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS + "_sumOfCounts");
    Mantid::API::AnalysisDataService::Instance().remove(outputWS + "_sumOfNormFactors");
  }

  void testThatDxIsNotPopulatedWhenQResolutionIsNotChoosen() {
    // Arrange
    Mantid::Algorithms::Q1D2 Q1D;
    TS_ASSERT_THROWS_NOTHING(Q1D.initialize());
    TS_ASSERT(Q1D.isInitialized())

    const std::string outputWS("Q1D2Test_result");
    Q1D.setProperty("DetBankWorkspace", m_inputWS);
    Q1D.setProperty("WavelengthAdj", m_wavNorm);
    Q1D.setProperty("PixelAdj", m_pixel);
    Q1D.setPropertyValue("OutputWorkspace", outputWS);
    Q1D.setPropertyValue("OutputBinning", "0.1,-0.02,0.5");
    TS_ASSERT_THROWS_NOTHING(Q1D.execute());

    TS_ASSERT(Q1D.isExecuted())

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(result = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)))

    // Make sure that the Q resolution is not calculated
    TS_ASSERT(!result->sharedDx(0));
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

  /// stop the constructor from being run every time algorithms test suite is
  /// initialised
  static Q1D2Test *createSuite() { return new Q1D2Test(); }
  static void destroySuite(Q1D2Test *suite) { delete suite; }
  Q1D2Test() : m_noGrav("Q1D2Test_no_gravity_result") {
    // create the input workspaces with a few spectra
    createInputWorkspaces(8603, 8632, m_inputWS, m_wavNorm, m_pixel);
  }

private:
  Mantid::API::MatrixWorkspace_sptr m_inputWS, m_wavNorm, m_pixel;
  std::string m_noGrav;
};

class Q1D2TestPerformance : public CxxTest::TestSuite {
public:
  Mantid::API::MatrixWorkspace_sptr m_inputWS, m_wavNorm, m_pixel;
  std::string m_outputWS;

  void setUp() override {
    // load all the spectra from the LOQ workspace
    createInputWorkspaces(1, 17792, m_inputWS, m_wavNorm, m_pixel);
    m_outputWS = "Q1D2Test_result";
  }

  void tearDown() override { Mantid::API::AnalysisDataService::Instance().remove(m_outputWS); }

  void test_slow_performance() {
    Mantid::Algorithms::Q1D2 Q1D;
    Q1D.initialize();

    Q1D.setProperty("DetBankWorkspace", m_inputWS);
    Q1D.setProperty("WavelengthAdj", m_wavNorm);
    Q1D.setPropertyValue("OutputWorkspace", m_outputWS);
    Q1D.setPropertyValue("OutputBinning", "0.1,-0.02,0.5");
    Q1D.setPropertyValue("AccountForGravity", "1");

    Q1D.execute();
  }
};

void createInputWorkspaces(int start, int end, Mantid::API::MatrixWorkspace_sptr &input,
                           Mantid::API::MatrixWorkspace_sptr &wave, Mantid::API::MatrixWorkspace_sptr &pixels) {

  std::string wsName("Q1D2Test_inputworkspace"), wavNorm("Q1D2Test_wave");

  bool forMasking = (start > 9000); // If start is late we assume we a creating
                                    // a 2nd set of workspaces for masking
  if (forMasking)                   // avoid workspace name clash if creating workspaces for
                                    // masking
  {
    wsName = "Q1D2Test_inputworkspace_for_masking", wavNorm = "Q1D2Test_wave_for_masking";
  }

  LoadRaw3 loader;
  loader.initialize();
  loader.setPropertyValue("Filename", "LOQ48097.raw");

  loader.setPropertyValue("OutputWorkspace", wavNorm);
  loader.setProperty("LoadLogFiles", false);
  loader.setProperty("SpectrumMin", start);
  loader.setProperty("SpectrumMax", end);
  loader.execute();

  Mantid::Algorithms::ConvertUnits convert;
  convert.initialize();
  convert.setPropertyValue("InputWorkspace", wavNorm);
  convert.setPropertyValue("OutputWorkspace", wavNorm);
  convert.setPropertyValue("Target", "Wavelength");
  convert.execute();

  Mantid::Algorithms::Rebin rebin;
  rebin.initialize();
  rebin.setPropertyValue("InputWorkspace", wavNorm);
  rebin.setPropertyValue("OutputWorkspace", wavNorm);
  rebin.setPropertyValue("Params", "0,0.5,30");
  rebin.execute();

  Mantid::Algorithms::CropWorkspace crop;
  crop.initialize();
  crop.setPropertyValue("InputWorkspace", wavNorm);
  crop.setPropertyValue("OutputWorkspace", wsName);
  crop.setPropertyValue("StartWorkspaceIndex", "1");
  crop.execute();

  crop.setPropertyValue("InputWorkspace", wavNorm);
  crop.setPropertyValue("OutputWorkspace", wavNorm);
  crop.setPropertyValue("StartWorkspaceIndex", "0");
  crop.setPropertyValue("EndWorkspaceIndex", "0");
  crop.execute();

  input = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve(wsName));
  wave = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve(wavNorm));
  pixels = WorkspaceCreationHelper::create2DWorkspaceBinned(29, 1);
  for (int i = 0; i < 29; ++i) {
    pixels->mutableY(i)[0] = flat_cell061Ys[i];
    pixels->mutableE(i)[0] = flat_cell061Es[i];
  }
  if (forMasking) {
    pixels->mutableY(5)[0] = 0.0;  // This pixels should be masked
    pixels->mutableY(6)[0] = -1.0; // This pixel should be masked
    AnalysisDataService::Instance().add("Q1DTest_flat_file", pixels);
  } else
    AnalysisDataService::Instance().add("Q1DTest_flat_file_for_masking", pixels);
}
void createInputWorkSpacesForMasking(Mantid::API::MatrixWorkspace_sptr &input, Mantid::API::MatrixWorkspace_sptr &wave,
                                     Mantid::API::MatrixWorkspace_sptr &pixels) {
  createInputWorkspaces(9001, 9030, input, wave, pixels);
}

void createQResolutionWorkspace(Mantid::API::MatrixWorkspace_sptr &qResolution,
                                Mantid::API::MatrixWorkspace_sptr &alteredInput,
                                Mantid::API::MatrixWorkspace_sptr &input, double value1, double value2) {
  // The q resolution workspace is almost the same to the input workspace,
  // except for the y value, we set all Y values to 1
  qResolution = input->clone();
  alteredInput = input->clone();

  // Populate Y with Value1
  for (size_t i = 0; i < qResolution->getNumberHistograms(); ++i) {
    qResolution->mutableY(i) = value1;
  }

  // Populate Y with Value2
  for (size_t i = 0; i < alteredInput->getNumberHistograms(); ++i) {
    alteredInput->mutableY(i) = value2;
  }
}
