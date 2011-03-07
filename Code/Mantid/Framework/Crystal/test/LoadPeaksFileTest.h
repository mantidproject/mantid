#ifndef MANTID_CRYSTAL_LOADPEAKSFILETEST_H_
#define MANTID_CRYSTAL_LOADPEAKSFILETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include <iostream>
#include <iomanip>

#include "MantidCrystal/LoadPeaksFile.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Crystal;

class LoadPeaksFileTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    LoadPeaksFile alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    LoadPeaksFile alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    alg.setPropertyValue("Filename", "TOPAZ_1204.peaks");
    alg.setPropertyValue("OutputWorkspace", "TOPAZ");

    TS_ASSERT( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve("TOPAZ") ) );
    TS_ASSERT(ws);
    if (!ws) return;
    TS_ASSERT_EQUALS( ws->getNumberPeaks(), 36);
    TS_ASSERT_DELTA( ws->getPeakIntegrationCount(0), 0.0, 0.01);
    TS_ASSERT_DELTA( ws->get_column(0), 43.0, 0.01);
  }


};


#endif /* MANTID_CRYSTAL_LOADPEAKSFILETEST_H_ */

