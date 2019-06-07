// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef FLATBACKGROUNDTEST_H_
#define FLATBACKGROUNDTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAlgorithms/CalculateFlatBackground.h"
#include "MantidAlgorithms/CompareWorkspaces.h"
#include "MantidAlgorithms/Minus.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cmath>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

static const int NUMBINS = 31;
static const int NUMSPECS = 4;

class CalculateFlatBackgroundTest : public CxxTest::TestSuite {
  /// Tests each method in CalculateFlatBackground using different parameter
  /// sets to make sure the returns are as expected
private:
  /// Run CalculateFlatBackground with options specific to the function calling
  /// it. Each function has a preset relating to that specific test's needs.
  ///@param functindex - an integer identifying the function running the
  /// algorithm in order to set the properties
  void runCalculateFlatBackground(int functindex) {
    // functindex key
    // 1 = exec (Linear Fit)
    // 2 = execwithreturnbg
    // 3 = testMeanFirst
    // 4 = testMeanFirstWithReturnBackground
    // 5 = testMeanSecond
    // 6 = testLeaveNegativeValues
    // 7 = testExecPointData (Linear Fit)

    // common beginning
    Mantid::Algorithms::CalculateFlatBackground flatBG;
    flatBG.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(flatBG.initialize())
    TS_ASSERT(flatBG.isInitialized())

    if (functindex == 1 || functindex == 2) {
      // exec or execWithReturnBackground
      TS_ASSERT_THROWS_NOTHING(
          flatBG.setPropertyValue("InputWorkspace", "calcFlatBG"))
      TS_ASSERT_THROWS_NOTHING(
          flatBG.setPropertyValue("OutputWorkspace", "Removed"))
      TS_ASSERT_THROWS_NOTHING(
          flatBG.setPropertyValue("WorkspaceIndexList", "0"))
      TS_ASSERT_THROWS_NOTHING(flatBG.setPropertyValue("StartX", "9.5"))
      TS_ASSERT_THROWS_NOTHING(flatBG.setPropertyValue("EndX", "20.5"))
      TS_ASSERT_THROWS_NOTHING(flatBG.setPropertyValue("Mode", "Linear Fit"))

      if (functindex == 2) {
        // execWithReturnBackground
        TS_ASSERT_THROWS_NOTHING(
            flatBG.setPropertyValue("OutputMode", "Return Background"))
      }
    } else if (functindex == 3 || functindex == 4 || functindex == 5 ||
               functindex == 6) {
      flatBG.setPropertyValue("InputWorkspace",
                              "calculateflatbackgroundtest_ramp");
      flatBG.setPropertyValue("WorkspaceIndexList", "");
      flatBG.setPropertyValue("Mode", "Mean");

      if (functindex == 3 || functindex == 4) {
        // testMeanFirst or testMeanFirstWithReturnBackground
        flatBG.setPropertyValue("OutputWorkspace",
                                "calculateflatbackgroundtest_first");
        // remove the first half of the spectrum
        flatBG.setPropertyValue("StartX", "0");
        flatBG.setPropertyValue("EndX", "15");
        if (functindex == 4) {
          // testMeanFirstWithReturnBackground
          flatBG.setPropertyValue("OutputMode", "Return Background");
        }
      } else if (functindex == 5) {
        // testMeanSecond
        flatBG.setPropertyValue("OutputWorkspace",
                                "calculateflatbackgroundtest_second");
        // remove the last half of the spectrum
        flatBG.setProperty("StartX", 2 * double(NUMBINS) / 3);
        flatBG.setProperty("EndX", double(NUMBINS));
      } else if (functindex == 6) {
        flatBG.setPropertyValue("OutputWorkspace",
                                "calculateflatbackgroundtest_second");

        flatBG.setProperty("StartX", 2 * double(NUMBINS) / 3);
        flatBG.setProperty("EndX", double(NUMBINS));

        flatBG.setProperty("NullifyNegativeValues", false);
      }
    } else if (functindex == 7) {
      // exec for point data option Linear Fit
      TS_ASSERT_THROWS_NOTHING(
          flatBG.setPropertyValue("InputWorkspace", "calcFlatBGpointdata"))
      TS_ASSERT_THROWS_NOTHING(
          flatBG.setPropertyValue("OutputWorkspace", "Removed"))
      TS_ASSERT_THROWS_NOTHING(
          flatBG.setPropertyValue("WorkspaceIndexList", "0"))
      TS_ASSERT_THROWS_NOTHING(flatBG.setPropertyValue("StartX", "9.5"))
      TS_ASSERT_THROWS_NOTHING(flatBG.setPropertyValue("EndX", "20.5"))
      TS_ASSERT_THROWS_NOTHING(flatBG.setPropertyValue("Mode", "Linear Fit"))
    }

    // common ending
    TS_ASSERT_THROWS_NOTHING(flatBG.execute())
    TS_ASSERT(flatBG.isExecuted())
  }

public:
  static CalculateFlatBackgroundTest *createSuite() {
    return new CalculateFlatBackgroundTest();
  }
  static void destroySuite(CalculateFlatBackgroundTest *suite) { delete suite; }

