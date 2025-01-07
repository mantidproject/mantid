// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidFrameworkTestHelpers/FakeObjects.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/WarningSuppressions.h"
#include "PropertyManagerHelper.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class WorkspaceGroupTest_WorkspaceGroupObserver {
  Poco::NObserver<WorkspaceGroupTest_WorkspaceGroupObserver, Mantid::API::GroupUpdatedNotification>
      m_workspaceGroupUpdateObserver;

public:
  bool received;
  WorkspaceGroupTest_WorkspaceGroupObserver()
      : m_workspaceGroupUpdateObserver(*this, &WorkspaceGroupTest_WorkspaceGroupObserver::handleWorkspaceGroupUpdate),
        received(false) {
    AnalysisDataService::Instance().notificationCenter.addObserver(m_workspaceGroupUpdateObserver);
  }
  // handles notification send by a WorkspaceGroup instance
  void handleWorkspaceGroupUpdate(Mantid::API::GroupUpdatedNotification_ptr) { received = true; }
};

class WorkspaceGroupTest : public CxxTest::TestSuite {
private:
  /// Helper method to add an 'nperiods' log value to each workspace in a group.
  void add_periods_logs(const WorkspaceGroup_sptr &ws, int nperiods = -1) {
    for (size_t i = 0; i < ws->size(); ++i) {
      MatrixWorkspace_sptr currentWS = std::dynamic_pointer_cast<MatrixWorkspace>(ws->getItem(i));

      PropertyWithValue<int> *nperiodsProp = new PropertyWithValue<int>("nperiods", nperiods);
      currentWS->mutableRun().addLogData(nperiodsProp);
    }
  }

  // Helper type, representing some concrete workspace type.
  class MockWorkspace : public Mantid::API::Workspace {
    GNU_DIAG_OFF_SUGGEST_OVERRIDE
    MOCK_CONST_METHOD0(id, const std::string());
    MOCK_CONST_METHOD0(name, const std::string());
    MOCK_CONST_METHOD0(threadSafe, bool());
    MOCK_CONST_METHOD0(toString, const std::string());
    MOCK_CONST_METHOD0(getMemorySize, size_t());
    GNU_DIAG_ON_SUGGEST_OVERRIDE
  private:
    MockWorkspace *doClone() const override {
      throw std::runtime_error("Cloning of MockWorkspace is not implemented.");
    }
    MockWorkspace *doCloneEmpty() const override {
      throw std::runtime_error("Cloning of MockWorkspace is not implemented.");
    }
  };

  /// Make a simple group
  WorkspaceGroup_sptr makeGroup() {
    for (size_t i = 0; i < 3; i++) {
      std::shared_ptr<WorkspaceTester> ws = std::make_shared<WorkspaceTester>();
      ws->initialize(2, 4, 3);
      AnalysisDataService::Instance().addOrReplace("ws" + Strings::toString(i), ws);
    }
    WorkspaceGroup_sptr group(new WorkspaceGroup());
    AnalysisDataService::Instance().addOrReplace("group", group);
    group->add("ws0");
    group->add("ws1");
    group->add("ws2");
    return group;
  }

public:
  void test_toString_Produces_Expected_String() {
    WorkspaceGroup_sptr group = makeGroup();

    const std::string expected = "WorkspaceGroup\n"
                                 " -- ws0\n"
                                 " -- ws1\n"
                                 " -- ws2\n";
    TS_ASSERT_EQUALS(expected, group->toString());
  }

  void test_sortByName() {
    WorkspaceGroup_sptr group = makeGroup();
    AnalysisDataService::Instance().rename("ws0", "ws3");
    AnalysisDataService::Instance().sortGroupByName("group");
    const std::string expected = "WorkspaceGroup\n"
                                 " -- ws1\n"
                                 " -- ws2\n"
                                 " -- ws3\n";
    TS_ASSERT_EQUALS(expected, group->toString());
    AnalysisDataService::Instance().rename("ws1", "ws5");
    const std::string expected2 = "WorkspaceGroup\n"
                                  " -- ws2\n"
                                  " -- ws3\n"
                                  " -- ws5\n";
    group->sortMembersByName();
    TS_ASSERT_EQUALS(expected2, group->toString());
    AnalysisDataService::Instance().clear();
  }

