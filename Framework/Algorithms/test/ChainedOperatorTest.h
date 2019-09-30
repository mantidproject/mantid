// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CHAINEDOPERATORTEST_H_
#define CHAINEDOPERATORTEST_H_

#include <cmath>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAlgorithms/Minus.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class ComplexOpTest : public Algorithm {
public:
  ComplexOpTest() : Algorithm() {}
  ~ComplexOpTest() override {}
  void init() override {
    declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
        "InputWorkspace_1", "", Direction::Input));
    declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
        "InputWorkspace_2", "", Direction::Input));
    declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
        "OutputWorkspace", "", Direction::Output));
  }
  void exec() override {
    MatrixWorkspace_sptr in_work1 = getProperty("InputWorkspace_1");
    MatrixWorkspace_sptr in_work2 = getProperty("InputWorkspace_2");

    MatrixWorkspace_sptr out_work = (in_work1 + in_work2) / 3 + 5;
    setProperty("OutputWorkspace", out_work);
  }
  const std::string name() const override { return "ComplexOpTest"; }
  int version() const override { return (1); }
  const std::string summary() const override { return "Summary of this test."; }
};

class ChainedOperatorTest : public CxxTest::TestSuite {
public:
  void testChainedOperator() {
    int nHist = 10, nBins = 20;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 =
        WorkspaceCreationHelper::create2DWorkspace123(nHist, nBins);
    MatrixWorkspace_sptr work_in2 =
        WorkspaceCreationHelper::create2DWorkspace154(nHist, nBins);

    performTest(work_in1, work_in2);
  }

  void testChainedOperatorEventWS() {
    int nHist = 10, nBins = 20;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 =
        WorkspaceCreationHelper::createEventWorkspace(nHist, nBins);
    MatrixWorkspace_sptr work_in2 =
        WorkspaceCreationHelper::createEventWorkspace(nHist, nBins);

    performTest(work_in1, work_in2);
  }

  void performTest(MatrixWorkspace_sptr work_in1,
                   MatrixWorkspace_sptr work_in2) {
    ComplexOpTest alg;

    std::string wsNameIn1 = "testChainedOperator_in21";
    std::string wsNameIn2 = "testChainedOperator_in22";
    std::string wsNameOut = "testChainedOperator_out";
    AnalysisDataService::Instance().add(wsNameIn1, work_in1);
    AnalysisDataService::Instance().add(wsNameIn2, work_in2);
    alg.initialize();
    alg.setPropertyValue("InputWorkspace_1", wsNameIn1);
    alg.setPropertyValue("InputWorkspace_2", wsNameIn2);
    alg.setPropertyValue("OutputWorkspace", wsNameOut);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(
        work_out1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            wsNameOut));

    checkData(work_in1, work_in2, work_out1);

    AnalysisDataService::Instance().remove(wsNameIn1);
    AnalysisDataService::Instance().remove(wsNameIn2);
    AnalysisDataService::Instance().remove(wsNameOut);
  }

private:
  void checkData(const MatrixWorkspace_sptr work_in1,
                 const MatrixWorkspace_sptr work_in2,
                 const MatrixWorkspace_sptr work_out1) {
    size_t ws2LoopCount = 0;
    if (work_in2->size() > 0) {
      ws2LoopCount = work_in1->size() / work_in2->size();
    }
    ws2LoopCount = (ws2LoopCount == 0) ? 1 : ws2LoopCount;

    for (size_t i = 0; i < work_out1->size(); i++) {
      checkDataItem(work_in1, work_in2, work_out1, i, i / ws2LoopCount);
    }
  }

  void checkDataItem(const MatrixWorkspace_sptr work_in1,
                     const MatrixWorkspace_sptr work_in2,
                     const MatrixWorkspace_sptr work_out1, size_t i,
                     size_t ws2Index) {
    double sig1 =
        work_in1->readY(i / work_in1->blocksize())[i % work_in1->blocksize()];
    double sig2 = work_in2->readY(
        ws2Index / work_in1->blocksize())[ws2Index % work_in1->blocksize()];
    double sig3 =
        work_out1->readY(i / work_in1->blocksize())[i % work_in1->blocksize()];
    TS_ASSERT_DELTA((sig1 + sig2) / 3 + 5, sig3, 0.0001);
    // Note err calculation not checked due to complexity.
  }
};

#endif /*CHAINEDOPERATORTEST_H_*/
