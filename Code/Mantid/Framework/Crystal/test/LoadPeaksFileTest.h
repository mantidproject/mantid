#ifndef MANTID_CRYSTAL_LOADPEAKSFILETEST_H_
#define MANTID_CRYSTAL_LOADPEAKSFILETEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidCrystal/LoadPeaksFile.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Math/Matrix.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid::API;
using namespace Mantid::Crystal;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class LoadPeaksFileTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    LoadPeaksFile alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }


  /** Test for the older TOPAZ geometry. */
  void xtest_exec_TOPAZ_1241()
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
    TS_ASSERT_EQUALS( ws->getNumberPeaks(), 271);

    Peak p = ws->getPeaks()[0];
    TS_ASSERT_EQUALS( p.getRunNumber(), 1241)
    TS_ASSERT_DELTA( p.getH(), 3, 1e-4);
    TS_ASSERT_DELTA( p.getK(), -1, 1e-4);
    TS_ASSERT_DELTA( p.getL(), -1, 1e-4);
    TS_ASSERT_EQUALS( p.getBankName(), "bank1");
    TS_ASSERT_DELTA( p.getCol(), 34, 1e-4);
    TS_ASSERT_DELTA( p.getRow(), 232, 1e-4);
    TS_ASSERT_DELTA( p.getIntensity(), 8334.62, 0.01);
    TS_ASSERT_DELTA( p.getSigmaIntensity(), 97, 0.01);
    TS_ASSERT_DELTA( p.getBinCount(), 49, 0.01);

    TS_ASSERT_DELTA( p.getWavelength(), 1.757, 0.001);
    TS_ASSERT_DELTA( p.getL1(), 18.0, 1e-3);
    TS_ASSERT_DELTA( p.getL2(), 0.39801, 0.01);

    TS_ASSERT_DELTA( p.getDSpacing(), 4.3241, 0.1);
    TS_ASSERT_DELTA( ws->getPeaks()[30].getDSpacing(), 2.8410, 0.12);
    TS_ASSERT_DELTA( ws->getPeaks()[30].getL2(), 0.45, 0.01);
  }


  /* Test for the newer TOPAZ geometry */
  void test_exec_TOPAZ_2479()
  {
    LoadPeaksFile alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    alg.setPropertyValue("Filename", "TOPAZ_2479.peaks");
    alg.setPropertyValue("OutputWorkspace", "TOPAZ_2479");

    TS_ASSERT( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve("TOPAZ_2479") ) );
    TS_ASSERT(ws);
    if (!ws) return;
    TS_ASSERT_EQUALS( ws->getNumberPeaks(), 46);

    Peak p = ws->getPeaks()[0];
    TS_ASSERT_EQUALS( p.getRunNumber(), 2479)
    TS_ASSERT_DELTA( p.getH(), 1, 1e-4);
    TS_ASSERT_DELTA( p.getK(), 2, 1e-4);
    TS_ASSERT_DELTA( p.getL(), 27, 1e-4);
    TS_ASSERT_EQUALS( p.getBankName(), "bank17");
    TS_ASSERT_DELTA( p.getCol(), 87, 1e-4);
    TS_ASSERT_DELTA( p.getRow(), 16, 1e-4);
    TS_ASSERT_DELTA( p.getIntensity(), 221.83, 0.01);
    TS_ASSERT_DELTA( p.getSigmaIntensity(), 15.02, 0.01);
    TS_ASSERT_DELTA( p.getBinCount(), 8, 0.01);

    TS_ASSERT_DELTA( p.getWavelength(), 0.761095, 0.001);
    TS_ASSERT_DELTA( p.getL1(), 18.0, 1e-3);
    TS_ASSERT_DELTA( p.getL2(), 0.461, 1e-3);
    TS_ASSERT_DELTA( p.getTOF(), 3560., 10.); // channel number is about TOF

    TS_ASSERT_DELTA( p.getDSpacing(), 0.4723, 0.001);
    TS_ASSERT_DELTA( ws->getPeaks()[1].getDSpacing(), 0.6425, 0.001);
    TS_ASSERT_DELTA( ws->getPeaks()[2].getDSpacing(), 0.8138, 0.001);

    // Now test the goniometer matrix
    Matrix<double> r1(3,3,true);
    // First peak has 0,0,0 angles so identity matrix
    TS_ASSERT( p.getGoniometerMatrix().equals(r1, 1e-5) );

    // Peak 3 is phi,chi,omega of 90,0,0; giving this matrix:
    Matrix<double> r2(3,3,false);
    r2[0][2] = 1;
    r2[1][1] = 1;
    r2[2][0] = -1;
    TS_ASSERT( ws->getPeaks()[2].getGoniometerMatrix().equals(r2, 1e-5) );
  }


};


#endif /* MANTID_CRYSTAL_LOADPEAKSFILETEST_H_ */

