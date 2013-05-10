#ifndef MANTID_API_WORKSPACEGROUPTEST_H_
#define MANTID_API_WORKSPACEGROUPTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidTestHelpers/FakeObjects.h"

#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Mantid;
using namespace Mantid::API;

class WorkspaceGroupTest_WorkspaceGroupObserver
{
  Poco::NObserver<WorkspaceGroupTest_WorkspaceGroupObserver, Mantid::API::GroupUpdatedNotification> m_workspaceGroupUpdateObserver;
public:
  bool received;
  WorkspaceGroupTest_WorkspaceGroupObserver():
    m_workspaceGroupUpdateObserver(*this,&WorkspaceGroupTest_WorkspaceGroupObserver::handleWorkspaceGroupUpdate),
    received(false)
  {
    AnalysisDataService::Instance().notificationCenter.addObserver(m_workspaceGroupUpdateObserver);
  }
  //handles notification send by a WorkspaceGroup instance
  void handleWorkspaceGroupUpdate(Mantid::API::GroupUpdatedNotification_ptr)
  {
    received = true;
  }
};

class WorkspaceGroupTest : public CxxTest::TestSuite
{
private:

    /// Helper method to add an 'nperiods' log value to each workspace in a group.
  void add_periods_logs(WorkspaceGroup_sptr ws, int nperiods = -1)
  {
    for(size_t i = 0; i < ws->size(); ++i)
    { 
      MatrixWorkspace_sptr currentWS = boost::dynamic_pointer_cast<MatrixWorkspace>(ws->getItem(i));

      PropertyWithValue<int>* nperiodsProp = new PropertyWithValue<int>("nperiods", nperiods);
      currentWS->mutableRun().addLogData(nperiodsProp);
    }
  }

  // Helper type, representing some concrete workspace type.
  class MockWorkspace : public Mantid::API::Workspace
  {
    MOCK_CONST_METHOD0(id, const std::string());
    MOCK_CONST_METHOD0(name, const std::string());
    MOCK_CONST_METHOD0(threadSafe, bool());
    MOCK_CONST_METHOD0(toString, std::string());
    MOCK_CONST_METHOD0(getMemorySize, size_t());
  };

public:

  boost::shared_ptr<WorkspaceTester> makeWorkspace(size_t nv = 2)
  {
      boost::shared_ptr<WorkspaceTester> ws(new WorkspaceTester());
      ws->initialize(nv,3,4);
      return ws;
  }

  /// Make a simple group outside ADS
  WorkspaceGroup_sptr makeGroup()
  {
    WorkspaceGroup_sptr group(new WorkspaceGroup());
    for (size_t i=0; i<3; i++)
    {
      group->addWorkspace( makeWorkspace() );
    }
    return group;
  }

  void test_empty_group_has_size_0()
  {
      WorkspaceGroup_sptr group(new WorkspaceGroup());
      TS_ASSERT_EQUALS( group->size(), 0 );
  }

  void test_addWorkspace_increases_group_size_by_1()
  {
    WorkspaceGroup_sptr group(new WorkspaceGroup());
    Workspace_sptr ws1(new WorkspaceTester());
    group->addWorkspace( ws1 );
    TS_ASSERT_EQUALS( group->size(), 1 );
    Workspace_sptr ws2(new WorkspaceTester());
    group->addWorkspace( ws2 );
    TS_ASSERT_EQUALS( group->size(), 2 );
  }

  void test_group_counts_as_1_in_ADS()
  {
      WorkspaceGroup_sptr group = makeGroup();
      TS_ASSERT_EQUALS( group->size(), 3);
      TS_ASSERT_EQUALS( AnalysisDataService::Instance().size(), 0 );
      AnalysisDataService::Instance().add("group", group);
      TS_ASSERT_EQUALS( AnalysisDataService::Instance().size(), 1 );
      AnalysisDataService::Instance().clear();
  }


  void test_group_containing_groups_counts_as_1_in_ADS()
  {
      WorkspaceGroup_sptr group = makeGroup();
      group->addWorkspace( makeGroup() );
      group->addWorkspace( makeGroup() );
      TS_ASSERT_EQUALS( group->size(), 5);
      TS_ASSERT_EQUALS( AnalysisDataService::Instance().size(), 0 );
      AnalysisDataService::Instance().add("group", group);
      TS_ASSERT_EQUALS( AnalysisDataService::Instance().size(), 1 );
      AnalysisDataService::Instance().clear();
  }

