#ifndef WORKSPACEFACTORYTEST_H_
#define WORKSPACEFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include <iostream>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/MemoryManager.h"

using Mantid::MantidVec;
using namespace Mantid::Kernel;
using namespace Mantid::API;

class WorkspaceFactoryTest : public CxxTest::TestSuite
{
  //private test classes - using this removes the dependency on the DataObjects library
  class WorkspaceTest: public MatrixWorkspace
  {
  public:
    virtual int getNumberHistograms() const { return 1;}

    WorkspaceTest() : data(MantidVec(1,1)) {}
   	//  static std::string WSTYPE;
	virtual const std::string id() const {return "WorkspaceTest";}
    //section required to support iteration
    virtual int size() const {return 1000000;}
    virtual int blocksize() const  {return 10000;}
    virtual MantidVec& dataX(int const) {return data;}
    ///Returns the y data
    virtual MantidVec& dataY(int const) {return data;}
    ///Returns the error data
    virtual MantidVec& dataE(int const) {return data;}

    virtual const MantidVec& dataX(int const)const {return data;}
    ///Returns the y data
    virtual const MantidVec& dataY(int const)const {return data;}
    ///Returns the error data
    virtual const MantidVec& dataE(int const)const {return data;}
    cow_ptr<MantidVec> refX(const int) const {return cow_ptr<MantidVec>();}
    void setX(const int, const cow_ptr<MantidVec>& ) {}
    
    virtual void init(const int &, const int &, const int &){};

  private:
    MantidVec data;
    int dummy;
  };

  class Workspace1DTest: public WorkspaceTest
  {
  public:
    int getNumberHistograms() const { return 1;}
    //  static std::string WSTYPE;
    const std::string id() const {return "Workspace1DTest";}
  };

  class Workspace2DTest: public WorkspaceTest
  {
  public:
  	//  static std::string WSTYPE;
    const std::string id() const {return "Workspace2DTest";}
    int getNumberHistograms() const { return 2;}

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
  	//  static std::string WSTYPE;
    const std::string id() const {return "ManagedWorkspace2D";}
    int getNumberHistograms() const { return 2;}
  };

  class NotInFactory : public WorkspaceTest
  {
  public:
	//  static std::string WSTYPE;
    const std::string id() const {return "NotInFactory";}
  };

// Now the testing class itself
public:
  void testSetup()
  {
    ConfigService::Instance().updateConfig("MantidTest.properties");

    WorkspaceFactory::Instance().subscribe<Workspace1DTest>("Workspace1DTest");
    WorkspaceFactory::Instance().subscribe<Workspace2DTest>("Workspace2DTest");
    try
    {
      WorkspaceFactory::Instance().subscribe<ManagedWorkspace2DTest>("ManagedWorkspace2D");
    }
    catch (std::runtime_error& e)
    {
      // In theory, we shouldn't have the 'real' ManagedWorkspace2D when running this test, but
      // in reality we do so need catch the error from trying to subscribe again
    }
  }

  void testReturnType()
  {
    WorkspaceFactory::Instance().subscribe<WorkspaceTest>("work");
    MatrixWorkspace_sptr space;
    TS_ASSERT_THROWS_NOTHING( space = WorkspaceFactory::Instance().create("work",1,1,1) );
    TS_ASSERT_THROWS_NOTHING( dynamic_cast<WorkspaceTest*>(space.get()) );
  }

  void testCreateFromParent()
  {
    MatrixWorkspace_sptr ws1D(new Workspace1DTest);
    MatrixWorkspace_sptr child;
    TS_ASSERT_THROWS_NOTHING( child = WorkspaceFactory::Instance().create(ws1D) );
    TS_ASSERT( ! child->id().compare("Workspace1DTest") );

    MatrixWorkspace_sptr ws2D(new Workspace2DTest);
    TS_ASSERT_THROWS_NOTHING( child = WorkspaceFactory::Instance().create(ws2D) );
        TS_ASSERT( child->id().find("2D") != std::string::npos );

    //Workspace_sptr mws2D(new ManagedWorkspace2DTest);
    //TS_ASSERT_THROWS_NOTHING( child = WorkspaceFactory::Instance().create(mws2D) );
    //TS_ASSERT_EQUALS( child->id(), "ManagedWorkspace2D");

    MatrixWorkspace_sptr nif(new NotInFactory);
    TS_ASSERT_THROWS( child = WorkspaceFactory::Instance().create(nif), std::runtime_error );
  }

  void testAccordingToSize()
  {
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = WorkspaceFactory::Instance().create("Workspace2DTest",1,2,3) );
    TS_ASSERT( ! ws->id().compare("Workspace2DTest") );
    Workspace2DTest& space = dynamic_cast<Workspace2DTest&>(*ws);
    TS_ASSERT_EQUALS( space.size[0], 1 );
    TS_ASSERT_EQUALS( space.size[1], 2 );
    TS_ASSERT_EQUALS( space.size[2], 3 );

    // ManagedWorkspace.LowerMemoryLimit should be set to 1 in MantidTest.properties file
    MemoryInfo mi = MemoryManager::Instance().getMemoryInfo();
    int nHist = mi.availMemory / 50 / 100 / 3 * 1024 / 8;// this shoulf fill about 2% of free memory
    //TS_ASSERT_THROWS_NOTHING( ws = WorkspaceFactory::Instance().create("Workspace2DTest",nHist,100,100) )
    //TS_ASSERT_EQUALS( ws->id(), "Workspace2D")

    TS_ASSERT_THROWS_NOTHING( ws = WorkspaceFactory::Instance().create("Workspace1DTest",1,1,1) );
    TS_ASSERT( ! ws->id().compare("Workspace1DTest") );

    TS_ASSERT_THROWS_NOTHING( ws = WorkspaceFactory::Instance().create("Workspace1DTest",nHist,100,100) );
    TS_ASSERT( ! ws->id().compare("Workspace1DTest") );

    TS_ASSERT_THROWS( WorkspaceFactory::Instance().create("NotInFactory",1,1,1), std::runtime_error );
    TS_ASSERT_THROWS( WorkspaceFactory::Instance().create("NotInFactory",10,10,10), std::runtime_error );

    ConfigService::Instance().updateConfig("Mantid.properties");
  }
};

#endif /*WORKSPACEFACTORYTEST_H_*/
