#ifndef MANTID_ALGORITHM_LOGTEST_H_
#define MANTID_ALGORITHM_LOGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAlgorithms/Logarithm.h"

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Algorithms;

class LogarithmTest : public CxxTest::TestSuite {
public:
  void testInit(void) {
    Mantid::Algorithms::Logarithm alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void testExec1D(void) {
    int sizex = 10;

    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 =
        WorkspaceCreationHelper::create1DWorkspaceFib(sizex, true);
    AnalysisDataService::Instance().add("test_inLn", work_in1);

    Logarithm alg;

    alg.initialize();

    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", "test_inLn");
        alg.setPropertyValue("OutputWorkspace", "test_inLn");
        alg.setPropertyValue("Filler", "10");
        alg.setPropertyValue("Natural", "0"););

    alg.execute();

    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(
        work_out1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "test_inLn"));

    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().remove("test_outLn");
        AnalysisDataService::Instance().remove("test_inLn"););
  }

  void testExec2D(void) {
    int nHist = 10, nBins = 20;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in2 =
        WorkspaceCreationHelper::create2DWorkspace154(nHist, nBins);
    Workspace2D_sptr work_ou2 =
        WorkspaceCreationHelper::create2DWorkspace(nHist, nBins);

    Logarithm alg;
    AnalysisDataService::Instance().add("test_inLn2", work_in2);
    AnalysisDataService::Instance().add("test_outLn2", work_ou2);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", "test_inLn2");
        alg.setPropertyValue("OutputWorkspace", "test_outLn2");
        alg.setPropertyValue("Natural", "1");

    );

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr work_out2;
    TS_ASSERT_THROWS_NOTHING(
        work_out2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "test_outLn2"));

    //    checkData(work_in1, work_in2, work_out1);
    AnalysisDataService::Instance().remove("test_inLn2");
    AnalysisDataService::Instance().remove("test_outLn2");
  }

  void testEvents() {
    // evin has 0 events per bin in pixel0, 1 in pixel 1, 2 in pixel2, ...
    EventWorkspace_sptr evin = WorkspaceCreationHelper::createEventWorkspace(
                            5, 3, 1000, 0, 1, 4),
                        evout;
    AnalysisDataService::Instance().add("test_ev_log", evin);

    Logarithm alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", "test_ev_log"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "test_ev_log_out"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filler", "123"));
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(
        evout = boost::dynamic_pointer_cast<EventWorkspace>(
            AnalysisDataService::Instance().retrieve("test_ev_log_out")));

    TS_ASSERT(!evout); // should not be an event workspace

    MatrixWorkspace_sptr histo_out;
    TS_ASSERT_THROWS_NOTHING(
        histo_out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "test_ev_log_out"));
    TS_ASSERT(histo_out); // this should be a 2d workspace

    TS_ASSERT_DELTA(histo_out->y(0)[0], 123, 1e-10);
    for (size_t i = 1; i < 5; ++i) {
      TS_ASSERT_DELTA(histo_out->y(i)[0], std::log(static_cast<double>(i)),
                      1e-10);
    }
    AnalysisDataService::Instance().remove("test_ev_log");
    AnalysisDataService::Instance().remove("test_ev_log_out");
  }
};

#endif /* MANTID_ALGORITHM_LOGTEST_H_ */
