// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include <memory>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace {
class MockWorkspace : public Workspace {
  const std::string id() const override { return "MockWorkspace"; }
  const std::string toString() const override { return ""; }
  size_t getMemorySize() const override { return 1; }

private:
  MockWorkspace *doClone() const override { throw std::runtime_error("Cloning of MockWorkspace is not implemented."); }
  MockWorkspace *doCloneEmpty() const override {
    throw std::runtime_error("Cloning of MockWorkspace is not implemented.");
  }
};
using MockWorkspace_sptr = std::shared_ptr<MockWorkspace>;
} // namespace

class AnalysisDataServiceTest : public CxxTest::TestSuite {
private:
  AnalysisDataServiceImpl &ads;

public:
  static AnalysisDataServiceTest *createSuite() { return new AnalysisDataServiceTest(); }
  static void destroySuite(AnalysisDataServiceTest *suite) { delete suite; }

  AnalysisDataServiceTest() : ads(AnalysisDataService::Instance()) {}

  void setUp() override { ads.clear(); }

  void testValidateNameReturnsEmptyStringForValidPythonNames() {
    TS_ASSERT_EQUALS(ads.validateName("a"), "");
    TS_ASSERT_EQUALS(ads.validateName("Z"), "");
    TS_ASSERT_EQUALS(ads.validateName("camelCase"), "");
    TS_ASSERT_EQUALS(ads.validateName("PascalCase"), "");
    TS_ASSERT_EQUALS(ads.validateName("has_Underscore"), "");
    TS_ASSERT_EQUALS(ads.validateName("_starts_with_underscore"), "");
    TS_ASSERT_EQUALS(ads.validateName("ends_with_underscore_"), "");
    TS_ASSERT_EQUALS(ads.validateName("__l_o_t_s__o_f__u_n_d_e_r_s_c_o_r_e_s__"), "");
    TS_ASSERT_EQUALS(ads.validateName("alllowercase"), "");
    TS_ASSERT_EQUALS(ads.validateName("ALLUPPERCASE"), "");
    TS_ASSERT_EQUALS(ads.validateName("Numb3rs"), "");
    TS_ASSERT_EQUALS(ads.validateName("_m0r3_numb3r5"), "");
    TS_ASSERT_EQUALS(ads.validateName("_"), "");
    TS_ASSERT_EQUALS(ads.validateName("___"), "");
  }

  void testValidateNameReturnsErrorStringForNamesContainingIllegalCharacters() {
    const std::string illegalChars = " +-/*\\%<>&|^~=!@()[]{},:.`$'\"?";
    for (const char &illegalChar : illegalChars) {
      const std::string illegalName = std::string("variable_name") + illegalChar;
      std::string expectedError =
          "Invalid object name '" + illegalName +
          "'. Names must start with a letter or underscore and contain only alpha-numeric characters and underscores.";
      TS_ASSERT_EQUALS(ads.validateName(illegalName), expectedError);
    }
  }

  void testValidateNameReturnsErrorStringForNamesStartingWithNumbers() {
    TS_ASSERT_EQUALS(ads.validateName("7dodgy_name"),
                     "Invalid object name '7dodgy_name'. Names must start with a letter or underscore and contain only "
                     "alpha-numeric characters and underscores.");
  }

  void test_Retrieve_Case_Insensitive() {
    addToADS("z");
    TS_ASSERT_THROWS_NOTHING(ads.retrieve("z"));
    TS_ASSERT_THROWS_NOTHING(ads.retrieve("Z"));

    ads.remove("Z");
    TS_ASSERT_THROWS(ads.retrieve("z"), const Exception::NotFoundError &);
  }

  void test_retrieveWorkspaces_with_empty_list_returns_empty_list() {
    std::vector<Workspace_sptr> empty;
    TS_ASSERT_EQUALS(empty, ads.retrieveWorkspaces({}));
  }

