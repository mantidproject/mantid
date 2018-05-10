#ifndef STRIPPEAKSTEST_H_
#define STRIPPEAKSTEST_H_

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAlgorithms/StripPeaks.h"
#include "MantidKernel/UnitFactory.h"

using namespace Mantid::API;
using Mantid::Algorithms::StripPeaks;

class StripPeaksTest : public CxxTest::TestSuite {
public:
  static StripPeaksTest *createSuite() { return new StripPeaksTest(); }
  static void destroySuite(StripPeaksTest *suite) { delete suite; }

  StripPeaksTest() {
    FrameworkManager::Instance();
    MatrixWorkspace_sptr WS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(2, 200, 0.5, 0.02);
    WS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("dSpacing");

    auto &X = WS->x(1);
    auto &Y1 = WS->mutableY(1);
    auto &E1 = WS->mutableE(1);
    auto &Y0 = WS->mutableY(0);
    for (size_t i = 0; i < Y1.size(); ++i) {
      // Spectrum 0
      Y0[i] = 5000;

      // Spectrum 1
      const double x = (X[i] + X[i + 1]) / 2;
      double funcVal = 2500 * exp(-0.5 * pow((x - 3.14) / 0.022, 2));
      funcVal += 1000 * exp(-0.5 * pow((x - 1.22) / 0.02, 2));
      Y1[i] = 5000 + funcVal;
      E1[i] = sqrt(Y1[i]);
    }

    AnalysisDataService::Instance().add("toStrip", WS);
  }

  void testTheBasics() {
    TS_ASSERT_EQUALS(strip.name(), "StripPeaks");
    TS_ASSERT_EQUALS(strip.version(), 1);
  }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(strip.initialize());
    TS_ASSERT(strip.isInitialized());
  }

  void testExec() {
    if (!strip.isInitialized())
      strip.initialize();

    TS_ASSERT_THROWS_NOTHING(
        strip.setPropertyValue("InputWorkspace", "toStrip"));
    std::string outputWS("stripped");
    TS_ASSERT_THROWS_NOTHING(
        strip.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(strip.setProperty("HighBackground", false));
    TS_ASSERT_THROWS_NOTHING(strip.setProperty("FWHM", 7));

    TS_ASSERT_THROWS_NOTHING(strip.execute());
    TS_ASSERT(strip.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputWS));

    MatrixWorkspace_const_sptr input =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("toStrip");

    const size_t nhist = output->getNumberHistograms();
    const size_t nbins = output->blocksize();
    TS_ASSERT_EQUALS(nhist, input->getNumberHistograms());
    TS_ASSERT_EQUALS(nbins, input->blocksize());

    for (size_t i = 0; i < nhist; ++i) {
      const auto &inX = input->x(i);
      const auto &inE = input->e(i);
      const auto &outX = output->x(i);
      const auto &outY = output->y(i);
      const auto &outE = output->e(i);
      for (size_t j = 0; j < nbins; ++j) {
        TS_ASSERT_EQUALS(outX[j], inX[j]);
        TS_ASSERT_DELTA(outY[j], 5000.0, 0.5);
        if (fabs(outY[j] - 5000.) > 0.5)
          std::cout << "Spectrum " << i << " at X = " << inX[j]
                    << " indexed of " << j << "\n";
        TS_ASSERT_EQUALS(outE[j], inE[j]);
      }
    }

    AnalysisDataService::Instance().remove(outputWS);
    AnalysisDataService::Instance().remove("toStrip");
  }

private:
  StripPeaks strip;
};

class StripPeaksTestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    FrameworkManager::Instance();
    MatrixWorkspace_sptr WS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(2, 200, 0.5, 0.02);
    WS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("dSpacing");

    auto X = WS->points(1);
    auto &Y1 = WS->mutableY(1);
    auto &E1 = WS->mutableE(1);
    auto &Y0 = WS->mutableY(0);
    for (size_t i = 0; i < Y1.size(); ++i) {
      // Spectrum 0
      Y0[i] = 5000;

      // Spectrum 1
      const double x = X[i];
      double funcVal = 2500 * exp(-0.5 * pow((x - 3.14) / 0.022, 2));
      funcVal += 1000 * exp(-0.5 * pow((x - 1.22) / 0.02, 2));
      Y1[i] = 5000 + funcVal;
      E1[i] = sqrt(Y1[i]);
    }

    AnalysisDataService::Instance().add("toStrip", WS);

    // Setup algorithm and prep for run
    stripAlg.initialize();
    stripAlg.setPropertyValue("InputWorkspace", "toStrip");
    stripAlg.setPropertyValue("OutputWorkspace", "Stripped");
    stripAlg.setProperty("HighBackground", false);
    stripAlg.setProperty("FWHM", 7);
  }

  void test_strip_peaks() { TS_ASSERT_THROWS_NOTHING(stripAlg.execute()); }

  void tearDown() override {
    AnalysisDataService::Instance().remove("Stripped");
    AnalysisDataService::Instance().remove("toStrip");
  }

private:
  StripPeaks stripAlg;
};

#endif /*STRIPPEAKSTEST_H_*/
