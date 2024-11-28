#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/CreateMonteCarloWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidHistogramData/Histogram.h"
#include <cxxtest/TestSuite.h>

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

  void test_computeNumberOfIterations() {
    CreateMonteCarloWorkspace alg;
    Mantid::HistogramData::HistogramY yData = {1.0, 2.0, 3.0, 4.0};
    int iterations = alg.computeNumberOfIterations(yData, 0);
    TS_ASSERT_EQUALS(iterations, 10); // Verify yData rounds correctly

    // Test custom Monte Carlo events
    iterations = alg.computeNumberOfIterations(yData, 20);
    TS_ASSERT_EQUALS(iterations, 20); // Should override with the custom number
  }

  void test_computeNormalizedCDF() {
    CreateMonteCarloWorkspace alg;
    Mantid::HistogramData::HistogramY yData = {1.0, 2.0, 3.0, 4.0};
    std::vector<double> cdf = alg.computeNormalizedCDF(yData);
    TS_ASSERT_EQUALS(cdf.size(), yData.size());
    TS_ASSERT_DELTA(cdf.back(), 1.0, 1e-6); // Check the last element is normalized to 1.0
  }

  void test_fillHistogramWithRandomData() {
    CreateMonteCarloWorkspace alg;
    std::vector<double> cdf = {0.1, 0.3, 0.6, 1.0};
    Mantid::API::Progress progress(nullptr, 0.0, 1.0, 1); // Dummy progress
    Mantid::HistogramData::HistogramY outputY = alg.fillHistogramWithRandomData(cdf, 100, 32, progress);

    auto sumCounts = std::accumulate(outputY.begin(), outputY.end(), 0.0);
    TS_ASSERT_EQUALS(sumCounts, 100); // Ensure the total number of counts is correct
  }

  void test_scaleInputToMatchMCEvents() {
    CreateMonteCarloWorkspace alg;
    Mantid::HistogramData::HistogramY yData = {1.0, 2.0, 3.0, 4.0};

    // Test scaling
    int targetMCEvents = 20;
    Mantid::HistogramData::HistogramY scaledY = alg.scaleInputToMatchMCEvents(yData, targetMCEvents);

    double totalScaledCounts = std::accumulate(scaledY.begin(), scaledY.end(), 0.0);
    TS_ASSERT_DELTA(totalScaledCounts, 20.0, 1e-6); // Verify the scaled sum matches targetMCEvents
  }

  MatrixWorkspace_sptr createInputWorkspace(int numBins, double initialValue) {
    auto ws = WorkspaceCreationHelper::create2DWorkspace(1, numBins);
    auto &yData = ws->mutableY(0);
    std::fill(yData.begin(), yData.end(), initialValue);
    return ws;
  }

  MatrixWorkspace_sptr runMonteCarloWorkspace(const MatrixWorkspace_sptr &inputWS, int seed, int mcEvents,
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

  void removeWorkspace(const std::string &workspaceName) {
    using Mantid::API::AnalysisDataService;
    AnalysisDataService::Instance().remove(workspaceName);
  }

  void test_exec_with_scaling() {
    auto inputWS = createInputWorkspace(10, 5.0); // 10 bins, initial value 5.0
    auto outputWS = runMonteCarloWorkspace(inputWS, 32, 100, "MonteCarloTest_Scaled");

    TS_ASSERT(outputWS);

    // Verify the output data
    const auto &outputY = outputWS->y(0);
    auto sumOutput = std::accumulate(outputY.begin(), outputY.end(), 0.0);
    TS_ASSERT_DELTA(sumOutput, 100.0, 1e-6); // Verify sum matches custom MC events

    // Clean up
    removeWorkspace("MonteCarloTest_Scaled");
  }

  void test_exec_without_scaling() {
    auto inputWS = createInputWorkspace(10, 5.0); // 10 bins, initial value 5.0
    auto outputWS = runMonteCarloWorkspace(inputWS, 32, 0, "MonteCarloTest_Default");

    TS_ASSERT(outputWS);

    // Verify the output data
    const auto &outputY = outputWS->y(0);
    auto sumOutput = std::accumulate(outputY.begin(), outputY.end(), 0.0);
    TS_ASSERT_DELTA(sumOutput, 50.0, 1e-6); // Verify sum matches input data's total counts

    // Clean up
    removeWorkspace("MonteCarloTest_Default");
  }

  void test_reproducibility_with_seed() {
    auto inputWS = createInputWorkspace(10, 5.0); // 10 bins, initial value 5.0

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

    // Clean up
    removeWorkspace("MonteCarloTest_WS1");
    removeWorkspace("MonteCarloTest_WS2");
  }

  void test_error_calculation() {
    auto inputWS = WorkspaceCreationHelper::create2DWorkspace(1, 10);
    auto &yData = inputWS->mutableY(0);

    // Using perfect squares (for sqrt testing)
    yData = {1.0, 4.0, 9.0, 16.0, 25.0, 36.0, 49.0, 64.0, 81.0, 100.0};

    auto outputWS = runMonteCarloWorkspace(inputWS, 32, 0, "MonteCarloTest_Error");
    TS_ASSERT(outputWS);

    const auto &outputY = outputWS->y(0);
    const auto &outputE = outputWS->e(0);

    TS_ASSERT_EQUALS(outputY.size(), outputE.size());
    for (size_t i = 0; i < outputY.size(); ++i) {
      TS_ASSERT_DELTA(outputE[i], std::sqrt(outputY[i]), 1e-6);
    }

    // Clean up
    removeWorkspace("MonteCarloTest_Error");
  }
};
