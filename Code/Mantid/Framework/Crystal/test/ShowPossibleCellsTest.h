#ifndef MANTID_CRYSTAL_SHOW_POSSIBLE_CELLS_TEST_H_
#define MANTID_CRYSTAL_SHOW_POSSIBLE_CELLS_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidCrystal/ShowPossibleCells.h"
#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidCrystal/LoadIsawUB.h"

using namespace Mantid;
using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class ShowPossibleCellsTest : public CxxTest::TestSuite
{
public:
    
  void test_Init()
  {
    ShowPossibleCells alg;
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
                                           // set a Niggli UB in the 
                                           // oriented lattice
    V3D row_0( -0.101246, -0.040644, -0.061869 );
    V3D row_1(  0.014004, -0.079212,  0.007344 );
    V3D row_2( -0.063451,  0.011072,  0.064430 );

    Matrix<double> UB(3,3,false);
    UB.setRow( 0, row_0 ); 
    UB.setRow( 1, row_1 ); 
    UB.setRow( 2, row_2 ); 

    OrientedLattice o_lattice;
    o_lattice.setUB( UB );
    ws->mutableSample().setOrientedLattice( &o_lattice );

                                           // now get the UB back from the WS
    UB = o_lattice.getUB();

    ShowPossibleCells alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("PeaksWorkspace", WSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("MaxScalarError","0.2") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("BestOnly",true) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Check the number of cells found for different input parameters
    int num_cells = alg.getProperty("NumberOfCells");
    TS_ASSERT_EQUALS( num_cells, 2 );

    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("PeaksWorkspace", WSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("MaxScalarError","10") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("BestOnly",true) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    num_cells = alg.getProperty("NumberOfCells");
    TS_ASSERT_EQUALS( num_cells, 14 );

    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("PeaksWorkspace", WSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("MaxScalarError","10") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("BestOnly",false) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    num_cells = alg.getProperty("NumberOfCells");
    TS_ASSERT_EQUALS( num_cells, 42 );


    AnalysisDataService::Instance().remove(WSName);
  }

};


#endif /* MANTID_CRYSTAL_SHOW_POSSIBLE_CELLS_TEST_H_ */

