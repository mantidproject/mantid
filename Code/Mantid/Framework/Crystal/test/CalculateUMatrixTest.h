#ifndef MANTID_CRYSTAL_CALCULATEUMATRIXTEST_H_
#define MANTID_CRYSTAL_CALCULATEUMATRIXTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidCrystal/CalculateUMatrix.h"
#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidCrystal/LoadIsawUB.h"

using namespace Mantid;
using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
class CalculateUMatrixTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    CalculateUMatrix alg;
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
    CalculateUMatrix alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("a", "14.1526") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("b", "19.2903") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("c", "8.5813") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("alpha", "90") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("beta", "105.0738") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("gamma", "90") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("PeaksWorkspace", WSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Check that we set an oriented lattice
    TS_ASSERT(ws->mutableSample().hasOrientedLattice());
    // Check that the UB matrix is the same as in TOPAZ_3007.mat
    OrientedLattice latt=ws->mutableSample().getOrientedLattice();

    LoadIsawUB alg1;
    TS_ASSERT_THROWS_NOTHING( alg1.initialize() )
    TS_ASSERT( alg1.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg1.setPropertyValue("Filename", "TOPAZ_3007.mat") );
    TS_ASSERT_THROWS_NOTHING( alg1.setPropertyValue("InputWorkspace", WSName) );
    TS_ASSERT_THROWS_NOTHING( alg1.execute(); );
    TS_ASSERT_THROWS_NOTHING( alg1.isExecuted(); );
    OrientedLattice lattFromUB=ws->mutableSample().getOrientedLattice();

    TS_ASSERT(latt.getUB().equals(lattFromUB.getUB(),2e-4)); //Some values differ by up to 1.7e-4
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(WSName);
  }
  

};


#endif /* MANTID_CRYSTAL_CALCULATEUMATRIXTEST_H_ */