  void test_getMemorySize_returns_sum_of_member_sizes()
  {
      WorkspaceGroup_sptr group = makeGroup();
      group->addWorkspace( makeGroup() );
      TS_ASSERT_EQUALS( group->getMemorySize(), 6 * makeWorkspace()->getMemorySize() );
  }

  void test_getNumberOfEntries_returns_number_of_top_level_items()
  {
      WorkspaceGroup_sptr group = makeGroup();
      group->addWorkspace( makeGroup() );
      TS_ASSERT_EQUALS( group->getNumberOfEntries(), 4 );
      TS_ASSERT_EQUALS( group->size(), 4 );
  }

  void test_getItem_index()
  {
    WorkspaceGroup_sptr group(new WorkspaceGroup() );
    auto ws1 = makeWorkspace(1);
    auto ws2 = makeWorkspace(2);
    auto ws3 = makeWorkspace(3);
    group->addWorkspace( ws1 );
    group->addWorkspace( ws2 );
    group->addWorkspace( ws3 );
    Workspace_sptr ws21 = group->getItem(1);
    TS_ASSERT_EQUALS( boost::dynamic_pointer_cast<Workspace>(ws2), ws21 );
    // Test for failure too
    TS_ASSERT_THROWS( group->getItem(3), std::out_of_range );
    TS_ASSERT_THROWS( group->getItem(30), std::out_of_range );
    TS_ASSERT_THROWS( group->getItem("ws0"), std::out_of_range );
    WorkspaceGroup_sptr empty_group(new WorkspaceGroup() );
    TS_ASSERT_THROWS( empty_group->getItem(0), std::out_of_range );
    TS_ASSERT_THROWS( empty_group->getItem(30), std::out_of_range );
    TS_ASSERT_THROWS( empty_group->getItem("ws0"), std::out_of_range );
  }

  void test_add_group_in_ADS()
  {
    // if group and future member are in top level of ADS
    // member moves from top level to the group
    // ADS keeps only 1 pointer to member workspace
    WorkspaceGroup_sptr group( new WorkspaceGroup() );
    TS_ASSERT_EQUALS( group->size(), 0);
    AnalysisDataService::Instance().add( "group", group );
    auto ws = makeWorkspace();
    AnalysisDataService::Instance().add( "ws", ws );
    group->add("ws");
    TS_ASSERT( group->contains("ws") );
    TS_ASSERT_EQUALS( group->size(), 1 );
    TS_ASSERT_EQUALS( boost::dynamic_pointer_cast<Workspace>(group->getItem(0)), ws );
    TS_ASSERT_EQUALS( AnalysisDataService::Instance().size(), 1);
    // cannot add a workspace which doesn't exist
    TS_ASSERT_THROWS( group->add("noworkspace"), Kernel::Exception::NotFoundError );
    AnalysisDataService::Instance().clear();
  }

  void test_add_group_in_ADS_trying_to_add_same_workspace()
  {
    // if group and future member are in top level of ADS
    // member moves from top level to the group
    // ADS keeps only 1 pointer to member workspace
    WorkspaceGroup_sptr group( new WorkspaceGroup() );
    AnalysisDataService::Instance().add( "group", group );
    AnalysisDataService::Instance().add( "ws", makeWorkspace() );
    group->add("ws");
    TS_ASSERT( group->contains("ws") );
    TS_ASSERT_EQUALS( AnalysisDataService::Instance().size(), 1);
    group->add("ws");
    AnalysisDataService::Instance().clear();
  }