  CalculateFlatBackgroundTest() {
    FrameworkManager::Instance();

    bg = 100.0;
    const size_t seed(12345);
    const double lower(-1.0), upper(1.0);

    MersenneTwister randGen(seed, lower, upper);

    // histogram
    Mantid::DataObjects::Workspace2D_sptr WS(
        new Mantid::DataObjects::Workspace2D);
    WS->initialize(1, NUMBINS + 1, NUMBINS);

    for (int i = 0; i < NUMBINS; ++i) {
      WS->mutableX(0)[i] = i;
      WS->mutableY(0)[i] = bg + randGen.nextValue();
      WS->mutableE(0)[i] = 0.05 * WS->y(0)[i];
    }
    WS->mutableX(0)[NUMBINS] = NUMBINS;

    AnalysisDataService::Instance().add("calcFlatBG", WS);

    // create another test workspace (histogram)
    Mantid::DataObjects::Workspace2D_sptr WS2D(
        new Mantid::DataObjects::Workspace2D);
    WS2D->initialize(NUMSPECS, NUMBINS + 1, NUMBINS);

    for (int j = 0; j < NUMSPECS; ++j) {
      for (int i = 0; i < NUMBINS; ++i) {
        WS2D->mutableX(j)[i] = i;
        // any function that means the calculation is non-trivial
        WS2D->mutableY(j)[i] = j + 4 * (i + 1) - (i * i) / 10;
        WS2D->mutableE(j)[i] = 2 * i;
      }
      WS2D->mutableX(j)[NUMBINS] = NUMBINS;
    }
    // used only in the last test
    this->addInstrument(WS2D);

    AnalysisDataService::Instance().add("calculateflatbackgroundtest_ramp",
                                        WS2D);

    // test workspace, which contains point data
    Mantid::DataObjects::Workspace2D_sptr WSpointdata(
        new Mantid::DataObjects::Workspace2D);
    WSpointdata->initialize(1, NUMBINS, NUMBINS);

    for (int i = 0; i < NUMBINS; ++i) {
      WSpointdata->mutableX(0)[i] = i;
      WSpointdata->mutableY(0)[i] = bg + randGen.nextValue();
      WSpointdata->mutableE(0)[i] = 0.05 * WSpointdata->y(0)[i];
    }

    AnalysisDataService::Instance().add("calcFlatBGpointdata", WSpointdata);
  }

  ~CalculateFlatBackgroundTest() override {
    AnalysisDataService::Instance().remove("calcFlatBG");
    AnalysisDataService::Instance().remove("calculateflatbackgroundtest_ramp");
    AnalysisDataService::Instance().remove("calcFlatBGpointdata");
  }

  void testStatics() {
    Mantid::Algorithms::CalculateFlatBackground flatBG;
    TS_ASSERT_EQUALS(flatBG.name(), "CalculateFlatBackground")
    TS_ASSERT_EQUALS(flatBG.version(), 1)
  }

