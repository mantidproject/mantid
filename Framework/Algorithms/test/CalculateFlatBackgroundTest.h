#ifndef FLATBACKGROUNDTEST_H_
#define FLATBACKGROUNDTEST_H_

#include "MantidAlgorithms/CalculateFlatBackground.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

static const int NUMBINS = 31;
static const int NUMSPECS = 4;

namespace {
/// Run CalculateFlatBackground with options specific to the function calling
/// it. Each function has a preset relating to that specific test's needs.
///@param functindex - an integer identifying the function running the
/// algorithm in order to set the properties
void runCalculateFlatBackground(int functindex) {
  // functindex key
  // 1 = exec
  // 2 = execwithreturnbg
  // 3 = testMeanFirst
  // 4 = testMeanFirstWithReturnBackground
  // 5 = testMeanSecond
  // 6 = testLeaveNegativeValues

  // common beginning
  Mantid::Algorithms::CalculateFlatBackground flatBG;
  TS_ASSERT_THROWS_NOTHING(flatBG.initialize())
  TS_ASSERT(flatBG.isInitialized())

  if (functindex == 1 || functindex == 2) {
    // exec or execWithReturnBackground
    TS_ASSERT_THROWS_NOTHING(
        flatBG.setPropertyValue("InputWorkspace", "calcFlatBG"))
    TS_ASSERT_THROWS_NOTHING(
        flatBG.setPropertyValue("OutputWorkspace", "Removed"))
    TS_ASSERT_THROWS_NOTHING(flatBG.setPropertyValue("WorkspaceIndexList", "0"))
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
  }

  // common ending
  TS_ASSERT_THROWS_NOTHING(flatBG.execute())
  TS_ASSERT(flatBG.isExecuted())
}

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
  WS->setInstrument(testInst);

  const double pixelRadius(0.05);
  Geometry::Object_sptr pixelShape =
      ComponentCreationHelper::createCappedCylinder(
          pixelRadius, 0.02, V3D(0.0, 0.0, 0.0), V3D(0., 1.0, 0.), "tube");

  const double detXPos(5.0);
  for (int i = 0; i < ndets; ++i) {
    std::ostringstream lexer;
    lexer << "pixel-" << i << ")";
    Geometry::Detector *physicalPixel = new Geometry::Detector(
        lexer.str(), WS->getAxis(1)->spectraNo(i), pixelShape, testInst.get());
    const double ypos = i * 2.0 * pixelRadius;
    physicalPixel->setPos(detXPos, ypos, 0.0);
    testInst->add(physicalPixel);
    testInst->markAsMonitor(physicalPixel);
    WS->getSpectrum(i).addDetectorID(physicalPixel->getID());
  }
}

void setupTestWorkspace() {
  FrameworkManager::Instance();

  double bg = 100.0;
  Mantid::DataObjects::Workspace2D_sptr WS(
      new Mantid::DataObjects::Workspace2D);
  WS->initialize(1, NUMBINS + 1, NUMBINS);
  const size_t seed(12345);
  const double lower(-1.0), upper(1.0);
  MersenneTwister randGen(seed, lower, upper);

  HistogramData::BinEdges generatedBins(NUMBINS + 1,
                                        HistogramData::LinearGenerator(0, 1));
  { // explicit scope to re-use variables
    WS->setBinEdges(0, generatedBins);
    auto &y = WS->mutableY(0);

    auto &e = WS->mutableE(0);
    auto &refY = WS->y(0);
    for (int i = 0; i < NUMBINS; ++i) {
      // x[i] = i;
      y[i] = bg + randGen.nextValue();
      e[i] = 0.05 * refY[i];
    }
    // x[NUMBINS] = NUMBINS;
  } // end of explicit scope

  AnalysisDataService::Instance().add("calcFlatBG", WS);

  // create another test workspace
  Mantid::DataObjects::Workspace2D_sptr WS2D(
      new Mantid::DataObjects::Workspace2D);
  WS2D->initialize(NUMSPECS, NUMBINS + 1, NUMBINS);

  for (int j = 0; j < NUMSPECS; ++j) {
    WS2D->setBinEdges(j, generatedBins);
    auto &y = WS2D->mutableY(j);
    auto &e = WS2D->mutableE(j);
    for (int i = 0; i < NUMBINS; ++i) {
      // any function that means the calculation is non-trivial
      y[i] = j + 4 * (i + 1) - (i * i) / 10;
      e[i] = 2 * i;
    }
  }
  // used only in the last test
  addInstrument(WS2D);

  AnalysisDataService::Instance().add("calculateflatbackgroundtest_ramp", WS2D);
}
}
class CalculateFlatBackgroundTest : public CxxTest::TestSuite {
  /// Tests each method in CalculateFlatBackground using different parameter
  /// sets to make sure the returns are as expected
public:
  static CalculateFlatBackgroundTest *createSuite() {
    return new CalculateFlatBackgroundTest();
  }
  static void destroySuite(CalculateFlatBackgroundTest *suite) { delete suite; }