  void test_add() {
    WorkspaceGroup_sptr group = makeGroup();
    TS_ASSERT_EQUALS(group->size(), 3);
    TS_ASSERT(group->contains("ws0"));
    // cannot add a workspace which doesn't exist
    TS_ASSERT_THROWS(group->add("noworkspace"), const Kernel::Exception::NotFoundError &);
    AnalysisDataService::Instance().clear();
  }

  void test_addWorkspace() {
    WorkspaceGroup_sptr group(new WorkspaceGroup());
    Workspace_sptr ws1(new WorkspaceTester());
    group->addWorkspace(ws1);
    TS_ASSERT_EQUALS(group->size(), 1);
    Workspace_sptr ws2(new WorkspaceTester());
    group->addWorkspace(ws2);
    TS_ASSERT_EQUALS(group->size(), 2);
    TS_ASSERT_EQUALS(AnalysisDataService::Instance().size(), 0);
    AnalysisDataService::Instance().add("group", group);
    TS_ASSERT_EQUALS(AnalysisDataService::Instance().size(), 3);
    AnalysisDataService::Instance().clear();
  }

  void test_addWorkspace_when_group_in_ADS() {
    WorkspaceGroup_sptr group(new WorkspaceGroup());
    Workspace_sptr ws1(new WorkspaceTester());
    Workspace_sptr ws2(new WorkspaceTester());

    TS_ASSERT_EQUALS(AnalysisDataService::Instance().size(), 0);
    AnalysisDataService::Instance().add("group", group);

    group->addWorkspace(ws1);
    TS_ASSERT_EQUALS(group->size(), 1);
    group->addWorkspace(ws2);
    TS_ASSERT_EQUALS(group->size(), 2);

    TS_ASSERT_EQUALS(AnalysisDataService::Instance().size(), 1);
    AnalysisDataService::Instance().clear();
  }

  void test_getNames() {
    WorkspaceGroup_sptr group(new WorkspaceGroup());
    Workspace_sptr ws1(new WorkspaceTester());
    group->addWorkspace(ws1);
    Workspace_sptr ws2(new WorkspaceTester());
    group->addWorkspace(ws2);
    AnalysisDataService::Instance().add("Workspace2", ws2);
    auto names = group->getNames();
    TS_ASSERT_EQUALS(names.size(), 2);
    TS_ASSERT_EQUALS(names[0], "");
    TS_ASSERT_EQUALS(names[1], "Workspace2");
  }

  void test_reportMembers_Does_Not_Clear_List_Already_Passed_In() {
    Workspace_sptr leaf1(new WorkspaceTester());
    std::set<Workspace_sptr> topLevel;
    topLevel.insert(leaf1);
    WorkspaceGroup_sptr group(new WorkspaceGroup());
    Workspace_sptr ws1(new WorkspaceTester());
    group->addWorkspace(ws1);
    Workspace_sptr ws2(new WorkspaceTester());
    group->addWorkspace(ws2);

    group->reportMembers(topLevel);
    TS_ASSERT_EQUALS(3, topLevel.size());
    TS_ASSERT_EQUALS(1, topLevel.count(leaf1));
    TS_ASSERT_EQUALS(1, topLevel.count(ws1));
    TS_ASSERT_EQUALS(1, topLevel.count(ws2));
  }

  void test_getItem() {
    WorkspaceGroup_sptr group = makeGroup();
    Workspace_sptr ws1 = group->getItem(1);
    TS_ASSERT_EQUALS(ws1->getName(), "ws1");
    // Test the 'by name' overload
    Workspace_sptr ws11 = group->getItem("ws1");
    TS_ASSERT_EQUALS(ws1, ws11);
    // Test for failure too
    TS_ASSERT_THROWS(group->getItem("non-existent"), const std::out_of_range &);
    TS_ASSERT_THROWS(group->getItem(""), const std::out_of_range &);
    AnalysisDataService::Instance().clear();
  }

