#ifndef WORKSPACEPROPERTYTEST_H_
#define WORKSPACEPROPERTYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::DataObjects::Workspace1D;
using Mantid::DataObjects::Workspace2D;

class WorkspacePropertyTest : public CxxTest::TestSuite
{
public:
  WorkspacePropertyTest()
  {
    wsp = new WorkspaceProperty<Workspace>("workspace1","ws1",Direction::Input);
    wsp1D = new WorkspaceProperty<Workspace1D>("workspace2","",Direction::Output);
    wsp2D = new WorkspaceProperty<Workspace2D>("workspace3","ws3",Direction::InOut);
  }

  ~WorkspacePropertyTest()
  {
    delete wsp;
    delete wsp1D;
    delete wsp2D;
  }
  
//	void testOperator=()
//	{
//		// TODO: Implement testOperator =() function.
//	}

	void testValue()
	{
		TS_ASSERT( ! wsp->value().compare("ws1") )
    TS_ASSERT( ! wsp1D->value().compare("") )
    TS_ASSERT( ! wsp2D->value().compare("ws3") )
	}

	void testSetValue()
	{
		TS_ASSERT( ! wsp->setValue("") )
		TS_ASSERT( ! wsp->value().compare("ws1") )
		TS_ASSERT( wsp->setValue("newValue") )
		TS_ASSERT( ! wsp->value().compare("newValue") )
		TS_ASSERT( wsp->setValue("ws1") )
	}

	void testIsValid()
	{
	  TS_ASSERT( ! wsp->isValid() )
    TS_ASSERT( ! wsp1D->isValid() )
    TS_ASSERT( ! wsp2D->isValid() )

    TS_ASSERT( wsp1D->setValue("ws2") )
    TS_ASSERT( wsp1D->isValid() )

    Workspace *ws;
    TS_ASSERT_THROWS_NOTHING( ws = WorkspaceFactory::Instance()->create("Workspace1D") )
    TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance()->add("ws1", ws) );
    TS_ASSERT( wsp->isValid() )
    TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance()->add("ws3", ws) );
    TS_ASSERT( ! wsp2D->isValid() )
    TS_ASSERT_THROWS_NOTHING( ws = WorkspaceFactory::Instance()->create("Workspace2D") )
    TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance()->addOrReplace("ws3", ws) );
    TS_ASSERT( wsp2D->isValid() )

	}

	void testStore()
	{
		// TODO: Implement testStore() function.
	}

	void testClear()
	{
		// TODO: Implement testClear() function.
	}

private:
	WorkspaceProperty<Workspace> *wsp;
  WorkspaceProperty<Workspace1D> *wsp1D;
  WorkspaceProperty<Workspace2D> *wsp2D;
};

#endif /*WORKSPACEPROPERTYTEST_H_*/
