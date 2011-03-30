#ifndef MANTID_MDEVENTS_FAKEMDEVENTDATATEST_H_
#define MANTID_MDEVENTS_FAKEMDEVENTDATATEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/FakeMDEventData.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MDEventsTestHelper.hh"

using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class FakeMDEventDataTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    FakeMDEventData alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    FakeMDEventData alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )

    IMDEventWorkspace_sptr in_ws = MDEventsHelper::makeMDEW<3>(10, 0.0, 10.0, 1);
    AnalysisDataService::Instance().addOrReplace("FakeMDEventDataTest_ws", in_ws);

    // 1000 boxes with 1 event each
    TS_ASSERT_EQUALS( in_ws->getNPoints(), 1000);

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "FakeMDEventDataTest_ws") );
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeakParams", "1000, 5.0,5.0,5.0, 1.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("UniformParams", "10000"));

    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
    TS_ASSERT( alg.isExecuted() );

    // Now there are 1000 more points.
    TS_ASSERT_EQUALS( in_ws->getNPoints(), 12000);

    AnalysisDataService::Instance().remove("FakeMDEventDataTest_ws");

  }


};


#endif /* MANTID_MDEVENTS_FAKEMDEVENTDATATEST_H_ */

