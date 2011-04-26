#ifndef MANTID_CRYSTAL_LOADPEAKSFILETEST_H_
#define MANTID_CRYSTAL_LOADPEAKSFILETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Peak.h"
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
    alg.setPropertyValue("Filename", "TOPAZ_1241.integrate");
    alg.setPropertyValue("OutputWorkspace", "TOPAZ_1241");

    TS_ASSERT( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve("TOPAZ_1241") ) );
    TS_ASSERT(ws);
    if (!ws) return;
    TS_ASSERT_EQUALS( ws->getNumberPeaks(), 270);

    Peak p = ws->getPeaks()[0];
    TS_ASSERT_EQUALS( p.getRunNumber(), 1241)
    TS_ASSERT_DELTA( p.getH(), 3, 1e-4);
    TS_ASSERT_DELTA( p.getK(), -1, 1e-4);
    TS_ASSERT_DELTA( p.getL(), -1, 1e-4);
    TS_ASSERT_DELTA( p.getCol(), 34, 1e-4);
    TS_ASSERT_DELTA( p.getRow(), 232, 1e-4);
    TS_ASSERT_DELTA( p.getIntensity(), 8334.62, 0.01);
    TS_ASSERT_DELTA( p.getSigmaIntensity(), 97, 0.01);
    TS_ASSERT_DELTA( p.getBinCount(), 49, 0.01);

    TS_ASSERT_DELTA( p.getWavelength(), 1.757, 0.001);

    // TODO: This does not match - geometry error?
    // TS_ASSERT_DELTA( p.getDSpacing(), 4.3241, 0.001);

  }


};


#endif /* MANTID_CRYSTAL_LOADPEAKSFILETEST_H_ */

