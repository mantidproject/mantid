// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MUON_MUONALGORITHMHELPERTEST_H_
#define MANTID_MUON_MUONALGORITHMHELPERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidMuon/MuonAlgorithmHelper.h"

#include "MantidTestHelpers/MuonWorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::MuonAlgorithmHelper;

class MuonAlgorithmHelperTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonAlgorithmHelperTest *createSuite() {
    return new MuonAlgorithmHelperTest();
  }
  static void destroySuite(MuonAlgorithmHelperTest *suite) { delete suite; }

  MuonAlgorithmHelperTest() {
    FrameworkManager::Instance(); // So that framework is initialized
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

  void test_createStringFromRange_noRange() {
    std::pair<int, int> range;
    range.first = 1000;
    range.second = 1000;
    std::string rangeString = createStringFromRange(range, 0);
    TS_ASSERT_EQUALS(rangeString, "1000");
  }

  void test_createStringFromRange_range() {
    std::pair<int, int> range;
    range.first = 1000;
    range.second = 1234;
    std::string rangeString = createStringFromRange(range, 0);
    TS_ASSERT_EQUALS(rangeString, "1000-234");
  }

  void test_createStringFromRange_paddedRange() {
    std::pair<int, int> range;
    range.first = 1;
    range.second = 1000;
    std::string rangeString = createStringFromRange(range, 4);
    TS_ASSERT_EQUALS(rangeString, "0001-1000");
  }

  void test_createStringFromRange_negativeRange() {
    std::pair<int, int> range;
    range.first = 1000;
    range.second = 10;
    std::string rangeString = createStringFromRange(range, 3);
    TS_ASSERT_EQUALS(rangeString, "010-1000");
  }

  void test_getRunLabel_singleWs() {
    std::string label = getRunLabel(
        MuonWorkspaceCreationHelper::createWorkspaceWithInstrumentandRun(
            "MUSR", 15189));
    TS_ASSERT_EQUALS(label, "MUSR00015189");
  }

  void test_getRunLabel_argus() {
    std::string label = getRunLabel(
        MuonWorkspaceCreationHelper::createWorkspaceWithInstrumentandRun(
            "ARGUS", 26577));
    TS_ASSERT_EQUALS(label, "ARGUS0026577");
  }

  void test_getRunLabel_singleWs_tooBigRunNumber() {
    std::string label = getRunLabel(
        MuonWorkspaceCreationHelper::createWorkspaceWithInstrumentandRun(
            "EMU", 999999999));
    TS_ASSERT_EQUALS(label, "EMU999999999");
  }

  void test_getRunLabel_wsList() {
    std::vector<Workspace_sptr> list;

    for (int i = 15189; i <= 15193; ++i) {
      list.push_back(
          MuonWorkspaceCreationHelper::createWorkspaceWithInstrumentandRun(
              "MUSR", i));
    }

    std::string label = getRunLabel(list);
    TS_ASSERT_EQUALS(label, "MUSR00015189-93");
  }

  void test_getRunLabel_wsList_wrongOrder() {
    std::vector<int> runNumbers{10, 3, 5, 1, 6, 2, 4, 8, 7, 9};
    std::vector<Workspace_sptr> list;

    for (auto it = runNumbers.begin(); it != runNumbers.end(); ++it) {
      list.push_back(
          MuonWorkspaceCreationHelper::createWorkspaceWithInstrumentandRun(
              "EMU", *it));
    }

    std::string label = getRunLabel(list);
    TS_ASSERT_EQUALS(label, "EMU00000001-10");
  }

  void test_getRunLabel_wsList_nonConsecutive() {
    std::vector<int> runNumbers{1, 2, 3, 5, 6, 8, 10, 11, 12, 13, 14};
    std::vector<Workspace_sptr> list;
    for (auto it = runNumbers.begin(); it != runNumbers.end(); it++) {
      list.push_back(
          MuonWorkspaceCreationHelper::createWorkspaceWithInstrumentandRun(
              "EMU", *it));
    }
    std::string label = getRunLabel(list);
    TS_ASSERT_EQUALS(label, "EMU00000001-3, 5-6, 8, 10-4");
  }

  void test_getRunLabel_wsList_nonConsecutive_wrongOrder() {
    std::vector<int> runNumbers{5, 14, 8, 1, 11, 3, 10, 6, 13, 12, 2};
    std::vector<Workspace_sptr> list;
    for (auto it = runNumbers.begin(); it != runNumbers.end(); it++) {
      list.push_back(
          MuonWorkspaceCreationHelper::createWorkspaceWithInstrumentandRun(
              "EMU", *it));
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

  void test_firstPeriod_singleWorkspace() {
    MatrixWorkspace_sptr ws =
        MuonWorkspaceCreationHelper::createCountsWorkspace(2, 10, 0.0);
    Mantid::API::MatrixWorkspace_sptr wsFirstPeriod =
        Mantid::MuonAlgorithmHelper::firstPeriod(ws);
    AnalysisDataService::Instance().addOrReplace("wsSingle", ws);
    TS_ASSERT(wsFirstPeriod);
    TS_ASSERT_EQUALS(wsFirstPeriod->getName(), "wsSingle");
    AnalysisDataService::Instance().clear();
  }

  void test_firstPeriod_groupWorkspace() {
    WorkspaceGroup_sptr ws =
        MuonWorkspaceCreationHelper::createMultiPeriodWorkspaceGroup(
            3, 1, 10, "MuonAnalysis");
    Mantid::API::MatrixWorkspace_sptr wsFirstPeriod =
        Mantid::MuonAlgorithmHelper::firstPeriod(ws);
    TS_ASSERT(wsFirstPeriod);
    TS_ASSERT_EQUALS(wsFirstPeriod->getName(), "MuonDataPeriod_1");
    AnalysisDataService::Instance().clear();
  }

  void test_generateWorkspaceName() {
    Mantid::Muon::DatasetParams params;
    params.instrument = "MUSR";
    params.runs = {15192, 15190, 15189};
    params.itemType = Mantid::Muon::ItemType::Group;
    params.itemName = "fwd";
    params.plotType = Mantid::Muon::PlotType::Counts;
    params.periods = "1+3-2+4";
    params.version = 2;
    const std::string wsName = generateWorkspaceName(params);
    const std::string expected =
        "MUSR00015189-90, 15192; Group; fwd; Counts; 1+3-2+4; #2";
    TS_ASSERT_EQUALS(expected, wsName);
  }

  void test_generateWorkspaceName_noPeriods() {
    Mantid::Muon::DatasetParams params;
    params.instrument = "MUSR";
    params.runs = {15192, 15190, 15189};
    params.itemType = Mantid::Muon::ItemType::Group;
    params.itemName = "fwd";
    params.plotType = Mantid::Muon::PlotType::Counts;
    params.periods = "";
    params.version = 2;
    const std::string wsName = generateWorkspaceName(params);
    const std::string expected =
        "MUSR00015189-90, 15192; Group; fwd; Counts; #2";
    TS_ASSERT_EQUALS(expected, wsName);
  }

  void test_generateWorkspaceName_givenLabel() {
    Mantid::Muon::DatasetParams params;
    params.instrument = "MUSR";
    params.runs = {15192, 15190, 15189};
    params.label = "MyLabel00123"; // should be used in preference to
    params.itemType = Mantid::Muon::ItemType::Group;
    params.itemName = "fwd";
    params.plotType = Mantid::Muon::PlotType::Counts;
    params.periods = "1+3-2+4";
    params.version = 2;
    const std::string wsName = generateWorkspaceName(params);
    const std::string expected =
        "MyLabel00123; Group; fwd; Counts; 1+3-2+4; #2";
    TS_ASSERT_EQUALS(expected, wsName);
  }

  void test_groupWorkspaces_workspacesInGroupAlready() {

    WorkspaceGroup_sptr testGroup =
        MuonWorkspaceCreationHelper::createMultiPeriodWorkspaceGroup(
            5, 2, 10, "TestGroup");
    std::vector<std::string> names = testGroup->getNames();
    groupWorkspaces("TestGroup", names);
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TestGroup"));
    WorkspaceGroup_sptr checkGroup =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("TestGroup");
    TS_ASSERT_EQUALS(checkGroup->getNames().size(), 5);
    TS_ASSERT_EQUALS(checkGroup->getNames()[0], names[0]);
    TS_ASSERT_EQUALS(checkGroup->getNames()[4], names[4]);
    AnalysisDataService::Instance().clear();
  }

  void test_groupWorkspacesOnUngroupedWorkspaces() {

    // Create the workspaces but not the group.
    std::string wsNameStem = "MuonDataPeriod_";
    std::string wsName;
    std::vector<std::string> names;
    for (int numWorkspaces = 1; numWorkspaces < 6; numWorkspaces++) {
      MatrixWorkspace_sptr ws =
          MuonWorkspaceCreationHelper::createCountsWorkspace(2, 10,
                                                             numWorkspaces);
      wsName = wsNameStem + std::to_string(numWorkspaces);
      names.emplace_back(wsName);
      AnalysisDataService::Instance().addOrReplace(wsName, ws);
    }

    groupWorkspaces("TestGroup", names);

    TS_ASSERT(AnalysisDataService::Instance().doesExist("TestGroup"));
    WorkspaceGroup_sptr checkGroup =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("TestGroup");
    TS_ASSERT_EQUALS(checkGroup->getNames().size(), 5);
    TS_ASSERT_EQUALS(checkGroup->getNames()[0], names[0]);
    TS_ASSERT_EQUALS(checkGroup->getNames()[4], names[4]);
    AnalysisDataService::Instance().clear();
  }

  void test_ifGroupWorkspacesOverwritesExistingWS() {

    // Create some workspaces
    std::string wsNameStem = "MuonDataPeriod_";
    std::string wsName;
    std::vector<std::string> names;
    for (int numWorkspaces = 1; numWorkspaces < 6; numWorkspaces++) {
      MatrixWorkspace_sptr ws =
          MuonWorkspaceCreationHelper::createCountsWorkspace(2, 10,
                                                             numWorkspaces);
      wsName = wsNameStem + std::to_string(numWorkspaces);
      names.emplace_back(wsName);
      AnalysisDataService::Instance().addOrReplace(wsName, ws);
    }

    // Create a workspace with the same name as the intended group
    MatrixWorkspace_sptr ws =
        MuonWorkspaceCreationHelper::createCountsWorkspace(2, 10, 1);
    AnalysisDataService::Instance().addOrReplace("TestGroup", ws);

    groupWorkspaces("TestGroup", names);
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TestGroup"));
    WorkspaceGroup_sptr checkGroup =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("TestGroup");
    TS_ASSERT_EQUALS(checkGroup->getNames().size(), 5);
    TS_ASSERT_EQUALS(checkGroup->getNames()[0], names[0]);
    TS_ASSERT_EQUALS(checkGroup->getNames()[4], names[4]);
    AnalysisDataService::Instance().clear();
  }

  void test_checkItemsInSet() {
    std::set<int> set = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::set<int> emptySet = {};
    std::vector<int> itemsIn = {1, 2, 3, 8, 9, 10};
    std::vector<int> itemsNotIn = {11, 100, 0, -1};
    std::vector<int> noItems = {};
    TS_ASSERT(checkItemsInSet(itemsIn, set));
    TS_ASSERT(!checkItemsInSet(itemsNotIn, set));
    TS_ASSERT(checkItemsInSet(noItems, set));
    TS_ASSERT(checkItemsInSet(noItems, set));
    TS_ASSERT(!checkItemsInSet(itemsNotIn, emptySet));
    TS_ASSERT(checkItemsInSet(noItems, emptySet));
  }

  void test_getAllDetectorIDsFromMatrixWorkspace() {
    auto ws = MuonWorkspaceCreationHelper::createCountsWorkspace(5, 3, 0);
    std::set<Mantid::detid_t> ids = getAllDetectorIDsFromMatrixWorkspace(ws);
    for (auto i = 1; i < 6; i++) {
      TS_ASSERT_DIFFERS(ids.find(i), ids.end());
    }
  }

  void test_getAllDetectorIDsWorkspace_Matrix() {
    Workspace_sptr ws =
        MuonWorkspaceCreationHelper::createCountsWorkspace(5, 3, 0);
    std::set<Mantid::detid_t> ids = getAllDetectorIDsFromWorkspace(ws);
    for (auto i = 1; i < 6; i++) {
      TS_ASSERT_DIFFERS(ids.find(i), ids.end());
    }
  }

  void test_getAllDetectorIDsFromGroupWorkspace() {
    auto ws =
        MuonWorkspaceCreationHelper::createWorkspaceGroupConsecutiveDetectorIDs(
            3, 3, 2, "group");
    std::set<Mantid::detid_t> ids = getAllDetectorIDsFromGroupWorkspace(ws);
    for (auto i = 1; i < 10; i++) {
      TS_ASSERT_DIFFERS(ids.find(i), ids.end());
    }
    AnalysisDataService::Instance().clear();
  }

  void test_getAllDetectorIDsWorkspace_Group() {
    Workspace_sptr ws =
        MuonWorkspaceCreationHelper::createWorkspaceGroupConsecutiveDetectorIDs(
            3, 3, 2, "group");
    std::set<Mantid::detid_t> ids = getAllDetectorIDsFromWorkspace(ws);
    for (auto i = 1; i < 10; i++) {
      TS_ASSERT_DIFFERS(ids.find(i), ids.end());
    }
    AnalysisDataService::Instance().clear();
  }

  void test_getAllDetectorIDsFromGroup() {
    // Duplicates are allowed, and no ordering is implied.
    API::Grouping grouping;
    const std::vector<std::string> groups = {"1", "2", "3,4,5", "6-9"};
    grouping.groups = groups;
    std::vector<int> ids = getAllDetectorIDsFromGroup(grouping);
    for (auto i = 1; i < 10; i++) {
      TS_ASSERT_DIFFERS(std::find(ids.begin(), ids.end(), i), ids.end());
    }
    AnalysisDataService::Instance().clear();
  }

  void test_checkGroupDetectorsInWorkspaceTrue() {
    API::Grouping grouping;
    const std::vector<std::string> groups = {"1", "2", "3,4,5", "6-9"};
    grouping.groups = groups;
    Workspace_sptr ws =
        MuonWorkspaceCreationHelper::createWorkspaceGroupConsecutiveDetectorIDs(
            3, 3, 2, "group");
    TS_ASSERT(checkGroupDetectorsInWorkspace(grouping, ws));

    AnalysisDataService::Instance().clear();
  }

  void test_checkGroupDetectorsInWorkspaceFalse() {
    API::Grouping grouping;
    const std::vector<std::string> groups = {"1", "2", "3,4,5", "6-9", "10"};
    grouping.groups = groups;
    Workspace_sptr ws =
        MuonWorkspaceCreationHelper::createWorkspaceGroupConsecutiveDetectorIDs(
            3, 3, 2, "group");
    TS_ASSERT(!checkGroupDetectorsInWorkspace(grouping, ws));

    AnalysisDataService::Instance().clear();
  };

  void test_parseWorkspaceNameParsesCorrectly() {
    const std::string workspaceName =
        "MUSR00015189-90, 15192; Group; fwd; Counts; 1+3-2+4; #2";
    const std::vector<int> expectedRuns{15189, 15190, 15192};
    const auto params = parseWorkspaceName(workspaceName);
    TS_ASSERT_EQUALS(params.instrument, "MUSR");
    TS_ASSERT_EQUALS(params.runs, expectedRuns);
    TS_ASSERT_EQUALS(params.label, "MUSR00015189-90, 15192");
    TS_ASSERT_EQUALS(params.itemType, Muon::ItemType::Group);
    TS_ASSERT_EQUALS(params.itemName, "fwd");
    TS_ASSERT_EQUALS(params.plotType, Muon::PlotType::Counts);
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
    TS_ASSERT_EQUALS(params.itemType, Muon::ItemType::Group);
    TS_ASSERT_EQUALS(params.itemName, "fwd");
    TS_ASSERT_EQUALS(params.plotType, Muon::PlotType::Counts);
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

  void test_checkValidPair_throws_if_incorrect_name_format() {
    const std::string validWorkspaceName =
        "MUSR00015189; Group; fwd; Counts; 1+2; #1";
    std::vector<std::string> invalidWorkspaceNames = {
        "MUSR00015189; Soup; fwd; Counts; 1+2; #1",
        "MUSR00015189; Group; fwd; Couts; 1+2; #1", "MuonGroupWorkspace"};

    for (auto &&invalidName : invalidWorkspaceNames)
      TS_ASSERT_THROWS(checkValidPair(validWorkspaceName, invalidName),
                       const std::invalid_argument &);
  }

  void test_checkValidPair_throws_if_ItemType_Asym() {
    const std::string validWorkspaceName =
        "EMU000015189; Group; fwd; Counts; 1+2; #1";
    const std::string invalidWorkspaceName =
        "EMU000015189; Group; fwd; Asym; 1+2; #1";
    TS_ASSERT_THROWS(checkValidPair(validWorkspaceName, invalidWorkspaceName),
                     const std::invalid_argument &);
  }

  void test_checkValidPair_throws_if_different_instruments() {
    const std::string validWorkspaceName =
        "EMU000015189; Group; fwd; Counts; 1+2; #1";
    const std::string invalidWorkspaceName =
        "MUSR00015189; Group; fwd; Counts; 1+2; #1";
    TS_ASSERT_THROWS(checkValidPair(validWorkspaceName, invalidWorkspaceName),
                     const std::invalid_argument &);
  }

  void test_checkValidPair_does_not_throw_if_same_group() {
    const std::string validWorkspaceName =
        "EMU000015189; Group; fwd; Counts; 1+2; #1";
    const std::string invalidWorkspaceName =
        "EMU000015189; Group; fwd; Counts; 1+2; #1";
    TS_ASSERT_THROWS(checkValidPair(validWorkspaceName, invalidWorkspaceName),
                     const std::invalid_argument &);
  }

  void test_checkValidGroupPairName_invalid_names() {
    std::vector<std::string> badNames = {"",      "_name", "name_", "name;",
                                         "#name", "Group", "Pair"};
    for (auto &&badName : badNames) {
      TS_ASSERT(!MuonAlgorithmHelper::checkValidGroupPairName(badName));
    }
  }

  void test_checkValidGroupPairName_valid_names() {
    std::vector<std::string> goodNames = {"name", "group", "pair", "123"};
    for (auto &&goodName : goodNames) {
      TS_ASSERT(MuonAlgorithmHelper::checkValidGroupPairName(goodName));
    }
  }
};

#endif /* MANTID_MUON_MUONALGOIRTHMHELPER_H_ */
