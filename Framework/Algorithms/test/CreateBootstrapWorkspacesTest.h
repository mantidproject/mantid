#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidAlgorithms/CreateBootstrapWorkspaces.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::HistogramData;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class CreateBootstrapWorkspacesTest : public CxxTest::TestSuite {
public:
  static CreateBootstrapWorkspacesTest *createSuite() { return new CreateBootstrapWorkspacesTest(); }
  static void destroySuite(CreateBootstrapWorkspacesTest *suite) { delete suite; }

  void test_Init() {
    CreateBootstrapWorkspaces alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_sampleHistogramFromGaussian_with_zero_errors() {
    // Gaussian with zero deviation should return mean exactly

    CreateBootstrapWorkspaces alg;
    Progress progress(nullptr, 0.0, 1.0, 1); // Dummy progress
    HistogramY dataY = {1.0, 2.0, 3.0, 4.0};
    HistogramE dataE = {0.0, 0.0, 0.0, 0.0};
    std::mt19937 gen(32);

    HistogramY outputY = alg.sampleHistogramFromGaussian(dataY, dataE, gen);

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

    runBootstrapWorkspace(inputWS, 32, 5, "ErrorSampling", "Boot1_sample_", "Boot1_Group");
    runBootstrapWorkspace(inputWS, 32, 5, "ErrorSampling", "Boot2_sample_", "Boot2_Group");

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

    ADS.remove("Boot1_Group");
    ADS.remove("Boot2_Group");
  }

  void test_number_of_bootstrap_samples() {

    auto inputWS = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    inputWS->mutableY(0) = {1.0};
    inputWS->mutableE(0) = {0.1};

    runBootstrapWorkspace(inputWS, 32, 10, "ErrorSampling", "Nsample_", "BootNSamples_Group");

    auto &ADS = AnalysisDataService::Instance();
    auto ws_group = ADS.retrieveWS<WorkspaceGroup>("BootNSamples_Group");

    TS_ASSERT_EQUALS(ws_group->getNumberOfEntries(), 10);

    ADS.remove("BootNSamples_Group");
  }

  void test_bootstrap_with_error_sampling() {

    auto inputWS = WorkspaceCreationHelper::create2DWorkspace(1, 5);
    inputWS->mutableY(0) = {1.0, 2.0, 3.0, 4.0, 5.0};
    inputWS->mutableE(0) = {0.1, 0.2, 0.3, 0.4, 0.5};

    runBootstrapWorkspace(inputWS, 32, 1, "ErrorSampling", "BootErr_sample_", "BootErr_Group");
    auto &ADS = AnalysisDataService::Instance();
    auto ws = ADS.retrieveWS<MatrixWorkspace>("BootErr_sample_1");

    TS_ASSERT(ws);

    const auto &outputY = ws->y(0);
    const auto &outputE = ws->e(0);

    HistogramY expectedY = {0.988227, 2.23355, 2.75155, 3.28837, 5.32049};

    TS_ASSERT_EQUALS(outputY.size(), outputE.size());
    for (size_t i = 0; i < outputY.size(); ++i) {
      TS_ASSERT_DELTA(outputY[i], expectedY[i], 1e-5);
      TS_ASSERT_EQUALS(outputE[i], inputWS->e(0)[i]);
    }
    std::cout << "\n";

    ADS.remove("BootErr_Group");
  }

  void test_bootstrap_with_spectra_sampling() {

    auto inputWS = WorkspaceCreationHelper::create2DWorkspace(3, 5);
    inputWS->mutableY(0) = {1.0, 1.0, 1.0, 1.0, 1.0};
    inputWS->mutableE(0) = {0.1, 0.1, 0.1, 0.1, 0.1};

    inputWS->mutableY(1) = {2.0, 2.0, 2.0, 2.0, 2.0};
    inputWS->mutableE(1) = {0.2, 0.2, 0.2, 0.2, 0.2};

    inputWS->mutableY(2) = {3.0, 3.0, 3.0, 3.0, 3.0};
    inputWS->mutableE(2) = {0.3, 0.3, 0.3, 0.3, 0.3};

    runBootstrapWorkspace(inputWS, 32, 5, "SpectraSampling", "BootSpec_sample_", "BootSpec_Group");
    auto &ADS = AnalysisDataService::Instance();

    auto ws = ADS.retrieveWS<MatrixWorkspace>("BootSpec_sample_2");

    TS_ASSERT(ws);

    const auto &outputY = ws->y(0);
    const auto &outputE = ws->e(0);

    // Check that output is a particular resampling of entire spectrums
    TS_ASSERT_EQUALS(outputY.size(), outputE.size());
    for (size_t i = 0; i < inputWS->blocksize(); ++i) {
      TS_ASSERT_EQUALS(ws->y(0)[i], inputWS->y(1)[i]);
      TS_ASSERT_EQUALS(ws->e(0)[i], inputWS->e(1)[i]);

      TS_ASSERT_EQUALS(ws->y(1)[i], inputWS->y(1)[i]);
      TS_ASSERT_EQUALS(ws->e(1)[i], inputWS->e(1)[i]);

      TS_ASSERT_EQUALS(ws->y(2)[i], inputWS->y(0)[i]);
      TS_ASSERT_EQUALS(ws->e(2)[i], inputWS->e(0)[i]);
    }

    ADS.remove("BootSpec_Group");
  }

private:
  // -- Helper method implementations below --

  static void runBootstrapWorkspace(const MatrixWorkspace_sptr &inputWS, int seed, int numReplicas,
                                    const std::string &bootType, const std::string &prefix,
                                    const std::string &outputName) {
    CreateBootstrapWorkspaces alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inputWS);
    alg.setProperty("Seed", seed);
    alg.setProperty("NumberOfReplicas", numReplicas);
    alg.setProperty("BootstrapType", bootType);
    alg.setProperty("OutputPrefix", prefix);
    alg.setPropertyValue("OutputWorkspaceGroup", outputName);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
  }
};
