// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef GENERALISEDSECONDDIFFERENCETEST_H_
#define GENERALISEDSECONDDIFFERENCETEST_H_

#include <cxxtest/TestSuite.h>

#include "../../DataObjects/test/EventWorkspaceTest.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include <MantidAlgorithms/GeneralisedSecondDifference.h>
#include <MantidDataObjects/EventWorkspace.h>

using namespace Mantid::API;
using namespace Mantid::HistogramData;

class GeneralisedSecondDifferenceTest : public CxxTest::TestSuite {

public:
  void testInit() {

    IAlgorithm_sptr gsd = Mantid::API::AlgorithmManager::Instance().create(
        "GeneralisedSecondDifference", 1);

    TS_ASSERT_EQUALS(gsd->name(), "GeneralisedSecondDifference");
    TS_ASSERT_EQUALS(gsd->category(), "Arithmetic");
    TS_ASSERT_THROWS_NOTHING(gsd->initialize());
    TS_ASSERT(gsd->isInitialized());
  }

  void testExec() {

    IAlgorithm_sptr gsd = Mantid::API::AlgorithmManager::Instance().create(
        "GeneralisedSecondDifference", 1);

    auto x = Points{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto y = Counts{0.3, 0.3, 0.3, 0.47, 3.9, 10.3, 3.9, 0.47, 0.3, 0.3};

    MatrixWorkspace_sptr inputWs = WorkspaceFactory::Instance().create(
        "Workspace2D", 1, y.size(), y.size());
    inputWs->setHistogram(0, x, y);

    gsd->setProperty("InputWorkspace", inputWs);
    gsd->setProperty("M", "1");
    gsd->setProperty("Z", "2");
    gsd->setPropertyValue("OutputWorkspace", "secondDiff");

    gsd->execute();
    TS_ASSERT(gsd->isExecuted());

    MatrixWorkspace_sptr outWs = Mantid::API::AnalysisDataService::Instance()
                                     .retrieveWS<MatrixWorkspace>("secondDiff");
    TS_ASSERT(outWs);

    TS_ASSERT_EQUALS(outWs->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outWs->blocksize(), 4);

    const auto &x1 = outWs->x(0);
    TS_ASSERT_EQUALS(x1[0], 3);
    TS_ASSERT_EQUALS(x1[3], 6);

    const auto &y1 = outWs->y(0);
    TS_ASSERT_DELTA(y1[1], -7.0300, 0.0001);
    TS_ASSERT_DELTA(y1[2], -20.0000, 0.0001);

    Mantid::API::AnalysisDataService::Instance().remove("secondDiff");
  }
};

class GeneralisedSecondDifferenceTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GeneralisedSecondDifferenceTestPerformance *createSuite() {
    return new GeneralisedSecondDifferenceTestPerformance();
  }

  static void destroySuite(GeneralisedSecondDifferenceTestPerformance *suite) {
    delete suite;
  }

  void setUp() override {
    inputMatrix = WorkspaceCreationHelper::create2DWorkspaceBinned(10000, 1000);
    inputEvent =
        WorkspaceCreationHelper::createEventWorkspace(10000, 1000, 5000);
  }

  void tearDown() override {
    Mantid::API::AnalysisDataService::Instance().remove("output");
    Mantid::API::AnalysisDataService::Instance().remove("output2");
  }

  void testPerformanceMatrixWS() {
    Mantid::Algorithms::GeneralisedSecondDifference genSecDiff;
    genSecDiff.initialize();
    genSecDiff.setProperty("InputWorkspace", inputMatrix);
    genSecDiff.setPropertyValue("OutputWorkspace", "output");
    genSecDiff.execute();
  }

  void testPerformanceEventWS() {
    Mantid::Algorithms::GeneralisedSecondDifference genSecDiff;
    genSecDiff.initialize();
    genSecDiff.setProperty("InputWorkspace", inputEvent);
    genSecDiff.setPropertyValue("OutputWorkspace", "output2");
    genSecDiff.execute();
  }

private:
  Mantid::API::MatrixWorkspace_sptr inputMatrix;
  Mantid::DataObjects::EventWorkspace_sptr inputEvent;
};

#endif /* GENERALISEDSECONDDIFERENCETEST_H_ */