  void test_retrieveWorkspaces_with_all_missing_items_throws_exception() {
    TS_ASSERT_THROWS(ads.retrieveWorkspaces({"a"}), const Exception::NotFoundError &);
    TS_ASSERT_THROWS(ads.retrieveWorkspaces({"a", "b"}), const Exception::NotFoundError &);
  }

  void test_retrieveWorkspaces_with_some_missing_items_throws_exception() {
    const std::string name("test_some_missing_items");
    addToADS(name);
    TS_ASSERT_THROWS(ads.retrieveWorkspaces({"a", "b"}), const Exception::NotFoundError &);
    ads.remove(name);
  }

  void test_retrieveWorkspaces_with_all_items_present_and_no_group_unrolling() {
    const std::vector<std::string> names{"test_all_items_present_1", "test_all_items_present_2"};
    std::vector<Workspace_sptr> expected;
    for (const auto &name : names) {
      expected.emplace_back(addToADS(name));
    }
    std::vector<Workspace_sptr> items;
    TS_ASSERT_THROWS_NOTHING(items = ads.retrieveWorkspaces(names));
    TS_ASSERT_EQUALS(expected, expected);

    for (const auto &name : names) {
      ads.remove(name);
    }
  }

  void test_retrieveWorkspaces_with_group_unrolling() {
    const std::vector<std::string> names{"test_all_items_present_unroll_1", "test_all_items_present_unroll_2"};
    std::vector<Workspace_sptr> expected;
    expected.emplace_back(addToADS(names[0]));
    const size_t nitems{4u};
    WorkspaceGroup_sptr groupWS{addGroupToADS(names[1], nitems)};
    for (auto i = 0u; i < nitems; ++i) {
      expected.emplace_back(groupWS->getItem(i));
    }
    std::vector<Workspace_sptr> items;
    TS_ASSERT_THROWS_NOTHING(items = ads.retrieveWorkspaces(names, true));
    TS_ASSERT_EQUALS(expected.size(), items.size());
    TS_ASSERT_EQUALS(expected, items);

    for (const auto &name : names) {
      ads.remove(name);
    }
  }

  void test_Add_With_Name_That_Has_No_Special_Chars_Is_Accpeted() {
    const std::string name = "MySpace";
    TS_ASSERT_THROWS_NOTHING(addToADS(name));
    TS_ASSERT(ads.doesExist(name));
    TS_ASSERT_THROWS_NOTHING(ads.remove(name));
  }

  void test_Adding_A_Second_Item_Of_Same_Name_Throws_Runtime_Error() {
    const std::string name = "SameName";
    TS_ASSERT_THROWS_NOTHING(addToADS(name));
    // Adding again will throw
    TS_ASSERT_THROWS(addToADS(name), const std::runtime_error &);
    TS_ASSERT_THROWS_NOTHING(ads.remove(name));
  }

  void testAddWithInvalidName() { this->doAddingOnInvalidNameTests(false /*Don't use replace*/); }

  void testAddOrReplaceWithInvalidName() { this->doAddingOnInvalidNameTests(true /*Use replace*/); }

  void test_AddOrReplace_Does_Not_Throw_When_Adding_Object_That_Has_A_Name_That_Already_Exists() {
    const std::string name("MySpaceAddOrReplace");
    TS_ASSERT_THROWS_NOTHING(addOrReplaceToADS(name));
    TS_ASSERT_THROWS(addToADS(name), const std::runtime_error &);
    TS_ASSERT_THROWS_NOTHING(addOrReplaceToADS(name));
    TS_ASSERT_THROWS_NOTHING(ads.remove(name));
  }

  void testRemove() {
    const std::string name("MySpace");
    addToADS(name);
    TS_ASSERT_THROWS_NOTHING(ads.remove(name));
    TS_ASSERT_THROWS(ads.retrieve(name), const std::runtime_error &);
    // Remove should not throw but give a warning in the log file, changed by
    // LCC 05/2008
    TS_ASSERT_THROWS_NOTHING(ads.remove("ttttt"));
    TS_ASSERT(!ads.remove("ttttt"));
  }

