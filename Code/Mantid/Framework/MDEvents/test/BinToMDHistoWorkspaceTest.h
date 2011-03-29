#ifndef MANTID_MDEVENTS_BINTOMDHISTOWORKSPACETEST_H_
#define MANTID_MDEVENTS_BINTOMDHISTOWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/BinToMDHistoWorkspace.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MDEventsTestHelper.hh"

using namespace Mantid::MDEvents;
using namespace Mantid::API;

class BinToMDHistoWorkspaceTest : public CxxTest::TestSuite
{
public:


  void test_Init()
  {
    BinToMDHistoWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    BinToMDHistoWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )

    IMDEventWorkspace_sptr in_ws = MDEventsHelper::makeMDEW<3>(10, 0.0, 10.0, 1);
    // 1000 boxes with 1 event each
    TS_ASSERT_EQUALS( in_ws->getNPoints(), 1000);

    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", boost::dynamic_pointer_cast<Workspace>(in_ws)) );
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimX", "Axis0,2.0,8.0, 6"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimY", "Axis1,2.0,8.0, 6"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimZ", "Axis2,2.0,8.0, 6"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimT", "NONE,0.0,10.0,1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "BinToMDHistoWorkspaceTest"));

    alg.execute();

    TS_ASSERT( alg.isExecuted() );

    IMDWorkspace_sptr out ;
    TS_ASSERT_THROWS_NOTHING( out = boost::dynamic_pointer_cast<IMDWorkspace>(
        AnalysisDataService::Instance().retrieve("BinToMDHistoWorkspaceTest")); )
    TS_ASSERT(out);

    // Took 6x6x6 bins in the middle of the box
    TS_ASSERT_EQUALS(out->getNPoints(), 6*6*6);
    // Every box has a single event summed into it, so 1.0 weight
    for (size_t i=0; i < out->getNPoints(); i++)
    {
      TS_ASSERT_DELTA(out->getSignalAt(i), 1.0, 1e-5);
      TS_ASSERT_DELTA(out->getErrorAt(i), 1.0, 1e-5);
    }

    AnalysisDataService::Instance().remove("BinToMDHistoWorkspaceTest");
  }


};


#endif /* MANTID_MDEVENTS_BINTOMDHISTOWORKSPACETEST_H_ */

