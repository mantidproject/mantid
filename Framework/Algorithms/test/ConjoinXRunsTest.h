#ifndef MANTID_ALGORITHMS_CONJOINXRUNSTEST_H_
#define MANTID_ALGORITHMS_CONJOINXRUNSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/AddSampleLog.h"
#include "MantidAlgorithms/AddTimeSeriesLog.h"
#include "MantidAlgorithms/ConjoinXRuns.h"
#include "MantidHistogramData/Counts.h"
#include "MantidHistogramData/HistogramDx.h"
#include "MantidHistogramData/HistogramE.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidHistogramData/Points.h"
#include "MantidKernel/Unit.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::ConjoinXRuns;
using Mantid::Algorithms::AddSampleLog;
using Mantid::Algorithms::AddTimeSeriesLog;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::HistogramDx;
using Mantid::HistogramData::HistogramE;
using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::HistogramY;
using Mantid::HistogramData::Points;
using namespace Mantid::API;
using namespace WorkspaceCreationHelper;

class ConjoinXRunsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConjoinXRunsTest *createSuite() { return new ConjoinXRunsTest(); }
  static void destroySuite(ConjoinXRunsTest *suite) { delete suite; }

  void setUp() override {
    std::vector<MatrixWorkspace_sptr> ws(6);
    // Workspaces have 5 spectra must be point data, don't have masks and have
    // dx
    ws[0] = create2DWorkspace123(5, 3, false, std::set<int64_t>(),
                                 true); // 3 points
    ws[1] = create2DWorkspace154(5, 2, false, std::set<int64_t>(),
                                 true); // 2 points
    ws[2] =
        create2DWorkspace123(5, 1, false, std::set<int64_t>(), true); // 1 point
    ws[3] =
        create2DWorkspace154(5, 1, false, std::set<int64_t>(), true); // 1 point
    ws[4] = create2DWorkspace123(5, 3, false, std::set<int64_t>(),
                                 true); // 3 points
    ws[5] = create2DWorkspace123(5, 3, false, std::set<int64_t>(),
                                 true); // 3 points
    m_testWS = {"ws1", "ws2", "ws3", "ws4", "ws5", "ws6"};

    for (unsigned int i; i < ws.size(); ++i) {
      ws[i]->getAxis(0)->setUnit("TOF");
      storeWS(m_testWS[i], ws[i]);
    }
  }

  void tearDown() override {
    for (unsigned int i; i < m_testWS.size(); ++i) {
      removeWS(m_testWS[i]);
    }
    m_testWS.clear();
  }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(m_testee.initialize());
    TS_ASSERT(m_testee.isInitialized());
  }

  void testHappyCase() {
    m_testee.setProperty("InputWorkspaces",
                         std::vector<std::string>{"ws1", "ws2", "ws3", "ws4"});
    m_testee.setProperty("OutputWorkspace", "out");
    TS_ASSERT_THROWS_NOTHING(m_testee.execute());
    TS_ASSERT(m_testee.isExecuted());

    MatrixWorkspace_sptr out =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out");

    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->getNumberHistograms(), 5);
    TS_ASSERT_EQUALS(out->blocksize(), 7);
    TS_ASSERT(!out->isHistogramData());
    TS_ASSERT_EQUALS(out->getAxis(0)->unit()->unitID(), "TOF");
    auto spectrum = out->y(0);
    auto error = out->e(0);
    auto xaxis = out->x(0);
    HistogramDx dx = out->dx(0);
    std::vector<double> x{1., 2., 3., 1., 2., 1., 1.};
    std::vector<double> y{2., 2., 2., 5., 5., 2., 5.};
    std::vector<double> e{3., 3., 3., 4., 4., 3., 4.};
    for (size_t j = 0; j < 7; ++j) {
      TS_ASSERT_EQUALS(xaxis[j], x[j]);
      TS_ASSERT_EQUALS(spectrum[j], y[j]);
      TS_ASSERT_EQUALS(error[j], e[j]);
      TSM_ASSERT_EQUALS("Dx and y values are the same", dx[j], y[j]);
    }
  }

  void testWSWithoutDxValues() {
    // Workspaces have 5 spectra must be point data
    MatrixWorkspace_sptr ws0 = create2DWorkspace123(5, 3); // 3 points
    MatrixWorkspace_sptr ws1 = create2DWorkspace154(5, 2); // 2 points
    ws0->getAxis(0)->setUnit("TOF");
    ws1->getAxis(0)->setUnit("TOF");
    storeWS("ws_0", ws0);
    storeWS("ws_1", ws1);
    m_testee.setProperty("InputWorkspaces",
                         std::vector<std::string>{"ws_0", "ws_1"});
    m_testee.setProperty("OutputWorkspace", "out");
    TS_ASSERT_THROWS_NOTHING(m_testee.execute());
    TS_ASSERT(m_testee.isExecuted());

    MatrixWorkspace_sptr out =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out");

    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->getNumberHistograms(), 5);
    TS_ASSERT_EQUALS(out->blocksize(), 5);
    TS_ASSERT(!out->isHistogramData());
    TS_ASSERT_EQUALS(out->getAxis(0)->unit()->unitID(), "TOF");
    auto spectrum = out->y(0);
    auto error = out->e(0);
    auto xaxis = out->x(0);
    std::vector<double> x{1., 2., 3., 1., 2.};
    std::vector<double> y{2., 2., 2., 5., 5.};
    std::vector<double> e{3., 3., 3., 4., 4.};
    for (size_t j = 0; j < 5; ++j) {
      TS_ASSERT_EQUALS(xaxis[j], x[j]);
      TS_ASSERT_EQUALS(spectrum[j], y[j]);
      TS_ASSERT_EQUALS(error[j], e[j]);
    }
  }

  void testFailDifferentNumberBins() {
    MatrixWorkspace_sptr ws5 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("ws5");

    Counts counts{{5, 8}};
    Points points{{0.4, 0.9}};
    ws5->setHistogram(3, points, counts);

    m_testee.setProperty("InputWorkspaces",
                         std::vector<std::string>{"ws1", "ws5"});
    TS_ASSERT_THROWS(m_testee.execute(), std::runtime_error);
  }

  void testPassDifferentAxes() {
    MatrixWorkspace_sptr ws6 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("ws6");

    for (auto i = 0; i < 5;
         i++) { // modify all 5 spectra of ws6 in terms of y and x
      ws6->mutableY(i) = {4., 9., 16.};
      ws6->mutableX(i) = {0.4, 0.9, 1.1};
    }

    m_testee.setProperty("InputWorkspaces",
                         std::vector<std::string>{"ws1", "ws6"});

    TS_ASSERT_THROWS_NOTHING(m_testee.execute());
    TS_ASSERT(m_testee.isExecuted());

    MatrixWorkspace_sptr out =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out");

    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->getNumberHistograms(), 5);
    TS_ASSERT_EQUALS(out->blocksize(), 6);
    TS_ASSERT(!out->isHistogramData());
    TS_ASSERT_EQUALS(out->getAxis(0)->unit()->unitID(), "TOF");

    // Check all 5 spectra
    for (auto i = 0; i < 5; i++) {
      TS_ASSERT_EQUALS(out->y(i)[0], 2.);
      TS_ASSERT_EQUALS(out->y(i)[1], 2.);
      TS_ASSERT_EQUALS(out->y(i)[2], 2.);
      TS_ASSERT_EQUALS(out->y(i)[3], 4.); // or ws6->y(i)[3]
      TS_ASSERT_EQUALS(out->y(i)[4], 9.);
      TS_ASSERT_EQUALS(out->y(i)[5], 16.);

      TS_ASSERT_EQUALS(out->e(i)[0], 3.);
      TS_ASSERT_EQUALS(out->e(i)[1], 3.);
      TS_ASSERT_EQUALS(out->e(i)[2], 3.);
      TS_ASSERT_EQUALS(out->e(i)[3], 3.);
      TS_ASSERT_EQUALS(out->e(i)[4], 3.);
      TS_ASSERT_EQUALS(out->e(i)[5], 3.);

      TS_ASSERT_EQUALS(out->x(i)[0], 1.);
      TS_ASSERT_EQUALS(out->x(i)[1], 2.);
      TS_ASSERT_EQUALS(out->x(i)[2], 3.);
      TS_ASSERT_EQUALS(out->x(i)[3], 0.4);
      TS_ASSERT_EQUALS(out->x(i)[4], 0.9);
      TS_ASSERT_EQUALS(out->x(i)[5], 1.1);
    }
  }

  void testFailWithNumLog() {
    AddSampleLog logAdder;
    logAdder.initialize();
    logAdder.setProperty("LogName", "TestLog");
    logAdder.setProperty("LogType", "Number");

    logAdder.setProperty("Workspace", "ws1");
    logAdder.setProperty("LogText", "0.7");
    logAdder.execute();

    m_testee.setProperty("InputWorkspaces",
                         std::vector<std::string>{"ws1", "ws2", "ws3", "ws4"});

    m_testee.setProperty("SampleLogAsXAxis", "TestNumLog");

    // blocksize must be one in case of scalar log, fail
    TS_ASSERT_THROWS(m_testee.execute(), std::runtime_error);
  }

  void testPassWithNumLog() {
    AddSampleLog logAdder;
    logAdder.initialize();
    logAdder.setProperty("LogName", "TestNumLog");
    logAdder.setProperty("LogType", "Number");
    logAdder.setProperty("LogUnit", "Energy");

    logAdder.setProperty("Workspace", "ws3");
    logAdder.setProperty("LogText", "0.7");
    logAdder.execute();

    logAdder.setProperty("Workspace", "ws4");
    logAdder.setProperty("LogText", "1.1");
    logAdder.execute();

    m_testee.setProperty("InputWorkspaces",
                         std::vector<std::string>{"ws3", "ws4"});

    m_testee.setProperty("SampleLogAsXAxis", "TestNumLog");

    TS_ASSERT_THROWS_NOTHING(m_testee.execute());

    MatrixWorkspace_sptr out =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out");
    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->blocksize(), 2);
    TS_ASSERT_EQUALS(out->getNumberHistograms(), 5);
    TS_ASSERT_EQUALS(out->getAxis(0)->unit()->unitID(), "Energy");

    auto xaxis = out->x(0);
    auto spectrum = out->y(0);
    auto error = out->e(0);

    TS_ASSERT_EQUALS(xaxis[0], 0.7);
    TS_ASSERT_EQUALS(xaxis[1], 1.1);
    TS_ASSERT_EQUALS(spectrum[0], 2.);
    TS_ASSERT_EQUALS(spectrum[1], 5.);
    TS_ASSERT_EQUALS(error[0], 3.);
    TS_ASSERT_EQUALS(error[1], 4.);
  }

  void testFailWithStringLog() {
    AddSampleLog logAdder;
    logAdder.initialize();
    logAdder.setProperty("LogName", "TestStrLog");
    logAdder.setProperty("LogType", "String");

    logAdder.setProperty("Workspace", "ws1");
    logAdder.setProperty("LogText", "str");
    logAdder.execute();

    m_testee.setProperty("InputWorkspaces",
                         std::vector<std::string>{"ws1", "ws2", "ws3", "ws4"});
    m_testee.setProperty("SampleLogAsXAxis", "TestStrLog");

    // string log not supported, fail
    TS_ASSERT_THROWS(m_testee.execute(), std::runtime_error);
  }

  void testPassWithNumSeriesLog() {

    AddTimeSeriesLog timeLogAdder;
    timeLogAdder.initialize();
    timeLogAdder.setProperty("Workspace", "ws1");
    timeLogAdder.setProperty("Name", "TestTimeLog");

    timeLogAdder.setProperty("Time", "2010-09-14T04:20:12");
    timeLogAdder.setProperty("Value", 5.7);
    timeLogAdder.execute();

    timeLogAdder.setProperty("Time", "2010-09-14T04:21:12");
    timeLogAdder.setProperty("Value", 6.1);
    timeLogAdder.execute();

    timeLogAdder.setProperty("Time", "2010-09-14T04:22:12");
    timeLogAdder.setProperty("Value", 6.7);
    timeLogAdder.execute();

    timeLogAdder.setProperty("Workspace", "ws2");

    timeLogAdder.setProperty("Time", "2010-09-14T04:25:12");
    timeLogAdder.setProperty("Value", 8.3);
    timeLogAdder.execute();

    timeLogAdder.setProperty("Time", "2010-09-14T04:26:12");
    timeLogAdder.setProperty("Value", 9.5);
    timeLogAdder.execute();

    m_testee.setProperty("SampleLogAsXAxis", "TestTimeLog");
    m_testee.setProperty("InputWorkspaces",
                         std::vector<std::string>{"ws1", "ws2"});

    TS_ASSERT_THROWS_NOTHING(m_testee.execute());
    MatrixWorkspace_sptr out =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out");
    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->blocksize(), 5);
    TS_ASSERT_EQUALS(out->getNumberHistograms(), 5);

    auto spectrum = out->y(0);
    auto xaxis = out->x(0);
    auto error = out->e(0);

    TS_ASSERT_EQUALS(spectrum[0], 2.);
    TS_ASSERT_EQUALS(spectrum[1], 2.);
    TS_ASSERT_EQUALS(spectrum[2], 2.);
    TS_ASSERT_EQUALS(spectrum[3], 5.);
    TS_ASSERT_EQUALS(spectrum[4], 5.);

    TS_ASSERT_EQUALS(error[0], 3.);
    TS_ASSERT_EQUALS(error[1], 3.);
    TS_ASSERT_EQUALS(error[2], 3.);
    TS_ASSERT_EQUALS(error[3], 4.);
    TS_ASSERT_EQUALS(error[4], 4.);

    TS_ASSERT_EQUALS(xaxis[0], 5.7);
    TS_ASSERT_EQUALS(xaxis[1], 6.1);
    TS_ASSERT_EQUALS(xaxis[2], 6.7);
    TS_ASSERT_EQUALS(xaxis[3], 8.3);
    TS_ASSERT_EQUALS(xaxis[4], 9.5);
  }

  void testFailWithNumSeriesLog() {

    AddTimeSeriesLog timeLogAdder;
    timeLogAdder.initialize();
    timeLogAdder.setProperty("Workspace", "ws1");
    timeLogAdder.setProperty("Name", "TestTimeLog");
    timeLogAdder.setProperty("Time", "2010-09-14T04:20:12");
    timeLogAdder.setProperty("Value", 5.7);
    timeLogAdder.execute();

    timeLogAdder.setProperty("Workspace", "ws2");
    timeLogAdder.setProperty("Time", "2010-09-14T04:25:12");
    timeLogAdder.setProperty("Value", 8.3);
    timeLogAdder.execute();

    m_testee.setProperty("SampleLogAsXAxis", "TestTimeLog");
    m_testee.setProperty("InputWorkspaces",
                         std::vector<std::string>{"ws1", "ws2"});

    // ws1 has 3 bins, ws2 has 2, fail
    TS_ASSERT_THROWS(m_testee.execute(), std::runtime_error);
  }

  void testMergeSampleLogFail() {
    AddSampleLog logAdder;
    logAdder.initialize();
    logAdder.setProperty("LogName", "Wavelength");
    logAdder.setProperty("LogType", "Number");

    logAdder.setProperty("Workspace", "ws1");
    logAdder.setProperty("LogText", "1.2");
    logAdder.execute();

    logAdder.setProperty("Workspace", "ws2");
    logAdder.setProperty("LogText", "1.5");
    logAdder.execute();

    m_testee.setProperty("SampleLogsFail", "Wavelength");
    m_testee.setProperty("SampleLogsFailTolerances", "0.1");
    m_testee.setProperty("FailBehaviour", "Stop");

    m_testee.setProperty("InputWorkspaces",
                         std::vector<std::string>{"ws1", "ws2"});

    TS_ASSERT_THROWS(m_testee.execute(), std::runtime_error);
  }

