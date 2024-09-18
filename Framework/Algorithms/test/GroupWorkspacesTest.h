// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include <algorithm>

class GroupWorkspacesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GroupWorkspacesTest *createSuite() { return new GroupWorkspacesTest(); }

  static void destroySuite(GroupWorkspacesTest *suite) { delete suite; }

  //========================= Success Cases
  //===========================================
  void testName() {
    Mantid::Algorithms::GroupWorkspaces alg;
    TS_ASSERT_EQUALS(alg.name(), "GroupWorkspaces");
  }

  void testVersion() {
    Mantid::Algorithms::GroupWorkspaces alg;
    TS_ASSERT_EQUALS(alg.version(), 1);
  }

  void testInit() {
    using Mantid::Algorithms::GroupWorkspaces;
    using Mantid::API::WorkspaceGroup;
    using Mantid::API::WorkspaceProperty;

    GroupWorkspaces alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    const auto &props = alg.getProperties();
    TS_ASSERT_EQUALS(props.size(), 3);

    TS_ASSERT_EQUALS(props[0]->name(), "InputWorkspaces");
    TS_ASSERT(props[0]->isDefault());

    TS_ASSERT_EQUALS(props[1]->name(), "GlobExpression");
    TS_ASSERT(props[1]->isDefault());

    TS_ASSERT_EQUALS(props[2]->name(), "OutputWorkspace");
    TS_ASSERT(props[2]->isDefault());
    TS_ASSERT(dynamic_cast<WorkspaceProperty<WorkspaceGroup> *>(props[2]));
  }

  void test_Exec_With_Single_Workspace_Succeeds() {
    std::vector<std::string> inputs(1, "test_Exec_With_Two_Workspaces_Of_Same_Type_Succeeds");
    addTestMatrixWorkspacesToADS(inputs);

    const std::string groupName = inputs[0] + "_grouped";
    TS_ASSERT_THROWS_NOTHING(runAlgorithm(inputs, groupName));

    checkGroupExistsWithMembers(groupName, inputs);
    removeFromADS(groupName, inputs);
  }

  void test_Exec_With_Two_Workspaces_Of_Same_Type_Succeeds() {
    std::vector<std::string> inputs(2, "test_Exec_With_Two_Workspaces_Of_Same_Type_Succeeds_1");
    inputs[1] = "test_Exec_With_Two_Workspaces_Of_Same_Type_Succeeds_2";
    addTestMatrixWorkspacesToADS(inputs);

    const std::string groupName = "test_Exec_With_Two_Workspaces_Of_Same_Type_Succeeds_grouped";
    TS_ASSERT_THROWS_NOTHING(runAlgorithm(inputs, groupName));

    checkGroupExistsWithMembers(groupName, inputs);
    removeFromADS(groupName, inputs);
  }

  void test_Exec_With_Three_Workspaces_Of_Same_Type_Succeeds() {
    std::vector<std::string> inputs(3, "test_Exec_With_Two_Workspaces_Of_Same_Type_Succeeds_1");
    inputs[1] = "test_Exec_With_Two_Workspaces_Of_Same_Type_Succeeds_2";
    inputs[2] = "test_Exec_With_Two_Workspaces_Of_Same_Type_Succeeds_3";
    addTestMatrixWorkspacesToADS(inputs);

    const std::string groupName = "test_Exec_With_Three_Workspaces_Of_Same_Type_Succeeds_grouped";
    TS_ASSERT_THROWS_NOTHING(runAlgorithm(inputs, groupName));

    checkGroupExistsWithMembers(groupName, inputs);
    removeFromADS(groupName, inputs);
  }

  void test_Exec_With_Single_Workspace_And_WorkspaceGroup_Unrolls_Group_And_Adds_Both_Together() {
    // Create Group
    std::vector<std::string> groupNames(3, "test_Exec_With_Single_Workspace_"
                                           "And_WorkspaceGroup_Unrolls_Group_"
                                           "And_Adds_Both_Together_1");
    groupNames[1] = "test_Exec_With_Single_Workspace_And_WorkspaceGroup_"
                    "Unrolls_Group_And_Adds_Both_Together_2";
    groupNames[2] = "test_Exec_With_Single_Workspace_And_WorkspaceGroup_"
                    "Unrolls_Group_And_Adds_Both_Together_3";
    addTestMatrixWorkspacesToADS(groupNames);
    const std::string inputGroupName = "test_Exec_With_Single_Workspace_And_"
                                       "WorkspaceGroup_Unrolls_Group_And_Adds_"
                                       "Both_Together";
    runAlgorithm(groupNames, inputGroupName);
    TS_ASSERT_EQUALS(true, Mantid::API::AnalysisDataService::Instance().doesExist(inputGroupName));

    // Single workspace
    std::string singleWS = "test_Exec_With_Single_Workspace_And_WorkspaceGroup_"
                           "Unrolls_Group_And_Adds_Both_Together_4";
    addTestMatrixWorkspaceToADS(singleWS);

    // Single WS + group
    std::vector<std::string> inputNames(2, inputGroupName);
    inputNames[1] = singleWS;
    const std::string finalGroupName = "test_Exec_With_Single_Workspace_And_"
                                       "WorkspaceGroup_Unrolls_Group_And_Adds_"
                                       "Both_Together_FinalGroup";
    runAlgorithm(inputNames, finalGroupName);

    // Assert
    groupNames.reserve(4);
    groupNames.emplace_back(singleWS);
    checkGroupExistsWithMembers(finalGroupName, groupNames);
    TS_ASSERT_EQUALS(false, Mantid::API::AnalysisDataService::Instance().doesExist(inputGroupName));
    removeFromADS(finalGroupName, groupNames);
  }

  void test_Exec_With_Mixture_Of_TableWorkspace_And_Other_Workspace_Type_Succeeds() {
    std::string matrixWS = "test_Exec_With_Mixture_Of_WorkspaceTypes_Not_"
                           "Including_TableWorkspace_Throws_Error_Matrix";
    addTestMatrixWorkspaceToADS(matrixWS);
    std::string tableWS = "test_Exec_With_Mixture_Of_WorkspaceTypes_Not_"
                          "Including_TableWorkspace_Throws_Error_Table";
    addTestTableWorkspaceToADS(tableWS);

    std::vector<std::string> inputs(2, matrixWS);
    inputs[1] = tableWS;
    const std::string groupName = "test_Exec_With_Mixture_Of_TableWorkspace_"
                                  "And_Other_Workspace_Type_Succeeds_Group";
    TS_ASSERT_THROWS_NOTHING(runAlgorithm(inputs, groupName));

    checkGroupExistsWithMembers(groupName, inputs);
    removeFromADS(groupName, inputs);
  }

  void test_Exec_With_Mixture_Of_WorkspaceTypes_Not_Including_TableWorkspace_Succeeds() {
    std::string matrixWS = "test_Exec_With_Mixture_Of_WorkspaceTypes_Not_"
                           "Including_TableWorkspace_Succeeds_Matrix";
    addTestMatrixWorkspaceToADS(matrixWS);
    std::string eventWS = "test_Exec_With_Mixture_Of_WorkspaceTypes_Not_"
                          "Including_TableWorkspace_Succeeds_Event";
    addTestEventWorkspaceToADS(eventWS);

    std::vector<std::string> inputs(2, matrixWS);
    inputs[1] = eventWS;
    const std::string groupName = "test_Exec_With_Mixture_Of_WorkspaceTypes_"
                                  "Not_Including_TableWorkspace_Throws_Error_"
                                  "Group";
    TS_ASSERT_THROWS_NOTHING(runAlgorithm(inputs, groupName));

    checkGroupExistsWithMembers(groupName, inputs);
    removeFromADS(groupName, inputs);
  }

  void test_GlobExpression_Star_Succeeds() {
    std::vector<std::string> inputs{"test_name_1", "test_name_2", "test_name_20"};
    addTestMatrixWorkspacesToADS(inputs);
    std::string glob{"test_name_*"};
    std::string groupName{"test_name_output"};

    TS_ASSERT_THROWS_NOTHING(runAlgorithm(glob, groupName));

    checkGroupExistsWithMembers(groupName, inputs);

    removeFromADS(groupName, inputs);
  }

  void test_GlobExpression_Question_Succeeds() {
    std::vector<std::string> inputs{"test_name_1", "test_name_2", "test_name_20"};
    addTestMatrixWorkspacesToADS(inputs);
    std::string glob{"test_name_?"};
    std::string groupName{"test_name_output"};

    TS_ASSERT_THROWS_NOTHING(runAlgorithm(glob, groupName));

    std::vector<std::string> group_members(inputs.begin(), inputs.begin() + 2);

    checkGroupExistsWithMembers(groupName, group_members);

    removeFromADS(groupName, inputs);
  }

  void test_GlobExpression_Brackets_Succeeds() {

    std::vector<std::string> inputs{"test_name_1", "test_name_2", "test_name_3"};
    addTestMatrixWorkspacesToADS(inputs);
    std::string glob{"test_name_[0-2]"};
    std::string groupName{"test_name_output"};

    TS_ASSERT_THROWS_NOTHING(runAlgorithm(glob, groupName));

    checkGroupExistsWithMembers(groupName, {inputs[0], inputs[1]});

    removeFromADS(groupName, inputs);
  }

  void test_GlobExpression_Brackets_Succeeds_2() {

    std::vector<std::string> inputs{"test_name_1", "test_name_2", "test_name_3"};
    addTestMatrixWorkspacesToADS(inputs);
    std::string glob{"test_name_[0-3]"};
    std::string groupName{"test_name_output"};

    TS_ASSERT_THROWS_NOTHING(runAlgorithm(glob, groupName));

    checkGroupExistsWithMembers(groupName, inputs);

    removeFromADS(groupName, inputs);
  }

  void test_GlobExpression_List_And_Glob_Succeeds() {

    std::vector<std::string> inputs{"test_name_1", "test_name_2", "test_name_3"};
    addTestMatrixWorkspacesToADS(inputs);
    std::string glob{"test_name_[0-2]"};
    std::string groupName{"test_name_output"};

    TS_ASSERT_THROWS_NOTHING(runAlgorithm({inputs[2]}, glob, groupName));

    checkGroupExistsWithMembers(groupName, inputs);

    removeFromADS(groupName, inputs);
  }

  void test_GlobExpression_List_And_Glob_Succeeds_2() {

    std::vector<std::string> inputs{"test_name_1", "test_name_2", "test_name_3"};
    addTestMatrixWorkspacesToADS(inputs);
    std::string glob{"test_name_[0-3]"};
    std::string groupName{"test_name_output"};

    TS_ASSERT_THROWS_NOTHING(runAlgorithm({inputs[0]}, glob, groupName));

    checkGroupExistsWithMembers(groupName, inputs);

    removeFromADS(groupName, inputs);
  }

  void test_GlobExpression_EscapedCharacter_Succeeds() {
    std::vector<std::string> inputs{"test_name_1", "test_?_2", "test_n_3"};
    addTestMatrixWorkspacesToADS(inputs);
    std::string glob{"test_\\?_?"};
    std::string groupName{"test_name_output"};

    TS_ASSERT_THROWS_NOTHING(runAlgorithm(glob, groupName));

    checkGroupExistsWithMembers(groupName, {inputs[1]});

    removeFromADS(groupName, inputs);
  }

  void test_GlobExpression_EscapedCharacter_Succeeds_2() {
    std::vector<std::string> inputs{"test_name_1", "test_name_2", "test_name_3"};
    addTestMatrixWorkspacesToADS(inputs);
    std::string glob{"test_name_[2-3]"};
    std::string groupName{"test_name_output"};

    TS_ASSERT_THROWS_NOTHING(runAlgorithm({inputs[0]}, glob, groupName));

    checkGroupExistsWithMembers(groupName, inputs);

    removeFromADS(groupName, inputs);
  }

  //========================= Failure Cases
  //===========================================

  void test_Exec_With_Input_That_Is_Not_In_ADS_Fails() {
    std::vector<std::string> inputs(2, "test_Exec_With_Input_That_Is_Not_In_ADS_Fails_1");
    inputs[1] = "test_Exec_With_Two_Workspaces_Of_Same_Type_Succeeds_2";
    addTestMatrixWorkspacesToADS(inputs);

    // Add another to the input list
    inputs.reserve(3);
    inputs.emplace_back("test_Exec_With_Two_Workspaces_Of_Same_Type_Succeeds_3");

    const std::string groupName = "test_Exec_With_Input_That_Is_Not_In_ADS_Fails";
    runAlgorithm(inputs, groupName, true);

    TS_ASSERT_EQUALS(false, Mantid::API::AnalysisDataService::Instance().doesExist(groupName));
    removeFromADS("", inputs);
  }

  void test_GlobExpression_Mismatched_Brackets_Fails() {

    std::vector<std::string> inputs{"test_name_1", "test_name_2", "test_name_3"};
    addTestMatrixWorkspacesToADS(inputs);
    std::string glob{"test_name_[1-3]]"};
    std::string groupName{"test_name_output"};

    runAlgorithm(glob, groupName, true);

    TS_ASSERT_EQUALS(false, Mantid::API::AnalysisDataService::Instance().doesExist(groupName));

    removeFromADS("", inputs);
  }

  void test_GlobExpression_Fails() {

    std::vector<std::string> inputs{"test_name_1", "test_name_2", "test_name_3"};
    addTestMatrixWorkspacesToADS(inputs);
    std::string glob{"test_name_1"};
    std::string groupName{"test_name_output"};

    runAlgorithm(glob, groupName, true);

    TS_ASSERT_EQUALS(false, Mantid::API::AnalysisDataService::Instance().doesExist(groupName));

    removeFromADS("", inputs);
  }

  void test_GlobExpression_Empty_Output_Fails() {

    std::vector<std::string> inputs{"test_name_1", "test_name_2", "test_name_3"};
    addTestMatrixWorkspacesToADS(inputs);
    std::string glob{"test_name_[!1-3]"};
    std::string groupName{"test_name_output"};

    runAlgorithm(glob, groupName, true);

    TS_ASSERT_EQUALS(false, Mantid::API::AnalysisDataService::Instance().doesExist(groupName));

    removeFromADS("", inputs);
  }

  void test_GlobExpression_Empty_Output_Fails_2() {

    std::vector<std::string> inputs{"ws1"};
    addTestMatrixWorkspacesToADS(inputs);
    std::string glob{"ws\\*"};
    std::string groupName{"test_name_output"};

    runAlgorithm(glob, groupName, true);

    TS_ASSERT_EQUALS(false, Mantid::API::AnalysisDataService::Instance().doesExist(groupName));

    removeFromADS("", inputs);
  }

  void test_OutputGroup_can_supplant_a_workspace_with_the_same_name_in_the_ADS() {
    std::vector<std::string> inputs{"ws1", "ws2"};
    addTestMatrixWorkspacesToADS(inputs);
    Mantid::Algorithms::GroupWorkspaces alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", inputs));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "ws1"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    auto &ads = Mantid::API::AnalysisDataService::Instance();
    TS_ASSERT(ads.doesExist("ws1"));
    TS_ASSERT(ads.doesExist("ws2"));
    TS_ASSERT(ads.doesExist("ws1_1"));
    removeFromADS("ws1", {"ws1_1", "ws2"});
  }

  void test_OutputWorkspace_can_overwrite_input_group_workspaces() {
    const std::vector<std::string> inputs{"ws1", "ws2", "ws3"};
    addTestMatrixWorkspacesToADS(inputs);
    const std::vector<std::string> group1{"ws1", "ws2"};
    runAlgorithm(group1, "Group", false);

    const std::vector<std::string> group2{"Group", "ws3"};
    runAlgorithm(group2, "Group", false);

    checkGroupExistsWithMembers("Group", {"ws1", "ws2", "ws3"});
    removeFromADS("Group", inputs);
  }

