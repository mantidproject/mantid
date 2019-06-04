// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_MUONANALYSISHELPERTEST_H_
#define MANTID_CUSTOMINTERFACES_MUONANALYSISHELPERTEST_H_

#include <cxxtest/TestSuite.h>

#include "../Muon/MuonAnalysisHelper.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace MantidQt::CustomInterfaces::MuonAnalysisHelper;

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Types::Core::DateAndTime;

/// This is a wrapper for the ADS that automatically clears itself on
/// destruction
class RAII_ADS {
public:
  void add(const std::string &name, const Workspace_sptr &ws) {
    AnalysisDataService::Instance().add(name, ws);
  }
  void addToGroup(const std::string &group, const std::string &name) {
    AnalysisDataService::Instance().addToGroup(group, name);
  }
  virtual ~RAII_ADS() { AnalysisDataService::Instance().clear(); }
};

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
  void test_isNumberInt() { TS_ASSERT(isNumber(QString("12"))); }

  void test_isNumberDouble() { TS_ASSERT(isNumber(QString("1.2"))); }
  void test_isNumberNegInt() { TS_ASSERT(isNumber(QString("-12"))); }
  void test_isNumberNegDouble() { TS_ASSERT(isNumber(QString("-1.2"))); }
  void test_isNumberFail() { TS_ASSERT(!isNumber(QString("ABC"))); }
  void test_isNumberFailMix() { TS_ASSERT(!isNumber(QString("1BC4"))); }
  void test_isNumberFailRange() { TS_ASSERT(!isNumber(QString("1-4"))); }
  void test_isNumberFailRange2() { TS_ASSERT(!isNumber(QString("1 to 4"))); }

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

  void test_getRunLabel_noWS_singleRun() {
    const std::string label = getRunLabel("MUSR", {15189});
    TS_ASSERT_EQUALS(label, "MUSR00015189");
  }

  void test_getRunLabel_noWS_severalRuns() {
    const std::string label = getRunLabel("MUSR", {15189, 15190, 15192});
    TS_ASSERT_EQUALS(label, "MUSR00015189-90, 15192");
  }

  /// Test an instrument with no IDF and a run number of zero
  /// (which can occur when loading data from this old instrument)
  void test_getRunLabel_DEVA() {
    const std::string label = getRunLabel("DEVA", {0});
    TS_ASSERT_EQUALS(label, "DEVA000");
  }

  void test_sumWorkspaces() {
    MatrixWorkspace_sptr ws1 =
        WorkspaceCreationHelper::create2DWorkspace123(1, 3);
    MatrixWorkspace_sptr ws2 =
        WorkspaceCreationHelper::create2DWorkspace123(1, 3);
    MatrixWorkspace_sptr ws3 =
        WorkspaceCreationHelper::create2DWorkspace123(1, 3);
    DateAndTime start{"2015-12-23T15:32:40Z"};
    DateAndTime end{"2015-12-24T09:00:00Z"};
    addLog(ws1, "run_start", start.toSimpleString());
    addLog(ws1, "run_end", end.toSimpleString());
    addLog(ws1, "run_number", "15189");
    addLog(ws2, "run_start", start.toSimpleString());
    addLog(ws2, "run_end", end.toSimpleString());
    addLog(ws2, "run_number", "15190");
    addLog(ws3, "run_start", start.toSimpleString());
    addLog(ws3, "run_end", end.toSimpleString());
    addLog(ws3, "run_number", "15191");

    std::vector<Workspace_sptr> wsList{ws1, ws2, ws3};

    auto result =
        boost::dynamic_pointer_cast<MatrixWorkspace>(sumWorkspaces(wsList));

    TS_ASSERT(result);
    if (!result)
      return; // Nothing to check

    TS_ASSERT_EQUALS(result->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(result->blocksize(), 3);

    TS_ASSERT_EQUALS(result->y(0)[0], 6);
    TS_ASSERT_EQUALS(result->y(0)[1], 6);
    TS_ASSERT_EQUALS(result->y(0)[2], 6);

    TS_ASSERT_EQUALS(result->run().getProperty("run_number")->value(),
                     "15189-91");
    TS_ASSERT_EQUALS(result->run().getProperty("run_start")->value(),
                     start.toSimpleString());
    TS_ASSERT_EQUALS(result->run().getProperty("run_end")->value(),
                     end.toSimpleString());

    // Original workspaces shouldn't be touched
    TS_ASSERT_EQUALS(ws1->y(0)[0], 2);
    TS_ASSERT_EQUALS(ws3->y(0)[2], 2);
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
    auto range =
        findLogRange(ws, "run_start",
                     [](const std::string &first, const std::string &second) {
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

  // This can happen when loading very old files in which the stored run number
  // is zero
  void test_runNumberString_zeroRunNumber() {
    const std::string sep("; ");
    std::ostringstream wsName;
    wsName << "DEVA000" << sep;
    wsName << "Pair" << sep;
    wsName << "long" << sep;
    wsName << "Asym" << sep;
    wsName << "1+2" << sep;
    wsName << "#1";

    // create expected output
    QString expected = "0: 1+2";
    QString result;

    // test
    TS_ASSERT_THROWS_NOTHING(result = runNumberString(wsName.str(), "0"));
    TS_ASSERT_EQUALS(expected, result);
  }

  void test_isReloadGroupingNecessary_No() {
    const auto currentWs = createWs("MUSR", 15189);
    const auto loadedWs = createWs("MUSR", 15190);
    addLog(currentWs, "main_field_direction", "Longitudinal");
    addLog(loadedWs, "main_field_direction", "Longitudinal");
    bool result = true;
    TS_ASSERT_THROWS_NOTHING(
        result = isReloadGroupingNecessary(currentWs, loadedWs));
    TS_ASSERT_EQUALS(result, false);
  }

  void test_isReloadGroupingNecessary_nullCurrent() {
    const auto currentWs = nullptr;
    const auto loadedWs = createWs("MUSR", 15190);
    bool result = false;
    TS_ASSERT_THROWS_NOTHING(
        result = isReloadGroupingNecessary(currentWs, loadedWs));
    TS_ASSERT_EQUALS(result, true);
  }

  void test_isReloadGroupingNecessary_nullLoaded() {
    const auto currentWs = createWs("MUSR", 15189);
    const auto loadedWs = nullptr;
    TS_ASSERT_THROWS(isReloadGroupingNecessary(currentWs, loadedWs),
                     const std::invalid_argument &);
  }

  void test_isReloadGroupingNecessary_noLogs() {
    const auto currentWs = createWs("MUSR", 15189);
    const auto loadedWs = createWs("MUSR", 15190);
    bool result = true;
    TS_ASSERT_THROWS_NOTHING(
        result = isReloadGroupingNecessary(currentWs, loadedWs));
    TS_ASSERT_EQUALS(result, false);
  }

  void test_isReloadGroupingNecessary_differentInstrument() {
    const auto currentWs = createWs("MUSR", 15189);
    const auto loadedWs = createWs("EMU", 15190);
    addLog(currentWs, "main_field_direction", "Longitudinal");
    addLog(loadedWs, "main_field_direction", "Longitudinal");
    bool result = false;
    TS_ASSERT_THROWS_NOTHING(
        result = isReloadGroupingNecessary(currentWs, loadedWs));
    TS_ASSERT_EQUALS(result, true);
  }

  void test_isReloadGroupingNecessary_differentFieldDirection() {
    const auto currentWs = createWs("MUSR", 15189);
    const auto loadedWs = createWs("MUSR", 22725);
    addLog(currentWs, "main_field_direction", "Longitudinal");
    addLog(loadedWs, "main_field_direction", "Transverse");
    bool result = false;
    TS_ASSERT_THROWS_NOTHING(
        result = isReloadGroupingNecessary(currentWs, loadedWs));
    TS_ASSERT_EQUALS(result, true);
  }

  void test_isReloadGroupingNecessary_differentNumberSpectra() {
    const auto currentWs = createWs("MUSR", 15189);
    const auto loadedWs = createWs("MUSR", 15190, 2);
    addLog(currentWs, "main_field_direction", "Longitudinal");
    addLog(loadedWs, "main_field_direction", "Longitudinal");
    bool result = false;
    TS_ASSERT_THROWS_NOTHING(
        result = isReloadGroupingNecessary(currentWs, loadedWs));
    TS_ASSERT_EQUALS(result, true);
  }

  void test_generateWorkspaceName() {
    MantidQt::CustomInterfaces::Muon::DatasetParams params;
    params.instrument = "MUSR";
    params.runs = {15192, 15190, 15189};
    params.itemType = MantidQt::CustomInterfaces::Muon::Group;
    params.itemName = "fwd";
    params.plotType = MantidQt::CustomInterfaces::Muon::Counts;
    params.periods = "1+3-2+4";
    params.version = 2;
    const std::string wsName = generateWorkspaceName(params);
    const std::string expected =
        "MUSR00015189-90, 15192; Group; fwd; Counts; 1+3-2+4; #2";
    TS_ASSERT_EQUALS(expected, wsName);
  }

  void test_generateWorkspaceName_noPeriods() {
    MantidQt::CustomInterfaces::Muon::DatasetParams params;
    params.instrument = "MUSR";
    params.runs = {15192, 15190, 15189};
    params.itemType = MantidQt::CustomInterfaces::Muon::Group;
    params.itemName = "fwd";
    params.plotType = MantidQt::CustomInterfaces::Muon::Counts;
    params.periods = "";
    params.version = 2;
    const std::string wsName = generateWorkspaceName(params);
    const std::string expected =
        "MUSR00015189-90, 15192; Group; fwd; Counts; #2";
    TS_ASSERT_EQUALS(expected, wsName);
  }

  void test_generateWorkspaceName_givenLabel() {
    MantidQt::CustomInterfaces::Muon::DatasetParams params;
    params.instrument = "MUSR";
    params.runs = {15192, 15190, 15189};
    params.label = "MyLabel00123"; // should be used in preference to inst/runs
    params.itemType = MantidQt::CustomInterfaces::Muon::Group;
    params.itemName = "fwd";
    params.plotType = MantidQt::CustomInterfaces::Muon::Counts;
    params.periods = "1+3-2+4";
    params.version = 2;
    const std::string wsName = generateWorkspaceName(params);
    const std::string expected =
        "MyLabel00123; Group; fwd; Counts; 1+3-2+4; #2";
    TS_ASSERT_EQUALS(expected, wsName);
  }

  void test_parseWorkspaceName() {
    const std::string workspaceName =
        "MUSR00015189-90, 15192; Group; fwd; Counts; 1+3-2+4; #2";
    const std::vector<int> expectedRuns{15189, 15190, 15192};
    const auto params = parseWorkspaceName(workspaceName);
    TS_ASSERT_EQUALS(params.instrument, "MUSR");
    TS_ASSERT_EQUALS(params.runs, expectedRuns);
    TS_ASSERT_EQUALS(params.label, "MUSR00015189-90, 15192");
    TS_ASSERT_EQUALS(params.itemType, MantidQt::CustomInterfaces::Muon::Group);
    TS_ASSERT_EQUALS(params.itemName, "fwd");
    TS_ASSERT_EQUALS(params.plotType, MantidQt::CustomInterfaces::Muon::Counts);
    TS_ASSERT_EQUALS(params.periods, "1+3-2+4");
    TS_ASSERT_EQUALS(params.version, 2);
  }

  void test_parseWorkspaceName_noPeriods() {
    const std::string workspaceName =
        "MUSR00015189-90, 15192; Group; fwd; Counts; #2";
    const std::vector<int> expectedRuns{15189, 15190, 15192};
    const auto params = parseWorkspaceName(workspaceName);
    TS_ASSERT_EQUALS(params.instrument, "MUSR");
    TS_ASSERT_EQUALS(params.runs, expectedRuns);
    TS_ASSERT_EQUALS(params.label, "MUSR00015189-90, 15192");
    TS_ASSERT_EQUALS(params.itemType, MantidQt::CustomInterfaces::Muon::Group);
    TS_ASSERT_EQUALS(params.itemName, "fwd");
    TS_ASSERT_EQUALS(params.plotType, MantidQt::CustomInterfaces::Muon::Counts);
    TS_ASSERT_EQUALS(params.periods, "");
    TS_ASSERT_EQUALS(params.version, 2);
  }

  void test_parseRunLabel() {
    const std::string runLabel = "MUSR00015189-91, 15193-4, 15196";
    std::string instrument = "";
    std::vector<int> runs;
    std::vector<int> expectedRuns{15189, 15190, 15191, 15193, 15194, 15196};
    TS_ASSERT_THROWS_NOTHING(parseRunLabel(runLabel, instrument, runs));
    TS_ASSERT_EQUALS(instrument, "MUSR");
    TS_ASSERT_EQUALS(runs, expectedRuns);
  }

  void test_parseRunLabel_noZeros() {
    const std::string runLabel = "EMU12345-8";
    std::string instrument = "";
    std::vector<int> runs;
    std::vector<int> expectedRuns{12345, 12346, 12347, 12348};
    TS_ASSERT_THROWS_NOTHING(parseRunLabel(runLabel, instrument, runs));
    TS_ASSERT_EQUALS(instrument, "EMU");
    TS_ASSERT_EQUALS(runs, expectedRuns);
  }

  /// This can happen with very old NeXus files where the stored run number is
  /// zero
  void test_parseRunLabel_allZeros() {
    const std::string runLabel = "DEVA000";
    std::string instrument = "";
    std::vector<int> runs;
    std::vector<int> expectedRuns{0};
    TS_ASSERT_THROWS_NOTHING(parseRunLabel(runLabel, instrument, runs));
    TS_ASSERT_EQUALS(instrument, "DEVA");
    TS_ASSERT_EQUALS(runs, expectedRuns);
  }

  /// No zero padding, but a zero does appear later in the label
  void test_parseRunLabel_noPadding_zeroInRunNumber() {
    const std::string runLabel = "MUSR15190";
    std::string instrument = "";
    std::vector<int> runs;
    std::vector<int> expectedRuns{15190};
    TS_ASSERT_THROWS_NOTHING(parseRunLabel(runLabel, instrument, runs));
    TS_ASSERT_EQUALS(instrument, "MUSR");
    TS_ASSERT_EQUALS(runs, expectedRuns);
  }

  void test_getWorkspaceColors_singleRuns_allSame() {
    const auto &tableOne = createResultsTable({"A0", "A1"});
    const auto &tableTwo = createResultsTable({"A0", "A1"});
    const auto &tableThree = createResultsTable({"A0", "A1"});
    QMap<int, QColor> results;
    TS_ASSERT_THROWS_NOTHING(
        results = getWorkspaceColors({tableOne, tableTwo, tableThree}));
    TS_ASSERT_EQUALS(3, results.count());
    for (const auto &color : results) {
      TS_ASSERT_EQUALS(color, QColor("black"));
    }
  }

  void test_getWorkspaceColors_singleRuns_differentModels() {
    const auto &tableOne = createResultsTable({"A0", "A1"});
    const auto &tableTwo = createResultsTable({"A0", "A1"});
    const auto &tableThree = createResultsTable({"Tau", "Delta"});
    const auto &tableFour = createResultsTable({"A0", "A1"});
    const auto &tableFive = createResultsTable({"Alpha", "Delta"});
    QMap<int, QColor> results;
    TS_ASSERT_THROWS_NOTHING(
        results = getWorkspaceColors(
            {tableOne, tableTwo, tableThree, tableFour, tableFive}));
    TS_ASSERT_EQUALS(5, results.count());
    TS_ASSERT_EQUALS(results[0], QColor("black"));
    TS_ASSERT_EQUALS(results[1], QColor("black"));
    TS_ASSERT_EQUALS(results[2], QColor("red"));
    TS_ASSERT_EQUALS(results[3], QColor("black"));
    TS_ASSERT_EQUALS(results[4], QColor("green"));
  }

  // This test uses WorkspaceGroups - since the names of the workspaces are
  // important, it unfortunately has to use the ADS
  void test_getWorkspaceColors_groups_allSame() {
    RAII_ADS ads; // will clear itself on destruction

    const auto &tableOne = createResultsTable({"A0", "A1"});
    ads.add("MuonSimulFit_Label_Parameters", tableOne);
    const auto &wsOne = createWs("MUSR", 15189);
    ads.add("MuonSimulFit_Label_MUSR15189_Workspace", wsOne);
    const auto &wsTwo = createWs("MUSR", 15190);
    ads.add("MuonSimulFit_Label_MUSR15190_Workspace", wsTwo);
    const auto &groupOne = boost::make_shared<WorkspaceGroup>();
    ads.add("GroupOne", groupOne);
    ads.addToGroup("GroupOne", "MuonSimulFit_Label_Parameters");
    ads.addToGroup("GroupOne", "MuonSimulFit_Label_MUSR15189_Workspace");
    ads.addToGroup("GroupOne", "MuonSimulFit_Label_MUSR15190_Workspace");

    const auto &tableTwo = createResultsTable({"A0", "A1"});
    ads.add("MuonSimulFit_Label#2_Parameters", tableTwo);
    const auto &wsThree = createWs("MUSR", 15191);
    ads.add("MuonSimulFit_Label#2_MUSR15191_Workspace", wsThree);
    const auto &wsFour = createWs("MUSR", 15192);
    ads.add("MuonSimulFit_Label#2_MUSR15192_Workspace", wsFour);
    const auto &groupTwo = boost::make_shared<WorkspaceGroup>();
    ads.add("GroupTwo", groupTwo);
    ads.addToGroup("GroupTwo", "MuonSimulFit_Label#2_Parameters");
    ads.addToGroup("GroupTwo", "MuonSimulFit_Label#2_MUSR15191_Workspace");
    ads.addToGroup("GroupTwo", "MuonSimulFit_Label#2_MUSR15192_Workspace");

    QMap<int, QColor> results;
    TS_ASSERT_THROWS_NOTHING(results =
                                 getWorkspaceColors({groupOne, groupTwo}));
    TS_ASSERT_EQUALS(2, results.count());
    TS_ASSERT_EQUALS(results[0], QColor("black"));
    TS_ASSERT_EQUALS(results[1], QColor("black"));
  }

  void test_getWorkspaceColors_groups_sameModel_differentNumberOfRuns() {
    RAII_ADS ads; // will clear itself on destruction

    const auto &tableOne = createResultsTable({"A0", "A1"});
    ads.add("MuonSimulFit_Label_Parameters", tableOne);
    const auto &wsOne = createWs("MUSR", 15189);
    ads.add("MuonSimulFit_Label_MUSR15189_Workspace", wsOne);
    const auto &wsTwo = createWs("MUSR", 15190);
    ads.add("MuonSimulFit_Label_MUSR15190_Workspace", wsTwo);
    const auto &wsThree = createWs("MUSR", 15191);
    ads.add("MuonSimulFit_Label_MUSR15191_Workspace", wsThree);
    const auto &groupOne = boost::make_shared<WorkspaceGroup>();
    ads.add("GroupOne", groupOne);
    ads.addToGroup("GroupOne", "MuonSimulFit_Label_Parameters");
    ads.addToGroup("GroupOne", "MuonSimulFit_Label_MUSR15189_Workspace");
    ads.addToGroup("GroupOne", "MuonSimulFit_Label_MUSR15190_Workspace");
    ads.addToGroup("GroupOne", "MuonSimulFit_Label_MUSR15191_Workspace");

    const auto &tableTwo = createResultsTable({"A0", "A1"});
    ads.add("MuonSimulFit_Label#2_Parameters", tableTwo);
    const auto &wsFour = createWs("MUSR", 15192);
    ads.add("MuonSimulFit_Label#2_MUSR15192_Workspace", wsFour);
    const auto &wsFive = createWs("MUSR", 15193);
    ads.add("MuonSimulFit_Label#2_MUSR15193_Workspace", wsFive);
    const auto &groupTwo = boost::make_shared<WorkspaceGroup>();
    ads.add("GroupTwo", groupTwo);
    ads.addToGroup("GroupTwo", "MuonSimulFit_Label#2_Parameters");
    ads.addToGroup("GroupTwo", "MuonSimulFit_Label#2_MUSR15192_Workspace");
    ads.addToGroup("GroupTwo", "MuonSimulFit_Label#2_MUSR15193_Workspace");

    QMap<int, QColor> results;
    TS_ASSERT_THROWS_NOTHING(results =
                                 getWorkspaceColors({groupOne, groupTwo}));
    TS_ASSERT_EQUALS(2, results.count());
    TS_ASSERT_EQUALS(results[0], QColor("black"));
    TS_ASSERT_EQUALS(results[1], QColor("red"));
  }

  void test_getWorkspaceColors_groups_differentModel_sameNumberOfRuns() {
    RAII_ADS ads; // will clear itself on destruction

    const auto &tableOne = createResultsTable({"A0", "A1"});
    ads.add("MuonSimulFit_Label_Parameters", tableOne);
    const auto &wsOne = createWs("MUSR", 15189);
    ads.add("MuonSimulFit_Label_MUSR15189_Workspace", wsOne);
    const auto &wsTwo = createWs("MUSR", 15190);
    ads.add("MuonSimulFit_Label_MUSR15190_Workspace", wsTwo);
    const auto &groupOne = boost::make_shared<WorkspaceGroup>();
    ads.add("GroupOne", groupOne);
    ads.addToGroup("GroupOne", "MuonSimulFit_Label_Parameters");
    ads.addToGroup("GroupOne", "MuonSimulFit_Label_MUSR15189_Workspace");
    ads.addToGroup("GroupOne", "MuonSimulFit_Label_MUSR15190_Workspace");

    const auto &tableTwo = createResultsTable({"Tau", "Delta"});
    ads.add("MuonSimulFit_Label#2_Parameters", tableTwo);
    const auto &wsThree = createWs("MUSR", 15191);
    ads.add("MuonSimulFit_Label#2_MUSR15191_Workspace", wsThree);
    const auto &wsFour = createWs("MUSR", 15192);
    ads.add("MuonSimulFit_Label#2_MUSR15192_Workspace", wsFour);
    const auto &groupTwo = boost::make_shared<WorkspaceGroup>();
    ads.add("GroupTwo", groupTwo);
    ads.addToGroup("GroupTwo", "MuonSimulFit_Label#2_Parameters");
    ads.addToGroup("GroupTwo", "MuonSimulFit_Label#2_MUSR15191_Workspace");
    ads.addToGroup("GroupTwo", "MuonSimulFit_Label#2_MUSR15192_Workspace");

    QMap<int, QColor> results;
    TS_ASSERT_THROWS_NOTHING(results =
                                 getWorkspaceColors({groupOne, groupTwo}));
    TS_ASSERT_EQUALS(2, results.count());
    TS_ASSERT_EQUALS(results[0], QColor("black"));
    TS_ASSERT_EQUALS(results[1], QColor("red"));
  }

private:
  // Creates a single-point workspace with instrument and runNumber set
  Workspace_sptr createWs(const std::string &instrName, int runNumber,
                          size_t nSpectra = 1) {
    Geometry::Instrument_const_sptr instr =
        boost::make_shared<Geometry::Instrument>(instrName);

    MatrixWorkspace_sptr ws =
        WorkspaceFactory::Instance().create("Workspace2D", nSpectra, 1, 1);
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
  void
  addTimeSeriesLog(const Workspace_sptr &ws, const std::string &logName,
                   const std::vector<Mantid::Types::Core::DateAndTime> &times,
                   const std::vector<double> &values) {
    TS_ASSERT_EQUALS(times.size(), values.size());
    auto matrixWS = boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
    TS_ASSERT(matrixWS);
    auto prop =
        std::make_unique<TimeSeriesProperty<double>>(logName);
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

  /// Create a results table with the given parameters
  ITableWorkspace_sptr
  createResultsTable(const std::vector<std::string> &params) {
    auto table = WorkspaceFactory::Instance().createTable();
    auto nameCol = table->addColumn("str", "Name");
    nameCol->setPlotType(6); // label
    auto valCol = table->addColumn("double", "Value");
    valCol->setPlotType(2); // Y
    auto errCol = table->addColumn("double", "Error");
    errCol->setPlotType(5); // YErr

    for (size_t i = 0; i < params.size(); ++i) {
      TableRow row = table->appendRow();
      row << params[i] << static_cast<double>(i) << 0.1;
    }

    TableRow chiSqRow = table->appendRow();
    chiSqRow << "Cost function" << 0.5 << 0.0;

    return table;
  }
};

#endif /* MANTID_CUSTOMINTERFACES_MUONANALYSISHELPERTEST_H_ */
