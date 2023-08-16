// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAlgorithms/UnGroupWorkspace.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

class UnGroupWorkspaceTest : public CxxTest::TestSuite {
public:
  void testName() {
    Mantid::Algorithms::UnGroupWorkspace alg;
    TS_ASSERT_EQUALS(alg.name(), "UnGroupWorkspace");
  }

  void testVersion() {
    Mantid::Algorithms::UnGroupWorkspace alg;
    TS_ASSERT_EQUALS(alg.version(), 1);
  }

  void testInit() {
    Mantid::Algorithms::UnGroupWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    const auto props = alg.getProperties();
    TS_ASSERT_EQUALS(props.size(), 1);

    TS_ASSERT_EQUALS(props[0]->name(), "InputWorkspace");
    TS_ASSERT(props[0]->isDefault());
  }

  void test_Exec_UnGroup_With_GroupWorkspace_With_Multiple_Members_Removes_Group_Leaving_Members() {
    std::vector<std::string> members(2, "test_Exec_UnGroup_With_GroupWorkspace_"
                                        "With_Multiple_Members_Removes_Group_"
                                        "Leaving_Members_1");
    members[1] = "test_Exec_UnGroup_With_GroupWorkspace_With_Multiple_Members_"
                 "Removes_Group_Leaving_Members_2";
    const std::string groupName = "test_Exec_UnGroup_With_GroupWorkspace_With_"
                                  "Multiple_Members_Removes_Group_Leaving_"
                                  "Members_Group";
    addTestGroupToADS(groupName, members);

    Mantid::Algorithms::UnGroupWorkspace alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", groupName));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    auto &ads = Mantid::API::AnalysisDataService::Instance();
    // Group no longer exists
    TS_ASSERT_EQUALS(false, ads.doesExist(groupName));
    // but members do
    TS_ASSERT_EQUALS(true, ads.doesExist(members[0]));
    TS_ASSERT_EQUALS(true, ads.doesExist(members[1]));

    removeFromADS(members);
  }

  void test_Exec_UnGroup_With_GroupWorkspace_With_Single_Member_Removes_Group_Leaving_Members() {
    std::vector<std::string> members(1, "test_Exec_UnGroup_With_GroupWorkspace_"
                                        "With_Single_Member_Removes_Group_"
                                        "Leaving_Members_1");
    const std::string groupName = "test_Exec_UnGroup_With_GroupWorkspace_With_"
                                  "Single_Member_Removes_Group_Leaving_Members_"
                                  "Group";
    addTestGroupToADS(groupName, members);

    Mantid::Algorithms::UnGroupWorkspace alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", groupName));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    auto &ads = Mantid::API::AnalysisDataService::Instance();
    // Group no longer exists
    TS_ASSERT_EQUALS(false, ads.doesExist(groupName));
    // but member does
    TS_ASSERT_EQUALS(true, ads.doesExist(members[0]));

    removeFromADS(members);
  }

  void test_Exec_With_NonGroupWorkspace_Throws_Error_On_Setting_Property() {
    const std::string wsName("test_Exec_With_NonGroupWorkspace_Throws_Error");
    addTestMatrixWorkspaceToADS(wsName);

    Mantid::Algorithms::UnGroupWorkspace alg;
    alg.initialize();
    TS_ASSERT_THROWS(alg.setProperty("InputWorkspace", wsName), const std::invalid_argument &);

    removeFromADS(std::vector<std::string>(1, wsName));
  }

  void test_Leaving_Members_Are_Given_Algorithm_History() {
    std::vector<std::string> members{"test_Leaving_Members_Are_Given_Algorithm_History_Members_1",
                                     "test_Leaving_Members_Are_Given_Algorithm_History_Members_2"};
    const std::string groupName = "test_Leaving_Members_Are_Given_Algorithm_History_Group";
    addTestGroupToADS(groupName, members);

    Mantid::Algorithms::UnGroupWorkspace alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", groupName));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    auto &ads = Mantid::API::AnalysisDataService::Instance();
    const auto m1 = ads.retrieve(members[0]);
    const auto m2 = ads.retrieve(members[1]);

    TS_ASSERT(isAlgorithmInHistory(m1));
    TS_ASSERT(isAlgorithmInHistory(m2));

    removeFromADS(members);
  }

private:
  void addTestGroupToADS(const std::string &name, const std::vector<std::string> &inputs) {
    auto newGroup = std::make_shared<Mantid::API::WorkspaceGroup>();

    auto &ads = Mantid::API::AnalysisDataService::Instance();
    for (const auto &input : inputs) {
      auto ws = addTestMatrixWorkspaceToADS(input);
      newGroup->addWorkspace(ws);
    }
    ads.add(name, newGroup);
  }

  Mantid::API::MatrixWorkspace_sptr addTestMatrixWorkspaceToADS(const std::string &name) {
    auto &ads = Mantid::API::AnalysisDataService::Instance();
    auto ws = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    ads.add(name, ws);
    return ws;
  }

  void removeFromADS(const std::vector<std::string> &members) {
    auto &ads = Mantid::API::AnalysisDataService::Instance();

    for (const auto &member : members) {
      if (ads.doesExist(member))
        ads.remove(member);
    }
  }

  bool isAlgorithmInHistory(const Mantid::API::Workspace_sptr &ws) {
    auto wsHistories = ws->getHistory().getAlgorithmHistories();
    auto iter = std::find_if(wsHistories.cbegin(), wsHistories.cend(),
                             [](auto const &algHistory) { return algHistory->name() == "UnGroupWorkspace"; });
    return iter != wsHistories.cend();
  }
};
