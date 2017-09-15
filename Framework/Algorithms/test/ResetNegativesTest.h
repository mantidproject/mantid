#ifndef MANTID_ALGORITHMS_RESETNEGATIVESTEST_H_
#define MANTID_ALGORITHMS_RESETNEGATIVESTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <string>

#include "MantidAlgorithms/ResetNegatives.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using std::size_t;

/// Name of the input workspace.
static const std::string INPUT_WS_NAME("ResetNegativesTest_InputWS");

/// Name of the output workspace.
static const std::string OUTPUT_WS_NAME("ResetNegativesTest_OutputWS");

class ResetNegativesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ResetNegativesTest *createSuite() { return new ResetNegativesTest(); }
  static void destroySuite(ResetNegativesTest *suite) { delete suite; }

  void test_Init() {
    ResetNegatives alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_donothing() {
    // if all the values are positive it should just copy input to output
    MatrixWorkspace_sptr inputWS = this->generateInput(1., 1.);

    ResetNegatives alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", INPUT_WS_NAME));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", OUTPUT_WS_NAME));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // get the output workspace
    MatrixWorkspace_sptr outputWS;
    TS_ASSERT_THROWS_NOTHING(
        outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            OUTPUT_WS_NAME));
    TS_ASSERT(outputWS);
    if (!outputWS)
      return;

    // verify the results
    const auto &yIn = inputWS->y(0);
    const auto &yOut = outputWS->y(0);
    for (size_t i = 0; i < yIn.size(); i++)
      TS_ASSERT_DELTA(yOut[i], yIn[i], .000001);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(INPUT_WS_NAME);
    AnalysisDataService::Instance().remove(OUTPUT_WS_NAME);
  }

  void test_addminimum() {
    // if all the values are positive it should just copy input to output
    MatrixWorkspace_sptr inputWS = this->generateInput(-1.);

    ResetNegatives alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", INPUT_WS_NAME));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", OUTPUT_WS_NAME));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AddMinimum", true));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // get the output workspace
    MatrixWorkspace_sptr outputWS;
    TS_ASSERT_THROWS_NOTHING(
        outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            OUTPUT_WS_NAME));
    TS_ASSERT(outputWS);
    if (!outputWS)
      return;

    // verify the results
    const auto &yOut = outputWS->y(0);
    for (size_t i = 0; i < yOut.size(); i++)
      TS_ASSERT_DELTA(yOut[i], 0., .000001);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(INPUT_WS_NAME);
    AnalysisDataService::Instance().remove(OUTPUT_WS_NAME);
  }

  void test_resetvalue() {
    // if all the values are positive it should just copy input to output
    MatrixWorkspace_sptr inputWS = this->generateInput(-5., .5);

    double resetValue = 10.;

    ResetNegatives alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", INPUT_WS_NAME));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", OUTPUT_WS_NAME));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AddMinimum", false));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ResetValue", resetValue));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // get the output workspace
    MatrixWorkspace_sptr outputWS;
    TS_ASSERT_THROWS_NOTHING(
        outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            OUTPUT_WS_NAME));
    TS_ASSERT(outputWS);
    if (!outputWS)
      return;

    // verify the results
    const auto &yOut = outputWS->y(0);
    for (size_t i = 0; i < yOut.size(); i++)
      TS_ASSERT(yOut[i] >= 0.);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(INPUT_WS_NAME);
    AnalysisDataService::Instance().remove(OUTPUT_WS_NAME);
  }

private:
  MatrixWorkspace_sptr generateInput(const double offset,
                                     const double delta = 0.) {
    constexpr int nhist = 3;
    constexpr int nbins = 256;
    MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(nhist, nbins, 1., .2);
    for (int i = 0; i < nhist; i++) {
      double value = offset + static_cast<double>(i);
      auto &y = inputWS->mutableY(i);
      for (size_t j = 0; j < y.size(); j++) {
        y[j] = value + delta * static_cast<double>(j);
      }
    }
    AnalysisDataService::Instance().add(INPUT_WS_NAME, inputWS);
    return inputWS;
  }
};

class ResetNegativesTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ResetNegativesTestPerformance *createSuite() {
    return new ResetNegativesTestPerformance();
  }
  static void destroySuite(ResetNegativesTestPerformance *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  void setUp() override { input = generateInput(-1.0, 0.01); }

  void testPerformance() {
    ResetNegatives alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", input);
    alg.setPropertyValue("OutputWorkspace", "output");
    alg.setProperty("AddMinimum", true);
    alg.execute();
  }

private:
  MatrixWorkspace_sptr input;
  MatrixWorkspace_sptr generateInput(const double offset,
                                     const double delta = 0.) {
    constexpr int nhist = 50000;
    constexpr int nbins = 1000;
    MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(nhist, nbins, 1., .2);
    for (int i = 0; i < nhist; i++) {
      double value = offset + static_cast<double>(i);
      auto &y = inputWS->mutableY(i);
      for (size_t j = 0; j < y.size(); j++) {
        y[j] = value + delta * static_cast<double>(j);
      }
    }
    return inputWS;
  }
};

#endif /* MANTID_ALGORITHMS_RESETNEGATIVESTEST_H_ */
