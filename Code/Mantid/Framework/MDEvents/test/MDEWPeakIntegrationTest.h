#ifndef MANTID_MDEVENTS_MDEWPEAKINTEGRATIONTEST_H_
#define MANTID_MDEVENTS_MDEWPEAKINTEGRATIONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/MDEWPeakIntegration.h"

using namespace Mantid::MDEvents;

class MDEWPeakIntegrationTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    MDEWPeakIntegration alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_Something()
  {
  }


};


#endif /* MANTID_MDEVENTS_MDEWPEAKINTEGRATIONTEST_H_ */

