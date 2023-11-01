// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAlgorithms/FindPeaksConvolve.h"
#include "MantidDataHandling/LoadNexusProcessed.h"

using Mantid::Algorithms::FindPeaksConvolve;

namespace {
/// Load focussed data file - from FindPeaksTest
void loadNexusProcessed(const std::string &filename, const std::string &wsname) {
  if (!Mantid::API::AnalysisDataService::Instance().doesExist(wsname)) {
    Mantid::DataHandling::LoadNexusProcessed loader;
    loader.initialize();
    loader.setProperty("Filename", filename);
    loader.setProperty("OutputWorkspace", wsname);
    loader.execute();
    TS_ASSERT(loader.isExecuted());
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist(wsname));
  }
}

Mantid::API::Algorithm_sptr set_up_alg(const std::string &input_ws_name, const std::string &output_ws_name) {
  auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("FindPeaksConvolve");
  // Don't put output in ADS by default
  alg->setChild(true);
  TS_ASSERT_THROWS_NOTHING(alg->initialize());
  TS_ASSERT(alg->isInitialized())
  TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", input_ws_name));
  TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputWorkspace", output_ws_name));
  return alg;
}

void assert_peak_centres_equal(const Mantid::API::ITableWorkspace_sptr &resultWs,
                               std::vector<double> &expectedPeakCentres) {
  const auto colNames = resultWs->getColumnNames();
  for (int i = static_cast<int>(resultWs->columnCount()) - 1; i >= 0; i--) {
    std::string colName{colNames[i]};
    if (colName.find("PeakCentre") != std::string::npos) {
      double cellValue{resultWs->Double(0, i)};
      TS_ASSERT_DELTA(expectedPeakCentres.back(), cellValue, 0.01);
      expectedPeakCentres.pop_back();
    }
  }
  TS_ASSERT(expectedPeakCentres.size() == 0);
}

const std::string INPUT_TEST_WS_NAME = "FindPeaksConvolveTest_input";
const std::string OUTPUT_TEST_WS_NAME = "FindPeaksConvolveTest_output";
} // anonymous namespace

class FindPeaksConvolveTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FindPeaksConvolveTest *createSuite() { return new FindPeaksConvolveTest(); }
  static void destroySuite(FindPeaksConvolveTest *suite) { delete suite; }

  void setUp() override {
    // Load data file into ADS once
    loadNexusProcessed("ENGINX_277208_focused_bank_2.nxs", INPUT_TEST_WS_NAME);
    loadNexusProcessed("VesuvioCalibSpec177.nxs", INPUT_TEST_WS_NAME + "_noisy");
  }

  void test_exec() {
    auto alg = set_up_alg(INPUT_TEST_WS_NAME, OUTPUT_TEST_WS_NAME);
    alg->setProperty("EstimatedPeakExtent", "100");
    alg->setProperty("IOversigmaThreshold", "3");
    TS_ASSERT_THROWS_NOTHING(alg->execute(););
    TS_ASSERT(alg->isExecuted());

    std::vector<double> expectedPeakCentres{16179.53, 16873.24, 17391.53, 18188.9,  19584.29,
                                            20636.82, 21553.79, 22678.08, 22973.11, 24527.98,
                                            27151.32, 31784.04, 41272.73, 43098.7,  46997.84};
    const Mantid::API::ITableWorkspace_sptr resultWs = alg->getProperty("OutputWorkspace");
    assert_peak_centres_equal(resultWs, expectedPeakCentres);
  }

  void test_execPeakExtentNBins() {
    auto alg = set_up_alg(INPUT_TEST_WS_NAME, OUTPUT_TEST_WS_NAME);
    alg->setProperty("EstimatedPeakExtentNBins", "25");
    alg->setProperty("IOversigmaThreshold", "3");
    TS_ASSERT_THROWS_NOTHING(alg->execute(););
    TS_ASSERT(alg->isExecuted());

    std::vector<double> expectedPeakCentres{16179.53, 16873.24, 17391.53, 18188.9,  19584.29,
                                            20636.82, 21553.79, 22678.08, 22973.11, 24527.98,
                                            27151.32, 31784.04, 41272.73, 43098.7,  46997.84};
    const Mantid::API::ITableWorkspace_sptr resultWs = alg->getProperty("OutputWorkspace");
    assert_peak_centres_equal(resultWs, expectedPeakCentres);
  }

  void test_execHighestDataPoint() {
    auto alg = set_up_alg(INPUT_TEST_WS_NAME, OUTPUT_TEST_WS_NAME);
    alg->setProperty("EstimatedPeakExtent", "100");
    alg->setProperty("IOversigmaThreshold", "3");
    alg->setProperty("FindHighestDataPointInPeak", true);
    TS_ASSERT_THROWS_NOTHING(alg->execute(););
    TS_ASSERT(alg->isExecuted());

    std::vector<double> expectedPeakCentres{16179.53, 16873.24, 17391.53, 18188.9,  19584.29,
                                            20636.82, 21553.79, 22678.08, 22973.11, 24527.98,
                                            27151.32, 31784.04, 41280.7,  43098.7,  46997.84};
    const Mantid::API::ITableWorkspace_sptr resultWs = alg->getProperty("OutputWorkspace");
    assert_peak_centres_equal(resultWs, expectedPeakCentres);
  }

  void test_execNoisyData() {
    auto alg = set_up_alg(INPUT_TEST_WS_NAME + "_noisy", OUTPUT_TEST_WS_NAME);
    alg->setProperty("EstimatedPeakExtent", "400");
    TS_ASSERT_THROWS_NOTHING(alg->execute(););
    TS_ASSERT(alg->isExecuted());

    std::vector<double> expectedPeakCentres{2706.06, 3540.81, 4188.21, 4717.19, 5635.71, 6780.36, 7932.37};
    const Mantid::API::ITableWorkspace_sptr resultWs = alg->getProperty("OutputWorkspace");
    assert_peak_centres_equal(resultWs, expectedPeakCentres);
  }

  void test_execNoisyDataLargeKernelWithBinaryClosing() {
    auto alg = set_up_alg(INPUT_TEST_WS_NAME + "_noisy", OUTPUT_TEST_WS_NAME);
    alg->setProperty("EstimatedPeakExtent", "500");
    alg->setProperty("IOversigmaThreshold", "5");
    TS_ASSERT_THROWS_NOTHING(alg->execute(););
    TS_ASSERT(alg->isExecuted());

    std::vector<double> expectedPeakCentres{2788.43, 3505.61, 5635.71, 7932.37};
    const Mantid::API::ITableWorkspace_sptr resultWs = alg->getProperty("OutputWorkspace");
    assert_peak_centres_equal(resultWs, expectedPeakCentres);
  }

  void test_execNoisyDataLargeKernelNoBinaryClosing() {
    auto alg = set_up_alg(INPUT_TEST_WS_NAME + "_noisy", OUTPUT_TEST_WS_NAME);
    alg->setProperty("EstimatedPeakExtent", "500");
    alg->setProperty("IOversigmaThreshold", "5");
    alg->setProperty("PerformBinaryClosing", false);
    TS_ASSERT_THROWS_NOTHING(alg->execute(););
    TS_ASSERT(alg->isExecuted());

    std::vector<double> expectedPeakCentres{2788.43, 3505.61, 5635.71, 6780.36, 7932.37};
    const Mantid::API::ITableWorkspace_sptr resultWs = alg->getProperty("OutputWorkspace");
    assert_peak_centres_equal(resultWs, expectedPeakCentres);
  }

  void test_execSpecifyPeakExtentAndBins() {
    auto alg = set_up_alg(INPUT_TEST_WS_NAME, OUTPUT_TEST_WS_NAME);
    alg->setProperty("EstimatedPeakExtent", "100");
    alg->setProperty("EstimatedPeakExtentNBins", "100");
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument &);
  }
};