  void testRemoveReturnsTheWorkspaceSptr() {
    const std::string name("MySpace");
    addToADS(name);
    auto const workspace = ads.remove(name);
    TS_ASSERT(workspace);
    TS_ASSERT_EQUALS("MockWorkspace", workspace->id());

    TS_ASSERT_THROWS_NOTHING(ads.remove("ttttt"));
    // Should return a nullptr as the workspace does not exist
    TS_ASSERT(!ads.remove("ttttt"));
  }

  void testRetrieve() {
    const std::string name("MySpace");
    Workspace_sptr work = addToADS(name);
    Workspace_sptr workBack;
    TS_ASSERT_THROWS_NOTHING(workBack = ads.retrieve(name));
    TS_ASSERT_EQUALS(work, workBack);
  }

  void testRetrieveWS() {
    const std::string name("MySpace");
    Workspace_sptr work = addToADS(name);
    MockWorkspace_sptr workBack;
    TS_ASSERT_THROWS_NOTHING(workBack = ads.retrieveWS<MockWorkspace>(name));
    TS_ASSERT_EQUALS(work, workBack);
  }

  void test_Rename() {
    const std::string oldName = "Old";
    const std::string newName = "New";
    Workspace_sptr work = addToADS(oldName);
    TS_ASSERT_THROWS_NOTHING(ads.rename(oldName, newName));
    auto workBack = ads.retrieve(newName);
    TS_ASSERT_EQUALS(work, workBack);
    TS_ASSERT(!ads.doesExist(oldName));
    TS_ASSERT(ads.doesExist(newName));
  }

  void test_Rename_Overwrites_Existing_WS() {
    const std::string oldName = "Old";
    const std::string newName = "New";
    Workspace_sptr work1 = addToADS(oldName);
    Workspace_sptr work2 = addToADS(newName);
    TS_ASSERT_THROWS_NOTHING(ads.rename(oldName, newName));
    auto workBack = ads.retrieve(newName);
    TS_ASSERT_EQUALS(work1, workBack);
    TS_ASSERT(!ads.doesExist(oldName));
    TS_ASSERT(ads.doesExist(newName));
    TS_ASSERT_EQUALS(ads.size(), 1);
  }

  void test_add_workspace_group() {
    // create a group
    WorkspaceGroup_sptr group(new WorkspaceGroup);
    // create anonymous workspaces
    MockWorkspace_sptr ws1 = MockWorkspace_sptr(new MockWorkspace);
    MockWorkspace_sptr ws2 = MockWorkspace_sptr(new MockWorkspace);
    // add them to the group
    group->addWorkspace(ws1);
    group->addWorkspace(ws2);
    // ADS must be empty
    TS_ASSERT_EQUALS(ads.size(), 0);
    ads.add("Group", group);
    // there must be 3 workspaces in the ADS
    TS_ASSERT_EQUALS(ads.size(), 3);
    TS_ASSERT(ads.doesExist("Group"));
    TS_ASSERT(ads.doesExist("Group_1"));
    TS_ASSERT(ads.doesExist("Group_2"));
  }

  void test_add_workspace_group_keeps_existing_workspaces() {
    // populate the ADS
    Workspace_sptr work1 = addToADS("work1");
    Workspace_sptr work2 = addToADS("work2");
    // create a group
    WorkspaceGroup_sptr group(new WorkspaceGroup);
    // create anonymous workspace
    MockWorkspace_sptr ws1 = MockWorkspace_sptr(new MockWorkspace);
    // add one anonymous ...
    group->addWorkspace(ws1);
    // and one existing workspace
    group->addWorkspace(work2);
    // ADS must have 2 workspaces
    TS_ASSERT_EQUALS(ads.size(), 2);
    ads.add("Group", group);
    // there must be 4 workspaces in the ADS
    TS_ASSERT_EQUALS(ads.size(), 4);
    TS_ASSERT(ads.doesExist("Group"));
    TS_ASSERT(ads.doesExist("Group_1"));
    TS_ASSERT(!ads.doesExist("Group_2"));
    TS_ASSERT(ads.doesExist("work1"));
    TS_ASSERT(ads.doesExist("work2"));

    auto names = group->getNames();
    TS_ASSERT_EQUALS(names.size(), 2);
    TS_ASSERT_EQUALS(names[0], "Group_1");
    TS_ASSERT_EQUALS(names[1], "work2");
  }