  void testExec() {
    runCalculateFlatBackground(1);

    MatrixWorkspace_sptr inputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "calcFlatBG");
    MatrixWorkspace_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("Removed");
    // The X vectors should be the same
    TS_ASSERT_DELTA(inputWS->x(0).rawData(), outputWS->x(0).rawData(), 1e-6)
    // Just do a spot-check on Y & E
    auto &Y = outputWS->y(0);
    for (unsigned int i = 0; i < Y.size(); ++i) {
      TS_ASSERT_LESS_THAN(Y[i], 1.5)
    }
  }

  void testExecPointData() {
    runCalculateFlatBackground(1);

    MatrixWorkspace_sptr inputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "calcFlatBGpointdata");
    MatrixWorkspace_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("Removed");
    // The X vectors should be the same
    // TS_ASSERT_DELTA(inputWS->x(0).rawData(), outputWS->x(0).rawData(), 1e-6)
    // Just do a spot-check on Y & E
    auto &Y = outputWS->y(0);
    for (unsigned int i = 0; i < Y.size(); ++i) {
      TS_ASSERT_LESS_THAN(Y[i], 1.5)
    }
  }

  void testExecWithReturnBackground() {
    runCalculateFlatBackground(2);

    MatrixWorkspace_sptr inputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "calcFlatBG");
    MatrixWorkspace_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("Removed");
    // The X vectors should be the same
    TS_ASSERT_DELTA(inputWS->x(0).rawData(), outputWS->x(0).rawData(), 1e-6)
    // Just do a spot-check on Y & E
    auto &Y = outputWS->y(0);
    for (unsigned int i = 0; i < Y.size(); ++i) {
      TS_ASSERT(100.3431 > Y[i]);
    }
  }

  void testMeanFirst() {
    runCalculateFlatBackground(3);

    MatrixWorkspace_sptr inputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "calculateflatbackgroundtest_ramp");
    MatrixWorkspace_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "calculateflatbackgroundtest_first");
    // The X vectors should be the same
    TS_ASSERT_DELTA(inputWS->x(0).rawData(), outputWS->x(0).rawData(), 1e-6)

    for (int j = 0; j < NUMSPECS; ++j) {
      auto &YIn = inputWS->y(j);
      auto &EIn = inputWS->e(j);
      auto &YOut = outputWS->y(j);
      auto &EOut = outputWS->e(j);
      // do our own calculation of the background and its error to check with
      // later
      double background = 0, backError = 0;
      for (int k = 0; k < 15; ++k) {
        background += YIn[k];
        backError += EIn[k] * EIn[k];
      }
      background /= 15.0;

      backError = std::sqrt(backError) / 15.0;
      for (int i = 0; i < NUMBINS; ++i) {
        double correct = (YIn[i] - background) > 0 ? YIn[i] - background : 0;
        TS_ASSERT_DELTA(YOut[i], correct, 1e-6)

        if (YIn[i] - background < 0) {
          TS_ASSERT_DELTA(EOut[i], background, 1e-6)
        } else {
          TS_ASSERT_DELTA(
              EOut[i], std::sqrt((EIn[i] * EIn[i]) + (backError * backError)),
              1e-6)
        }
      }
    }
  }

  void testMeanFirstWithReturnBackground() {
    runCalculateFlatBackground(4);

    MatrixWorkspace_sptr inputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "calculateflatbackgroundtest_ramp");
    MatrixWorkspace_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "calculateflatbackgroundtest_first");
    // The X vectors should be the same
    TS_ASSERT_DELTA(inputWS->x(0).rawData(), outputWS->x(0).rawData(), 1e-6)

    for (int j = 0; j < NUMSPECS; ++j) {
      auto &YIn = inputWS->y(j);
      auto &EIn = inputWS->e(j);
      auto &YOut = outputWS->y(j);
      auto &EOut = outputWS->e(j);
      // do our own calculation of the background and its error to check with
      // later
      double background = 0, backError = 0;

      for (int k = 0; k < 15; ++k) {
        background += YIn[k];
        backError += EIn[k] * EIn[k];
      }
      background /= 15.0;
      backError = std::sqrt(backError) / 15.0;
      for (int i = 0; i < NUMBINS; ++i) {
        if (background <= YIn[i]) {
          TS_ASSERT_DELTA(YOut[i], background, 1e-6)
          TS_ASSERT_DELTA(EOut[i], backError, 1e-6)
        } else {
          TS_ASSERT_DELTA(YOut[i], YIn[i], 1e-6)
          if (background < EIn[i]) {
            TS_ASSERT_DELTA(EOut[i], EIn[i], 1e-6)
          } else {
            TS_ASSERT_DELTA(EOut[i], background, 1e-6)
          }
        }
      }
    }
  }

  void testMeanSecond() {
    runCalculateFlatBackground(5);

    MatrixWorkspace_sptr inputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "calculateflatbackgroundtest_ramp");
    MatrixWorkspace_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "calculateflatbackgroundtest_second");
    // The X vectors should be the same
    TS_ASSERT_DELTA(inputWS->x(0).rawData(), outputWS->x(0).rawData(), 1e-6)

    for (int j = 0; j < NUMSPECS; ++j) {
      auto &YIn = inputWS->y(j);
      auto &EIn = inputWS->e(j);
      auto &YOut = outputWS->y(j);
      auto &EOut = outputWS->e(j);
      // do our own calculation of the background and its error to check with
      // later
      double background = 0, backError = 0, numSummed = 0;
      // 2*NUMBINS/3 makes use of the truncation of integer division
      for (int k = 2 * NUMBINS / 3; k < NUMBINS; ++k) {
        background += YIn[k];
        backError += EIn[k] * EIn[k];
        numSummed++;
      }
      background /= numSummed;
      backError = std::sqrt(backError) / numSummed;
      for (int i = 0; i < NUMBINS; ++i) {
        double correct = YIn[i] > background ? YIn[i] - background : 0;
        if (YIn[i] > background) {
          TS_ASSERT_DELTA(YOut[i], correct, 1e-6)
          TS_ASSERT_DELTA(
              EOut[i], std::sqrt((EIn[i] * EIn[i]) + (backError * backError)),
              1e-6)
        } else {
          TS_ASSERT_DELTA(YOut[i], 0, 1e-6)
          if (EIn[i] < background) {
            TS_ASSERT_DELTA(EOut[i], background, 1e-6)
          } else {
            TS_ASSERT_DELTA(EOut[i], EIn[i], 1e-6)
          }
        }
      }
    }
  }

  void testLeaveNegatives() {

    runCalculateFlatBackground(6);
    MatrixWorkspace_sptr inputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "calculateflatbackgroundtest_ramp");
    MatrixWorkspace_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "calculateflatbackgroundtest_second");
    // The X vectors should be the same
    TS_ASSERT_DELTA(inputWS->x(0).rawData(), outputWS->x(0).rawData(), 1e-6)

    for (int j = 0; j < NUMSPECS; ++j) {
      auto &YIn = inputWS->y(j);
      auto &EIn = inputWS->e(j);
      auto &YOut = outputWS->y(j);
      auto &EOut = outputWS->e(j);
      // do our own calculation of the background and its error to check with
      // later
      double background = 0, backError = 0, numSummed = 0;
      // 2*NUMBINS/3 makes use of the truncation of integer division
      for (int k = 2 * NUMBINS / 3; k < NUMBINS; ++k) {
        background += YIn[k];
        backError += EIn[k] * EIn[k];
        numSummed++;
      }
      background /= numSummed;
      backError = std::sqrt(backError) / numSummed;
      for (int i = 0; i < NUMBINS; ++i) {
        double correct = (YIn[i] - background);
        double err = std::sqrt((EIn[i] * EIn[i]) + (backError * backError));
        TS_ASSERT_DELTA(YOut[i], correct, 1e-6)
        TS_ASSERT_DELTA(EOut[i], err, 1e-6)
      }
    }
  }

  void testVariedWidths() {
    const double YVALUE = 100.0;
    Mantid::DataObjects::Workspace2D_sptr WS(
        new Mantid::DataObjects::Workspace2D);
    WS->initialize(1, NUMBINS + 1, NUMBINS);

    for (int i = 0; i < NUMBINS; ++i) {
      WS->mutableX(0)[i] = 2 * i;
      WS->mutableY(0)[i] = YVALUE;
      WS->mutableE(0)[i] = YVALUE / 3.0;
    }
    WS->mutableX(0)[NUMBINS] = 2 * (NUMBINS - 1) + 4.0;

    Mantid::Algorithms::CalculateFlatBackground back;
    back.initialize();

    back.setProperty(
        "InputWorkspace",
        boost::static_pointer_cast<Mantid::API::MatrixWorkspace>(WS));
    back.setPropertyValue("OutputWorkspace",
                          "calculateflatbackgroundtest_third");
    back.setPropertyValue("WorkspaceIndexList", "");
    back.setPropertyValue("Mode", "Mean");
    // sample the background from the last (wider) bin only
    back.setProperty("StartX", 2.0 * NUMBINS + 1);
    back.setProperty("EndX", 2.0 * (NUMBINS + 1));

    back.execute();
    TS_ASSERT(back.isExecuted())

    MatrixWorkspace_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "calculateflatbackgroundtest_third");
    // The X vectors should be the same
    TS_ASSERT_DELTA(WS->x(0).rawData(), outputWS->x(0).rawData(), 1e-6)

    auto &YOut = outputWS->y(0);
    auto &EOut = outputWS->e(0);

    TS_ASSERT_DELTA(YOut[5], 50.0, 1e-6)
    TS_ASSERT_DELTA(YOut[25], 50.0, 1e-6)
    TS_ASSERT_DELTA(YOut[NUMBINS - 1], 0.0, 1e-6)

    TS_ASSERT_DELTA(EOut[10], 37.2677, 0.001)
    TS_ASSERT_DELTA(EOut[20], 37.2677, 0.001)
  }

  void testSkipMonitors() {

    Mantid::Algorithms::CalculateFlatBackground flatBG;
    TS_ASSERT_THROWS_NOTHING(flatBG.initialize())
    TS_ASSERT(flatBG.isInitialized())
    flatBG.setPropertyValue("InputWorkspace",
                            "calculateflatbackgroundtest_ramp");
    flatBG.setPropertyValue("OutputWorkspace", "Removed1");
    flatBG.setProperty("StartX", 1.e-6);
    flatBG.setProperty("EndX", double(NUMBINS));

    flatBG.setPropertyValue("WorkspaceIndexList", "");
    flatBG.setPropertyValue("Mode", "Mean");
    flatBG.setPropertyValue("SkipMonitors", "1");

    TS_ASSERT_THROWS_NOTHING(flatBG.execute())
    TS_ASSERT(flatBG.isExecuted())

    //------------------------------------------------------------------------------
    MatrixWorkspace_sptr inputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "calculateflatbackgroundtest_ramp");
    MatrixWorkspace_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("Removed1");
    // The X vectors should be the same
    TS_ASSERT_DELTA(inputWS->x(0).rawData(), outputWS->x(0).rawData(), 1e-6)

    for (int j = 0; j < NUMSPECS; ++j) {
      auto &YIn = inputWS->y(j);
      auto &EIn = inputWS->e(j);
      auto &YOut = outputWS->y(j);
      auto &EOut = outputWS->e(j);

      for (int i = 0; i < NUMBINS; ++i) {
        TS_ASSERT_DELTA(YIn[i], YOut[i], 1e-12)
        TS_ASSERT_DELTA(EIn[i], EOut[i], 1e-12)
      }
    }

    //------------------------------------------------------------------------------

    AnalysisDataService::Instance().remove("Removed1");
  }

  void testMovingAverageThrowsOnlyWithInvalidWindowWidth() {
    size_t binCount = 5;
    movingAverageWindowWidthTest(0, binCount, true);
    movingAverageWindowWidthTest(1, binCount, false);
    movingAverageWindowWidthTest(binCount - 1, binCount, false);
    movingAverageWindowWidthTest(binCount, binCount, false);
    movingAverageWindowWidthTest(binCount + 1, binCount, true);
  }

  void testMovingAverageModeWhenWindowWidthIsOne() {
    const size_t spectraCount = 3;
    const size_t binCount = 4;
    movingAverageTest(1, spectraCount, binCount);
  }

  void testMovingAverageModeWhenWindowWidthIsTwo() {
    const size_t spectraCount = 2;
    const size_t binCount = 7;
    movingAverageTest(2, spectraCount, binCount);
  }

  void testMovingAverageModeWithMaximalWindowWidth() {
    const size_t spectraCount = 4;
    const size_t binCount = 9;
    movingAverageTest(binCount, spectraCount, binCount);
  }

  void testSpectraLeftUnchangedIfUnableToCalculate() {
    const double y1 = -23;
    const double y2 = -42;
    const std::string outWsName("Removed1");
    const std::vector<std::string> modes{"Linear Fit", "Mean",
                                         "Moving Average"};
    const std::array<bool, 2> nullifyOptions{{true, false}};
    for (const auto nullifyNegatives : nullifyOptions) {
      for (const auto &mode : modes) {
        executeWithTwoBinInputWorkspace(y1, y2, 1, outWsName, mode, "",
                                        "Subtract Background",
                                        nullifyNegatives);
        MatrixWorkspace_sptr outputWS =
            AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
                outWsName);
        TS_ASSERT_DELTA(outputWS->y(0)[0], y1, 1e-12)
        TS_ASSERT_DELTA(outputWS->y(0)[1], y2, 1e-12)
        AnalysisDataService::Instance().remove(outWsName);
      }
      for (const auto &mode : modes) {
        executeWithTwoBinInputWorkspace(y1, y2, 1, outWsName, mode, "",
                                        "Return Background", nullifyNegatives);
        MatrixWorkspace_sptr outputWS =
            AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
                outWsName);
        TS_ASSERT_DELTA(outputWS->y(0)[0], 0, 1e-12)
        TS_ASSERT_DELTA(outputWS->y(0)[1], 0, 1e-12)
        AnalysisDataService::Instance().remove(outWsName);
      }
    }
  }

  void testSubtractBackgroundWhenNotAllSpectraSelected() {
    const double y1 = 23;
    const double y2 = 42;
    const std::string outWsName("Removed1");
    // Subtract background only from spectrum index 1.
    // The spectrum at index 0 should be left unchanged.
    executeWithTwoBinInputWorkspace(y1, y2, 2, outWsName, "Mean", "1",
                                    "Subtract Background", false);
    MatrixWorkspace_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWsName);
    TS_ASSERT_DELTA(outputWS->y(0)[0], y1, 1e-12)
    TS_ASSERT_DELTA(outputWS->y(0)[1], y2, 1e-12)
    TS_ASSERT_DELTA(outputWS->y(1)[0], y1 - (y1 + y2) / 2, 1e-12)
    TS_ASSERT_DELTA(outputWS->y(1)[1], y2 - (y1 + y2) / 2, 1e-12)
    AnalysisDataService::Instance().remove(outWsName);
  }

  void testReturnBackgroundWhenNotAllSpectraSelected() {
    const double y1 = 23;
    const double y2 = 42;
    const std::string outWsName("Removed1");
    const bool nullifyNegatives = false;
    // Return background only for spectrum index 1.
    // The output at spectrum index 0 should be zero.
    executeWithTwoBinInputWorkspace(y1, y2, 2, outWsName, "Mean", "1",
                                    "Return Background", nullifyNegatives);
    MatrixWorkspace_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWsName);
    TS_ASSERT_DELTA(outputWS->y(0)[0], 0, 1e-12)
    TS_ASSERT_DELTA(outputWS->y(0)[1], 0, 1e-12)
    TS_ASSERT_DELTA(outputWS->y(1)[0], (y1 + y2) / 2, 1e-12)
    TS_ASSERT_DELTA(outputWS->y(1)[1], (y1 + y2) / 2, 1e-12)
    AnalysisDataService::Instance().remove(outWsName);
  }

  void testMeanSubtractBackgroundAndReturnBackgroundEquivalent() {
    const double y1 = 31;
    const double y2 = 39;
    const std::string outBackgroundWSName("background");
    const std::string outSubtractedWSName("subtracted");
    const bool nullifyNegatives = false;
    executeWithTwoBinInputWorkspace(y1, y2, 1, outBackgroundWSName, "Mean", "",
                                    "Return Background", nullifyNegatives);
    MatrixWorkspace_sptr inputWS = executeWithTwoBinInputWorkspace(
        y1, y2, 1, outSubtractedWSName, "Mean", "", "Subtract Background",
        nullifyNegatives);
    compareSubtractedAndBackgroundWorkspaces(inputWS, outSubtractedWSName,
                                             outBackgroundWSName);
    AnalysisDataService::Instance().remove(outBackgroundWSName);
    AnalysisDataService::Instance().remove(outSubtractedWSName);
  }

  void testLinearFitSubtractBackgroundAndReturnBackgroundEquivalent() {
    const double y1 = 31;
    const double y2 = 39;
    const std::string outBackgroundWSName("background");
    const std::string outSubtractedWSName("subtracted");
    const bool nullifyNegatives = false;
    executeWithTwoBinInputWorkspace(y1, y2, 1, outBackgroundWSName,
                                    "Linear Fit", "", "Return Background",
                                    nullifyNegatives);
    MatrixWorkspace_sptr inputWS = executeWithTwoBinInputWorkspace(
        y1, y2, 1, outSubtractedWSName, "Linear Fit", "", "Subtract Background",
        nullifyNegatives);
    compareSubtractedAndBackgroundWorkspaces(inputWS, outSubtractedWSName,
                                             outBackgroundWSName);
    AnalysisDataService::Instance().remove(outBackgroundWSName);
    AnalysisDataService::Instance().remove(outSubtractedWSName);
  }

  void testMovingAverageSubtractBackgroundAndReturnBackgroundEquivalent() {
    const double y1 = 31;
    const double y2 = 39;
    const std::string outBackgroundWSName("background");
    const std::string outSubtractedWSName("subtracted");
    const bool nullifyNegatives = false;
    executeWithTwoBinInputWorkspace(y1, y2, 1, outBackgroundWSName,
                                    "Moving Average", "", "Return Background",
                                    nullifyNegatives);
    MatrixWorkspace_sptr inputWS = executeWithTwoBinInputWorkspace(
        y1, y2, 1, outSubtractedWSName, "Moving Average", "",
        "Subtract Background", nullifyNegatives);
    compareSubtractedAndBackgroundWorkspaces(inputWS, outSubtractedWSName,
                                             outBackgroundWSName);
    AnalysisDataService::Instance().remove(outBackgroundWSName);
    AnalysisDataService::Instance().remove(outSubtractedWSName);
  }

  void testReturnBackgroundReturnsOriginalValuesIfNullifyNegativeValuesIsSet() {
    const double y1 = 23;
    const double y2 = 42;
    const std::string outWsName("Removed1");
    const bool nullifyNegatives = true;
    executeWithTwoBinInputWorkspace(y1, y2, 1, outWsName, "Mean", "",
                                    "Return Background", nullifyNegatives);
    MatrixWorkspace_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWsName);
    TS_ASSERT_DELTA(outputWS->y(0)[0], y1, 1e-12)
    TS_ASSERT_DELTA(outputWS->y(0)[1], (y1 + y2) / 2, 1e-12)
    AnalysisDataService::Instance().remove(outWsName);
  }

