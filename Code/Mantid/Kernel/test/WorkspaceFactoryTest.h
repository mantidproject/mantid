#ifndef WORKSPACEFACTORYTEST_H_
#define WORKSPACEFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "../inc/WorkspaceFactory.h"
#include "../../DataObjects/inc/Workspace1D.h"

using namespace Mantid;

class WorkspaceFactoryTest : public CxxTest::TestSuite
{
public: 

  WorkspaceFactoryTest()
  {
    factory = Mantid::WorkspaceFactory::Instance();
  }
  
  void testInstance()
  {
    WorkspaceFactory *tester = WorkspaceFactory::Instance();
    TS_ASSERT_EQUALS( factory, tester);
  }
  
  void testReturnType()
  {
    factory->subscribe<Workspace1D>("work");
    Workspace *space;
    TS_ASSERT_THROWS_NOTHING( space = factory->create("work") );
    TS_ASSERT_THROWS_NOTHING( dynamic_cast<Workspace1D*>(space) );
  }
  
  void testCast()
  {
    TS_ASSERT_THROWS_NOTHING( dynamic_cast<DynamicFactory<Workspace>*>(factory) );
  }
  
private:
  WorkspaceFactory *factory;
  
};

#endif /*WORKSPACEFACTORYTEST_H_*/
