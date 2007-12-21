#ifndef WORKSPACEFACTORYTEST_H_
#define WORKSPACEFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>

#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

class WorkspaceFactoryTest : public CxxTest::TestSuite
{
	//private test class - using this removes the dependency on the DataObjects library
	class WorkspaceTest: public Workspace
	{
	public:
		const std::string id() const {return "WorkspaceTest";}
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