  void test_addOrReplace_workspace_group_replaces_existing_workspaces() {
    // populate the ADS
    Workspace_sptr work1 = addToADS("work1");
    Workspace_sptr work2 = addToADS("Group_2");
    // create a group
    WorkspaceGroup_sptr group(new WorkspaceGroup);
    // create anonymous workspace
    MockWorkspace_sptr ws1 = MockWorkspace_sptr(new MockWorkspace);
    MockWorkspace_sptr ws2 = MockWorkspace_sptr(new MockWorkspace);
    // add them
    group->addWorkspace(ws1);
    group->addWorkspace(ws2);
    // ADS must have 2 workspaces
    TS_ASSERT_EQUALS(ads.size(), 2);
    ads.addOrReplace("Group", group);
    // there must be 4 workspaces in the ADS
    TS_ASSERT_EQUALS(ads.size(), 4);
    TS_ASSERT(ads.doesExist("Group"));
    TS_ASSERT(ads.doesExist("Group_1"));
    TS_ASSERT(ads.doesExist("Group_2"));
    TS_ASSERT(ads.doesExist("work1"));
    TS_ASSERT(!ads.doesExist("work2"));

    auto names = group->getNames();
    TS_ASSERT_EQUALS(names.size(), 2);
    TS_ASSERT_EQUALS(names[0], "Group_1");
    TS_ASSERT_EQUALS(names[1], "Group_2");
  }

  void test_add_workspace_group_throws_if_adding_existing_names() {
    // populate the ADS
    Workspace_sptr work1 = addToADS("work1");
    Workspace_sptr work2 = addToADS("Group_2");
    // create a group
    WorkspaceGroup_sptr group(new WorkspaceGroup);
    // create anonymous workspace
    MockWorkspace_sptr ws1 = MockWorkspace_sptr(new MockWorkspace);
    MockWorkspace_sptr ws2 = MockWorkspace_sptr(new MockWorkspace);
    // add them
    group->addWorkspace(ws1);
    group->addWorkspace(ws2);
    // ADS must have 2 workspaces
    TS_ASSERT_EQUALS(ads.size(), 2);
    TS_ASSERT_THROWS(ads.add("Group", group), const std::runtime_error &);
    // there must be 4 workspaces in the ADS
    TS_ASSERT_EQUALS(ads.size(), 4);
    TS_ASSERT(ads.doesExist("Group"));
    TS_ASSERT(ads.doesExist("Group_1"));
    TS_ASSERT(ads.doesExist("Group_2"));
    TS_ASSERT(ads.doesExist("work1"));
    TS_ASSERT(!ads.doesExist("work2"));

    auto names = group->getNames();
    TS_ASSERT_EQUALS(names.size(), 2);
    TS_ASSERT_EQUALS(names[0], "Group_1");
    TS_ASSERT_EQUALS(names[1], "Group_2");
  }

  // Test base DataService class methods to make sure behaviour w.r.t. hidden
  // objects
  // persists, as this class is where it will most be used.
  void test_size() {
    ConfigService::Instance().setString("MantidOptions.InvisibleWorkspaces", "0");
    TS_ASSERT_EQUALS(ads.size(), 0);
    addToADS("something");
    TS_ASSERT_EQUALS(ads.size(), 1);
    addToADS("__hidden");
    TSM_ASSERT_EQUALS("Hidden workspaces should not be counted", ads.size(), 1);

    ConfigService::Instance().setString("MantidOptions.InvisibleWorkspaces", "1");
    TS_ASSERT_EQUALS(ads.size(), 2);
    ConfigService::Instance().setString("MantidOptions.InvisibleWorkspaces", "0");
  }

