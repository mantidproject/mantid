#ifndef CLONEWORKSPACETEST_H_
#define CLONEWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CloneWorkspace.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAlgorithms/CheckWorkspacesMatch.h"
#include "WorkspaceCreationHelper.hh"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class CloneWorkspaceTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( cloner.name(), "CloneWorkspace" );
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( cloner.version(), 1 );
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( cloner.category(), "General" );
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( cloner.initialize() );
    TS_ASSERT( cloner.isInitialized() );
  }

  void testExec()
  {
    if ( !cloner.isInitialized() ) cloner.initialize();
    
    Mantid::DataHandling::LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "../../../../Test/AutoTestData/LOQ48127.raw");
    loader.setPropertyValue("OutputWorkspace", "in");
    loader.execute();
    
    TS_ASSERT_THROWS_NOTHING( cloner.setPropertyValue("InputWorkspace","in") );
    TS_ASSERT_THROWS_NOTHING( cloner.setPropertyValue("OutputWorkspace","out") );

    TS_ASSERT( cloner.execute() )   ;

    // Best way to test this is to use the CheckWorkspacesMatch algorithm
    Mantid::Algorithms::CheckWorkspacesMatch checker;
    checker.initialize();
    checker.setPropertyValue("Workspace1","in");
    checker.setPropertyValue("Workspace2","out");
    checker.execute();

    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Success!" );
  }


  void testExecEvent()
  {
    // First make the algorithm
    EventWorkspace_sptr ew  = WorkspaceCreationHelper::CreateEventWorkspace(100, 60, 50);
    AnalysisDataService::Instance().addOrReplace("in_event", ew);

    Mantid::Algorithms::CloneWorkspace alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace","in_event");
    alg.setPropertyValue("OutputWorkspace","out_event");
    TS_ASSERT( alg.execute() );
    TS_ASSERT( alg.isExecuted() );
    
    // Best way to test this is to use the CheckWorkspacesMatch algorithm
    Mantid::Algorithms::CheckWorkspacesMatch checker;
    checker.initialize();
    checker.setPropertyValue("Workspace1","in_event");
    checker.setPropertyValue("Workspace2","out_event");
    checker.execute();
    
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Success!" );
  }
  
private:
  Mantid::Algorithms::CloneWorkspace cloner;
};

#endif /*CLONEWORKSPACETEST_H_*/
