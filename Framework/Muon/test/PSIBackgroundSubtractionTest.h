// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FrameworkManager.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidMuon/PSIBackgroundSubtraction.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Muon;

namespace {

constexpr const char *WORKSPACE_NAME = "DummyWS";

MatrixWorkspace_sptr createCountsTestWorkspace(const size_t numberOfHistograms, const size_t numberOfBins,
                                               bool addLogs = true) {

  MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(numberOfHistograms, numberOfBins);
  ws->setYUnit("Counts");
  if (addLogs) {
    for (size_t index = 0; index < numberOfHistograms; index++) {

      ws->mutableRun().addProperty("First good spectra " + std::to_string(index), int(double(numberOfBins) / 2.));
      ws->mutableRun().addProperty("Last good spectra " + std::to_string(index), numberOfBins);
    }
  }
  AnalysisDataService::Instance().addOrReplace(WORKSPACE_NAME, ws);

  return ws;
}

MatrixWorkspace_sptr createInvalidTestWorkspace(const size_t numberOfHistograms, const size_t numberOfBins) {

  MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(numberOfHistograms, numberOfBins);
  ws->setYUnit("Asymmetry");
  AnalysisDataService::Instance().addOrReplace(WORKSPACE_NAME, ws);

  return ws;
}
void clearADS() { AnalysisDataService::Instance().clear(); }

} // namespace

class MockPSIBackgroundSubtraction : public PSIBackgroundSubtraction {
public:
  MockPSIBackgroundSubtraction() {}

  // set return fit and backgrounds
  void setReturnFitQuality(const double fitQuality) { m_fitQuality = fitQuality; }
  void setReturnBackground(const double background) { m_background = background; }

private:
  std::tuple<double, double> calculateBackgroundFromFit(IAlgorithm_sptr &, const std::pair<double, double> &range,
                                                        const int &index) override {
    (void)range;
    (void)index;
    return std::make_tuple(m_background, m_fitQuality);
  }
  double m_background{0.00};
  double m_fitQuality{0.00};
};

class PSIBackgroundSubtractionTest : public CxxTest::TestSuite {
public:
  PSIBackgroundSubtractionTest() { Mantid::API::FrameworkManager::Instance(); }

  static PSIBackgroundSubtractionTest *createSuite() { return new PSIBackgroundSubtractionTest(); }
  static void destroySuite(PSIBackgroundSubtractionTest *suite) { delete suite; }

  void test_algorithm_initializes() {
    PSIBackgroundSubtraction alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_that_algorithm_does_not_execute_if_invalid_y_label() {
    PSIBackgroundSubtraction alg;
    auto ws = createInvalidTestWorkspace(2, 100);

    alg.initialize();
    alg.setProperty("InputWorkspace", ws);

    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    clearADS();
  }

  void test_that_algorithm_does_not_execute_if_no_good_data() {
    PSIBackgroundSubtraction alg;
    auto ws = createCountsTestWorkspace(2, 100, false);

    alg.initialize();
    alg.setProperty("InputWorkspace", ws);

    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    clearADS();
  }

  void test_that_algorithm_does_not_execute_if_bad_first_good_data() {
    PSIBackgroundSubtraction alg;
    int numberOfHistograms = 2;
    int numberOfBins = 100;
    auto ws = createCountsTestWorkspace(numberOfHistograms, numberOfBins, false);
    for (int index = 0; index < numberOfHistograms; index++) {
      ws->mutableRun().addProperty("First good spectra " + std::to_string(index), -1);
      ws->mutableRun().addProperty("Last good spectra " + std::to_string(index), numberOfBins - 10);
    }

    alg.initialize();
    alg.setProperty("InputWorkspace", ws);

    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    clearADS();
  }

  void test_that_algorithm_does_not_execute_if_bad_last_good_data() {
    PSIBackgroundSubtraction alg;
    int numberOfHistograms = 2;
    int numberOfBins = 100;
    auto ws = createCountsTestWorkspace(numberOfHistograms, numberOfBins, false);
    for (int index = 0; index < numberOfHistograms; index++) {
      ws->mutableRun().addProperty("First good spectra " + std::to_string(index), 1);
      ws->mutableRun().addProperty("Last good spectra " + std::to_string(index), numberOfBins * 2);
    }

    alg.initialize();
    alg.setProperty("InputWorkspace", ws);

    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    clearADS();
  }

  void test_that_algorithm_does_not_execute_if_last_before_first_good_data() {
    PSIBackgroundSubtraction alg;
    int numberOfHistograms = 2;
    int numberOfBins = 100;
    auto ws = createCountsTestWorkspace(numberOfHistograms, numberOfBins, false);
    for (int index = 0; index < numberOfHistograms; index++) {
      ws->mutableRun().addProperty("First good spectra " + std::to_string(index), 50);
      ws->mutableRun().addProperty("Last good spectra " + std::to_string(index), 40);
    }

    alg.initialize();
    alg.setProperty("InputWorkspace", ws);

    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    clearADS();
  }

  void test_background_correctly_removed_from_input_workspace() {
    MockPSIBackgroundSubtraction alg;
    double background = 20;
    double fitQuality = 1.00;
    auto ws = createCountsTestWorkspace(4, 100);
    auto wsClone = ws->clone();
    alg.setReturnBackground(background);
    alg.setReturnFitQuality(fitQuality);

    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    alg.execute();

    for (size_t index = 0; index < ws->getNumberHistograms(); ++index) {
      for (size_t i = 0; i < ws->y(index).size(); ++i)
        TS_ASSERT_EQUALS(ws->y(index)[i], wsClone->y(index)[i] - background)
    }
    clearADS();
  }

  void test_background_correctly_removed_from_input_workspace_when_startX_and_endX_are_set() {
    MockPSIBackgroundSubtraction alg;
    double background = 20;
    double fitQuality = 1.00;
    auto ws = createCountsTestWorkspace(4, 100);
    auto wsClone = ws->clone();
    alg.setReturnBackground(background);
    alg.setReturnFitQuality(fitQuality);

    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    alg.setProperty("StartX", 25.0);
    alg.setProperty("EndX", 75.0);
    alg.execute();

    for (auto wsIndex = 0u; wsIndex < ws->getNumberHistograms(); ++wsIndex) {
      for (auto i = 0u; i < ws->y(wsIndex).size(); ++i)
        TS_ASSERT_EQUALS(ws->y(wsIndex)[i], wsClone->y(wsIndex)[i] - background)
    }
    clearADS();
  }

  void test_background_subtraction_algorithm_is_called_ok_when_provided_a_function() {
    MockPSIBackgroundSubtraction alg;
    double background = 19.734;
    double fitQuality = 1.00;
    std::string const function = "name=GausOsc,A=500,Sigma=0.2,Frequency=40.,Phi=0";
    auto ws = createCountsTestWorkspace(4, 100);
    auto wsClone = ws->clone();
    alg.setReturnBackground(background);
    alg.setReturnFitQuality(fitQuality);

    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    alg.setProperty("StartX", 25.0);
    alg.setProperty("EndX", 75.0);
    alg.setProperty("Function", function);
    alg.execute();

    for (auto wsIndex = 0u; wsIndex < ws->getNumberHistograms(); ++wsIndex) {
      for (auto i = 0u; i < ws->y(wsIndex).size(); ++i)
        TS_ASSERT_EQUALS(ws->y(wsIndex)[i], wsClone->y(wsIndex)[i] - background)
    }
    clearADS();
  }
};
