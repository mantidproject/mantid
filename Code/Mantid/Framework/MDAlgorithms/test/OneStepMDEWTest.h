#ifndef MANTID_MDEVENTS_ONESTEPMDEWTEST_H_
#define MANTID_MDEVENTS_ONESTEPMDEWTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidMDAlgorithms/OneStepMDEW.h"

using namespace Mantid::API;
using namespace Mantid::MDAlgorithms;

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