  void test_remove() {
    WorkspaceGroup_sptr group = makeGroup();
    group->remove("ws0");
    TSM_ASSERT("remove() takes out from group", !group->contains("ws0"));
    TSM_ASSERT("remove() does not take out of ADS ", AnalysisDataService::Instance().doesExist("ws0"));
    AnalysisDataService::Instance().clear();
  }

  void test_removeItem() {
    WorkspaceGroup_sptr group1 = makeGroup();
    TS_ASSERT_THROWS(group1->removeItem(1), const std::runtime_error &);

    WorkspaceGroup_sptr group(new WorkspaceGroup());
    Workspace_sptr ws1(new WorkspaceTester());
    group->addWorkspace(ws1);
    Workspace_sptr ws2(new WorkspaceTester());
    group->addWorkspace(ws2);

    TS_ASSERT_EQUALS(group->size(), 2);
    TS_ASSERT_THROWS_NOTHING(group->removeItem(1));
    TS_ASSERT_EQUALS(group->size(), 1);
    TS_ASSERT_EQUALS(group->getItem(0), ws1);

    AnalysisDataService::Instance().clear();
  }

  void test_removeAll() {
    WorkspaceGroup_sptr group = makeGroup();
    group->removeAll();
    TS_ASSERT_EQUALS(group->size(), 0);
    TSM_ASSERT("removeAll() does not take out of ADS ", AnalysisDataService::Instance().doesExist("ws0"));
    AnalysisDataService::Instance().clear();
  }

  void test_getAllItems() {
    WorkspaceGroup_sptr group = makeGroup();
    auto items = group->getAllItems();
    TS_ASSERT_EQUALS(group->size(), 3);
    TS_ASSERT_EQUALS(items.size(), 3);
    TS_ASSERT_EQUALS(items[0], group->getItem(0));
    TS_ASSERT_EQUALS(items[1], group->getItem(1));
    TS_ASSERT_EQUALS(items[2], group->getItem(2));
    AnalysisDataService::Instance().clear();
  }

