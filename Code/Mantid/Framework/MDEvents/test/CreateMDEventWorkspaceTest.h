#ifndef MANTID_MDEVENTS_CREATEMDEVENTWORKSPACETEST_H_
#define MANTID_MDEVENTS_CREATEMDEVENTWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include <MantidKernel/Timer.h>
#include <MantidKernel/System.h>
#include <iostream>
#include <iomanip>

#include <MantidMDEvents/CreateMDEventWorkspace.h>
#include <MantidMDEvents/MDEventFactory.h>
#include <MantidAPI/IMDEventWorkspace.h>
#include <MantidAPI/AnalysisDataService.h>

using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid::Geometry;

class CreateMDEventWorkspaceTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    CreateMDEventWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    std::string wsName = "CreateMDEventWorkspaceTest_out";
    CreateMDEventWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    alg.setPropertyValue("Dimensions", "3");
    alg.setPropertyValue("Extents", "-1,1,-2,2,-3,3");
    alg.setPropertyValue("Names", "x,y,z");
    alg.setPropertyValue("Units", "m,mm,um");
    alg.setPropertyValue("SplitInto", "6");
    alg.setPropertyValue("SplitThreshold", "500");
    alg.setPropertyValue("MaxRecursionDepth", "7");
    alg.setPropertyValue("OutputWorkspace",wsName);

    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Get it from data service
    IMDEventWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = boost::dynamic_pointer_cast<IMDEventWorkspace>( AnalysisDataService::Instance().retrieve(wsName) ));
    TS_ASSERT( ws );

    // Correct info?
    TS_ASSERT_EQUALS( ws->getNumDims(), 3);
    TS_ASSERT_EQUALS( ws->getNPoints(), 0);

    IMDDimension_sptr dim;
    dim = ws->getDimension(0);
    TS_ASSERT_DELTA( dim->getMaximum(), 1.0, 1e-6);
    TS_ASSERT_EQUALS( dim->getName(), "x");
    TS_ASSERT_EQUALS( dim->getUnits(), "m");
    dim = ws->getDimension(1);
    TS_ASSERT_DELTA( dim->getMaximum(), 2.0, 1e-6);
    TS_ASSERT_EQUALS( dim->getName(), "y");
    TS_ASSERT_EQUALS( dim->getUnits(), "mm");
    dim = ws->getDimension(2);
    TS_ASSERT_DELTA( dim->getMaximum(), 3.0, 1e-6);
    TS_ASSERT_EQUALS( dim->getName(), "z");
    TS_ASSERT_EQUALS( dim->getUnits(), "um");

    // What about the box controller
    MDEventWorkspace3::sptr ews = boost::dynamic_pointer_cast<MDEventWorkspace3>(ws);
    TS_ASSERT( ews );
    if (!ews) return;
    BoxController_sptr bc = ews->getBoxController();
    TS_ASSERT( bc );
    if (!bc) return;
    TS_ASSERT_EQUALS(bc->getSplitInto(0), 6 );
    TS_ASSERT_EQUALS(bc->getSplitThreshold(), 500 );
    TS_ASSERT_EQUALS(bc->getMaxDepth(), 7 );
  }


};


#endif /* MANTID_MDEVENTS_CREATEMDEVENTWORKSPACETEST_H_ */