  void xtest_adding_group_to_ADS()
  {
      // if group's members are not in the ADS their names must become <group_name>_#
      {
          auto group = makeGroup();
          TS_ASSERT_EQUALS( group->getItem(0)->name() , "" );
          TS_ASSERT_EQUALS( group->getItem(1)->name() , "" );
          TS_ASSERT_EQUALS( group->getItem(2)->name() , "" );
          AnalysisDataService::Instance().add("group",group);
          TS_ASSERT_EQUALS( group->getItem(0)->name() , "group_1" );
          TS_ASSERT_EQUALS( group->getItem(1)->name() , "group_2" );
          TS_ASSERT_EQUALS( group->getItem(2)->name() , "group_3" );
          TS_ASSERT_EQUALS( AnalysisDataService::Instance().size(), 1);
          AnalysisDataService::Instance().clear();
      }
      // if some of the new members are already in ADS their names must not change
      {
          auto group = makeGroup();
          AnalysisDataService::Instance().add("workspace", group->getItem(1));
          TS_ASSERT_EQUALS( group->getItem(0)->name() , "" );
          TS_ASSERT_EQUALS( group->getItem(1)->name() , "workspace" );
          TS_ASSERT_EQUALS( group->getItem(2)->name() , "" );
          AnalysisDataService::Instance().add("group",group);
          TS_ASSERT_EQUALS( group->getItem(0)->name() , "group_1" );
          TS_ASSERT_EQUALS( group->getItem(1)->name() , "workspace" );
          TS_ASSERT_EQUALS( group->getItem(2)->name() , "group_3" );
          TS_ASSERT_EQUALS( AnalysisDataService::Instance().size(), 1);
          AnalysisDataService::Instance().clear();
      }
  }


/*

  void xtest_addWorkspace_when_group_in_ADS()
  {
    WorkspaceGroup_sptr group(new WorkspaceGroup());
    Workspace_sptr ws1(new WorkspaceTester());
    Workspace_sptr ws2(new WorkspaceTester());

    TS_ASSERT_EQUALS( AnalysisDataService::Instance().size(), 0 );
    AnalysisDataService::Instance().add("group", group);

    group->addWorkspace( ws1 );
    TS_ASSERT_EQUALS( group->size(), 1 );
    group->addWorkspace( ws2 );
    TS_ASSERT_EQUALS( group->size(), 2 );

    TS_ASSERT_EQUALS( AnalysisDataService::Instance().size(), 1 );
    AnalysisDataService::Instance().clear();
  }

  void xtest_getNames()
  {
    WorkspaceGroup_sptr group(new WorkspaceGroup());
    Workspace_sptr ws1(new WorkspaceTester());
    group->addWorkspace( ws1 );
    Workspace_sptr ws2(new WorkspaceTester());
    group->addWorkspace( ws2 );
    AnalysisDataService::Instance().add("Workspace2", ws2);
    auto names = group->getNames();
    TS_ASSERT_EQUALS(names.size(), 2);
    TS_ASSERT_EQUALS(names[0], "");
    TS_ASSERT_EQUALS(names[1], "Workspace2");
  }

  void xtest_remove()
  {
    WorkspaceGroup_sptr group = makeGroup();
    group->remove("ws0");
    TSM_ASSERT( "remove() takes out from group", !group->contains("ws0") );
    TSM_ASSERT( "remove() does not take out of ADS ", AnalysisDataService::Instance().doesExist("ws0") );
    AnalysisDataService::Instance().clear();
}

  void xtest_removeAll()
  {
    WorkspaceGroup_sptr group = makeGroup();
    group->removeAll();
    TS_ASSERT_EQUALS( group->size(), 0);
    TSM_ASSERT( "removeAll() does not take out of ADS ", AnalysisDataService::Instance().doesExist("ws0") );
    AnalysisDataService::Instance().clear();
  }

  void xtest_deleting_workspaces()
  {
    WorkspaceGroup_sptr group = makeGroup();
    TS_ASSERT( AnalysisDataService::Instance().doesExist("group") );

    // When you delete a workspace it gets removed from the group
    AnalysisDataService::Instance().remove("ws0");
    TS_ASSERT( AnalysisDataService::Instance().doesExist("group") );
    TS_ASSERT( !group->contains("ws0") );

    AnalysisDataService::Instance().remove("ws1");
    TS_ASSERT( AnalysisDataService::Instance().doesExist("group") );
    TS_ASSERT( !group->contains("ws1") );

    // When you remove the last one, the group deletes itself
    AnalysisDataService::Instance().remove("ws2");
    TS_ASSERT( !AnalysisDataService::Instance().doesExist("group") );
    AnalysisDataService::Instance().clear();
  }

  void xtest_areNamesSimilar()
  {
    WorkspaceGroup_sptr group(new WorkspaceGroup());
    group->setName("name");
    TSM_ASSERT( "Empty group is not similar", !group->areNamesSimilar() );

    boost::shared_ptr<WorkspaceTester> ws(new WorkspaceTester());
    ws->initialize(2,3,4);
    AnalysisDataService::Instance().addOrReplace("name_0", ws);

    ws.reset(new WorkspaceTester());
    ws->initialize(2,3,4);
    AnalysisDataService::Instance().addOrReplace("name_12", ws);

    ws.reset(new WorkspaceTester());
    ws->initialize(2,3,4);
    AnalysisDataService::Instance().addOrReplace("name_monkey", ws);

    ws.reset(new WorkspaceTester());
    ws->initialize(2,3,4);
    AnalysisDataService::Instance().addOrReplace("different_name", ws);

    group->add("name_0");
    TS_ASSERT( group->areNamesSimilar() );
    group->add("name_12");
    TS_ASSERT( group->areNamesSimilar() );
    group->add("name_monkey");
    TS_ASSERT( group->areNamesSimilar() );
    group->add("different_name");
    TS_ASSERT( !group->areNamesSimilar() );
    
    AnalysisDataService::Instance().clear();
  }

  void xtestUpdated()
  {
    WorkspaceGroupTest_WorkspaceGroupObserver observer;
    WorkspaceGroup_sptr group(new WorkspaceGroup());
    AnalysisDataService::Instance().add( "group", group );
    TS_ASSERT( !observer.received );

    boost::shared_ptr<WorkspaceTester> ws(new WorkspaceTester());
    ws->initialize(2,3,4);
    AnalysisDataService::Instance().addOrReplace("name_0", ws);

    ws.reset(new WorkspaceTester());
    ws->initialize(2,3,4);
    AnalysisDataService::Instance().addOrReplace("name_12", ws);

    group->add( "name_0" );
    TS_ASSERT( observer.received );
    observer.received = false;

    group->observeADSNotifications( false );

    group->add( "name_12" );
    TS_ASSERT( !observer.received );
    observer.received = false;

    group->observeADSNotifications( true );

    group->remove( "name_12" );
    TS_ASSERT( observer.received );
    observer.received = false;

    group->observeADSNotifications( false );

    group->remove( "name_0" );
    TS_ASSERT( !observer.received );
    observer.received = false;

    AnalysisDataService::Instance().clear();
  }

  void xtest_not_multiperiod_with_less_than_one_element()
  {
    WorkspaceGroup group;
    TSM_ASSERT("Cannot be multiperiod without entries", !group.isMultiperiod());
  }

  void xtest_not_multiperiod_without_matrix_workspaces()
  {
    Workspace_sptr a = boost::make_shared<MockWorkspace>();
    WorkspaceGroup group;
    group.addWorkspace(a);
    TSM_ASSERT("Cannot be multiperiod unless MatrixWorkspaces are used as elements.", !group.isMultiperiod());
  }

  void xtest_not_multiperiod_if_missing_nperiods_log()
  {
    Workspace_sptr a = boost::make_shared<WorkspaceTester>(); // workspace has no nperiods entry.
    WorkspaceGroup group;
    group.addWorkspace(a);
    TSM_ASSERT("Cannot be multiperiod without nperiods log.", !group.isMultiperiod());
  }

  void xtest_not_multiperiod_if_nperiods_log_less_than_one()
  {
    Workspace_sptr a = boost::make_shared<WorkspaceTester>();
    WorkspaceGroup_sptr group = boost::make_shared<WorkspaceGroup>();
    group->addWorkspace(a);
    add_periods_logs(group, 0); // nperiods set to 0.
    TSM_ASSERT("Cannot be multiperiod without nperiods log.", !group->isMultiperiod());
  }

  void xtest_positive_identification_of_multiperiod_data()
  {
    Workspace_sptr a = boost::make_shared<WorkspaceTester>();
    WorkspaceGroup_sptr group = boost::make_shared<WorkspaceGroup>();
    group->addWorkspace(a);
    add_periods_logs(group, 1); 
    TS_ASSERT(group->isMultiperiod());
  }
*/
};


#endif /* MANTID_API_WORKSPACEGROUPTEST_H_ */
