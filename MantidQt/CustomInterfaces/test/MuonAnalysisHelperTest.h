#ifndef MANTID_CUSTOMINTERFACES_MUONANALYSISHELPERTEST_H_
#define MANTID_CUSTOMINTERFACES_MUONANALYSISHELPERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtCustomInterfaces/Muon/MuonAnalysisHelper.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidGeometry/Instrument.h"
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

  void test_findStartAndEndTimes_singleWS() {
    Workspace_sptr ws = createWs("MUSR", 15189);
    DateAndTime start{"2015-12-23T15:32:40Z"};
    DateAndTime end{"2015-12-24T09:00:00Z"};
    addLog(ws, "run_start", start.toSimpleString());
    addLog(ws, "run_end", end.toSimpleString());
    auto times = findStartAndEndTimes(ws);
    TS_ASSERT_EQUALS(times.first, start);
    TS_ASSERT_EQUALS(times.second, end);
  }

  void test_findStartAndEndTimes_groupWS() {
    Workspace_sptr ws1 = createWs("MUSR", 15189);
    Workspace_sptr ws2 = createWs("MUSR", 15190);
    DateAndTime start1{"2015-12-23T15:32:40Z"};
    DateAndTime end1{"2016-12-24T09:00:00Z"};
    DateAndTime start2{"2014-12-23T15:32:40Z"};
    DateAndTime end2{"2015-12-24T09:00:00Z"};
    addLog(ws1, "run_start", start1.toSimpleString());
    addLog(ws1, "run_end", end1.toSimpleString());
    addLog(ws2, "run_start", start2.toSimpleString());
    addLog(ws2, "run_end", end2.toSimpleString());
    auto groupWS = groupWorkspaces({ws1, ws2});
    auto times = findStartAndEndTimes(groupWS);
    TS_ASSERT_EQUALS(times.first, start2);
    TS_ASSERT_EQUALS(times.second, end1);
  }

  void test_replaceLogValue() {
    ScopedWorkspace ws(createWs("MUSR", 15189));
    DateAndTime start1{"2015-12-23T15:32:40Z"};
    DateAndTime start2{"2014-12-23T15:32:40Z"};
    DateAndTime end{"2015-12-24T09:00:00Z"};
    addLog(ws.retrieve(), "run_start", start1.toSimpleString());
    addLog(ws.retrieve(), "run_end", end.toSimpleString());
    replaceLogValue(ws.name(), "run_start", start2.toSimpleString());
    auto times = findStartAndEndTimes(ws.retrieve());
    TS_ASSERT_EQUALS(times.first, start2);
    TS_ASSERT_EQUALS(times.second, end);
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
};

#endif /* MANTID_CUSTOMINTERFACES_MUONANALYSISHELPERTEST_H_ */
