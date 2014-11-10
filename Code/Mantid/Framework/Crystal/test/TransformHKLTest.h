#ifndef MANTID_CRYSTAL_TRANSFORM_HKL_TEST_H_
#define MANTID_CRYSTAL_TRANSFORM_HKL_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidCrystal/TransformHKL.h"
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

class TransformHKLTest : public CxxTest::TestSuite
{
public:
    
  void test_Init()
  {
    TransformHKL alg;
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
                                           // make a reasonable UB and 
                                           // put it in the workspace 
    V3D row_0( -0.0122354,  0.00480056, -0.0860404 );
    V3D row_1(  0.1165450,  0.00178145,  0.0045884 );
    V3D row_2(  0.0273738, -0.08973560,  0.0252595 );

    Matrix<double> UB(3,3,false);
    UB.setRow( 0, row_0 ); 
    UB.setRow( 1, row_1 ); 
    UB.setRow( 2, row_2 ); 

    OrientedLattice o_lattice;
    o_lattice.setUB( UB );
    ws->mutableSample().setOrientedLattice( &o_lattice );

    Matrix<double> UB_inverse(UB);
    UB_inverse.Invert();
                                           // index the peaks with UB_inverse
    std::vector<Peak> &peaks = ws->getPeaks();
    size_t n_peaks = ws->getNumberPeaks();
    for ( size_t i = 0; i < n_peaks; i++ )
    {
      V3D q_vec   = (peaks[i].getQSampleFrame()) * (0.5/M_PI);
      V3D hkl_vec = UB_inverse * q_vec;
      peaks[i].setHKL( hkl_vec );
    }
                                           // save original indexes, so we can
                                           // verify they were properly changed
    std::vector<V3D> original_hkl;
    for ( size_t i = 0; i < n_peaks; i++ )
    {
      original_hkl.push_back( V3D( peaks[i].getHKL() ) );
    }
                
    TransformHKL alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("PeaksWorkspace", WSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Tolerance","0.1") );

                                           // specify a matrix that will swap H and K and negate L
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("HKLTransform","0,1,0,1,0,0,0,0,-1") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

                                          // Check that the peaks were all indexed 
    double tolerance = alg.getProperty("Tolerance");
    peaks = ws->getPeaks();
    for ( size_t i = 0; i < n_peaks; i++ )
    {
      TS_ASSERT( IndexingUtils::ValidIndex( peaks[i].getHKL(), tolerance ) );
    } 

                                           // Check that the transform actually 
                                           // did swap H & K and negate L.  
    for ( size_t i = 0; i < n_peaks; i++ )
    {
      TS_ASSERT_DELTA( original_hkl[i][0],  peaks[i].getK(), 1.0e-5 );
      TS_ASSERT_DELTA( original_hkl[i][1],  peaks[i].getH(), 1.0e-5 );
      TS_ASSERT_DELTA( original_hkl[i][2], -peaks[i].getL(), 1.0e-5 );
    }
                                           // Check the output properties
    int numIndexed = alg.getProperty("NumIndexed");
    TS_ASSERT_EQUALS( numIndexed, 43);
    double averageError = alg.getProperty("AverageError");
    TS_ASSERT_DELTA( averageError, 0.0097, 1e-3);

    AnalysisDataService::Instance().remove(WSName);
  }

};


#endif /* MANTID_CRYSTAL_TRANSFORM_HKL_TEST_H_ */

