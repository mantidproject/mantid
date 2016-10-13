#ifndef REBUNCHTEST_H_
#define REBUNCHTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAlgorithms/Rebunch.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidHistogramData/LinearGenerator.h"

#include <numeric>

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::Points;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::CountVariances;
using Mantid::HistogramData::LinearGenerator;

class RebunchTest : public CxxTest::TestSuite {
public:
  void testworkspace1D_pnt_flush() {
    Workspace2D_sptr test_in1D = Create1DWorkspacePnt(50);
    AnalysisDataService::Instance().add("test_in1D", test_in1D);

    Rebunch rebunch;
    rebunch.initialize();
    rebunch.setPropertyValue("InputWorkspace", "test_in1D");
    rebunch.setPropertyValue("OutputWorkspace", "test_out");
    rebunch.setPropertyValue("NBunch", "5");
    rebunch.execute();
    MatrixWorkspace_sptr rebunchdata =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_out");
    auto &outX = rebunchdata->x(0);
    auto &outY = rebunchdata->y(0);
    auto &outE = rebunchdata->e(0);

    TS_ASSERT_DELTA(outX[0], 1.5, 0.000001);
    TS_ASSERT_DELTA(outY[0], 3.0, 0.000001);
    TS_ASSERT_DELTA(outE[0], sqrt(15.0) / 5.0, 0.000001);
    TS_ASSERT_DELTA(outX[4], 11.5, 0.000001);
    TS_ASSERT_DELTA(outY[4], 23.0, 0.000001);
    TS_ASSERT_DELTA(outE[4], sqrt(115.0) / 5.0, 0.000001);
    TS_ASSERT_DELTA(outX[9], 24.0, 0.000001);
    TS_ASSERT_DELTA(outY[9], 48.0, 0.000001);
    TS_ASSERT_DELTA(outE[9], sqrt(240.0) / 5.0, 0.000001);

    AnalysisDataService::Instance().remove("test_in1D");
    AnalysisDataService::Instance().remove("test_out");
  }

  void testworkspace1D_nondist() {
    Workspace2D_sptr test_in1D = Create1DWorkspaceHist(50);
    AnalysisDataService::Instance().add("test_in1D", test_in1D);

    Rebunch rebunch;
    rebunch.initialize();
    rebunch.setPropertyValue("InputWorkspace", "test_in1D");
    rebunch.setPropertyValue("OutputWorkspace", "test_out");
    rebunch.setPropertyValue("NBunch", "7");
    rebunch.execute();
    MatrixWorkspace_sptr rebunchdata =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_out");

    auto &outX = rebunchdata->x(0);
    auto &outY = rebunchdata->y(0);
    auto &outE = rebunchdata->e(0);

    TS_ASSERT_DELTA(outX[0], 0.5, 0.000001);
    TS_ASSERT_DELTA(outY[0], 28, 0.000001);
    TS_ASSERT_DELTA(outE[0], sqrt(28.0), 0.000001);
    TS_ASSERT_DELTA(outX[4], 21.5, 0.000001);
    TS_ASSERT_DELTA(outY[4], 224.0, 0.000001);
    TS_ASSERT_DELTA(outE[4], sqrt(224.0), 0.000001);
    TS_ASSERT_DELTA(outX[6], 32, 0.000001);
    TS_ASSERT_DELTA(outY[6], 322.0, 0.000001);
    TS_ASSERT_DELTA(outE[6], sqrt(322.0), 0.000001);
    bool dist = rebunchdata->isDistribution();
    TS_ASSERT(!dist);

    AnalysisDataService::Instance().remove("test_in1D");
    AnalysisDataService::Instance().remove("test_out");
  }

  void testworkspace2D_dist() {
    Workspace2D_sptr test_in2D = Create2DWorkspaceHist(50, 20);
    test_in2D->setDistribution(true);
    AnalysisDataService::Instance().add("test_in2D", test_in2D);

    Rebunch rebunch;
    rebunch.initialize();
    rebunch.setPropertyValue("InputWorkspace", "test_in2D");
    rebunch.setPropertyValue("OutputWorkspace", "test_out");
    rebunch.setPropertyValue("NBunch", "5");
    rebunch.execute();
    MatrixWorkspace_sptr rebunchdata =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_out");

    auto &outX = rebunchdata->x(5);
    auto &outY = rebunchdata->y(5);
    auto &outE = rebunchdata->e(5);

    TS_ASSERT_DELTA(outX[0], 0.5, 0.000001);
    TS_ASSERT_DELTA(outY[0], 3, 0.000001);
    TS_ASSERT_DELTA(outE[0], sqrt(8.4375) / 3.75, 0.000001);
    TS_ASSERT_DELTA(outX[4], 15.5, 0.000001);
    TS_ASSERT_DELTA(outY[4], 23, 0.000001);
    TS_ASSERT_DELTA(outE[4], sqrt(64.6875) / 3.75, 0.000001);
    TS_ASSERT_DELTA(outX[9], 34.25, 0.000001);
    TS_ASSERT_DELTA(outY[9], 47.5, 0.000001);
    TS_ASSERT_DELTA(outE[9], sqrt(106.875) / 3.0, 0.000001);

    TS_ASSERT(rebunchdata->isDistribution());

    AnalysisDataService::Instance().remove("test_in2D");
    AnalysisDataService::Instance().remove("test_out");
  }

