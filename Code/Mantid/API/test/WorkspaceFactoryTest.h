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
    ///Returns the error data
    virtual std::vector<double>& dataE2(int const index) {return data;}
 
    virtual const std::vector<double>& dataX(int const index)const {return data;}
    ///Returns the y data
    virtual const std::vector<double>& dataY(int const index)const {return data;}
    ///Returns the error data
    virtual const std::vector<double>& dataE(int const index)const {return data;}
    ///Returns the error data
    virtual const std::vector<double>& dataE2(int const index)const {return data;}
    virtual void init(const int &NVectors, const int &XLength, const int &YLength){};

  private:
    std::vector<double> data;
	};
public: 

  WorkspaceFactoryTest()
  {

  }
  
  void testReturnType()
  {
    WorkspaceFactory::Instance().subscribe<WorkspaceTest>("work");
    Workspace_sptr space;
    TS_ASSERT_THROWS_NOTHING( space = WorkspaceFactory::Instance().create("work") );
    TS_ASSERT_THROWS_NOTHING( dynamic_cast<WorkspaceTest*>(space.get()) );
  }
  
private:
  
};

#endif /*WORKSPACEFACTORYTEST_H_*/
