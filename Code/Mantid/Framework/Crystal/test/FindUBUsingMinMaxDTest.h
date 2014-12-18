#ifndef MANTID_CRYSTAL_FIND_UB_USING_MIN_MAX_D_TEST_H_
#define MANTID_CRYSTAL_FIND_UB_USING_MIN_MAX_D_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidCrystal/FindUBUsingMinMaxD.h"
#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidCrystal/LoadIsawUB.h"

using namespace Mantid;
using namespace Mantid::Crystal;
using Mantid::Geometry::OrientedLattice;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class FindUBUsingMinMaxDTest : public CxxTest::TestSuite
{
public:
    
  void test_Init()
  {
    FindUBUsingMinMaxD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    // Name of the output workspace.
    std::string WSName("peaks");
    LoadIsawPeaks loader;
    TS_ASSERT_THROWS_NOTHING( loader.initialize() );
    TS_ASSERT( loader.isInitialized() );
    loader.setPropertyValue("Filename", "TOPAZ_3007.peaks");
    loader.setPropertyValue("OutputWorkspace", WSName);

    TS_ASSERT( loader.execute() );
    TS_ASSERT( loader.isExecuted() );

    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve(WSName) ) );
    TS_ASSERT(ws);
    if (!ws) return;
    FindUBUsingMinMaxD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("PeaksWorkspace", WSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("MinD","8.0") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("MaxD","13.0") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("NumInitial","20") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Tolerance","0.15") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Check that we set an oriented lattice
    TS_ASSERT(ws->mutableSample().hasOrientedLattice());
    // Check that the UB matrix is the same as in TOPAZ_3007.mat
    OrientedLattice latt=ws->mutableSample().getOrientedLattice();

    double correct_UB[] = {  0.0122354,  0.00480056,  0.0860404,
                            -0.1165450,  0.00178145, -0.0045884,
                            -0.0273738, -0.08973560, -0.0252595 };

    std::vector<double> UB_calculated = latt.getUB().getVector();

    for ( size_t i = 0; i < 9; i++ )
    {
      TS_ASSERT_DELTA( correct_UB[i], UB_calculated[i], 5e-4 );
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(WSName);
  }

};


#endif /* MANTID_CRYSTAL_FIND_UB_USING_MIN_MAX_D_TEST_H_ */

