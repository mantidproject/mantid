#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/CreateMonteCarloWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidHistogramData/Histogram.h"
#include <cxxtest/TestSuite.h>

#include <numeric>
#include <random>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::HistogramData;

using Mantid::Algorithms::CreateMonteCarloWorkspace;

class CreateMonteCarloWorkspaceTest : public CxxTest::TestSuite {
public:
  static CreateMonteCarloWorkspaceTest *createSuite() { return new CreateMonteCarloWorkspaceTest(); }
  static void destroySuite(CreateMonteCarloWorkspaceTest *suite) { delete suite; }

  void test_Init() {
    CreateMonteCarloWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_integrateYData() {
    CreateMonteCarloWorkspace alg;
    Mantid::HistogramData::HistogramY yData = {1.0, 2.0, 3.0, 4.0};
    int iterations = alg.integrateYData(yData);
    TS_ASSERT_EQUALS(iterations, 10); // Verify yData sums to 10 (1+2+3+4)
  }

  void test_computeNormalizedCDF() {
    CreateMonteCarloWorkspace alg;
    Mantid::HistogramData::HistogramY yData = {1.0, 2.0, 3.0, 4.0};
    std::vector<double> cdf = alg.computeNormalizedCDF(yData);
    TS_ASSERT_EQUALS(cdf.size(), yData.size());
    TS_ASSERT_DELTA(cdf.back(), 1.0, 1e-6); // Check last element is normalized to 1.0
  }

  void test_fillHistogramWithRandomData() {
    CreateMonteCarloWorkspace alg;
    std::vector<double> cdf = {0.1, 0.3, 0.6, 1.0};
    Mantid::API::Progress progress(nullptr, 0.0, 1.0, 1); // Dummy progress
    Mantid::HistogramData::HistogramY outputY = alg.fillHistogramWithRandomData(cdf, 100, 32, progress);

    auto sumCounts = std::accumulate(outputY.begin(), outputY.end(), 0.0);
    TS_ASSERT_EQUALS(sumCounts, 100); // Ensure total number of counts is correct
  }

  void test_exec_with_custom_MCEvents() {
    auto inputWS = createInputWorkspace(10, 5.0); // 10 bins, each bin has 5.0
    auto outputWS = runMonteCarloWorkspace(inputWS, 32, 100, "MonteCarloTest_CustomMC");

    TS_ASSERT(outputWS);
    const auto &outputY = outputWS->y(0);
    auto sumOutput = std::accumulate(outputY.begin(), outputY.end(), 0.0);
    TS_ASSERT_DELTA(sumOutput, 100.0, 1e-6); // Verify sum matches custom MC events

    removeWorkspace("MonteCarloTest_CustomMC");
  }

  void test_exec_without_custom_events() {
    // Passing zero => use the input data's sum (10 bins * 5.0 = 50 total)
    auto inputWS = createInputWorkspace(10, 5.0);
    auto outputWS = runMonteCarloWorkspace(inputWS, 32, 0, "MonteCarloTest_Default");

    TS_ASSERT(outputWS);
    const auto &outputY = outputWS->y(0);
    auto sumOutput = std::accumulate(outputY.begin(), outputY.end(), 0.0);
    TS_ASSERT_DELTA(sumOutput, 50.0, 1e-6); // Sum matches input data's total counts

    removeWorkspace("MonteCarloTest_Default");
  }

  void test_reproducibility_with_seed() {
    // Both run with the same seed and should produce identical Y values
    auto inputWS = createInputWorkspace(10, 5.0);

    auto outputWS1 = runMonteCarloWorkspace(inputWS, 42, 0, "MonteCarloTest_WS1");
    auto outputWS2 = runMonteCarloWorkspace(inputWS, 42, 0, "MonteCarloTest_WS2");

    TS_ASSERT(outputWS1);
    TS_ASSERT(outputWS2);

    const auto &outputY1 = outputWS1->y(0);
    const auto &outputY2 = outputWS2->y(0);

    TS_ASSERT_EQUALS(outputY1.size(), outputY2.size());
    for (size_t i = 0; i < outputY1.size(); ++i) {
      TS_ASSERT_EQUALS(outputY1[i], outputY2[i]);
    }

    removeWorkspace("MonteCarloTest_WS1");
    removeWorkspace("MonteCarloTest_WS2");
  }

  void test_error_calculation() {
    // We fill the input with perfect squares to easily check sqrt in the result
    auto inputWS = WorkspaceCreationHelper::create2DWorkspace(1, 10);
    auto &yData = inputWS->mutableY(0);
    yData = {1.0, 4.0, 9.0, 16.0, 25.0, 36.0, 49.0, 64.0, 81.0, 100.0};

    auto outputWS = runMonteCarloWorkspace(inputWS, 32, 0, "MonteCarloTest_Error");
    TS_ASSERT(outputWS);

    const auto &outputY = outputWS->y(0);
    const auto &outputE = outputWS->e(0);

    TS_ASSERT_EQUALS(outputY.size(), outputE.size());
    for (size_t i = 0; i < outputY.size(); ++i) {
      TS_ASSERT_DELTA(outputE[i], std::sqrt(outputY[i]), 1e-6);
    }

    removeWorkspace("MonteCarloTest_Error");
  }

private:
  static MatrixWorkspace_sptr createInputWorkspace(int numBins, double initialValue);
  static MatrixWorkspace_sptr runMonteCarloWorkspace(const MatrixWorkspace_sptr &inputWS, int seed, int mcEvents,
                                                     const std::string &outputName);
  static void removeWorkspace(const std::string &workspaceName);
};

// -- Helper method implementations below --

MatrixWorkspace_sptr CreateMonteCarloWorkspaceTest::createInputWorkspace(int numBins, double initialValue) {
  auto ws = WorkspaceCreationHelper::create2DWorkspace(1, numBins);
  auto &yData = ws->mutableY(0);
  std::fill(yData.begin(), yData.end(), initialValue);
  return ws;
}

MatrixWorkspace_sptr CreateMonteCarloWorkspaceTest::runMonteCarloWorkspace(const MatrixWorkspace_sptr &inputWS,
                                                                           int seed, int mcEvents,
                                                                           const std::string &outputName) {
  CreateMonteCarloWorkspace alg;
  alg.initialize();
  alg.setProperty("InputWorkspace", inputWS);
  alg.setProperty("Seed", seed);
  alg.setProperty("MonteCarloEvents", mcEvents);
  alg.setPropertyValue("OutputWorkspace", outputName);

  TS_ASSERT_THROWS_NOTHING(alg.execute());
  TS_ASSERT(alg.isExecuted());

  using Mantid::API::AnalysisDataService;
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputName);
}

void CreateMonteCarloWorkspaceTest::removeWorkspace(const std::string &workspaceName) {
  using Mantid::API::AnalysisDataService;
  AnalysisDataService::Instance().remove(workspaceName);
}
