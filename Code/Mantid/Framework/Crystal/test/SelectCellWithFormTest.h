#ifndef MANTID_CRYSTAL_SELECT_CELL_WITH_FORM_TEST_H_
#define MANTID_CRYSTAL_SELECT_CELL_WITH_FORM_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidCrystal/SelectCellWithForm.h"
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

class SelectCellWithFormTest : public CxxTest::TestSuite
{
public:
    
  void test_Init()
  {
    SelectCellWithForm alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    // Name of the loader's output workspace.
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
                                           // set a Niggli UB for run 3007 
                                           // (CuTCA) in the oriented lattice
    V3D row_0(  0.0122354,  0.00480056,  0.0860404 );
    V3D row_1( -0.1165450,  0.00178145, -0.0045884 );
    V3D row_2( -0.0273738, -0.08973560, -0.0252595 );

    Matrix<double> UB(3,3,false);
    UB.setRow( 0, row_0 ); 
    UB.setRow( 1, row_1 ); 
    UB.setRow( 2, row_2 ); 

    OrientedLattice o_lattice;
    o_lattice.setUB( UB );
    ws->mutableSample().setOrientedLattice( &o_lattice );

                                           // now get the UB back from the WS
    UB = o_lattice.getUB();

    SelectCellWithForm alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("PeaksWorkspace", WSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("FormNumber", 35) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Apply",true) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Tolerance",0.12) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    int num_indexed = alg.getProperty("NumIndexed"); 
    TS_ASSERT_EQUALS( num_indexed, 43 );
    double average_error = alg.getProperty("AverageError");
    TS_ASSERT_DELTA( average_error, 0.00972862, .0001 );

    AnalysisDataService::Instance().remove(WSName);
  }

};


#endif /* MANTID_CRYSTAL_SELECT_CELL_WITH_FORM_TEST_H_ */