  void test_getObjectNames_and_getObjects() {
    addToADS("One");
    addToADS("Two");
    addToADS("__Three");

    auto names = ads.getObjectNames();
    auto objects = ads.getObjects();
    TSM_ASSERT_EQUALS("Hidden entries should not be returned", names.size(), 2);
    TSM_ASSERT_EQUALS("Hidden entries should not be returned", objects.size(), 2);
    TS_ASSERT_DIFFERS(std::find(names.cbegin(), names.cend(), "One"), names.end());
    TS_ASSERT_DIFFERS(std::find(names.cbegin(), names.cend(), "Two"), names.end());
    TS_ASSERT_EQUALS(std::find(names.cbegin(), names.cend(), "__Three"), names.end());
    TSM_ASSERT_EQUALS("Hidden entries should not be returned", std::find(names.cbegin(), names.cend(), "__Three"),
                      names.end());

    ConfigService::Instance().setString("MantidOptions.InvisibleWorkspaces", "1");
    names = ads.getObjectNames();
    objects = ads.getObjects();
    TS_ASSERT_EQUALS(names.size(), 3);
    TS_ASSERT_EQUALS(objects.size(), 3);
    TS_ASSERT_DIFFERS(std::find(names.cbegin(), names.cend(), "__Three"), names.end());
    ConfigService::Instance().setString("MantidOptions.InvisibleWorkspaces", "0");
  }

  void test_deepRemoveGroup() {
    addToADS("some_workspace");
    auto group = addGroupToADS("group");
    TS_ASSERT_EQUALS(ads.size(), 4);

    // name doesn't exist
    TS_ASSERT_THROWS(ads.deepRemoveGroup("abc"), const std::runtime_error &);
    // workspace isn't a group
    TS_ASSERT_THROWS(ads.deepRemoveGroup("group_1"), const std::runtime_error &);
    TS_ASSERT_THROWS_NOTHING(ads.deepRemoveGroup("group"));
    TS_ASSERT_EQUALS(ads.size(), 1);

    // check a group containing another group
    group = addGroupWithGroupToADS("group");
    TS_ASSERT_EQUALS(ads.size(), 6);
    TS_ASSERT_THROWS_NOTHING(ads.deepRemoveGroup("group"));
    TS_ASSERT_EQUALS(ads.size(), 1);
    ads.clear();
  }

  void test_removeFromGroup() {
    auto group = addGroupToADS("group");
    TS_ASSERT_EQUALS(ads.size(), 3);
    TS_ASSERT_EQUALS(group->size(), 2);
    ads.removeFromGroup("group", "group_2");
    TS_ASSERT_EQUALS(ads.size(), 3);
    TS_ASSERT_EQUALS(group->size(), 1);

    TS_ASSERT_THROWS(ads.removeFromGroup("group", "noworkspace"), const std::runtime_error &);
    TS_ASSERT_THROWS(ads.removeFromGroup("nogroup", "noworkspace"), const std::runtime_error &);
    TS_ASSERT_THROWS(ads.removeFromGroup("nogroup", "group_1"), const std::runtime_error &);
    ads.clear();
  }

  void test_removeFromGroup_group() {
    auto group = addGroupWithGroupToADS("group");
    TS_ASSERT_EQUALS(ads.size(), 5);
    TS_ASSERT_EQUALS(group->size(), 2);
    // remove group from group
    ads.removeFromGroup("group", "group_2");
    TS_ASSERT_EQUALS(ads.size(), 5);
    TS_ASSERT_EQUALS(group->size(), 1);
    ads.clear();
  }

  void test_addToGroup() {
    auto group = addGroupToADS("group");
    addToADS("workspace");
    TS_ASSERT(!group->contains("workspace"));
    ads.addToGroup("group", "workspace");
    TS_ASSERT(group->contains("workspace"));
    ads.clear();
  }