  void testworkspace2D_pnt_remainder() {
    Workspace2D_sptr test_in2D = Create2DWorkspacePnt(50, 20);
    AnalysisDataService::Instance().add("test_in2D", test_in2D);

    Rebunch rebunch;
    rebunch.initialize();
    rebunch.setPropertyValue("InputWorkspace", "test_in2D");
    rebunch.setPropertyValue("OutputWorkspace", "test_out");
    rebunch.setPropertyValue("NBunch", "7");
    rebunch.execute();
    MatrixWorkspace_sptr rebunchdata =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_out");
    auto &outX = rebunchdata->x(5);
    auto &outY = rebunchdata->y(5);
    auto &outE = rebunchdata->e(5);

    TS_ASSERT_DELTA(outX[0], 2.75, 0.000001);
    TS_ASSERT_DELTA(outY[0], 5.5, 0.000001);
    TS_ASSERT_DELTA(outE[0], sqrt(38.5) / 7.0, 0.000001);
    TS_ASSERT_DELTA(outX[2], 13.25, 0.000001);
    TS_ASSERT_DELTA(outY[2], 26.5, 0.000001);
    TS_ASSERT_DELTA(outE[2], sqrt(185.5) / 7.0, 0.000001);
    TS_ASSERT_DELTA(outX[7], 37.25, 0.000001);
    TS_ASSERT_DELTA(outY[7], 74.5, 0.000001);
    TS_ASSERT_DELTA(outE[7], sqrt(74.5), 0.000001);

    AnalysisDataService::Instance().remove("test_in2D");
    AnalysisDataService::Instance().remove("test_out");
  }

private:
  Workspace2D_sptr Create1DWorkspaceHist(int size) {
    Workspace2D_sptr retVal(new Workspace2D);
    retVal->initialize(1, size, size - 1);
    BinEdges x(static_cast<size_t>(size), LinearGenerator(0.5, 0.75));
    Counts y(size - 1, LinearGenerator(1.0, 1.0));

    retVal->setHistogram(0, x, y);

    return retVal;
  }

  Workspace2D_sptr Create1DWorkspacePnt(int size) {
    Workspace2D_sptr retVal(new Workspace2D);
    retVal->initialize(1, size, size);
    Points x(size, LinearGenerator(0.5, 0.5));
    Counts y(size, LinearGenerator(1.0, 1.0));

    retVal->setHistogram(0, x, y);

    return retVal;
  }

  Workspace2D_sptr Create2DWorkspaceHist(int xlen, int ylen) {
    BinEdges x1(xlen, LinearGenerator(0.5, 0.75));

    Workspace2D_sptr retVal(new Workspace2D);
    retVal->initialize(ylen, xlen, xlen - 1);

    Counts y1(xlen - 1, LinearGenerator(1.0, 1.0));

    for (int i = 0; i < ylen; i++) {
      retVal->setHistogram(i, x1, y1);
    }

    return retVal;
  }

  Workspace2D_sptr Create2DWorkspacePnt(int xlen, int ylen) {
    Points x1(xlen, LinearGenerator(0.5, 0.75));

    Workspace2D_sptr retVal(new Workspace2D);
    retVal->initialize(ylen, xlen, xlen);

    Counts y1(xlen, LinearGenerator(1.0, 1.5));

    for (int i = 0; i < ylen; i++) {
      retVal->setHistogram(i, x1, y1);
    }

    return retVal;
  }
};

class RebunchTestPerformance : public CxxTest::TestSuite {
public:
  static RebunchTestPerformance *createSuite() {
    return new RebunchTestPerformance();
  }

  static void destroySuite(RebunchTestPerformance *suite) { delete suite; }

  void setUp() override {
    input = boost::make_shared<Workspace2D>();
    input->initialize(100000, 3000, 2999);
    input->setDistribution(true);
    AnalysisDataService::Instance().add("input", input);
  }

  void tearDown() override {
    AnalysisDataService::Instance().remove("input");
    AnalysisDataService::Instance().remove("test_out");
  }

  void testExec() {
    Rebunch rebunch;
    rebunch.initialize();
    rebunch.setPropertyValue("InputWorkspace", "input");
    rebunch.setPropertyValue("OutputWorkspace", "test_out");
    rebunch.setPropertyValue("NBunch", "5");
    rebunch.execute();
  }

private:
  Workspace2D_sptr input;
};
#endif /* REBUNCHTEST */
