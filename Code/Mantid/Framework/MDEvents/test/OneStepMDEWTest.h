#ifndef MANTID_MDEVENTS_ONESTEPMDEWTEST_H_
#define MANTID_MDEVENTS_ONESTEPMDEWTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/OneStepMDEW.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::MDEvents;

class OneStepMDEWTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    OneStepMDEW alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    ConfigService::Instance().setString("default.facility", "SNS");

    OneStepMDEW alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    alg.setPropertyValue("Filename", "HYS_11092_event.nxs");
    alg.setPropertyValue("OutputWorkspace", "OneStepMDEWTest");
    alg.execute();
    TS_ASSERT( alg.isExecuted() );

    IMDEventWorkspace_sptr out;
    TS_ASSERT_THROWS_NOTHING( out = boost::dynamic_pointer_cast<IMDEventWorkspace>(
        AnalysisDataService::Instance().retrieve("OneStepMDEWTest")); );
    TS_ASSERT(out);
    if (!out) return;
  }


};


#endif /* MANTID_MDEVENTS_ONESTEPMDEWTEST_H_ */