  void test_addToGroup_group() {
    auto group = addGroupWithGroupToADS("group");
    addToADS("workspace");

    WorkspaceGroup_sptr grp = ads.retrieveWS<WorkspaceGroup>("group_2");
    TS_ASSERT(grp);
    TS_ASSERT(!group->contains("workspace"));
    TS_ASSERT(!grp->contains("workspace"));
    ads.addToGroup("group_2", "workspace");
    TS_ASSERT(!group->contains("workspace"));
    TS_ASSERT(grp->contains("workspace"));
    ads.clear();
  }

  void test_topLevelItems_Does_Not_Contain_Workspaces_That_Are_In_A_Group_In_The_List() {
    // this adds 1 group to the ADS (5 ws's altogether)
    auto group = addGroupWithGroupToADS("snapshot_group");
    // plus 1 more ws
    auto leaf = addToADS("single_workspace");
    // ADS must have 6 ws's now
    TS_ASSERT_EQUALS(ads.size(), 6);

    auto topLevelItems = ads.topLevelItems();
    // Only 2
    TS_ASSERT_EQUALS(2, topLevelItems.size());

    auto it = topLevelItems.find("snapshot_group");
    TS_ASSERT(it != topLevelItems.end());
    TS_ASSERT_EQUALS("snapshot_group", it->first);
    TS_ASSERT_EQUALS(group, it->second);

    it = topLevelItems.find("single_workspace");
    TS_ASSERT(it != topLevelItems.end());
    TS_ASSERT_EQUALS("single_workspace", it->first);
    TS_ASSERT_EQUALS(leaf, it->second);
  }

  void test_adding_null_workspace() {
    auto nullWS = MockWorkspace_sptr();

    // Shouldn't be able to add null pointers
    TS_ASSERT_THROWS(ads.add("null_workspace", nullWS), const std::runtime_error &);
    TS_ASSERT_THROWS(ads.addOrReplace("null_workspace", nullWS), const std::runtime_error &);

    TS_ASSERT(!ads.doesExist("null_workspace"));
  }

  void test_throws_when_adding_a_group_which_contains_a_ws_with_the_same_name() {
    auto ws1 = addToADS("ws1");
    auto ws2 = addToADS("ws2");
    WorkspaceGroup_sptr group(new WorkspaceGroup);
    group->addWorkspace(ws1);
    group->addWorkspace(ws2);

    TS_ASSERT_THROWS(ads.add("ws1", group), const std::invalid_argument &);
    TS_ASSERT_THROWS(ads.addOrReplace("ws1", group), const std::invalid_argument &);
  }

  void test_throws_when_adding_to_a_group_a_workspace_with_the_same_name() {
    auto ws1 = addToADS("ws1");
    auto ws2 = addToADS("ws2");
    auto group = addOrReplaceGroupToADS("ws1", 2);

    TS_ASSERT_THROWS(ads.addToGroup("ws1", "ws1"), const std::runtime_error &);
  }

  void test_unique_name() {
    auto unique_name = ads.uniqueName();
    TS_ASSERT_EQUALS(5, unique_name.size());

    const std::string prefix = "testPrefix_";
    auto uniqueWithPrefix = ads.uniqueName(4, prefix);

    TS_ASSERT_EQUALS(4 + prefix.size(), uniqueWithPrefix.size());
    TS_ASSERT_EQUALS(prefix, uniqueWithPrefix.substr(0, prefix.size()));

    TS_ASSERT_THROWS(ads.uniqueName(-4), const std::invalid_argument &);
  }

  void test_unique_name_no_collision() {
    for (char letter = 'a'; letter <= 'z'; letter++) {
      if (letter == 'c') {
        continue;
      }
      std::string wsName = "unique_" + std::string(1, letter);
      auto ws = addToADS(wsName);
    }
    auto objects = ads.getObjects();
    TS_ASSERT_EQUALS(25, objects.size()); // make sure we have all expected workspaces

    TS_ASSERT_EQUALS("unique_c", ads.uniqueName(1, "unique_"));
    auto ws = addToADS("unique_c");

    TS_ASSERT_THROWS(ads.uniqueName(1, "unique_"), std::runtime_error &);
  }