  // In constructor so that it can be run only once
  CalculateFlatBackgroundTest() { setupTestWorkspace(); }

  // In destructor so that it can be run only once
  ~CalculateFlatBackgroundTest() override {
    AnalysisDataService::Instance().remove("calculateflatbackgroundtest_ramp");
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
      const auto &YIn = inputWS->y(j);
      const auto &EIn = inputWS->e(j);
      const auto &YOut = outputWS->y(j);
      const auto &EOut = outputWS->e(j);
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
      const auto &YIn = inputWS->y(j);
      const auto &EIn = inputWS->e(j);
      const auto &YOut = outputWS->y(j);
      const auto &EOut = outputWS->e(j);
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
        TS_ASSERT_DELTA(YOut[i], background, 1e-6)
        TS_ASSERT_DELTA(EOut[i],
                        std::sqrt((EIn[i] * EIn[i]) + (backError * backError)),
                        1e-6)
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
      const auto &YIn = inputWS->y(j);
      const auto &EIn = inputWS->e(j);
      const auto &YOut = outputWS->y(j);
      const auto &EOut = outputWS->e(j);
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
        double correct = (YIn[i] - background) > 0 ? YIn[i] - background : 0;
        TS_ASSERT_DELTA(YOut[i], correct, 1e-6)

        if (YIn[i] - background < 0 && EIn[i] < background) {
          TS_ASSERT_DELTA(EOut[i], background, 1e-6)
        } else {
          TS_ASSERT_DELTA(
              EOut[i], std::sqrt((EIn[i] * EIn[i]) + (backError * backError)),
              1e-6)
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

    WS->setBinEdges(0, NUMBINS + 1, HistogramData::LinearGenerator(0, 2));
    auto &x = WS->mutableX(0);
    x[NUMBINS] = 2 * (NUMBINS - 1) + 4.0;
    WS->mutableY(0) = YVALUE;
    WS->mutableE(0) = YVALUE / 3.0;

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

    const auto &YOut = outputWS->y(0);
    const auto &EOut = outputWS->e(0);

    TS_ASSERT_DELTA(YOut[5], 50.0, 1e-6)
    TS_ASSERT_DELTA(YOut[25], 50.0, 1e-6)
    TS_ASSERT_DELTA(YOut[NUMBINS - 1], 0.0, 1e-6)

    TS_ASSERT_DELTA(EOut[10], 37.2677, 0.001)
    TS_ASSERT_DELTA(EOut[20], 37.2677, 0.001)
  }

  void test_skipMonitors() {

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
      const auto &YIn = inputWS->y(j);
      const auto &EIn = inputWS->e(j);
      const auto &YOut = outputWS->y(j);
      const auto &EOut = outputWS->e(j);

      for (int i = 0; i < NUMBINS; ++i) {
        TS_ASSERT_DELTA(YIn[i], YOut[i], 1e-12)
        TS_ASSERT_DELTA(EIn[i], EOut[i], 1e-12)
      }
    }

    //------------------------------------------------------------------------------

    AnalysisDataService::Instance().remove("Removed1");
  }

  void test_movingAverageThrowsOnlyWithInvalidWindowWidth() {
    size_t binCount = 5;
    movingAverageWindowWidthTest(0, binCount, true);
    movingAverageWindowWidthTest(1, binCount, false);
    movingAverageWindowWidthTest(binCount - 1, binCount, false);
    movingAverageWindowWidthTest(binCount, binCount, false);
    movingAverageWindowWidthTest(binCount + 1, binCount, true);
  }