private:
  void addTestMatrixWorkspacesToADS(const std::vector<std::string> &inputs) {
    for (const auto &input : inputs) {
      addTestMatrixWorkspaceToADS(input);
    }
  }

  void addTestMatrixWorkspaceToADS(const std::string &name) {
    auto &ads = Mantid::API::AnalysisDataService::Instance();
    ads.add(name, WorkspaceCreationHelper::create2DWorkspace(1, 1));
  }

  void addTestEventWorkspaceToADS(const std::string &name) {
    auto &ads = Mantid::API::AnalysisDataService::Instance();
    ads.add(name, WorkspaceCreationHelper::createEventWorkspace());
  }

  void addTestTableWorkspaceToADS(const std::string &name) {
    auto &ads = Mantid::API::AnalysisDataService::Instance();
    auto ws = Mantid::API::WorkspaceFactory::Instance().createTable();
    ads.add(name, ws);
  }

  void runAlgorithm(const std::vector<std::string> &inputs, const std::string &outputWorkspace,
                    bool errorExpected = false) {
    Mantid::Algorithms::GroupWorkspaces alg;
    alg.initialize();
    alg.setRethrows(true);

    if (errorExpected) {
      TS_ASSERT_THROWS_ANYTHING(alg.setProperty("InputWorkspaces", inputs));
    } else {
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", inputs));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", outputWorkspace));
      alg.execute();
      TS_ASSERT(alg.isExecuted());
    }
  }

  void runAlgorithm(const std::string &globExpression, const std::string &outputWorkspace, bool errorExpected = false) {
    Mantid::Algorithms::GroupWorkspaces alg;
    alg.initialize();
    alg.setRethrows(true);

    if (errorExpected) {
      alg.setProperty("GlobExpression", globExpression);
      alg.setProperty("OutputWorkspace", outputWorkspace);
      TS_ASSERT_THROWS_ANYTHING(alg.execute());
    } else {
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("GlobExpression", globExpression));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", outputWorkspace));
      alg.execute();
      TS_ASSERT(alg.isExecuted());
    }
  }

  void runAlgorithm(const std::vector<std::string> &inputs, const std::string &globExpression,
                    const std::string &outputWorkspace) {
    Mantid::Algorithms::GroupWorkspaces alg;
    alg.initialize();
    alg.setRethrows(true);

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", inputs));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GlobExpression", globExpression));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", outputWorkspace));
    alg.execute();
    TS_ASSERT(alg.isExecuted());
  }
  void checkGroupExistsWithMembers(const std::string &groupName, std::vector<std::string> expectedMembers) {
    using namespace Mantid::API;

    auto &ads = AnalysisDataService::Instance();
    WorkspaceGroup_sptr result;
    TS_ASSERT_THROWS_NOTHING(result = ads.retrieveWS<WorkspaceGroup>(groupName));
    std::vector<std::string> grpVec = result->getNames();
    TS_ASSERT_EQUALS(expectedMembers.size(), grpVec.size());

    std::sort(expectedMembers.begin(), expectedMembers.end());
    std::sort(grpVec.begin(), grpVec.end());

    if (expectedMembers.size() == grpVec.size()) {
      for (size_t i = 0; i < expectedMembers.size(); ++i) {
        TS_ASSERT_EQUALS(expectedMembers[i], grpVec[i]);
      }
    }
  }

  void removeFromADS(const std::string &groupName, const std::vector<std::string> &members) {
    auto &ads = Mantid::API::AnalysisDataService::Instance();

    for (const auto &member : members) {
      if (ads.doesExist(member))
        ads.remove(member);
    }
    if (ads.doesExist(groupName))
      ads.remove(groupName);
  }
};