private:
  double bg;

  double round(double value) { return floor(value * 100000 + 0.5) / 100000; }

  void addInstrument(DataObjects::Workspace2D_sptr &WS) {
    int ndets = static_cast<int>(WS->getNumberHistograms());

    WS->setTitle("Test histogram"); // actually adds a property call run_title
                                    // to the logs
    WS->getAxis(0)->setUnit("TOF");
    WS->setYUnit("Counts");

    boost::shared_ptr<Geometry::Instrument> testInst(
        new Geometry::Instrument("testInst"));
    // testInst->setReferenceFrame(boost::shared_ptr<Geometry::ReferenceFrame>(new
    // Geometry::ReferenceFrame(Geometry::PointingAlong::Y,Geometry::X,Geometry::Left,"")));

    const double pixelRadius(0.05);
    auto pixelShape = ComponentCreationHelper::createCappedCylinder(
        pixelRadius, 0.02, V3D(0.0, 0.0, 0.0), V3D(0., 1.0, 0.), "tube");

    const double detXPos(5.0);
    for (int i = 0; i < ndets; ++i) {
      std::ostringstream lexer;
      lexer << "pixel-" << i << ")";
      Geometry::Detector *physicalPixel =
          new Geometry::Detector(lexer.str(), WS->getAxis(1)->spectraNo(i),
                                 pixelShape, testInst.get());
      const double ypos = i * 2.0 * pixelRadius;
      physicalPixel->setPos(detXPos, ypos, 0.0);
      testInst->add(physicalPixel);
      testInst->markAsMonitor(physicalPixel);
      WS->getSpectrum(i).addDetectorID(physicalPixel->getID());
    }
    WS->setInstrument(testInst);
  }

  /// Creates a  workspace with a single special value in each spectrum.
  Mantid::DataObjects::Workspace2D_sptr
  movingAverageCreateWorkspace(const size_t spectraCount, const size_t binCount,
                               const size_t specialIndex) {
    Mantid::DataObjects::Workspace2D_sptr WS(
        new Mantid::DataObjects::Workspace2D);
    WS->initialize(spectraCount, binCount + 1, binCount);
    for (size_t i = 0; i < spectraCount; ++i) {
      for (size_t j = 0; j < binCount; ++j) {
        // Make non-trivial but still linear x axis.
        WS->mutableX(i)[j] = 0.78 * (static_cast<double>(j) -
                                     static_cast<double>(binCount) / 3.0) -
                             0.31 * static_cast<double>(i);
        // Compute some non-trivial y values.
        WS->mutableY(i)[j] = movingAverageStandardY(i);
        WS->mutableE(i)[j] = std::sqrt(WS->y(i)[j]);
      }
      // Add extra x value because histogram.
      WS->mutableX(i)[binCount] =
          0.78 * 2.0 / 3.0 * static_cast<double>(binCount) -
          0.31 * static_cast<double>(i);
      // The special background value is set here.
      WS->mutableY(i)[specialIndex] = movingAverageSpecialY(i);
    }
    return WS;
  }

  double movingAverageSpecialY(const size_t wsIndex) {
    // This value has to be smaller than what is returned by
    // movingAverageStandardY().
    return 0.23 * movingAverageStandardY(wsIndex);
  }

  double movingAverageStandardY(const size_t wsIndex) {
    return 9.34 + 3.2 * static_cast<double>(wsIndex);
  }

  void movingAverageTest(const size_t windowWidth, const size_t spectraCount,
                         const size_t binCount) {
    Mantid::DataObjects::Workspace2D_sptr WS;
    for (size_t i = 0; i < binCount; ++i) {
      WS = movingAverageCreateWorkspace(spectraCount, binCount, i);
      Mantid::Algorithms::CalculateFlatBackground flatBG;
      flatBG.setRethrows(true);
      TS_ASSERT_THROWS_NOTHING(flatBG.initialize())
      TS_ASSERT(flatBG.isInitialized())
      flatBG.setProperty("InputWorkspace", WS);
      flatBG.setPropertyValue("OutputWorkspace", "Removed1");
      flatBG.setPropertyValue("Mode", "Moving Average");
      flatBG.setPropertyValue("OutputMode", "Return Background");
      flatBG.setProperty("NullifyNegativeValues", false);
      flatBG.setProperty("AveragingWindowWidth", static_cast<int>(windowWidth));
      TS_ASSERT_THROWS_NOTHING(flatBG.execute())
      TS_ASSERT(flatBG.isExecuted())
      MatrixWorkspace_sptr outputWS =
          AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
              "Removed1");
      for (size_t j = 0; j < spectraCount; ++j) {
        const double expected =
            (movingAverageSpecialY(j) + (static_cast<double>(windowWidth) - 1) *
                                            movingAverageStandardY(j)) /
            static_cast<double>(windowWidth);
        TS_ASSERT_DELTA(outputWS->y(j)[0], expected, 1e-12)
        const double expectedError = std::sqrt(
            static_cast<double>(windowWidth) * movingAverageStandardY(j) /
            static_cast<double>(windowWidth * windowWidth));
        TS_ASSERT_DELTA(outputWS->e(j)[0], expectedError, 1e-12)
      }
      AnalysisDataService::Instance().remove("Removed1");
    }
  }

  void movingAverageWindowWidthTest(size_t windowWidth, size_t binCount,
                                    bool shouldThrow) {
    Mantid::DataObjects::Workspace2D_sptr WS;
    WS = movingAverageCreateWorkspace(1, binCount, 0);
    Mantid::Algorithms::CalculateFlatBackground flatBG;
    flatBG.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(flatBG.initialize())
    TS_ASSERT(flatBG.isInitialized())
    flatBG.setProperty("InputWorkspace", WS);
    flatBG.setPropertyValue("OutputWorkspace", "Removed1");
    flatBG.setPropertyValue("Mode", "Moving Average");
    flatBG.setProperty("AveragingWindowWidth", static_cast<int>(windowWidth));
    flatBG.setPropertyValue("OutputMode", "Return Background");
    if (shouldThrow) {
      TS_ASSERT_THROWS_ANYTHING(flatBG.execute())
      TS_ASSERT(!flatBG.isExecuted())
    } else {
      TS_ASSERT_THROWS_NOTHING(flatBG.execute())
      TS_ASSERT(flatBG.isExecuted())
    }
    AnalysisDataService::Instance().remove("Removed1");
  }

  MatrixWorkspace_sptr executeWithTwoBinInputWorkspace(
      const double y1, const double y2, const size_t histogramCount,
      const std::string &outWsName, const std::string &mode,
      const std::string &wsIndexList, const std::string &outputMode,
      bool nullifyNegatives) {
    const size_t binCount = 2;
    Mantid::DataObjects::Workspace2D_sptr WS(
        new Mantid::DataObjects::Workspace2D);
    WS->initialize(histogramCount, binCount + 1, binCount);
    const double xBegin = -0.2;
    const double xEnd = 0.6;
    for (size_t i = 0; i < histogramCount; ++i) {
      WS->mutableX(i)[0] = xBegin;
      WS->mutableX(i)[1] = xBegin + (xEnd - xBegin) / 2.0;
      WS->mutableX(i)[2] = xEnd;
      WS->mutableY(i)[0] = y1;
      WS->mutableY(i)[1] = y2;
      WS->mutableE(i)[0] = std::sqrt(std::abs(y1));
      WS->mutableE(i)[1] = std::sqrt(std::abs(y2));
    }
    Mantid::Algorithms::CalculateFlatBackground flatBG;
    TS_ASSERT_THROWS_NOTHING(flatBG.initialize());
    TS_ASSERT(flatBG.isInitialized())
    flatBG.setRethrows(true);
    flatBG.setProperty("InputWorkspace", WS);
    flatBG.setPropertyValue("OutputWorkspace", outWsName);
    flatBG.setProperty("StartX", xBegin);
    flatBG.setProperty("EndX", xEnd);
    flatBG.setPropertyValue("WorkspaceIndexList", wsIndexList);
    flatBG.setPropertyValue("Mode", mode);
    flatBG.setPropertyValue("OutputMode", outputMode);
    flatBG.setProperty("NullifyNegativeValues", nullifyNegatives);
    flatBG.setProperty("AveragingWindowWidth", 1);
    TS_ASSERT_THROWS_NOTHING(flatBG.execute())
    TS_ASSERT(flatBG.isExecuted())
    return WS;
  }

  void compareSubtractedAndBackgroundWorkspaces(
      MatrixWorkspace_sptr originalWS, const std::string &subtractedWSName,
      const std::string &backgroundWSName) {
    const std::string minusWSName("minused");
    MatrixWorkspace_sptr backgroundWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            backgroundWSName);
    MatrixWorkspace_sptr subtractedWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            subtractedWSName);
    Algorithms::Minus minus;
    minus.initialize();
    minus.setProperty("LHSWorkspace", originalWS);
    minus.setProperty("RHSWorkspace", backgroundWS);
    minus.setPropertyValue("OutputWorkspace", minusWSName);
    minus.execute();
    MatrixWorkspace_sptr minusWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            minusWSName);
    Algorithms::CompareWorkspaces comparison;
    comparison.initialize();
    comparison.setProperty("Workspace1", subtractedWS);
    comparison.setProperty("Workspace2", minusWSName);
    comparison.setProperty("Tolerance", 1e-6);
    comparison.setProperty("ToleranceRelErr", true);
    comparison.execute();
    bool result = comparison.getProperty("Result");
    TS_ASSERT(result)
    AnalysisDataService::Instance().remove(minusWSName);
  }
};

#endif /*FlatBackgroundTest_H_*/
