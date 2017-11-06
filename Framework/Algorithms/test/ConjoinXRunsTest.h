#ifndef MANTID_ALGORITHMS_CONJOINXRUNSTEST_H_
#define MANTID_ALGORITHMS_CONJOINXRUNSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/AddSampleLog.h"
#include "MantidAlgorithms/AddTimeSeriesLog.h"
#include "MantidAlgorithms/ConjoinXRuns.h"
#include "MantidKernel/Unit.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::ConjoinXRuns;
using Mantid::Algorithms::AddSampleLog;
using Mantid::Algorithms::AddTimeSeriesLog;
using namespace Mantid::API;
using namespace WorkspaceCreationHelper;

class ConjoinXRunsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConjoinXRunsTest *createSuite() { return new ConjoinXRunsTest(); }
  static void destroySuite(ConjoinXRunsTest *suite) { delete suite; }

  void setUp() override {
    MatrixWorkspace_sptr ws1 = create2DWorkspace123(5, 3); // 3 points
    MatrixWorkspace_sptr ws2 = create2DWorkspace154(5, 2); // 2 points
    MatrixWorkspace_sptr ws3 = create2DWorkspace123(5, 1); // 1 point
    MatrixWorkspace_sptr ws4 = create2DWorkspace154(5, 1); // 1 point

    ws1->getAxis(0)->setUnit("TOF");
    ws2->getAxis(0)->setUnit("TOF");
    ws3->getAxis(0)->setUnit("TOF");
    ws4->getAxis(0)->setUnit("TOF");

    storeWS("ws1", ws1);
    storeWS("ws2", ws2);
    storeWS("ws3", ws3);
    storeWS("ws4", ws4);

    m_testWS = {"ws1", "ws2", "ws3", "ws4"};
  }

  void tearDown() override {
    removeWS("ws1");
    removeWS("ws2");
    removeWS("ws3");
    removeWS("ws4");
    m_testWS.clear();
  }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(m_testee.initialize());
    TS_ASSERT(m_testee.isInitialized());
  }

  void testHappyCase() {
    m_testee.setProperty("InputWorkspaces", m_testWS);
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

    std::vector<double> spectrum = out->y(0).rawData();
    std::vector<double> error = out->e(0).rawData();
    std::vector<double> xaxis = out->x(0).rawData();

    TS_ASSERT_EQUALS(spectrum[0], 2.);
    TS_ASSERT_EQUALS(spectrum[1], 2.);
    TS_ASSERT_EQUALS(spectrum[2], 2.);
    TS_ASSERT_EQUALS(spectrum[3], 5.);
    TS_ASSERT_EQUALS(spectrum[4], 5.);
    TS_ASSERT_EQUALS(spectrum[5], 2.);
    TS_ASSERT_EQUALS(spectrum[6], 5.);

    TS_ASSERT_EQUALS(error[0], 3.);
    TS_ASSERT_EQUALS(error[1], 3.);
    TS_ASSERT_EQUALS(error[2], 3.);
    TS_ASSERT_EQUALS(error[3], 4.);
    TS_ASSERT_EQUALS(error[4], 4.);
    TS_ASSERT_EQUALS(error[5], 3.);
    TS_ASSERT_EQUALS(error[6], 4.);

    TS_ASSERT_EQUALS(xaxis[0], 1.);
    TS_ASSERT_EQUALS(xaxis[1], 2.);
    TS_ASSERT_EQUALS(xaxis[2], 3.);
    TS_ASSERT_EQUALS(xaxis[3], 1.);
    TS_ASSERT_EQUALS(xaxis[4], 2.);
    TS_ASSERT_EQUALS(xaxis[5], 1.);
    TS_ASSERT_EQUALS(xaxis[6], 1.);
  }

  void testFailWithNumLog() {
    AddSampleLog logAdder;
    logAdder.initialize();
    logAdder.setProperty("LogName", "TestLog");
    logAdder.setProperty("LogType", "Number");

    logAdder.setProperty("Workspace", "ws1");
    logAdder.setProperty("LogText", "0.7");
    logAdder.execute();

    m_testee.setProperty("InputWorkspaces", m_testWS);

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

    std::vector<double> xaxis = out->x(0).rawData();
    std::vector<double> spectrum = out->y(0).rawData();
    std::vector<double> error = out->e(0).rawData();

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

    m_testee.setProperty("InputWorkspaces", m_testWS);
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

    std::vector<double> spectrum = out->y(0).rawData();
    std::vector<double> xaxis = out->x(0).rawData();
    std::vector<double> error = out->e(0).rawData();

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
      MatrixWorkspace_sptr ws = create2DWorkspace123(10000, 100);
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
