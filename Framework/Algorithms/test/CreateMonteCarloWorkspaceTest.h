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
    int iterations = alg.computeNumberOfIterations(yData);
    TS_ASSERT_EQUALS(iterations, 10); // Verify if the sum of yData is rounded correctly
  }

  void test_computeNormalizedCDF() {
    CreateMonteCarloWorkspace alg;
    Mantid::HistogramData::HistogramY yData = {1.0, 2.0, 3.0, 4.0};
    std::vector<double> cdf = alg.computeNormalizedCDF(yData);
    TS_ASSERT_EQUALS(cdf.size(), yData.size());
    TS_ASSERT_DELTA(cdf.back(), 1.0, 1e-6); // Check if the last element is normalized to 1.0
  }

  void test_fillHistogramWithRandomData() {
    CreateMonteCarloWorkspace alg;
    std::vector<double> cdf = {0.1, 0.3, 0.6, 1.0};
    std::mt19937 gen(32);

    Mantid::HistogramData::HistogramY outputY = alg.fillHistogramWithRandomData(cdf, 100, gen);

    auto sumCounts = std::accumulate(outputY.begin(), outputY.end(), 0.0);
    TS_ASSERT_EQUALS(sumCounts, 100); // Ensure that the total number of counts is correct
  }

  void test_fillHistogramWithRandomData_indexOutOfBounds() {
    CreateMonteCarloWorkspace alg;
    // Create a CDF where the last value is less than 1.0
    std::vector<double> cdf = {0.2, 0.5, 0.7, 0.9};
    std::mt19937 gen(42);

    int numIterations = 1000;

    Mantid::HistogramData::HistogramY outputY = alg.fillHistogramWithRandomData(cdf, numIterations, gen);

    // The total counts should be less than numIterations because some random numbers are not counted
    auto sumCounts = std::accumulate(outputY.begin(), outputY.end(), 0.0);
    TS_ASSERT_LESS_THAN(sumCounts, numIterations);

    double expectedCounts = cdf.back() * numIterations;
    TS_ASSERT_DELTA(sumCounts, expectedCounts, numIterations * 0.05); // Allow 5% tolerance

    // Ensure no counts are negative or NaN
    for (size_t i = 0; i < outputY.size(); ++i) {
      TS_ASSERT_LESS_THAN_EQUALS(0.0, outputY[i]);
      TS_ASSERT(std::isfinite(outputY[i]));
    }
  }

  void test_exec() {
    CreateMonteCarloWorkspace alg;
    alg.initialize();

    auto inputWS = WorkspaceCreationHelper::create2DWorkspace(1, 10);
    auto &yData = inputWS->mutableY(0);
    std::fill(yData.begin(), yData.end(), 5.0);

    alg.setProperty("InputWorkspace", inputWS);
    alg.setProperty("Seed", 32);
    alg.setPropertyValue("OutputWorkspace", "MonteCarloTest_WS");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    using Mantid::API::AnalysisDataService; // To Retrieve the output workspace
    MatrixWorkspace_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("MonteCarloTest_WS");
    TS_ASSERT(outputWS);

    // Verify the output data
    const auto &outputY = outputWS->y(0);
    auto sumOutput = std::accumulate(outputY.begin(), outputY.end(), 0.0);
    TS_ASSERT_EQUALS(sumOutput, alg.computeNumberOfIterations(yData)); // Compare number of iterations

    // Clean up: Remove the workspace from ADS after test
    AnalysisDataService::Instance().remove("MonteCarloTest_WS");
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
};
