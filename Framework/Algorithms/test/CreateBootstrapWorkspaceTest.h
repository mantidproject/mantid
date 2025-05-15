#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidAlgorithms/CreateBootstrapWorkspace.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include <numeric>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::HistogramData;

using Mantid::Algorithms::CreateBootstrapWorkspace;
using Mantid::API::AnalysisDataService;

class CreateBootstrapWorkspaceTest : public CxxTest::TestSuite {
public:
  static CreateBootstrapWorkspaceTest *createSuite() { return new CreateBootstrapWorkspaceTest(); }
  static void destroySuite(CreateBootstrapWorkspaceTest *suite) { delete suite; }

  void test_Init() {
    CreateBootstrapWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_sampleHistogramFromGaussian_with_zero_errors() {
    CreateBootstrapWorkspace alg;
    Mantid::API::Progress progress(nullptr, 0.0, 1.0, 1); // Dummy progress
    Mantid::HistogramData::HistogramY dataY = {1.0, 2.0, 3.0, 4.0};
    Mantid::HistogramData::HistogramE dataE = {0.0, 0.0, 0.0, 0.0};
    std::mt19937 gen(32);

    Mantid::HistogramData::HistogramY outputY = alg.sampleHistogramFromGaussian(dataY, dataE, gen);

    // Expect result equal to input dataY
    TS_ASSERT_EQUALS(dataY.size(), outputY.size());
    for (size_t i = 0; i < dataY.size(); ++i) {
      TS_ASSERT_EQUALS(dataY[i], outputY[i]);
    }
  }

  void test_reproducibility_with_seed_and_error_sampling() {
    // Both run with the same seed and should produce identical Y values

    auto inputWS = WorkspaceCreationHelper::create2DWorkspace(1, 5);
    inputWS->mutableY(0) = {1.0, 2.0, 3.0, 4.0, 5.0};
    inputWS->mutableE(0) = {0.5, 0.5, 0.5, 0.5, 0.5};

    runBootstrapWorkspace(inputWS, 32, 5, true, "Boot1_sample_", "Boot1_Group");
    runBootstrapWorkspace(inputWS, 32, 5, true, "Boot2_sample_", "Boot2_Group");

    auto &ADS = AnalysisDataService::Instance();
    auto ws1 = ADS.retrieveWS<MatrixWorkspace>("Boot1_sample_5");
    auto ws2 = ADS.retrieveWS<MatrixWorkspace>("Boot2_sample_5");

    TS_ASSERT(ws1);
    TS_ASSERT(ws2);

    const auto &outputY1 = ws1->y(0);
    const auto &outputY2 = ws2->y(0);

    TS_ASSERT_EQUALS(outputY1.size(), outputY2.size());
    for (size_t i = 0; i < outputY1.size(); ++i) {
      TS_ASSERT_EQUALS(outputY1[i], outputY2[i]);
    }

    removeWorkspace("Boot1_Group");
    removeWorkspace("Boot2_Group");
  }

  void test_bootstrap_with_error_sampling() {
    auto inputWS = WorkspaceCreationHelper::create2DWorkspace(1, 5);
    inputWS->mutableY(0) = {1.0, 2.0, 3.0, 4.0, 5.0};
    inputWS->mutableE(0) = {0.5, 0.5, 0.5, 0.5, 0.5};

    runBootstrapWorkspace(inputWS, 32, 1, true, "BootErr_sample_", "BootErr_Group");
    auto &ADS = AnalysisDataService::Instance();
    auto ws = ADS.retrieveWS<MatrixWorkspace>("BootErr_sample_1");

    TS_ASSERT(ws);

    const auto &outputY = ws->y(0);
    const auto &outputE = ws->e(0);

    TS_ASSERT_EQUALS(outputY.size(), outputE.size());
    for (size_t i = 0; i < outputY.size(); ++i) {
      cout << outputY[i] << " ";
      TS_ASSERT_EQUALS(outputE[i], inputWS->e(0)[i]);
    }
    cout << std::endl;

    removeWorkspace("BootErr_Group");
  }

  void test_bootstrap_with_spectra_sampling() {
    auto inputWS = WorkspaceCreationHelper::create2DWorkspace(3, 5);
    inputWS->mutableY(0) = {1.0, 1.0, 1.0, 1.0, 1.0};
    inputWS->mutableY(1) = {2.0, 2.0, 2.0, 2.0, 2.0};
    inputWS->mutableY(2) = {3.0, 3.0, 3.0, 3.0, 3.0};

    runBootstrapWorkspace(inputWS, 32, 5, true, "BootSpec_sample_", "BootSpec_Group");
    auto &ADS = AnalysisDataService::Instance();

    auto ws = ADS.retrieveWS<MatrixWorkspace>("BootSpec_sample_3");

    TS_ASSERT(ws);

    const auto &outputY = ws->y(0);
    const auto &outputE = ws->e(0);

    TS_ASSERT_EQUALS(outputY.size(), outputE.size());
    for (size_t i = 0; i < inputWS->blocksize(); ++i) {
      cout << ws->y(0)[i] << " ";
      TS_ASSERT_EQUALS(ws->y(0)[i], inputWS->y(0)[i]);
      cout << ws->y(1)[i] << " ";
      TS_ASSERT_EQUALS(ws->y(1)[i], inputWS->y(0)[i]);
      cout << ws->y(2)[i] << " ";
      TS_ASSERT_EQUALS(ws->y(2)[i], inputWS->y(0));
    }
    cout << std::endl;

    removeWorkspace("BootErr_Group");
  }

private:
  static void runBootstrapWorkspace(const MatrixWorkspace_sptr &inputWS, int seed, int numReplicas,
                                    bool useErrorSampling, const std::string &prefix, const std::string &outputName);
  static void removeWorkspace(const std::string &workspaceName);
};

// -- Helper method implementations below --

void CreateBootstrapWorkspaceTest::runBootstrapWorkspace(const MatrixWorkspace_sptr &inputWS, int seed, int numReplicas,
                                                         bool useErrorSampling, const std::string &prefix,
                                                         const std::string &outputName) {
  CreateBootstrapWorkspace alg;
  alg.initialize();
  alg.setProperty("InputWorkspace", inputWS);
  alg.setProperty("Seed", seed);
  alg.setProperty("NumberOfReplicas", numReplicas);
  alg.setProperty("UseErrorSampling", useErrorSampling);
  alg.setProperty("OutputPrefix", prefix);
  alg.setPropertyValue("OutputWorkspaceGroup", outputName);

  TS_ASSERT_THROWS_NOTHING(alg.execute());
  TS_ASSERT(alg.isExecuted());
}

void CreateBootstrapWorkspaceTest::removeWorkspace(const std::string &workspaceName) {
  using Mantid::API::AnalysisDataService;
  AnalysisDataService::Instance().remove(workspaceName);
}
