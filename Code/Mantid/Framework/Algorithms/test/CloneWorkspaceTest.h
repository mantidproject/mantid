#ifndef CLONEWORKSPACETEST_H_
#define CLONEWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CloneWorkspace.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAlgorithms/CheckWorkspacesMatch.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidDataObjects/PeaksWorkspace.h"

using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDEvents;
using Mantid::MDEvents::MDEventsTestHelper::makeMDEW;

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
    loader.setPropertyValue("Filename", "LOQ48127.raw");
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

    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), checker.successString() );
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
    
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), checker.successString());
  }

  /** Test is not full, see CloneMDWorkspaceTest */
  void test_exec_MDEventWorkspace()
  {
    MDEventWorkspace3Lean::sptr ws = MDEventsTestHelper::makeMDEW<3>(5, 0.0, 10.0, 1);
    AnalysisDataService::Instance().addOrReplace("inWS", ws);
    Mantid::Algorithms::CloneWorkspace alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace","inWS");
    alg.setPropertyValue("OutputWorkspace","outWS");
    TS_ASSERT( alg.execute() );
    TS_ASSERT( alg.isExecuted() );
  }


  /** Build a test PeaksWorkspace
   * @return PeaksWorkspace   */
  PeaksWorkspace_sptr buildPW()
  {
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, 10);
    inst->setName("SillyInstrument");
    PeaksWorkspace * pw = new PeaksWorkspace();
    pw->setInstrument(inst);
    std::string val = "value";
    pw->mutableRun().addProperty("TestProp", val);
    Peak p(inst, 1, 3.0);
    pw->addPeak(p);
    return PeaksWorkspace_sptr(pw);
  }

  /** Test is not full, see PeaksWorkspaceTest */
  void test_exec_PeaksWorkspace()
  {
    PeaksWorkspace_sptr ws = buildPW();
    AnalysisDataService::Instance().addOrReplace("inWS", ws);
    Mantid::Algorithms::CloneWorkspace alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace","inWS");
    alg.setPropertyValue("OutputWorkspace","outWS");
    TS_ASSERT( alg.execute() );
    TS_ASSERT( alg.isExecuted() );
  }

private:
  Mantid::Algorithms::CloneWorkspace cloner;
};

#endif /*CLONEWORKSPACETEST_H_*/
