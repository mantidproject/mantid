#ifndef SCALETEST_H_
#define SCALETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/Scale.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/FrameworkManager.h"

using Mantid::MantidVec;

class ScaleTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(scale.name(), "Scale") }

  void testVersion() { TS_ASSERT_EQUALS(scale.version(), 1) }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(scale.initialize())
    TS_ASSERT(scale.isInitialized())
  }

  void testMultiply() {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    if (!scale.isInitialized())
      scale.initialize();

    AnalysisDataService::Instance().add(
        "tomultiply", WorkspaceCreationHelper::create2DWorkspace123(10, 10));
    TS_ASSERT_THROWS_NOTHING(
        scale.setPropertyValue("InputWorkspace", "tomultiply"));
    TS_ASSERT_THROWS_NOTHING(
        scale.setPropertyValue("OutputWorkspace", "multiplied"));
    TS_ASSERT_THROWS_NOTHING(scale.setPropertyValue("Factor", "2.5"));

    TS_ASSERT_THROWS_NOTHING(scale.execute());
    TS_ASSERT(scale.isExecuted());

    MatrixWorkspace_const_sptr in, result;
    TS_ASSERT_THROWS_NOTHING(
        in = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("tomultiply")));
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("multiplied")));

    testScaleFactorApplied(in, result, 2.5, true); // multiply=true

    AnalysisDataService::Instance().remove("tomultiply");
    AnalysisDataService::Instance().remove("multiplied");
  }

  void testAdd() {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    Mantid::Algorithms::Scale scale2;
    scale2.initialize();

    AnalysisDataService::Instance().add(
        "toadd", WorkspaceCreationHelper::create2DWorkspace123(10, 10));
    TS_ASSERT_THROWS_NOTHING(
        scale2.setPropertyValue("InputWorkspace", "toadd"));
    TS_ASSERT_THROWS_NOTHING(
        scale2.setPropertyValue("OutputWorkspace", "added"));
    TS_ASSERT_THROWS_NOTHING(scale2.setPropertyValue("Factor", "-100.0"));
    TS_ASSERT_THROWS_NOTHING(scale2.setPropertyValue("Operation", "Add"));

    TS_ASSERT_THROWS_NOTHING(scale2.execute());
    TS_ASSERT(scale2.isExecuted());

    MatrixWorkspace_const_sptr in, result;
    TS_ASSERT_THROWS_NOTHING(
        in = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("toadd")));
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("added")));

    testScaleFactorApplied(in, result, -100, false); // multiply=false

    AnalysisDataService::Instance().remove("toadd");
    AnalysisDataService::Instance().remove("added");
  }

  void test_multiply_with_dx_values() {
    bool outputWorkspaceIsInputWorkspace = false;
    doTestScaleWithDx("Multiply", outputWorkspaceIsInputWorkspace);
  }

  void test_multiply_with_dx_values_with_out_is_in() {
    bool outputWorkspaceIsInputWorkspace = true;
    doTestScaleWithDx("Multiply", outputWorkspaceIsInputWorkspace);
  }

  void test_add_with_dx_values() {
    bool outputWorkspaceIsInputWorkspace = false;
    doTestScaleWithDx("Add", outputWorkspaceIsInputWorkspace);
  }
  void test_add_with_dx_values_with_out_is_in() {
    bool outputWorkspaceIsInputWorkspace = true;
    doTestScaleWithDx("Add", outputWorkspaceIsInputWorkspace);
  }

private:
  void testScaleFactorApplied(
      const Mantid::API::MatrixWorkspace_const_sptr &inputWS,
      const Mantid::API::MatrixWorkspace_const_sptr &outputWS, double factor,
      bool multiply) {
    const size_t xsize = outputWS->blocksize();
    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      for (size_t j = 0; j < xsize; ++j) {
        TS_ASSERT_DELTA(outputWS->readX(i)[j], inputWS->readX(i)[j], 1e-12);
        double resultY = (multiply) ? factor * inputWS->readY(i)[j]
                                    : factor + inputWS->readY(i)[j];
        TS_ASSERT_DELTA(outputWS->readY(i)[j], resultY, 1e-12);
        double resultE =
            (multiply) ? factor * inputWS->readE(i)[j] : inputWS->readE(i)[j];
        TS_ASSERT_DELTA(outputWS->readE(i)[j], resultE, 1e-12);
      }
    }
  }

  void doTestScaleWithDx(std::string type, bool outIsIn = false) {
    // Arrange
    const double xValue = 1.222;
    const double value = 5;
    const double error = 1.5;
    const double xError = 1.1;
    int64_t nHist = 4;
    int64_t nBins = 10;
    bool isHist = true;
    std::string wsName = "input_scaling";
    Mantid::API::AnalysisDataService::Instance().add(
        wsName, WorkspaceCreationHelper::create2DWorkspaceWithValuesAndXerror(
                    nHist, nBins, isHist, xValue, value, error, xError));
    std::string outWorkspaceName;
    if (outIsIn) {
      outWorkspaceName = wsName;
    } else {
      outWorkspaceName = "dx_error_workspace_test";
    }

    // Act
    auto algScale =
        Mantid::API::FrameworkManager::Instance().createAlgorithm("Scale");
    algScale->initialize();
    algScale->setRethrows(true);
    algScale->setPropertyValue("InputWorkspace", wsName);
    algScale->setPropertyValue("OutputWorkspace", outWorkspaceName);
    algScale->setProperty("Operation", type);
    algScale->setProperty("Factor", "10");
    algScale->execute();

    // Assert
    TS_ASSERT(algScale->isExecuted());
    auto outWS =
        Mantid::API::AnalysisDataService::Instance()
            .retrieveWS<Mantid::API::MatrixWorkspace>(outWorkspaceName);
    TS_ASSERT(outWS.get());
    TSM_ASSERT("Output should contain x errors", outWS->hasDx(0));

    auto &dx = outWS->dx(0);
    double expectedDx = xError;
    for (size_t spectra = 0; spectra < outWS->getNumberHistograms();
         ++spectra) {
      for (size_t i = 0; i < static_cast<size_t>(nBins); ++i) {
        TSM_ASSERT_EQUALS("X Error should be 5", dx[i], expectedDx);
      }
    }
    // Clean the ADS
    if (!outIsIn) {
      Mantid::API::AnalysisDataService::Instance().remove(outWorkspaceName);
    }
    Mantid::API::AnalysisDataService::Instance().remove(wsName);
  }
  Mantid::Algorithms::Scale scale;
};

#endif /*SCALETEST_H_*/
