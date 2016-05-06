#ifndef MANTID_CUSTOMINTERFACES_MUONANALYSISHELPERTEST_H_
#define MANTID_CUSTOMINTERFACES_MUONANALYSISHELPERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtCustomInterfaces/Muon/MuonAnalysisHelper.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace MantidQt::CustomInterfaces::MuonAnalysisHelper;

using namespace Mantid;
using namespace Mantid::API;

class MuonAnalysisHelperTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonAnalysisHelperTest *createSuite() {
    return new MuonAnalysisHelperTest();
  }
  static void destroySuite(MuonAnalysisHelperTest *suite) { delete suite; }

  MuonAnalysisHelperTest() {
    FrameworkManager::Instance(); // So that framework is initialized
  }

  void test_getRunLabel_singleWs() {
    std::string label = getRunLabel(createWs("MUSR", 15189));
    TS_ASSERT_EQUALS(label, "MUSR00015189");
  }

  void test_getRunLabel_argus() {
    std::string label = getRunLabel(createWs("ARGUS", 26577));
    TS_ASSERT_EQUALS(label, "ARGUS0026577");
  }

  void test_getRunLabel_singleWs_tooBigRunNumber() {
    std::string label = getRunLabel(createWs("EMU", 999999999));
    TS_ASSERT_EQUALS(label, "EMU999999999");
  }

  void test_getRunLabel_wsList() {
    std::vector<Workspace_sptr> list;

    for (int i = 15189; i <= 15193; ++i) {
      list.push_back(createWs("MUSR", i));
    }

    std::string label = getRunLabel(list);
    TS_ASSERT_EQUALS(label, "MUSR00015189-93");
  }

  void test_getRunLabel_wsList_wrongOrder() {
    std::vector<int> runNumbers{10, 3, 5, 1, 6, 2, 4, 8, 7, 9};
    std::vector<Workspace_sptr> list;

    for (auto it = runNumbers.begin(); it != runNumbers.end(); ++it) {
      list.push_back(createWs("EMU", *it));
    }

    std::string label = getRunLabel(list);
    TS_ASSERT_EQUALS(label, "EMU00000001-10");
  }

  void test_getRunLabel_wsList_nonConsecutive() {
    std::vector<int> runNumbers{1, 2, 3, 5, 6, 8, 10, 11, 12, 13, 14};
    std::vector<Workspace_sptr> list;
    for (auto it = runNumbers.begin(); it != runNumbers.end(); it++) {
      list.push_back(createWs("EMU", *it));
    }
    std::string label = getRunLabel(list);
    TS_ASSERT_EQUALS(label, "EMU00000001-3, 5-6, 8, 10-4");
  }

  void test_getRunLabel_wsList_nonConsecutive_wrongOrder() {
    std::vector<int> runNumbers{5, 14, 8, 1, 11, 3, 10, 6, 13, 12, 2};
    std::vector<Workspace_sptr> list;
    for (auto it = runNumbers.begin(); it != runNumbers.end(); it++) {
      list.push_back(createWs("EMU", *it));
    }
    std::string label = getRunLabel(list);
    TS_ASSERT_EQUALS(label, "EMU00000001-3, 5-6, 8, 10-4");
  }

  void test_sumWorkspaces() {
    MatrixWorkspace_sptr ws1 =
        WorkspaceCreationHelper::Create2DWorkspace123(1, 3);
    MatrixWorkspace_sptr ws2 =
        WorkspaceCreationHelper::Create2DWorkspace123(1, 3);
    MatrixWorkspace_sptr ws3 =
        WorkspaceCreationHelper::Create2DWorkspace123(1, 3);
    DateAndTime start{"2015-12-23T15:32:40Z"};
    DateAndTime end{"2015-12-24T09:00:00Z"};
    addLog(ws1, "run_start", start.toSimpleString());
    addLog(ws1, "run_end", end.toSimpleString());
    addLog(ws2, "run_start", start.toSimpleString());
    addLog(ws2, "run_end", end.toSimpleString());
    addLog(ws3, "run_start", start.toSimpleString());
    addLog(ws3, "run_end", end.toSimpleString());

    std::vector<Workspace_sptr> wsList{ws1, ws2, ws3};

    auto result =
        boost::dynamic_pointer_cast<MatrixWorkspace>(sumWorkspaces(wsList));

    TS_ASSERT(result);
    if (!result)
      return; // Nothing to check

    TS_ASSERT_EQUALS(result->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(result->blocksize(), 3);

    TS_ASSERT_EQUALS(result->readY(0)[0], 6);
    TS_ASSERT_EQUALS(result->readY(0)[1], 6);
    TS_ASSERT_EQUALS(result->readY(0)[2], 6);

    // Original workspaces shouldn't be touched
    TS_ASSERT_EQUALS(ws1->readY(0)[0], 2);
    TS_ASSERT_EQUALS(ws3->readY(0)[2], 2);
  }

  void test_findConsecutiveRuns() {
    std::vector<int> testVec{1, 2, 3, 5, 6, 8, 10, 11, 12, 13, 14};
    auto ranges = findConsecutiveRuns(testVec);
    TS_ASSERT_EQUALS(ranges.size(), 4);
    TS_ASSERT_EQUALS(ranges[0], std::make_pair(1, 3));
    TS_ASSERT_EQUALS(ranges[1], std::make_pair(5, 6));
    TS_ASSERT_EQUALS(ranges[2], std::make_pair(8, 8));
    TS_ASSERT_EQUALS(ranges[3], std::make_pair(10, 14));
  }

  void test_replaceLogValue() {
    ScopedWorkspace ws(createWs("MUSR", 15189));
    DateAndTime start1{"2015-12-23T15:32:40Z"};
    DateAndTime start2{"2014-12-23T15:32:40Z"};
    addLog(ws.retrieve(), "run_start", start1.toSimpleString());
    replaceLogValue(ws.name(), "run_start", start2.toSimpleString());
    auto times = findLogValues(ws.retrieve(), "run_start");
    TS_ASSERT_EQUALS(times.size(), 1);
    TS_ASSERT_EQUALS(times[0], start2.toSimpleString());
  }

  void test_findLogValues() {
    Workspace_sptr ws1 = createWs("MUSR", 15189);
    Workspace_sptr ws2 = createWs("MUSR", 15190);
    DateAndTime start1{"2015-12-23T15:32:40Z"};
    DateAndTime start2{"2014-12-23T15:32:40Z"};
    addLog(ws1, "run_start", start1.toSimpleString());
    addLog(ws2, "run_start", start2.toSimpleString());
    auto groupWS = groupWorkspaces({ws1, ws2});
    auto starts = findLogValues(groupWS, "run_start");
    auto badLogs = findLogValues(groupWS, "not_present");
    auto singleStart = findLogValues(ws1, "run_start");
    TS_ASSERT_EQUALS(2, starts.size());
    TS_ASSERT_EQUALS(start1, starts[0]);
    TS_ASSERT_EQUALS(start2, starts[1]);
    TS_ASSERT_EQUALS(0, badLogs.size());
    TS_ASSERT_EQUALS(1, singleStart.size());
    TS_ASSERT_EQUALS(start1, singleStart[0]);
  }

  void test_findLogRange_singleWS() {
    Workspace_sptr ws = createWs("MUSR", 15189);
    DateAndTime start{"2015-12-23T15:32:40Z"};
    addLog(ws, "run_start", start.toSimpleString());
    auto range = findLogRange(ws, "run_start", [](const std::string &first,
                                                  const std::string &second) {
      return DateAndTime(first) < DateAndTime(second);
    });
    TS_ASSERT_EQUALS(range.first, start.toSimpleString());
    TS_ASSERT_EQUALS(range.second, start.toSimpleString());
  }

  void test_findLogRange_groupWS() {
    Workspace_sptr ws1 = createWs("MUSR", 15189);
    Workspace_sptr ws2 = createWs("MUSR", 15190);
    DateAndTime start1{"2015-12-23T15:32:40Z"};
    DateAndTime start2{"2014-12-23T15:32:40Z"};
    addLog(ws1, "run_start", start1.toSimpleString());
    addLog(ws2, "run_start", start2.toSimpleString());
    auto groupWS = groupWorkspaces({ws1, ws2});
    auto range =
        findLogRange(groupWS, "run_start",
                     [](const std::string &first, const std::string &second) {
                       return DateAndTime(first) < DateAndTime(second);
                     });
    TS_ASSERT_EQUALS(range.first, start2.toSimpleString());
    TS_ASSERT_EQUALS(range.second, start1.toSimpleString());
  }

  void test_findLogRange_vectorOfWorkspaces() {
    Workspace_sptr ws1 = createWs("MUSR", 15189);
    Workspace_sptr ws2 = createWs("MUSR", 15190);
    DateAndTime start1{"2015-12-23T15:32:40Z"};
    DateAndTime start2{"2014-12-23T15:32:40Z"};
    addLog(ws1, "run_start", start1.toSimpleString());
    addLog(ws2, "run_start", start2.toSimpleString());
    std::vector<Workspace_sptr> workspaces = {ws1, ws2};
    auto range =
        findLogRange(workspaces, "run_start",
                     [](const std::string &first, const std::string &second) {
                       return DateAndTime(first) < DateAndTime(second);
                     });
    TS_ASSERT_EQUALS(range.first, start2.toSimpleString());
    TS_ASSERT_EQUALS(range.second, start1.toSimpleString());
  }

  void test_findLogRange_notPresent() {
    Workspace_sptr ws = createWs("MUSR", 15189);
    // try a DateAndTime
    auto timeRange =
        findLogRange(ws, "run_start",
                     [](const std::string &first, const std::string &second) {
                       return DateAndTime(first) < DateAndTime(second);
                     });
    TS_ASSERT_EQUALS(timeRange.first, "");
    TS_ASSERT_EQUALS(timeRange.second, "");
    // now try a double
    auto numRange =
        findLogRange(ws, "sample_temp",
                     [](const std::string &first, const std::string &second) {
                       return boost::lexical_cast<double>(first) <
                              boost::lexical_cast<double>(second);
                     });
    TS_ASSERT_EQUALS(numRange.first, "");
    TS_ASSERT_EQUALS(numRange.second, "");
  }

  void test_findLogRange_numerical() {
    Workspace_sptr ws1 = createWs("MUSR", 15189);
    Workspace_sptr ws2 = createWs("MUSR", 15190);
    addLog(ws1, "sample_magn_field", "15.4");
    addLog(ws2, "sample_magn_field", "250");
    std::vector<Workspace_sptr> workspaces = {ws1, ws2};
    auto range =
        findLogRange(workspaces, "sample_magn_field",
                     [](const std::string &first, const std::string &second) {
                       return boost::lexical_cast<double>(first) <
                              boost::lexical_cast<double>(second);
                     });
    TS_ASSERT_EQUALS(range.first, "15.4");
    TS_ASSERT_EQUALS(range.second, "250");
  }

  void test_appendTimeSeriesLogs() {
    Workspace_sptr ws1 = createWs("MUSR", 15189);
    Workspace_sptr ws2 = createWs("MUSR", 15190);
    const DateAndTime time1{"2015-12-23T15:32:40Z"},
        time2{"2015-12-23T15:32:41Z"}, time3{"2015-12-23T15:32:42Z"},
        time4{"2015-12-23T15:32:43Z"}, time5{"2015-12-23T15:32:44Z"},
        time6{"2015-12-23T15:32:45Z"};
    const double value1(1), value2(2), value3(3), value4(4), value5(5),
        value6(6);
    const std::string logName = "TSLog";
    addTimeSeriesLog(ws1, logName, {time1, time2, time3},
                     {value1, value2, value3});
    addTimeSeriesLog(ws2, logName, {time4, time5, time6},
                     {value4, value5, value6});
    appendTimeSeriesLogs(ws2, ws1, logName);
    auto matrixWS = boost::dynamic_pointer_cast<MatrixWorkspace>(ws1);
    TS_ASSERT(matrixWS);
    auto prop = matrixWS->run().getTimeSeriesProperty<double>(logName);
    TS_ASSERT_EQUALS(time1, prop->firstTime());
    TS_ASSERT_EQUALS(value1, prop->firstValue());
    TS_ASSERT_EQUALS(time6, prop->lastTime());
    TS_ASSERT_EQUALS(value6, prop->lastValue());
    TS_ASSERT_EQUALS(6, prop->valueAsCorrectMap().size());
  }

  void test_runNumberString_singlePeriod() {
    doTestRunNumberString("15189", false);
  }

  void test_runNumberString_multiPeriod() {
    doTestRunNumberString("15189", true);
  }

  void test_runNumberString_singlePeriod_runRange() {
    doTestRunNumberString("15189-91", false);
  }

  void test_runNumberString_singlePeriod_runRangeNonContinuous() {
    doTestRunNumberString("15189-90, 15192", false);
  }

  void test_runNumberString_multiPeriod_runRange() {
    doTestRunNumberString("15189-91", true);
  }

  void test_runNumberString_multiPeriod_runRangeNonContinuous() {
    doTestRunNumberString("15189-90, 15192", true);
  }

private:
  // Creates a single-point workspace with instrument and runNumber set
  Workspace_sptr createWs(const std::string &instrName, int runNumber) {
    Geometry::Instrument_const_sptr instr =
        boost::make_shared<Geometry::Instrument>(instrName);

    MatrixWorkspace_sptr ws =
        WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
    ws->setInstrument(instr);

    ws->mutableRun().addProperty("run_number", runNumber);

    return ws;
  }
  // Adds a log to the workspace
  void addLog(const Workspace_sptr &ws, const std::string &logName,
              const std::string &logValue) {
    auto alg = AlgorithmManager::Instance().create("AddSampleLog");
    alg->setChild(true);
    alg->setLogging(false);
    alg->setRethrows(true);
    alg->setProperty("Workspace", ws);
    alg->setPropertyValue("LogName", logName);
    alg->setPropertyValue("LogText", logValue);
    alg->execute();
  }

  // Groups the supplied workspaces
  WorkspaceGroup_sptr
  groupWorkspaces(const std::vector<Workspace_sptr> &workspaces) {
    auto group = boost::make_shared<WorkspaceGroup>();
    for (auto ws : workspaces) {
      group->addWorkspace(ws);
    }
    return group;
  }

  // Adds a time series log to the workspace
  void addTimeSeriesLog(const Workspace_sptr &ws, const std::string &logName,
                        const std::vector<Mantid::Kernel::DateAndTime> &times,
                        const std::vector<double> &values) {
    TS_ASSERT_EQUALS(times.size(), values.size());
    auto matrixWS = boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
    TS_ASSERT(matrixWS);
    auto prop =
        Mantid::Kernel::make_unique<TimeSeriesProperty<double>>(logName);
    prop->addValues(times, values);
    matrixWS->mutableRun().addLogData(std::move(prop));
  }

  void doTestRunNumberString(const std::string &runs, bool multiPeriod) {
    // create workspace name
    const std::string sep("; ");
    std::ostringstream wsName;
    wsName << "MUSR000" << runs << sep; // MUSR00012345-8
    wsName << "Pair" << sep;
    wsName << "long" << sep;
    wsName << "Asym" << sep;
    if (multiPeriod) {
      wsName << "1+2-3+4" << sep;
    }
    wsName << "#1";

    // create expected output
    QString expected = QString::fromStdString(runs);
    if (multiPeriod) {
      expected.append(": 1+2-3+4");
    }

    // test
    const QString result = runNumberString(wsName.str(), runs);
    TS_ASSERT_EQUALS(expected, result);
  }
};

#endif /* MANTID_CUSTOMINTERFACES_MUONANALYSISHELPERTEST_H_ */
