#ifndef WORKSPACEFACTORYTEST_H_
#define WORKSPACEFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include <iostream>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidTestHelpers/FakeObjects.h"

using Mantid::MantidVec;
using std::size_t;
using namespace Mantid::Kernel;
using namespace Mantid::API;

class WorkspaceFactoryTest : public CxxTest::TestSuite
{


  class Workspace1DTest: public WorkspaceTester
  {
  public:
    const std::string id() const {return "Workspace1DTest";}
  };

  class Workspace2DTest: public WorkspaceTester
  {
  public:
    const std::string id() const {return "Workspace2DTest";}

    void init(const size_t &NVectors, const size_t &XLength, const size_t &YLength)
    {
      size.push_back(NVectors);
      size.push_back(XLength);
      size.push_back(YLength);
      WorkspaceTester::init(NVectors, XLength, YLength);
    }
    std::vector<size_t> size;
  };

  class NotInFactory : public WorkspaceTester
  {
  public:
    const std::string id() const {return "NotInFactory";}
  };

// Now the testing class itself
public:
  void testSetup()
  {
    WorkspaceFactory::Instance().subscribe<Workspace1DTest>("Workspace1DTest");
    WorkspaceFactory::Instance().subscribe<Workspace2DTest>("Workspace2DTest");
  }

  void testReturnType()
  {
    WorkspaceFactory::Instance().subscribe<WorkspaceTester>("work");
    MatrixWorkspace_sptr space;
    TS_ASSERT_THROWS_NOTHING( space = WorkspaceFactory::Instance().create("work",1,1,1) );
    // AppleClang gives warning if the result is unused.
#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value"
#endif
      TS_ASSERT_THROWS_NOTHING(dynamic_cast<WorkspaceTester*>(space.get()) );
#if __clang__
#pragma clang diagnostic pop
#endif
  
  }

  /** Make a parent, have the child be created with the same sizes */
  void testCreateFromParent()
  {
    MatrixWorkspace_sptr ws_child(new Workspace1DTest);
    ws_child->initialize(3,1,1);
    ws_child->getSpectrum(0)->setSpectrumNo(123);
    ws_child->getSpectrum(1)->setDetectorID(456);
    ws_child->getSpectrum(2)->dataY()[0]=789;

    ws_child->mutableRun().addProperty("Ei", 12.0);
    ws_child->mutableSample().setName("MySample");

    ws_child->setMonitorWorkspace(boost::make_shared<Workspace1DTest>());

    MatrixWorkspace_sptr child;
    TS_ASSERT_THROWS_NOTHING( child = WorkspaceFactory::Instance().create(ws_child) );
    TS_ASSERT_EQUALS( child->id(), "Workspace1DTest");
    TS_ASSERT_EQUALS( child->getSpectrum(0)->getSpectrumNo(), 123);
    TS_ASSERT_EQUALS( *child->getSpectrum(1)->getDetectorIDs().begin(), 456);
    TS_ASSERT_DIFFERS( child->getSpectrum(2)->dataY()[0], 789);

    // run/logs
    double ei(0.0);
    TS_ASSERT_THROWS_NOTHING(ei = child->run().getPropertyValueAsType<double>("Ei"));
    TS_ASSERT_DELTA(ei, 12.0, 1e-12);

    // sample
    TS_ASSERT_EQUALS("MySample", child->sample().getName());
    
    // Test change in child does not affect parent
    child->mutableRun().addProperty("Ei", 15.0, true);
    TS_ASSERT_THROWS_NOTHING(ei = child->run().getPropertyValueAsType<double>("Ei"));
    TS_ASSERT_DELTA(ei, 15.0, 1e-12)
    TS_ASSERT_THROWS_NOTHING(ei = ws_child->run().getPropertyValueAsType<double>("Ei"));
    TS_ASSERT_DELTA(ei, 12.0, 1e-12);

    // sample
    child->mutableSample().setName("MySampleChild");
    TS_ASSERT_EQUALS("MySample", ws_child->sample().getName());
    TS_ASSERT_EQUALS("MySampleChild", child->sample().getName());
   
    // Monitor workspace
    TSM_ASSERT( "The workspace factory should not propagate a monitor workspace", ! child->monitorWorkspace() );

    MatrixWorkspace_sptr ws2D(new Workspace2DTest);
    ws2D->initialize(3,1,1);
    MatrixWorkspace_sptr child2;
    TS_ASSERT_THROWS_NOTHING( child2 = WorkspaceFactory::Instance().create(ws2D) );
    TS_ASSERT(child2);
    TS_ASSERT_EQUALS( child2->id(), "Workspace2DTest");


    MatrixWorkspace_sptr nif(new NotInFactory);
    nif->initialize(1,1,1);
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

    TS_ASSERT_THROWS_NOTHING( ws = WorkspaceFactory::Instance().create("Workspace1DTest",1,1,1) );
    TS_ASSERT( ! ws->id().compare("Workspace1DTest") );

    TS_ASSERT_THROWS( WorkspaceFactory::Instance().create("NotInFactory",1,1,1), std::runtime_error );
    TS_ASSERT_THROWS( WorkspaceFactory::Instance().create("NotInFactory",10,10,10), std::runtime_error );
  }
};

#endif /*WORKSPACEFACTORYTEST_H_*/
