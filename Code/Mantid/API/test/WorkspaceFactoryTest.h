#ifndef WORKSPACEFACTORYTEST_H_
#define WORKSPACEFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "../inc/Workspace.h"
#include "../inc/WorkspaceFactory.h"

using namespace Mantid::Kernel;

class WorkspaceFactoryTest : public CxxTest::TestSuite
{
	//private test class - using this removes the dependency on the DataObjects library
	class WorkspaceTest: public Workspace
	{
	public:
		const std::string id() const {return "WorkspaceTest";}
	};
public: 

  WorkspaceFactoryTest()
  {
    factory = WorkspaceFactory::Instance();
  }
  
  void testInstance()
  {
    WorkspaceFactory *tester = WorkspaceFactory::Instance();
    TS_ASSERT_EQUALS( factory, tester);
  }
  
  void testReturnType()
  {
    factory->subscribe<WorkspaceTest>("work");
    Workspace *space;
    TS_ASSERT_THROWS_NOTHING( space = factory->create("work") );
    TS_ASSERT_THROWS_NOTHING( dynamic_cast<WorkspaceTest*>(space) );
  }
  
  void testCast()
  {
    TS_ASSERT_THROWS_NOTHING( dynamic_cast<DynamicFactory<Workspace>*>(factory) );
  }
  
private:
  WorkspaceFactory *factory;
  
};

#endif /*WORKSPACEFACTORYTEST_H_*/
