#ifndef MANTID_CRYSTAL_STATISTICSOFPEAKSWORKSPACETEST_H_
#define MANTID_CRYSTAL_STATISTICSOFPEAKSWORKSPACETEST_H_

#include "MantidCrystal/StatisticsOfPeaksWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <Poco/File.h>

using namespace Mantid;
using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::PhysicalConstants;

class StatisticsOfPeaksWorkspaceTest : public CxxTest::TestSuite
{
public:
    
  void test_Init()
  {
    StatisticsOfPeaksWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void do_test(int numRuns, size_t numBanks, size_t numPeaksPerBank)
  {
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(4, 10, 1.0);
    PeaksWorkspace_sptr ws(new PeaksWorkspace());
    ws->setInstrument(inst);

    auto lattice = new Mantid::Geometry::OrientedLattice;
    Mantid::Kernel::DblMatrix UB(3, 3, true);
    UB.identityMatrix();
    lattice->setUB(UB);
    ws->mutableSample().setOrientedLattice(lattice);

    for (int run=1000; run<numRuns+1000; run++)
      for (size_t b=1; b<=numBanks; b++)
      for (size_t i=0; i<numPeaksPerBank; i++)
      {
        V3D hkl(static_cast<double>(i), static_cast<double>(i), static_cast<double>(i));
        DblMatrix gon(3,3, true);
        Peak p(inst, static_cast<detid_t>(b*100 + i+1+i*10), static_cast<double>(i)*1.0+0.5, hkl, gon);
        p.setRunNumber(run);
        p.setBankName("bank1");
        p.setIntensity( static_cast<double>(i)+0.1);
        p.setSigmaIntensity( sqrt(static_cast<double>(i)+0.1));
        p.setBinCount( static_cast<double>(i) );
        ws->addPeak(p);
      }
    AnalysisDataService::Instance().addOrReplace("TOPAZ_peaks", ws);

    StatisticsOfPeaksWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", "TOPAZ_peaks") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("SortBy", "Overall") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("StatisticsTable", "stat") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("OutputWorkspace", "TOPAZ_peaks") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    ITableWorkspace_sptr tableOut;
    TS_ASSERT_THROWS_NOTHING( tableOut = boost::dynamic_pointer_cast<ITableWorkspace>(
        AnalysisDataService::Instance().retrieve("stat") ) );
    TS_ASSERT(tableOut);
    if (!tableOut) return;
    TS_ASSERT_EQUALS( tableOut->rowCount(),1);
    TS_ASSERT_EQUALS( tableOut->String(0,0), "Overall");
    TS_ASSERT_EQUALS( tableOut->Int(0,1), 3);
    TS_ASSERT_DELTA( tableOut->Double(0,2), 1.5,.1);
    TS_ASSERT_DELTA( tableOut->Double(0,3), 3.5,.1);
    TS_ASSERT_DELTA( tableOut->Double(0,4), 8.,.1);
    TS_ASSERT_DELTA( tableOut->Double(0,5), 1.4195,.1);
    TS_ASSERT_DELTA( tableOut->Double(0,6), 0.,.1);
    TS_ASSERT_DELTA( tableOut->Double(0,7), 0.,.1);

    PeaksWorkspace_sptr wsout;
    TS_ASSERT_THROWS_NOTHING( wsout = boost::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve("TOPAZ_peaks") ) );
    TS_ASSERT(wsout);
    if (!wsout) return;
    TS_ASSERT_EQUALS( wsout->getNumberPeaks(), 24);

    Peak p = wsout->getPeaks()[0];
    TS_ASSERT_EQUALS(p.getH(),1 );
    TS_ASSERT_EQUALS(p.getK(),1 );
    TS_ASSERT_EQUALS(p.getL(),1 );
    TS_ASSERT_DELTA(p.getIntensity(),1.1, 1e-4);
    TS_ASSERT_DELTA(p.getSigmaIntensity(),1.0488, 1e-4);
    TS_ASSERT_DELTA(p.getWavelength(),1.5, 1e-4 );
    TS_ASSERT_EQUALS(p.getRunNumber(),1000. );
    TS_ASSERT_DELTA(p.getDSpacing(),3.5933, 1e-4 );
  }

  /// Test with a few peaks
  void test_exec()
  {
    do_test(2, 4,4);
  }


};


#endif /* MANTID_CRYSTAL_STATISTICSOFPEAKSWORKSPACETEST_H_ */

