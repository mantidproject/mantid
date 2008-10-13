#ifndef WORKSPACEFACTORYTEST_H_
#define WORKSPACEFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include <iostream>

#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/GaussianErrorHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/MemoryManager.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

class WorkspaceFactoryTest : public CxxTest::TestSuite
{
  //private test classes - using this removes the dependency on the DataObjects library
  class WorkspaceTest: public Workspace
  {
  public:
	virtual const int getNumberHistograms() const { return 1;}

    WorkspaceTest() : data(std::vector<double>(1,1)) {}
    virtual const std::string id() const {return "WorkspaceTest";}
    //section required to support iteration
    virtual int size() const {return 1000000;}
    virtual int blocksize() const  {return 10000;}
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
    virtual void init(const int &NVectors, const int &XLength, const int &YLength){};

    ///Returns the ErrorHelper applicable for this spectra
    virtual const IErrorHelper* errorHelper(int const index) const { return GaussianErrorHelper::Instance();}
    ///Sets the ErrorHelper for this spectra
    virtual void setErrorHelper(int const index,IErrorHelper* errorHelper) {}
    ///Sets the ErrorHelper for this spectra
    virtual void setErrorHelper(int const index,const IErrorHelper* errorHelper) {}


  //Methods for getting data via python. Do not use for anything else!
  ///Returns the x data const
  virtual const std::vector<double>& getX(int const index) const {return data;}
  ///Returns the y data const
  virtual const std::vector<double>& getY(int const index) const {return data;}
  ///Returns the error const
  virtual const std::vector<double>& getE(int const index) const {return data;}


  private:
    std::vector<double> data;
    int dummy;
  };

  class Workspace1DTest: public WorkspaceTest
  {
  public:
    const int getNumberHistograms() const { return 1;}
    const std::string id() const {return "Workspace1DTest";}
  };

  class Workspace2DTest: public WorkspaceTest
  {
  public:
    const std::string id() const {return "Workspace2DTest";}
    const int getNumberHistograms() const { return 2;}

    void init(const int &NVectors, const int &XLength, const int &YLength)
    {
      size.push_back(NVectors);
      size.push_back(XLength);
      size.push_back(YLength);
    }
    std::vector<int> size;
  };

  class ManagedWorkspace2DTest: public Workspace2DTest
  {
  public:
    const std::string id() const {return "ManagedWorkspace2D";}
    const int getNumberHistograms() const { return 2;}
  };

  class NotInFactory : public WorkspaceTest
  {
  public:
    const std::string id() const {return "NotInFactory";}
  };

// Now the testing class itself
public:
  WorkspaceFactoryTest()
  {
    ConfigService::Instance().loadConfig("MantidTest.properties");

    WorkspaceFactory::Instance().subscribe<Workspace1DTest>("Workspace1DTest");
    WorkspaceFactory::Instance().subscribe<Workspace2DTest>("Workspace2DTest");
    try
    {
      WorkspaceFactory::Instance().subscribe<ManagedWorkspace2DTest>("ManagedWorkspace2D");
    }
    catch (std::runtime_error e)
    {
      // In theory, we shouldn't have the 'real' ManagedWorkspace2D when running this test, but
      // in reality we do so need catch the error from trying to subscribe again
    }
  }

  void testReturnType()
  {
    WorkspaceFactory::Instance().subscribe<WorkspaceTest>("work");
    Workspace_sptr space;
    TS_ASSERT_THROWS_NOTHING( space = WorkspaceFactory::Instance().create("work") );
    TS_ASSERT_THROWS_NOTHING( dynamic_cast<WorkspaceTest*>(space.get()) );
  }

  void testCreateFromParent()
  {
    Workspace_sptr ws1D(new Workspace1DTest);
    Workspace_sptr child;
    TS_ASSERT_THROWS_NOTHING( child = WorkspaceFactory::Instance().create(ws1D) )
    TS_ASSERT( ! child->id().compare("Workspace1DTest") )

    Workspace_sptr ws2D(new Workspace2DTest);
    TS_ASSERT_THROWS_NOTHING( child = WorkspaceFactory::Instance().create(ws2D) )
        TS_ASSERT( child->id().find("2D") != std::string::npos )

    Workspace_sptr mws2D(new ManagedWorkspace2DTest);
    TS_ASSERT_THROWS_NOTHING( child = WorkspaceFactory::Instance().create(mws2D) )
    TS_ASSERT( ! child->id().compare("ManagedWorkspace2D") )

    Workspace_sptr nif(new NotInFactory);
    TS_ASSERT_THROWS( child = WorkspaceFactory::Instance().create(nif), std::runtime_error )
  }

  void testAccordingToSize()
  {
    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = WorkspaceFactory::Instance().create("Workspace2DTest",1,2,3) )
    TS_ASSERT( ! ws->id().compare("Workspace2DTest") )
    Workspace2DTest& space = dynamic_cast<Workspace2DTest&>(*ws);
    TS_ASSERT_EQUALS( space.size[0], 1 )
    TS_ASSERT_EQUALS( space.size[1], 2 )
    TS_ASSERT_EQUALS( space.size[2], 3 )

    // ManagedWorkspace.MinSize should be set to 1 in MantidTest.properties file
    MemoryInfo mi = MemoryManager::Instance().getMemoryInfo();
    int nHist = mi.availMemory / 50 / 100 / 3 * 1024 / 8;// this shoulf fill about 2% of free memory
    TS_ASSERT_THROWS_NOTHING( ws = WorkspaceFactory::Instance().create("Workspace2DTest",nHist,100,100) )
    TS_ASSERT( ! ws->id().compare("ManagedWorkspace2D") )

    TS_ASSERT_THROWS_NOTHING( ws = WorkspaceFactory::Instance().create("Workspace1DTest",1,1,1) )
    TS_ASSERT( ! ws->id().compare("Workspace1DTest") )

    TS_ASSERT_THROWS_NOTHING( ws = WorkspaceFactory::Instance().create("Workspace1DTest",nHist,100,100) )
    TS_ASSERT( ! ws->id().compare("Workspace1DTest") )

    TS_ASSERT_THROWS( WorkspaceFactory::Instance().create("NotInFactory",1,1,1), std::runtime_error )
    TS_ASSERT_THROWS( WorkspaceFactory::Instance().create("NotInFactory",10,10,10), std::runtime_error )
  }
};

#endif /*WORKSPACEFACTORYTEST_H_*/
