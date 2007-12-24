#ifndef WORKSPACEPROPERTYTEST_H_
#define WORKSPACEPROPERTYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceProperty.h"
#include "boost/shared_ptr.hpp"

using namespace Mantid::Kernel;
using namespace Mantid::API;

class WorkspacePropertyTest : public CxxTest::TestSuite
{ 
  
  //private test class - using this removes the dependency on the DataObjects library
	class WorkspaceTest: public Workspace
	{
	public:
		const std::string id() const {return "WorkspacePropTest";}
    //section required to support iteration
    virtual int size() const {return 0;}
      virtual int blocksize() const  {return 1000000;}
    virtual std::vector<double>& dataX(int const index) {return data;}
    ///Returns the y data
    virtual std::vector<double>& dataY(int const index) {return data;}
    ///Returns the error data
    virtual std::vector<double>& dataE(int const index) {return data;}
 
    virtual const std::vector<double>& dataX(int const index)const {return data;}
    ///Returns the y data
    virtual const std::vector<double>& dataY(int const index)const {return data;}
    ///Returns the error data
    virtual const std::vector<double>& dataE(int const index)const {return data;}
  private:
    std::vector<double> data;
	};

public:
  WorkspacePropertyTest()
  {
    wsp = new WorkspaceProperty<Workspace>("workspace1","ws1",Direction::Input);
    wsp1D = new WorkspaceProperty<Workspace>("workspace2","",Direction::Output);
    wsp2D = new WorkspaceProperty<Workspace>("workspace3","ws3",Direction::InOut);
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

    WorkspaceFactory* factory = WorkspaceFactory::Instance();
    factory->subscribe<WorkspaceTest>("WorkspacePropertyTest");

    Workspace_sptr space;
    TS_ASSERT_THROWS_NOTHING( space = factory->create("WorkspacePropertyTest") );
    TS_ASSERT( !wsp->isValid() )
    TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance()->add("ws1", space) );
    TS_ASSERT( wsp->isValid() )

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
  WorkspaceProperty<Workspace> *wsp1D;
  WorkspaceProperty<Workspace> *wsp2D;
};

#endif /*WORKSPACEPROPERTYTEST_H_*/