private:
  ConjoinXRuns m_testee;
  std::vector<std::string> m_testWS;
};

class ConjoinXRunsTestPerformance : public CxxTest::TestSuite {
public:
  static ConjoinXRunsTestPerformance *createSuite() {
    return new ConjoinXRunsTestPerformance();
  }
  static void destroySuite(ConjoinXRunsTestPerformance *suite) { delete suite; }

  ConjoinXRunsTestPerformance() {}

  void setUp() override {
    m_ws.reserve(100);
    for (size_t i = 0; i < 100; ++i) {
      MatrixWorkspace_sptr ws =
          create2DWorkspace123(2000, 100, false, std::set<int64_t>(), true);
      std::string name = "ws" + std::to_string(i);
      storeWS(name, ws);
      m_ws.push_back(name);
    }
    m_alg.initialize();
    m_alg.isChild();
    m_alg.setProperty("InputWorkspaces", m_ws);
    m_alg.setProperty("OutputWorkspace", "__out");
  }

  void tearDown() override {
    for (const auto &ws : m_ws) {
      removeWS(ws);
    }
    m_ws.clear();
  }

  void test_performance() { m_alg.execute(); }

private:
  ConjoinXRuns m_alg;
  std::vector<std::string> m_ws;
};

#endif /* MANTID_ALGORITHMS_CONJOINXRUNSTEST_H_ */
