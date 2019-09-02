// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SMOOTHDATATEST_H_
#define SMOOTHDATATEST_H_

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/CreateGroupingWorkspace.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/SmoothData.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class SmoothDataTest : public CxxTest::TestSuite {
public:
  static SmoothDataTest *createSuite() { return new SmoothDataTest(); }
  static void destroySuite(SmoothDataTest *suite) { delete suite; }

  SmoothDataTest() {
    FrameworkManager::Instance();
    // Set up a small workspace for testing
    MatrixWorkspace_sptr space =
        WorkspaceFactory::Instance().create("Workspace2D", 2, 10, 10);

    auto &yVals = space->mutableY(0);
    auto &eVals = space->mutableE(0);

    for (int i = 0; i < 10; ++i) {
      yVals[i] = i + 1.0;
      eVals[i] = sqrt(i + 1.0);
    }

    // Register the workspace in the data service
    AnalysisDataService::Instance().add("noisy", space);
  }

  void testName() {
    SmoothData smooth;
    TS_ASSERT_EQUALS(smooth.name(), "SmoothData");
  }

  void testVersion() {
    SmoothData smooth;
    TS_ASSERT_EQUALS(smooth.version(), 1);
  }

  void testInit() {
    SmoothData smooth;
    TS_ASSERT_THROWS_NOTHING(smooth.initialize());
    TS_ASSERT(smooth.isInitialized());
  }

  void testInvalidInputs() {
    SmoothData smooth2;
    TS_ASSERT_THROWS_NOTHING(smooth2.initialize());
    TS_ASSERT_THROWS(smooth2.execute(), const std::runtime_error &);
    // Can't set Npoints to value less than 3
    TS_ASSERT_THROWS(smooth2.setPropertyValue("NPoints", "1"),
                     const std::invalid_argument &);

    TS_ASSERT_THROWS_NOTHING(
        smooth2.setPropertyValue("InputWorkspace", "noisy"));
    TS_ASSERT_THROWS_NOTHING(
        smooth2.setPropertyValue("OutputWorkspace", "something"));
    // Will also fail if NPoints is larger than spectrum length
    TS_ASSERT_THROWS_NOTHING(smooth2.setPropertyValue("NPoints", "11"));
    TS_ASSERT_THROWS_NOTHING(smooth2.execute());
    TS_ASSERT(!smooth2.isExecuted());
  }

  void testGroupspaceProperty() {

    MatrixWorkspace_sptr createSampleWorkspace;
    // Creating sample workspace
    CreateSampleWorkspace createWS;
    createWS.initialize();
    createWS.setChild(true);
    // setting property
    std::string outWSName("CreateWorkspaceTest_OutputWS");
    TS_ASSERT_THROWS_NOTHING(
        createWS.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(createWS.execute(););
    MatrixWorkspace_sptr outWS = createWS.getProperty("OutputWorkspace");

    // Creating group workspace
    CreateGroupingWorkspace alg;
    alg.initialize();
    alg.setChild(true);
    std::string outGWSName("CreateGroupingWorkspaceTest_OutputWS");
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", outWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("GroupNames", "bank1,bank2"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outGWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    GroupingWorkspace_sptr groupingWS = alg.getProperty("OutputWorkspace");

    SmoothData smooth;
    smooth.initialize();
    smooth.setChild(true);
    TS_ASSERT_THROWS_NOTHING(smooth.setProperty("InputWorkspace", outWS));
    std::string outputWS("smoothed");
    TS_ASSERT_THROWS_NOTHING(
        smooth.setPropertyValue("OutputWorkspace", outputWS));
    // Set to 4 - algorithm should change it to 5
    TS_ASSERT_THROWS_NOTHING(smooth.setPropertyValue("NPoints", "3,5"));
    TS_ASSERT_THROWS_NOTHING(
        smooth.setProperty("GroupingWorkspace", groupingWS));
    TS_ASSERT_THROWS_NOTHING(smooth.execute());
    TS_ASSERT(smooth.isExecuted());

    MatrixWorkspace_const_sptr output = smooth.getProperty("OutputWorkspace");
    // as alg child is set true, wouldnt need to use AnalysisDataService
    const auto &Y = output->y(0);
    const auto &X = output->x(0);
    const auto &E = output->e(0);
    TS_ASSERT_EQUALS(Y[0], 0.3);
    TS_ASSERT_EQUALS(X[6], 1200);
    TS_ASSERT_DIFFERS(E[2], Y[2]);
    for (int i = 0; i < 4; ++i) {
      TS_ASSERT_EQUALS(Y[i], 0.3);
      TS_ASSERT_EQUALS(X[i + 1], (i + 1) * 200);
    }
    TS_ASSERT_LESS_THAN(E[8], 0.31625);
    TS_ASSERT_EQUALS(Y[10], Y[15]);
    TS_ASSERT_DIFFERS(Y[50], Y[45]);
    TS_ASSERT_EQUALS(X[50], 10000);
    TS_ASSERT_DIFFERS(Y[51], 3.63333333333333);
    TS_ASSERT_LESS_THAN_EQUALS(E[15], 0.3169);
    TS_ASSERT_EQUALS(X[16], 3200);
    TS_ASSERT_LESS_THAN_EQUALS(Y[49], 3.64);
    TS_ASSERT_DIFFERS(X[2], E[2]);
    TS_ASSERT_DIFFERS(X[0], Y[0]);
    TS_ASSERT_DIFFERS(E[5], Y[5]);
    // Check X vectors are shared
    TS_ASSERT_EQUALS(&(output->x(0)), &(output->x(1)));
  }

  void testExec() {
    SmoothData smooth;
    smooth.initialize();

    TS_ASSERT_THROWS_NOTHING(
        smooth.setPropertyValue("InputWorkspace", "noisy"));
    std::string outputWS("smoothed");
    TS_ASSERT_THROWS_NOTHING(
        smooth.setPropertyValue("OutputWorkspace", outputWS));
    // Set to 4 - algorithm should change it to 5
    TS_ASSERT_THROWS_NOTHING(smooth.setPropertyValue("NPoints", "4"));

    TS_ASSERT_THROWS_NOTHING(smooth.execute());
    TS_ASSERT(smooth.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputWS));
    const auto &Y = output->y(0);
    const auto &E = output->e(0);
    TS_ASSERT_EQUALS(Y[0], 2);
    TS_ASSERT_DELTA(E[0], sqrt(Y[0] / 3.0), 0.0001);
    TS_ASSERT_EQUALS(Y[1], 2.5);
    TS_ASSERT_DELTA(E[1], sqrt(Y[1] / 4.0), 0.0001);
    for (size_t i = 2; i < Y.size() - 2; ++i) {
      TS_ASSERT_EQUALS(Y[i], static_cast<double>(i) + 1.0);
      TS_ASSERT_DELTA(E[i], sqrt(Y[i] / 5.0), 0.0001);
    }
    TS_ASSERT_EQUALS(Y[8], 8.5);
    TS_ASSERT_DELTA(E[8], sqrt(Y[8] / 4.0), 0.0001);
    TS_ASSERT_EQUALS(Y[9], 9);
    TS_ASSERT_DELTA(E[9], sqrt(Y[9] / 3.0), 0.0001);

    // Check X vectors are shared
    TS_ASSERT_EQUALS(&(output->x(0)), &(output->x(1)));

    AnalysisDataService::Instance().remove(outputWS);
  }
};

class SmoothDataTestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {

    // Set up a small workspace for testing
    constexpr size_t numHistograms(1);
    constexpr size_t numBins(1000000);

    inputWs =
        WorkspaceCreationHelper::create2DWorkspace(numHistograms, numBins);

    auto &yVals = inputWs->mutableY(0);
    auto &eVals = inputWs->mutableE(0);

    double currentVal(0.0);
    for (size_t i = 0; i < numBins; ++i) {
      yVals[i] = currentVal + 1.0;
      eVals[i] = sqrt(currentVal + 1.0);
      currentVal++;
    }

    smoothAlg.initialize();
    smoothAlg.setProperty("InputWorkspace", inputWs);
    smoothAlg.setPropertyValue("OutputWorkspace", "outputWS");
    smoothAlg.setRethrows(true);
  }

  void testSmoothDataPerformance() {
    TS_ASSERT_THROWS_NOTHING(smoothAlg.execute());
  }

  void tearDown() override {
    AnalysisDataService::Instance().remove("outputWS");
  }

private:
  Workspace2D_sptr inputWs;
  SmoothData smoothAlg;
};

#endif /*SMOOTHDATATEST_H_*/