  void test_movingAverageModeWhenWindowWidthIsOne() {
    const size_t spectraCount = 3;
    const size_t binCount = 4;
    movingAverageTest(1, spectraCount, binCount);
  }

  void test_movingAverageModeWhenWindowWidthIsTwo() {
    const size_t spectraCount = 2;
    const size_t binCount = 7;
    movingAverageTest(2, spectraCount, binCount);
  }

  void test_movingAverageModeWithMaximalWindowWidth() {
    const size_t spectraCount = 4;
    const size_t binCount = 9;
    movingAverageTest(binCount, spectraCount, binCount);
  }

private:
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
    WS->setInstrument(testInst);

    const double pixelRadius(0.05);
    Geometry::Object_sptr pixelShape =
        ComponentCreationHelper::createCappedCylinder(
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
  }

  /// Creates a  workspace with a single special value in each spectrum.
  Mantid::DataObjects::Workspace2D_sptr
  movingAverageCreateWorkspace(const size_t spectraCount, const size_t binCount,
                               const size_t specialIndex) {
    Mantid::DataObjects::Workspace2D_sptr WS(
        new Mantid::DataObjects::Workspace2D);
    WS->initialize(spectraCount, binCount + 1, binCount);
    for (size_t i = 0; i < spectraCount; ++i) {
      auto &X = WS->mutableX(i);
      auto &Y = WS->mutableY(i);
      auto &E = WS->mutableE(i);

      const auto &constY = WS->y(i);
      for (size_t j = 0; j < binCount; ++j) {
        // Make non-trivial but still linear x axis.
        X[j] = 0.78 * (static_cast<double>(j) -
                       static_cast<double>(binCount) / 3.0) -
               0.31 * static_cast<double>(i);
        // Compute some non-trivial y values.
        Y[j] = movingAverageStandardY(i);
        E[j] = std::sqrt(constY[j]);
      }
      // Add extra x value because histogram.
      X[binCount] = 0.78 * 2.0 / 3.0 * static_cast<double>(binCount) -
                    0.31 * static_cast<double>(i);
      // The special background value is set here.
      Y[specialIndex] = movingAverageSpecialY(i);
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
      flatBG.setProperty("AveragingWindowWidth", static_cast<int>(windowWidth));
      flatBG.setPropertyValue("OutputMode", "Return Background");
      TS_ASSERT_THROWS_NOTHING(flatBG.execute())
      TS_ASSERT(flatBG.isExecuted())
      MatrixWorkspace_sptr outputWS =
          AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
              "Removed1");
      for (size_t j = 0; j < spectraCount; ++j) {
        const double expected = (movingAverageSpecialY(j) +
                                 (static_cast<double>(windowWidth) - 1) *
                                     movingAverageStandardY(j)) /
                                static_cast<double>(windowWidth);
        TS_ASSERT_DELTA(outputWS->y(j)[0], expected, 1e-12)
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
};

class CalculateFlatBackgroundTestPerformance : public CxxTest::TestSuite {
  /// Tests each method in CalculateFlatBackground using different parameter
  /// sets to make sure the returns are as expected
public:
  static CalculateFlatBackgroundTestPerformance *createSuite() {
    return new CalculateFlatBackgroundTestPerformance();
  }
  static void destroySuite(CalculateFlatBackgroundTestPerformance *suite) {
    delete suite;
  }

  // In constructor so that it can be run only once
  CalculateFlatBackgroundTestPerformance() { setupTestWorkspace(); }

  // In destructor so that it can be run only once
  ~CalculateFlatBackgroundTestPerformance() override {
    AnalysisDataService::Instance().remove("calculateflatbackgroundtest_ramp");
  }

  void testExecPerformance() { runCalculateFlatBackground(1); }

  void testExecWithReturnBackgroundPerformance() {
    runCalculateFlatBackground(2);
  }

  void testMeanFirstPerformance() { runCalculateFlatBackground(3); }

  void testMeanFirstWithReturnBackgroundPerformance() {
    runCalculateFlatBackground(4);
  }

  void testMeanSecondPerformance() { runCalculateFlatBackground(5); }

  void testLeaveNegativesPerformance() { runCalculateFlatBackground(6); }
};
#endif /* FLATBACKGROUNDTEST_H_ */
