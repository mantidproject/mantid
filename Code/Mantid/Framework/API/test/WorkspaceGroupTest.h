#ifndef MANTID_API_WORKSPACEGROUPTEST_H_
#define MANTID_API_WORKSPACEGROUPTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/FakeObjects.h"
#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include "MantidAPI/WorkspaceGroup.h"

using namespace Mantid;
using namespace Mantid::API;

class WorkspaceGroupTest : public CxxTest::TestSuite
{
public:

  void setUp()
  {
    AnalysisDataService::Instance().clear();
    for (size_t i=0; i<3; i++)
    {
      boost::shared_ptr<WorkspaceTester> ws(new WorkspaceTester());
      ws->initialize(2,3,4);
      AnalysisDataService::Instance().addOrReplace("ws" + Strings::toString(i), ws);
    }
  }

  /// Make a simple group
  WorkspaceGroup_sptr makeGroup()
  {
    WorkspaceGroup_sptr group(new WorkspaceGroup());
    group->add("ws0");
    group->add("ws1");
    group->add("ws2");
    AnalysisDataService::Instance().addOrReplace("group", group);
    return group;
  }

  void test_add()
  {
    WorkspaceGroup_sptr group = makeGroup();
    TS_ASSERT_EQUALS( group->size(), 3);
    TS_ASSERT( group->contains("ws0") );
  }

  void test_getItem()
  {
    WorkspaceGroup_sptr group = makeGroup();
    Workspace_sptr ws1 = group->getItem(1);
    TS_ASSERT_EQUALS( ws1->name(), "ws1");
    Workspace_sptr ws11 = group->getItem("ws1");
    TS_ASSERT_EQUALS( ws1, ws11 );
  }

  void test_remove()
  {
    WorkspaceGroup_sptr group = makeGroup();
    group->remove("ws0");
    TSM_ASSERT( "remove() takes out from group", !group->contains("ws0") );
    TSM_ASSERT( "remove() does not take out of ADS ", AnalysisDataService::Instance().doesExist("ws0") );
  }

  void test_removeAll()
  {
    WorkspaceGroup_sptr group = makeGroup();
    group->removeAll();
    TS_ASSERT_EQUALS( group->size(), 0);
    TSM_ASSERT( "removeAll() does not take out of ADS ", AnalysisDataService::Instance().doesExist("ws0") );
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
  }

  void test_areNamesSimilar()
  {
    WorkspaceGroup_sptr group(new WorkspaceGroup());
    group->setName("name");
    TSM_ASSERT( "Empty group is not similar", !group->areNamesSimilar() );
    group->add("name");
    TSM_ASSERT( "No underscore is not similar", !group->areNamesSimilar() );
    group->removeAll();

    group->add("name_0");
    TS_ASSERT( group->areNamesSimilar() );
    group->add("name_12");
    TS_ASSERT( group->areNamesSimilar() );
    group->add("name_monkey");
    TS_ASSERT( group->areNamesSimilar() );
    group->add("different_name");
    TS_ASSERT( !group->areNamesSimilar() );
  }


};


#endif /* MANTID_API_WORKSPACEGROUPTEST_H_ */