  void test_deleting_workspaces() {
    WorkspaceGroup_sptr group = makeGroup();
    TS_ASSERT(AnalysisDataService::Instance().doesExist("group"));

    // When you delete a workspace it gets removed from the group
    AnalysisDataService::Instance().remove("ws0");
    TS_ASSERT(AnalysisDataService::Instance().doesExist("group"));
    TS_ASSERT(!group->contains("ws0"));

    AnalysisDataService::Instance().remove("ws1");
    TS_ASSERT(AnalysisDataService::Instance().doesExist("group"));
    TS_ASSERT(!group->contains("ws1"));

    // When you remove the last one, the group deletes itself
    AnalysisDataService::Instance().remove("ws2");
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("group"));
    AnalysisDataService::Instance().clear();
  }

  void test_areNamesSimilar() {
    WorkspaceGroup_sptr group(new WorkspaceGroup());
    // group->setName("name");
    AnalysisDataService::Instance().add("name", group);
    TSM_ASSERT("Empty group is not similar", !group->areNamesSimilar());

    std::shared_ptr<WorkspaceTester> ws = std::make_shared<WorkspaceTester>();
    ws->initialize(2, 4, 3);
    AnalysisDataService::Instance().addOrReplace("name_0", ws);

    ws.reset(new WorkspaceTester());
    ws->initialize(2, 4, 3);
    AnalysisDataService::Instance().addOrReplace("name_12", ws);

    ws.reset(new WorkspaceTester());
    ws->initialize(2, 4, 3);
    AnalysisDataService::Instance().addOrReplace("name_monkey", ws);

    ws.reset(new WorkspaceTester());
    ws->initialize(2, 4, 3);
    AnalysisDataService::Instance().addOrReplace("different_name", ws);

    group->add("name_0");
    TS_ASSERT(group->areNamesSimilar());
    group->add("name_12");
    TS_ASSERT(group->areNamesSimilar());
    group->add("name_monkey");
    TS_ASSERT(group->areNamesSimilar());
    group->add("different_name");
    TS_ASSERT(!group->areNamesSimilar());

    AnalysisDataService::Instance().clear();
  }

  void test_not_multiperiod_with_less_than_one_element() {
    WorkspaceGroup group;
    TSM_ASSERT("Cannot be multiperiod without entries", !group.isMultiperiod());
  }

  void test_not_multiperiod_without_matrix_workspaces() {
    Workspace_sptr a = std::make_shared<MockWorkspace>();
    WorkspaceGroup group;
    group.addWorkspace(a);
    TSM_ASSERT("Cannot be multiperiod unless MatrixWorkspaces are used as elements.", !group.isMultiperiod());
  }

  void test_not_multiperiod_if_missing_nperiods_log() {
    Workspace_sptr a = std::make_shared<WorkspaceTester>(); // workspace has
                                                            // no nperiods
                                                            // entry.
    WorkspaceGroup group;
    group.addWorkspace(a);
    TSM_ASSERT("Cannot be multiperiod without nperiods log.", !group.isMultiperiod());
  }

  void test_not_multiperiod_if_nperiods_log_less_than_one() {
    Workspace_sptr a = std::make_shared<WorkspaceTester>();
    WorkspaceGroup_sptr group = std::make_shared<WorkspaceGroup>();
    group->addWorkspace(a);
    add_periods_logs(group, 0); // nperiods set to 0.
    TSM_ASSERT("Cannot be multiperiod without nperiods log.", !group->isMultiperiod());
  }

  void test_positive_identification_of_multiperiod_data() {
    Workspace_sptr a = std::make_shared<WorkspaceTester>();
    WorkspaceGroup_sptr group = std::make_shared<WorkspaceGroup>();
    group->addWorkspace(a);
    add_periods_logs(group, 1);
    TS_ASSERT(group->isMultiperiod());
  }

  void test_isGroup() {
    WorkspaceGroup_sptr group = makeGroup();
    TS_ASSERT_EQUALS(group->isGroup(), true);
  }

  void test_isInGroup() {
    WorkspaceGroup_sptr group = makeGroup();
    auto ws1 = group->getItem(1);
    TS_ASSERT(group->isInGroup(*ws1));
    Workspace_sptr a = std::make_shared<WorkspaceTester>();
    TS_ASSERT(!group->isInGroup(*a));

    WorkspaceGroup_sptr group1 = std::make_shared<WorkspaceGroup>();
    group1->addWorkspace(a);
    group->addWorkspace(group1);
    TS_ASSERT(group->isInGroup(*a));

    // catch a cycle
    group1->addWorkspace(group);
    Workspace_sptr b = std::make_shared<WorkspaceTester>();
    TS_ASSERT_THROWS(group->isInGroup(*b), const std::runtime_error &);
    group1->removeAll();
  }

  /**
   * Test declaring an input workspace group and retrieving as const_sptr or
   * sptr
   */
  void testGetProperty_const_sptr() {
    const std::string wsName = "InputWorkspace";
    WorkspaceGroup_sptr wsInput(new WorkspaceGroup());
    PropertyManagerHelper manager;
    manager.declareProperty(wsName, wsInput, Direction::Input);

    // Check property can be obtained as const_sptr or sptr
    WorkspaceGroup_const_sptr wsConst;
    WorkspaceGroup_sptr wsNonConst;
    TS_ASSERT_THROWS_NOTHING(wsConst = manager.getValue<WorkspaceGroup_const_sptr>(wsName));
    TS_ASSERT(wsConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsNonConst = manager.getValue<WorkspaceGroup_sptr>(wsName));
    TS_ASSERT(wsNonConst != nullptr);
    TS_ASSERT_EQUALS(wsConst, wsNonConst);

    // Check TypedValue can be cast to const_sptr or to sptr
    PropertyManagerHelper::TypedValue val(manager, wsName);
    WorkspaceGroup_const_sptr wsCastConst;
    WorkspaceGroup_sptr wsCastNonConst;
    TS_ASSERT_THROWS_NOTHING(wsCastConst = (WorkspaceGroup_const_sptr)val);
    TS_ASSERT(wsCastConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsCastNonConst = (WorkspaceGroup_sptr)val);
    TS_ASSERT(wsCastNonConst != nullptr);
    TS_ASSERT_EQUALS(wsCastConst, wsCastNonConst);
  }

  /**
   * Test declaring an input workspace and retrieving as const_sptr or sptr
   * (here Workspace rather than WorkspaceGroup)
   */
  void testGetProperty_Workspace_const_sptr() {
    const std::string wsName = "InputWorkspace";
    Workspace_sptr wsInput(new WorkspaceTester());
    PropertyManagerHelper manager;
    manager.declareProperty(wsName, wsInput, Direction::Input);

    // Check property can be obtained as const_sptr or sptr
    Workspace_const_sptr wsConst;
    Workspace_sptr wsNonConst;
    TS_ASSERT_THROWS_NOTHING(wsConst = manager.getValue<Workspace_const_sptr>(wsName));
    TS_ASSERT(wsConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsNonConst = manager.getValue<Workspace_sptr>(wsName));
    TS_ASSERT(wsNonConst != nullptr);
    TS_ASSERT_EQUALS(wsConst, wsNonConst);

    // Check TypedValue can be cast to const_sptr or to sptr
    PropertyManagerHelper::TypedValue val(manager, wsName);
    Workspace_const_sptr wsCastConst;
    Workspace_sptr wsCastNonConst;
    TS_ASSERT_THROWS_NOTHING(wsCastConst = (Workspace_const_sptr)val);
    TS_ASSERT(wsCastConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsCastNonConst = (Workspace_sptr)val);
    TS_ASSERT(wsCastNonConst != nullptr);
    TS_ASSERT_EQUALS(wsCastConst, wsCastNonConst);
  }

  void test_unableToAddAGroupToItself() {
    WorkspaceGroup_sptr group(new WorkspaceGroup());
    Workspace_sptr wsInput(new WorkspaceTester());
    group->addWorkspace(wsInput);
    group->addWorkspace(group);
    TS_ASSERT(group->contains(wsInput));
    TS_ASSERT(!group->contains(group));
  }

  void test_containsInChildrenFindsChildrenWithGivenName1LayerDown() {
    WorkspaceGroup_sptr group0(new WorkspaceGroup());
    AnalysisDataService::Instance().addOrReplace("group0", group0);
    WorkspaceGroup_sptr group1(new WorkspaceGroup());
    AnalysisDataService::Instance().addOrReplace("group1", group1);
    Workspace_sptr wsInput(new WorkspaceTester());
    AnalysisDataService::Instance().addOrReplace("wsInput", wsInput);
    group1->addWorkspace(wsInput);
    group0->addWorkspace(group1);

    TS_ASSERT(group0->containsInChildren("wsInput"));
  }

  void test_containsInChildrenFindsChildrenWithGivenName4LayersDown() {
    WorkspaceGroup_sptr group0(new WorkspaceGroup());
    AnalysisDataService::Instance().addOrReplace("group0", group0);
    WorkspaceGroup_sptr group1(new WorkspaceGroup());
    AnalysisDataService::Instance().addOrReplace("group1", group1);
    WorkspaceGroup_sptr group2(new WorkspaceGroup());
    AnalysisDataService::Instance().addOrReplace("group2", group2);
    WorkspaceGroup_sptr group3(new WorkspaceGroup());
    AnalysisDataService::Instance().addOrReplace("group3", group3);
    WorkspaceGroup_sptr group4(new WorkspaceGroup());
    AnalysisDataService::Instance().addOrReplace("group4", group4);
    Workspace_sptr wsInput(new WorkspaceTester());
    AnalysisDataService::Instance().addOrReplace("wsInput", wsInput);
    group4->addWorkspace(wsInput);
    group3->addWorkspace(group4);
    group2->addWorkspace(group3);
    group1->addWorkspace(group2);
    group0->addWorkspace(group1);
    TS_ASSERT(group0->containsInChildren("wsInput"));
  }
};