  void test_unique_hidden_name() {
    auto hiddenName = ads.uniqueHiddenName();
    TS_ASSERT_EQUALS(11, hiddenName.size())
    TS_ASSERT_EQUALS("__", hiddenName.substr(0, 2))
  }

private:
  /// If replace=true then usea addOrReplace
  void doAddingOnInvalidNameTests(bool replace) {
    const std::string illegalChars = " +-/*\\%<>&|^~=!@()[]{},:.`$'\"?";
    const std::string allowed("WsName");

    for (const char &illegalChar : illegalChars) {
      // Build illegal name
      std::string name = std::string("ws") + illegalChar + "name";

#ifndef NDEBUG
      // In debug mode, illegal workspace names throw exceptions.
      std::string errorMsg =
          std::string("Expected ADS to throw with illegal character ") + illegalChar + " in workspace name.";
      if (replace) {
        TSM_ASSERT_THROWS(errorMsg, addToADS(name), const std::invalid_argument &);
      } else {
        TSM_ASSERT_THROWS(errorMsg, addOrReplaceToADS(name), const std::invalid_argument &);
      }
      TS_ASSERT(!ads.doesExist(name));
#else
      // In release mode, a warning is printed but no exception is thrown.
      std::string errorMsg =
          std::string("Expected ADS not to throw with illegal character ") + illegalChar + " in workspace name.";
      if (replace) {
        TSM_ASSERT_THROWS_NOTHING(errorMsg, addToADS(name));
      } else {
        TSM_ASSERT_THROWS_NOTHING(errorMsg, addOrReplaceToADS(name));
      }
      TS_ASSERT(ads.doesExist(name));
#endif
      ads.remove(name); // Clear up if the test fails so that it dones't impact on others.
    }
  }

  /// Add a ptr to the ADS with the given name
  Workspace_sptr addToADS(const std::string &name) {
    MockWorkspace_sptr space = MockWorkspace_sptr(new MockWorkspace);
    ads.add(name, space);
    return space;
  }

  /// Add a group with N simple workspaces to the ADS
  WorkspaceGroup_sptr addGroupToADS(const std::string &name, const size_t nitems = 2) {
    auto group(std::make_shared<WorkspaceGroup>());
    for (auto i = 0u; i < nitems; ++i) {
      group->addWorkspace(std::make_shared<MockWorkspace>());
    }
    ads.add(name, group);
    return group;
  }

  /// Add a group with N simple workspaces to the ADS
  WorkspaceGroup_sptr addOrReplaceGroupToADS(const std::string &name, const size_t nitems = 2) {
    auto group(std::make_shared<WorkspaceGroup>());
    for (auto i = 0u; i < nitems; ++i) {
      group->addWorkspace(std::make_shared<MockWorkspace>());
    }
    ads.addOrReplace(name, group);
    return group;
  }

  /// Add a group with 1 simple workspace and 1 group with 2 simple ws to the
  /// ADS
  WorkspaceGroup_sptr addGroupWithGroupToADS(const std::string &name) {
    WorkspaceGroup_sptr group(new WorkspaceGroup);
    group->addWorkspace(MockWorkspace_sptr(new MockWorkspace));
    WorkspaceGroup_sptr group1(new WorkspaceGroup);
    group1->addWorkspace(MockWorkspace_sptr(new MockWorkspace));
    group1->addWorkspace(MockWorkspace_sptr(new MockWorkspace));
    group->addWorkspace(group1);
    ads.add(name, group);
    return group;
  }

  /// Add or replace the given name
  void addOrReplaceToADS(const std::string &name) { ads.addOrReplace(name, Workspace_sptr(new MockWorkspace)); }
};
