// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_LINEPROFILETEST_H_
#define MANTID_ALGORITHMS_LINEPROFILETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/LineProfile.h"

#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAlgorithms/CompareWorkspaces.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

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

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
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
    MatrixWorkspace_sptr outputWS =
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

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
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

  void test_horizontal_profile_larger_than_workspace() {
    const size_t nHist = 1;
    const size_t nBins = 1;
    MatrixWorkspace_sptr inputWS = create2DWorkspace154(nHist, nBins);
    LineProfile alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Direction", "Horizontal"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("Centre", static_cast<double>(nHist) / 2))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HalfWidth", 2.0 * nBins))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1)
    const auto hist = outputWS->histogram(0);
    TS_ASSERT_EQUALS(hist.xMode(), Histogram::XMode::Points)
    TS_ASSERT_EQUALS(hist.size(), 1)
    TS_ASSERT_EQUALS(hist.x().front(), 1.0)
    TS_ASSERT_EQUALS(hist.y().front(), 5.0)
    TS_ASSERT_EQUALS(hist.e().front(), 4.0)
    const auto vertAxis = outputWS->getAxis(1);
    TS_ASSERT_EQUALS(vertAxis->getValue(0), 1.0)
    TS_ASSERT_EQUALS(vertAxis->getValue(1), 1.0)
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

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
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
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Direction", "Vertical"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("Centre", static_cast<double>(nBins) / 2))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HalfWidth", 3.0))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
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

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
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

  void test_input_sample_logs_preserved() {
    const size_t nHist = 13;
    const size_t nBins = 23;
    MatrixWorkspace_sptr inputWS = create2DWorkspace154(nHist, nBins);
    inputWS->mutableRun().addProperty("test_property", true);
    const int start = 2;
    const int end = nBins - 2;
    MatrixWorkspace_sptr outputWS =
        profileOverTwoSpectra(inputWS, start, end, "Sum");
    TS_ASSERT(outputWS);
    const auto &logs = outputWS->run();
    TS_ASSERT(logs.hasProperty("test_property"));
  }

  void test_input_history_preserved() {
    const size_t nHist = 13;
    const size_t nBins = 23;
    MatrixWorkspace_sptr inputWS = create2DWorkspace154(nHist, nBins);
    auto &oldHistory = inputWS->history();
    auto historyEntry = boost::make_shared<AlgorithmHistory>(
        "LineProfileTestDummyAlgorithmName", 1,
        boost::uuids::to_string(boost::uuids::random_generator()()));
    oldHistory.addHistory(historyEntry);
    const int start = 2;
    const int end = nBins - 2;
    LineProfile alg;
    // Cannot be run as child because we need the history.
    alg.setChild(false);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "OutputWorkspace", "LineProfileTest_test_input_history"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(
        "Centre", static_cast<double>(inputWS->getNumberHistograms()) / 2))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HalfWidth", 0.5))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("Start", static_cast<double>(start)))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("End", static_cast<double>(end)))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Mode", "Sum"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    MatrixWorkspace_sptr outputWS =
        boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(
                "LineProfileTest_test_input_history"));
    TS_ASSERT(outputWS);
    const auto &history = outputWS->getHistory();
    TS_ASSERT_EQUALS(history.size(), 2)
    TS_ASSERT_EQUALS(history[0]->name(), "LineProfileTestDummyAlgorithmName")
    TS_ASSERT_EQUALS(history[1]->name(), "LineProfile")
    AnalysisDataService::Instance().clear();
  }

  void test_horizontal_distribution_input_gives_distribution_output() {
    const size_t nHist = 13;
    const size_t nBins = 23;
    MatrixWorkspace_sptr inputWS = create2DWorkspace154(nHist, nBins, true);
    inputWS->setDistribution(true);
    TS_ASSERT(inputWS->isHistogramData());
    TS_ASSERT(inputWS->isDistribution());
    const int start = 2;
    const int end = nBins - 2;
    MatrixWorkspace_sptr outputWS =
        profileOverTwoSpectra(inputWS, start, end, "Sum");
    TS_ASSERT(outputWS);
    TS_ASSERT(outputWS->isHistogramData());
    TS_ASSERT(outputWS->isDistribution());
  }

  void test_horizontal_nondistribution_input_gives_nondistribution_output() {
    const size_t nHist = 13;
    const size_t nBins = 23;
    MatrixWorkspace_sptr inputWS = create2DWorkspace154(nHist, nBins, true);
    inputWS->setDistribution(false);
    TS_ASSERT(inputWS->isHistogramData());
    TS_ASSERT(!inputWS->isDistribution());
    const int start = 2;
    const int end = nBins - 2;
    MatrixWorkspace_sptr outputWS =
        profileOverTwoSpectra(inputWS, start, end, "Sum");
    TS_ASSERT(outputWS);
    TS_ASSERT(outputWS->isHistogramData());
    TS_ASSERT(!outputWS->isDistribution());
  }

  void test_vertical_histogram_input_gives_nondistribution_histogram_output() {
    const size_t nHist = 13;
    const size_t nBins = 23;
    MatrixWorkspace_sptr inputWS = create2DWorkspace154(nHist, nBins);
    std::vector<double> verticalBinEdges(nHist + 1);
    for (size_t i = 0; i < verticalBinEdges.size(); ++i) {
      verticalBinEdges[i] = static_cast<double>(i);
    }
    auto vertAxis = std::make_unique<BinEdgeAxis>(verticalBinEdges);
    inputWS->replaceAxis(1, vertAxis.release());
    LineProfile alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Direction", "Vertical"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("Centre", static_cast<double>(nBins) / 2))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HalfWidth", 3.0))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT(outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())
  }

  void
  test_vertical_point_data_input_gives_nondistribution_point_data_output() {
    const size_t nHist = 13;
    const size_t nBins = 23;
    MatrixWorkspace_sptr inputWS = create2DWorkspace154(nHist, nBins);
    const auto vertAxis = inputWS->getAxis(1);
    TS_ASSERT_EQUALS(vertAxis->length(), inputWS->getNumberHistograms())
    LineProfile alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Direction", "Vertical"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("Centre", static_cast<double>(nBins) / 2))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HalfWidth", 3.0))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT(!outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())
  }

  void test_vertical_profile_from_distribution_normalized_by_bin_widths() {
    const size_t nHist{4};
    const size_t nBins{3};
    const BinEdges edges{{0., 0.1, 1.1, 11.1}};
    const Frequencies frequencies{3., 2., 1.};
    const Histogram histogram(edges, frequencies);
    MatrixWorkspace_sptr inputWS = create<Workspace2D>(nHist, histogram);
    LineProfile alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Direction", "Vertical"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Centre", 11.1 / 2.))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HalfWidth", 6.))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT(!outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())
    const auto axis = static_cast<BinEdgeAxis *>(outputWS->getAxis(1));
    TS_ASSERT_EQUALS(axis->length(), 2)
    TS_ASSERT_EQUALS(axis->getMin(), edges.front())
    TS_ASSERT_EQUALS(axis->getMax(), edges.back())
    const auto binHeight = axis->getMax() - axis->getMin();
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1)
    const auto &Xs = outputWS->x(0);
    const std::vector<double> profilePoints{{1., 2., 3., 4.}};
    TS_ASSERT_EQUALS(Xs.rawData(), profilePoints)
    const auto &Ys = outputWS->y(0);
    const auto horizontalIntegral = (3. * 0.1 + 2. * 1. + 1. * 10.) / binHeight;
    for (const auto y : Ys) {
      TS_ASSERT_DELTA(y, horizontalIntegral / nBins, 1e-12)
    }
    const auto &Es = outputWS->e(0);
    const auto horizontalError =
        std::sqrt(3. * 0.1 * 0.1 + 2. * 1. * 1. + 1. * 10. * 10.) / binHeight;
    const std::vector<double> profileErrors(nHist, horizontalError / nBins);
    TS_ASSERT_EQUALS(Es.rawData(), profileErrors)
  }

private:
  MatrixWorkspace_sptr profileOverTwoSpectra(MatrixWorkspace_sptr inputWS,
                                             const int start, const int end,
                                             const std::string &mode) {
    LineProfile alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
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

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    return outputWS;
  }
};

#endif /* MANTID_ALGORITHMS_LINEPROFILETEST_H_ */
