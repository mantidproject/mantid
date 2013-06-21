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

  /// Make a simple group
  WorkspaceGroup_sptr makeGroup()
  {
    for (size_t i=0; i<3; i++)
    {
      boost::shared_ptr<WorkspaceTester> ws(new WorkspaceTester());
      ws->initialize(2,3,4);
      AnalysisDataService::Instance().addOrReplace("ws" + Strings::toString(i), ws);
    }
    WorkspaceGroup_sptr group(new WorkspaceGroup());
    AnalysisDataService::Instance().addOrReplace("group", group);
    group->add("ws0");
    group->add("ws1");
    group->add("ws2");
    return group;
  }

  void test_add()
  {
    WorkspaceGroup_sptr group = makeGroup();
    TS_ASSERT_EQUALS( group->size(), 3);
    TS_ASSERT( group->contains("ws0") );
    // cannot add a workspace which doesn't exist
    TS_ASSERT_THROWS( group->add("noworkspace"), Kernel::Exception::NotFoundError );
    AnalysisDataService::Instance().clear();
  }

  void test_addWorkspace()
  {
    WorkspaceGroup_sptr group(new WorkspaceGroup());
    Workspace_sptr ws1(new WorkspaceTester());
    group->addWorkspace( ws1 );
    TS_ASSERT_EQUALS( group->size(), 1 );
    Workspace_sptr ws2(new WorkspaceTester());
    group->addWorkspace( ws2 );
    TS_ASSERT_EQUALS( group->size(), 2 );
    TS_ASSERT_EQUALS( AnalysisDataService::Instance().size(), 0 );
    AnalysisDataService::Instance().add("group", group);
    TS_ASSERT_EQUALS( AnalysisDataService::Instance().size(), 3 );
    AnalysisDataService::Instance().clear();
  }

  void test_addWorkspace_when_group_in_ADS()
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

  void test_getNames()
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

  void test_getItem()
  {
    WorkspaceGroup_sptr group = makeGroup();
    Workspace_sptr ws1 = group->getItem(1);
    TS_ASSERT_EQUALS( ws1->name(), "ws1");
    // Test the 'by name' overload
    Workspace_sptr ws11 = group->getItem("ws1");
    TS_ASSERT_EQUALS( ws1, ws11 );
    // Test for failure too
    TS_ASSERT_THROWS( group->getItem("non-existent"), std::out_of_range);
    TS_ASSERT_THROWS( group->getItem(""), std::out_of_range);
    AnalysisDataService::Instance().clear();
  }

  void test_remove()
  {
    WorkspaceGroup_sptr group = makeGroup();
    group->remove("ws0");
    TSM_ASSERT( "remove() takes out from group", !group->contains("ws0") );
    TSM_ASSERT( "remove() does not take out of ADS ", AnalysisDataService::Instance().doesExist("ws0") );
    AnalysisDataService::Instance().clear();
}

  void test_removeItem()
  {
    WorkspaceGroup_sptr group1 = makeGroup();
    TS_ASSERT_THROWS( group1->removeItem(1), std::runtime_error );

    WorkspaceGroup_sptr group(new WorkspaceGroup());
    Workspace_sptr ws1(new WorkspaceTester());
    group->addWorkspace( ws1 );
    Workspace_sptr ws2(new WorkspaceTester());
    group->addWorkspace( ws2 );

    TS_ASSERT_EQUALS( group->size(), 2 );
    TS_ASSERT_THROWS_NOTHING( group->removeItem(1) );
    TS_ASSERT_EQUALS( group->size(), 1 );
    TS_ASSERT_EQUALS( group->getItem(0), ws1 );

    AnalysisDataService::Instance().clear();
  }

  void test_removeAll()
  {
    WorkspaceGroup_sptr group = makeGroup();
    group->removeAll();
    TS_ASSERT_EQUALS( group->size(), 0);
    TSM_ASSERT( "removeAll() does not take out of ADS ", AnalysisDataService::Instance().doesExist("ws0") );
    AnalysisDataService::Instance().clear();
  }

  void test_deleting_workspaces()
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

  void test_areNamesSimilar()
  {
    WorkspaceGroup_sptr group(new WorkspaceGroup());
    //group->setName("name");
    AnalysisDataService::Instance().add("name",group);
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

  void test_not_multiperiod_with_less_than_one_element()
  {
    WorkspaceGroup group;
    TSM_ASSERT("Cannot be multiperiod without entries", !group.isMultiperiod());
  }

  void test_not_multiperiod_without_matrix_workspaces()
  {
    Workspace_sptr a = boost::make_shared<MockWorkspace>();
    WorkspaceGroup group;
    group.addWorkspace(a);
    TSM_ASSERT("Cannot be multiperiod unless MatrixWorkspaces are used as elements.", !group.isMultiperiod());
  }

  void test_not_multiperiod_if_missing_nperiods_log()
  {
    Workspace_sptr a = boost::make_shared<WorkspaceTester>(); // workspace has no nperiods entry.
    WorkspaceGroup group;
    group.addWorkspace(a);
    TSM_ASSERT("Cannot be multiperiod without nperiods log.", !group.isMultiperiod());
  }

  void test_not_multiperiod_if_nperiods_log_less_than_one()
  {
    Workspace_sptr a = boost::make_shared<WorkspaceTester>();
    WorkspaceGroup_sptr group = boost::make_shared<WorkspaceGroup>();
    group->addWorkspace(a);
    add_periods_logs(group, 0); // nperiods set to 0.
    TSM_ASSERT("Cannot be multiperiod without nperiods log.", !group->isMultiperiod());
  }

  void test_positive_identification_of_multiperiod_data()
  {
    Workspace_sptr a = boost::make_shared<WorkspaceTester>();
    WorkspaceGroup_sptr group = boost::make_shared<WorkspaceGroup>();
    group->addWorkspace(a);
    add_periods_logs(group, 1); 
    TS_ASSERT(group->isMultiperiod());
  }

  void test_InfoNode()
  {
      WorkspaceGroup_sptr group = makeGroup();
      Mantid::API::Workspace::InfoNode rootNode( *group );
      group->addInfoNodeTo( rootNode );
      Mantid::API::Workspace::InfoNode &node = *rootNode.nodes()[0];
      TS_ASSERT_EQUALS( node.nodes().size(), 3 );

      TS_ASSERT_EQUALS( node.lines()[0], "group" );          // workspace name
      TS_ASSERT_EQUALS( node.lines()[1], "WorkspaceGroup" ); // workspace id
  }

  void test_isInGroup()
  {
      WorkspaceGroup_sptr group = makeGroup();
      auto ws1 = group->getItem(1);
      TS_ASSERT( group->isInGroup( *ws1 ) );
      Workspace_sptr a = boost::make_shared<WorkspaceTester>();
      TS_ASSERT( !group->isInGroup( *a ) );

      WorkspaceGroup_sptr group1 = boost::make_shared<WorkspaceGroup>();
      group1->addWorkspace( a );
      group->addWorkspace( group1 );
      TS_ASSERT( group->isInGroup( *a ) );

      // catch a cycle
      group1->addWorkspace( group );
      Workspace_sptr b = boost::make_shared<WorkspaceTester>();
      TS_ASSERT_THROWS( group->isInGroup( *b ), std::runtime_error );
  }

};


#endif /* MANTID_API_WORKSPACEGROUPTEST_H_ */
