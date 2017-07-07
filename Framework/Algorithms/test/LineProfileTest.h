#ifndef MANTID_ALGORITHMS_LINEPROFILETEST_H_
#define MANTID_ALGORITHMS_LINEPROFILETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/LineProfile.h"

#include "MantidAlgorithms/CompareWorkspaces.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::CompareWorkspaces;
using Mantid::Algorithms::LineProfile;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using namespace Mantid::Kernel;
using namespace WorkspaceCreationHelper;

class LineProfileTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LineProfileTest *createSuite() { return new LineProfileTest(); }
  static void destroySuite(LineProfileTest *suite) { delete suite; }

  void test_Init() {
    LineProfile alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_averaging_profile_of_single_horizontal_spectrum() {
    const size_t nHist = 13;
    const size_t nBins = 23;
    MatrixWorkspace_sptr inputWS = create2DWorkspace154(nHist, nBins);
    const auto inputXMode = inputWS->histogram(0).xMode();

    const int start = 2;
    const int end = nBins - 2;
    LineProfile alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Direction", "Horizontal"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("Centre", static_cast<double>(nHist) / 2))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HalfWidth", 0.49))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("Start", static_cast<double>(start)))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("End", static_cast<double>(end)))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    Workspace2D_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1)
    const auto hist = outputWS->histogram(0);
    TS_ASSERT_EQUALS(hist.xMode(), inputXMode)
    for (size_t i = 0; i < hist.x().size(); ++i) {
      TS_ASSERT_EQUALS(hist.x()[i], i + start)
    }
    for (const auto y : hist.y()) {
      TS_ASSERT_EQUALS(y, inputWS->y(0)[0])
    }
    for (const auto e : hist.e()) {
      TS_ASSERT_EQUALS(e, inputWS->e(0)[0])
    }
    const auto vertAxis = outputWS->getAxis(1);
    TS_ASSERT_EQUALS(vertAxis->getValue(0),
                     static_cast<double>(nHist) / 2 - 0.5)
    TS_ASSERT_EQUALS(vertAxis->getValue(1),
                     static_cast<double>(nHist) / 2 + 0.5)
  }

  void test_summing_profile() {
    const size_t nHist = 13;
    const size_t nBins = 23;
    MatrixWorkspace_sptr inputWS = create2DWorkspace154(nHist, nBins);
    for (size_t i = 0; i < nBins; ++i) {
      inputWS->mutableY(nHist / 2)[i] = std::nan("");
    }
    const int start = 2;
    const int end = nBins - 2;
    Workspace2D_sptr outputWS =
        profileOverTwoSpectra(inputWS, start, end, "Sum");
    TS_ASSERT(outputWS);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1)
    const auto hist = outputWS->histogram(0);
    for (size_t i = 0; i < hist.x().size(); ++i) {
      TS_ASSERT_EQUALS(hist.x()[i], i + start)
    }
    for (const auto y : hist.y()) {
      TS_ASSERT_EQUALS(y, 2 * inputWS->y(0)[0])
    }
    for (const auto e : hist.e()) {
      TS_ASSERT_EQUALS(e, 2 * inputWS->e(0)[0])
    }
    const auto vertAxis = outputWS->getAxis(1);
    TS_ASSERT_EQUALS(vertAxis->getValue(0),
                     static_cast<double>(nHist) / 2 - 0.5)
    TS_ASSERT_EQUALS(vertAxis->getValue(1),
                     static_cast<double>(nHist) / 2 + 1.5)
  }

  void test_horizontal_profile_linewidth_outside_workspace() {
    const size_t nHist = 13;
    const size_t nBins = 23;
    MatrixWorkspace_sptr inputWS = create2DWorkspace154(nHist, nBins);
    const auto inputXMode = inputWS->histogram(0).xMode();

    const int start = 2;
    const int end = nBins - 2;
    LineProfile alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Direction", "Horizontal"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Centre", 1.0))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HalfWidth", 3.0))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("Start", static_cast<double>(start)))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("End", static_cast<double>(end)))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    Workspace2D_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1)
    const auto hist = outputWS->histogram(0);
    TS_ASSERT_EQUALS(hist.xMode(), inputXMode)
    for (size_t i = 0; i < hist.x().size(); ++i) {
      TS_ASSERT_EQUALS(hist.x()[i], i + start)
    }
    for (const auto y : hist.y()) {
      TS_ASSERT_EQUALS(y, inputWS->y(0)[0])
    }
    for (const auto e : hist.e()) {
      TS_ASSERT_EQUALS(e,
                       std::sqrt(4 * inputWS->e(0)[0] * inputWS->e(0)[0]) / 4)
    }
    const auto vertAxis = outputWS->getAxis(1);
    TS_ASSERT_EQUALS(vertAxis->getValue(0), 1.0)
    TS_ASSERT_EQUALS(vertAxis->getValue(1), 5.0)
  }

  void test_vertical_profile() {
    const size_t nHist = 13;
    const size_t nBins = 23;
    MatrixWorkspace_sptr inputWS = create2DWorkspace154(nHist, nBins);

    const int start = 2;
    const int end = nHist - 2;
    LineProfile alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Direction", "Vertical"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("Centre", static_cast<double>(nBins) / 2))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HalfWidth", 3.0))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("Start", static_cast<double>(start)))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("End", static_cast<double>(end)))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    Workspace2D_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1)
    const auto hist = outputWS->histogram(0);
    TS_ASSERT_EQUALS(hist.xMode(), Histogram::XMode::Points)
    for (size_t i = 0; i < hist.x().size(); ++i) {
      TS_ASSERT_EQUALS(hist.x()[i], i + start)
    }
    for (const auto y : hist.y()) {
      TS_ASSERT_EQUALS(y, inputWS->y(0)[0])
    }
    for (const auto e : hist.e()) {
      TS_ASSERT_EQUALS(e,
                       std::sqrt(7 * inputWS->e(0)[0] * inputWS->e(0)[0]) / 7)
    }
    const auto vertAxis = outputWS->getAxis(1);
    TS_ASSERT_EQUALS(vertAxis->getValue(0),
                     static_cast<double>(nBins) / 2 - 3.5)
    TS_ASSERT_EQUALS(vertAxis->getValue(1),
                     static_cast<double>(nBins) / 2 + 3.5)
  }

  void test_vertical_profile_over_entire_workspace() {
    const size_t nHist = 13;
    const size_t nBins = 23;
    MatrixWorkspace_sptr inputWS = create2DWorkspace154(nHist, nBins);

    LineProfile alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Direction", "Vertical"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("Centre", static_cast<double>(nBins) / 2))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HalfWidth", 3.0))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    Workspace2D_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1)
    const auto hist = outputWS->histogram(0);
    TS_ASSERT_EQUALS(hist.xMode(), Histogram::XMode::Points)
    for (size_t i = 0; i < hist.x().size(); ++i) {
      TS_ASSERT_EQUALS(hist.x()[i], i + 1)
    }
    for (const auto y : hist.y()) {
      TS_ASSERT_EQUALS(y, inputWS->y(0)[0])
    }
    for (const auto e : hist.e()) {
      TS_ASSERT_EQUALS(e,
                       std::sqrt(7 * inputWS->e(0)[0] * inputWS->e(0)[0]) / 7)
    }
    const auto vertAxis = outputWS->getAxis(1);
    TS_ASSERT_EQUALS(vertAxis->getValue(0),
                     static_cast<double>(nBins) / 2 - 3.5)
    TS_ASSERT_EQUALS(vertAxis->getValue(1),
                     static_cast<double>(nBins) / 2 + 3.5)
  }

  void test_failure_when_profile_outside_workspace() {
    const size_t nHist = 13;
    const size_t nBins = 23;
    MatrixWorkspace_sptr inputWS = create2DWorkspace154(nHist, nBins);

    LineProfile alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Direction", "Horizontal"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Centre", -10.0))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HalfWidth", 1.0))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Start", 2.0))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("End", 9.0))
    TS_ASSERT_THROWS_ANYTHING(alg.execute())
    TS_ASSERT(!alg.isExecuted())
  }

  void test_failure_with_non_positive_width() {
    LineProfile alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("HalfWidth", std::numeric_limits<double>::min()))
    TS_ASSERT_THROWS_ANYTHING(alg.setProperty("HalfWidth", 0.0))
    TS_ASSERT_THROWS_ANYTHING(alg.setProperty("HalfWidth", -1.0))
  }

  void test_failure_start_smaller_than_end() {
    const size_t nHist = 13;
    const size_t nBins = 23;
    MatrixWorkspace_sptr inputWS = create2DWorkspace154(nHist, nBins);

    LineProfile alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Direction", "Horizontal"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Centre", -10.0))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HalfWidth", 1.0))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Start", 9.0))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("End", 2.0))
    const auto issues = alg.validateInputs();
    const auto it = issues.find("Start");
    TS_ASSERT_DIFFERS(it, issues.end())
  }

  void test_ignore_special_values() {
    const size_t nHist = 13;
    const size_t nBins = 23;
    MatrixWorkspace_sptr inputWS = create2DWorkspace154(nHist, nBins);
    inputWS->mutableY(2)[6] = std::numeric_limits<double>::quiet_NaN();
    inputWS->mutableY(3)[13] = std::numeric_limits<double>::infinity();
    const auto inputXMode = inputWS->histogram(0).xMode();

    LineProfile alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Direction", "Horizontal"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Centre", 3.5))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HalfWidth", 0.5))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Start", 0.0))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("End", static_cast<double>(nBins)))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IgnoreNans", true))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IgnoreInfs", true))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    Workspace2D_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1)
    const auto hist = outputWS->histogram(0);
    TS_ASSERT_EQUALS(hist.xMode(), inputXMode)
    for (size_t i = 0; i < hist.x().size(); ++i) {
      TS_ASSERT_EQUALS(hist.x()[i], i + 1)
    }
    for (const auto y : hist.y()) {
      TS_ASSERT_EQUALS(y, inputWS->y(0)[0])
    }
    for (size_t i = 0; i < hist.e().size(); ++i) {
      if (i == 6 || i == 13) {
        TS_ASSERT_EQUALS(hist.e()[i], inputWS->e(0)[0])
        continue;
      }
      TS_ASSERT_EQUALS(hist.e()[i],
                       std::sqrt(2 * inputWS->e(0)[0] * inputWS->e(0)[0]) / 2)
    }
    const auto vertAxis = outputWS->getAxis(1);
    TS_ASSERT_EQUALS(vertAxis->getValue(0), 3)
    TS_ASSERT_EQUALS(vertAxis->getValue(1), 5)
  }

private:
  Workspace2D_sptr profileOverTwoSpectra(MatrixWorkspace_sptr inputWS,
                                         const int start, const int end,
                                         const std::string &mode) {
    LineProfile alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(
        "Centre", static_cast<double>(inputWS->getNumberHistograms()) / 2))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HalfWidth", 0.5))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("Start", static_cast<double>(start)))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("End", static_cast<double>(end)))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Mode", mode))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    Workspace2D_sptr outputWS = alg.getProperty("OutputWorkspace");
    return outputWS;
  }
};

#endif /* MANTID_ALGORITHMS_LINEPROFILETEST_H_ */
