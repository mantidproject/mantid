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
    TS_ASSERT_EQUALS(iterations, 10);                         // Verify yData rounds correctly

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

void test_exec_with_scaling() {
    CreateMonteCarloWorkspace alg;
    alg.initialize();

    auto inputWS = WorkspaceCreationHelper::create2DWorkspace(1, 10);
    auto &yData = inputWS->mutableY(0);
    std::fill(yData.begin(), yData.end(), 5.0); // Set uniform data

    alg.setProperty("InputWorkspace", inputWS);
    alg.setProperty("Seed", 32);
    alg.setProperty("MonteCarloEvents", 100); // Custom MC events
    alg.setPropertyValue("OutputWorkspace", "MonteCarloTest_Scaled");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    using Mantid::API::AnalysisDataService;
    MatrixWorkspace_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("MonteCarloTest_Scaled");
    TS_ASSERT(outputWS);

    // Verify the output data
    const auto &outputY = outputWS->y(0);
    auto sumOutput = std::accumulate(outputY.begin(), outputY.end(), 0.0);
    TS_ASSERT_DELTA(sumOutput, 100.0, 1e-6); // Verify sum matches custom MC events

    // Clean up
    AnalysisDataService::Instance().remove("MonteCarloTest_Scaled");
  }

void test_exec_without_scaling() {
    CreateMonteCarloWorkspace alg;
    alg.initialize();

    auto inputWS = WorkspaceCreationHelper::create2DWorkspace(1, 10);
    auto &yData = inputWS->mutableY(0);
    std::fill(yData.begin(), yData.end(), 5.0); // Set uniform data

    alg.setProperty("InputWorkspace", inputWS);
    alg.setProperty("Seed", 32);
    alg.setProperty("MonteCarloEvents", 0); // Default behavior
    alg.setPropertyValue("OutputWorkspace", "MonteCarloTest_Default");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    using Mantid::API::AnalysisDataService;
    MatrixWorkspace_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("MonteCarloTest_Default");
    TS_ASSERT(outputWS);

    // Verify the output data
    const auto &outputY = outputWS->y(0);
    auto sumOutput = std::accumulate(outputY.begin(), outputY.end(), 0.0);
    TS_ASSERT_DELTA(sumOutput, 50.0, 1e-6); // Verify sum matches input data's total counts

    // Clean up
    AnalysisDataService::Instance().remove("MonteCarloTest_Default");
  }

  void test_reproducibility_with_seed() {
    CreateMonteCarloWorkspace alg1;
    alg1.initialize();

    auto inputWS = WorkspaceCreationHelper::create2DWorkspace(1, 10);
    auto &yData = inputWS->mutableY(0);
    std::fill(yData.begin(), yData.end(), 5.0);

    alg1.setProperty("InputWorkspace", inputWS);
    alg1.setProperty("Seed", 42);
    alg1.setPropertyValue("OutputWorkspace", "MonteCarloTest_WS1");

    TS_ASSERT_THROWS_NOTHING(alg1.execute());
    TS_ASSERT(alg1.isExecuted());

    // Second run to compare output data
    CreateMonteCarloWorkspace alg2;
    alg2.initialize();

    alg2.setProperty("InputWorkspace", inputWS);
    alg2.setProperty("Seed", 42);
    alg2.setPropertyValue("OutputWorkspace", "MonteCarloTest_WS2");

    TS_ASSERT_THROWS_NOTHING(alg2.execute());
    TS_ASSERT(alg2.isExecuted());

    using Mantid::API::AnalysisDataService;
    MatrixWorkspace_sptr outputWS1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("MonteCarloTest_WS1");
    MatrixWorkspace_sptr outputWS2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("MonteCarloTest_WS2");

    TS_ASSERT(outputWS1);
    TS_ASSERT(outputWS2);

    // Compare the output data
    const auto &outputY1 = outputWS1->y(0);
    const auto &outputY2 = outputWS2->y(0);

    TS_ASSERT_EQUALS(outputY1.size(), outputY2.size());

    for (size_t i = 0; i < outputY1.size(); ++i) {
      TS_ASSERT_EQUALS(outputY1[i], outputY2[i]);
    }

    // Remove the workspaces from ADS
    AnalysisDataService::Instance().remove("MonteCarloTest_WS1");
    AnalysisDataService::Instance().remove("MonteCarloTest_WS2");
  }

  void test_error_calculation() {
    CreateMonteCarloWorkspace alg;
    alg.initialize();

    // Create an input workspace with predefined Y data
    auto inputWS = WorkspaceCreationHelper::create2DWorkspace(1, 10);
    auto &yData = inputWS->mutableY(0);

    // Using perfect squares (for sqrt testing)
    yData = {1.0, 4.0, 9.0, 16.0, 25.0, 36.0, 49.0, 64.0, 81.0, 100.0};

    alg.setProperty("InputWorkspace", inputWS);
    alg.setProperty("Seed", 32);
    alg.setPropertyValue("OutputWorkspace", "MonteCarloTest_Error");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    using Mantid::API::AnalysisDataService;
    MatrixWorkspace_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("MonteCarloTest_Error");
    TS_ASSERT(outputWS);

    // Verify the errors in the output workspace
    const auto &outputY = outputWS->y(0);
    const auto &outputE = outputWS->e(0);

    TS_ASSERT_EQUALS(outputY.size(), outputE.size());

    for (size_t i = 0; i < outputY.size(); ++i) {
      TS_ASSERT_DELTA(outputE[i], std::sqrt(outputY[i]), 1e-6);

      // Clean up
      AnalysisDataService::Instance().remove("MonteCarloTest_Error");
    }
  }
};
