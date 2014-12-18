#ifndef MANTID_CRYSTAL_INDEX_PEAKS_TEST_H_
#define MANTID_CRYSTAL_INDEX_PEAKS_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidCrystal/IndexPeaks.h"
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

class IndexPeaksTest : public CxxTest::TestSuite
{
public:
    
  void test_Init()
  {
    IndexPeaks alg;
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
                                           // clear all the peak indexes
    std::vector<Peak> &peaks = ws->getPeaks();
    size_t n_peaks = ws->getNumberPeaks();
    for ( size_t i = 0; i < n_peaks; i++ )
    {
      peaks[i].setHKL( V3D(0,0,0) );
    }
                                           // now set a proper UB in the 
                                           // oriented lattice
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

                                           // index the peaks with the new UB
    IndexPeaks alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("PeaksWorkspace", WSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Tolerance","0.1") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("RoundHKLs",false) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    double tolerance = alg.getProperty("Tolerance");

                                       // Check that the peaks were all indexed 
    for ( size_t i = 0; i < n_peaks; i++ )
    {
      TS_ASSERT( IndexingUtils::ValidIndex( peaks[i].getHKL(), tolerance ) );
    } 

    // Check the output properties
    int numIndexed = alg.getProperty("NumIndexed");
    TS_ASSERT_EQUALS( numIndexed, 43);

    double averageError = alg.getProperty("AverageError");
    TS_ASSERT_DELTA( averageError, 0.0097, 1e-3);

                                       // spot check a few peaks for
                                       // fractional Miller indices

    V3D peak_0_hkl_d ( -4.03065, -0.9885090, -6.01095 );   // first peak
    V3D peak_1_hkl_d ( -2.99276,  0.9955220, -4.00375 );
    V3D peak_2_hkl_d ( -3.99311,  0.9856010, -5.00772 );
    V3D peak_10_hkl_d( -3.01107, -0.0155531, -7.01377 );
    V3D peak_42_hkl_d( -1.97065,  4.0283600, -6.97828 );   // last peak
    
    V3D error = peak_0_hkl_d -peaks[ 0].getHKL();
    TS_ASSERT_DELTA( error.norm(), 0.0, 1e-4 );

    error = peak_1_hkl_d  -peaks[ 1].getHKL();
    TS_ASSERT_DELTA( error.norm(), 0.0, 1e-4 );

    error = peak_2_hkl_d  -peaks[ 2].getHKL();
    TS_ASSERT_DELTA( error.norm(), 0.0, 1e-4 );

    error = peak_10_hkl_d -peaks[10].getHKL();
    TS_ASSERT_DELTA( error.norm(), 0.0, 1e-4 );

    error = peak_42_hkl_d -peaks[42].getHKL();
    TS_ASSERT_DELTA( error.norm(), 0.0, 1e-4 );

                                    // clear all the peak indexes then 
                                    // re-run the algorithm, rounding the HKLs
                                    // this time, and again check a few peaks
    for ( size_t i = 0; i < n_peaks; i++ )
    {
      peaks[i].setHKL( V3D(0,0,0) );
    }

    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("PeaksWorkspace", WSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Tolerance","0.1") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("RoundHKLs",true) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

                                       // Check that the peaks were all indexed 
    for ( size_t i = 0; i < n_peaks; i++ )
    {
      TS_ASSERT( IndexingUtils::ValidIndex( peaks[i].getHKL(), tolerance ) );
    }

    // Check the output properties
    numIndexed = alg.getProperty("NumIndexed");
    TS_ASSERT_EQUALS( numIndexed, 43);

    averageError = alg.getProperty("AverageError");
    TS_ASSERT_DELTA( averageError, 0.0097, 1e-3);

                                     // spot check a few peaks for 
                                     // integer Miller indices
    V3D peak_0_hkl ( -4, -1, -6 );
    V3D peak_1_hkl ( -3,  1, -4 );
    V3D peak_2_hkl ( -4,  1, -5 );
    V3D peak_10_hkl( -3,  0, -7 );
    V3D peak_42_hkl( -2,  4, -7 );   // last peak

    error = peak_0_hkl -peaks[ 0].getHKL();
    TS_ASSERT_DELTA( error.norm(), 0.0, 1e-10 );

    error = peak_1_hkl  -peaks[ 1].getHKL();
    TS_ASSERT_DELTA( error.norm(), 0.0, 1e-10 );

    error = peak_2_hkl  -peaks[ 2].getHKL();
    TS_ASSERT_DELTA( error.norm(), 0.0, 1e-10 );

    error = peak_10_hkl -peaks[10].getHKL();
    TS_ASSERT_DELTA( error.norm(), 0.0, 1e-10 );

    error = peak_42_hkl -peaks[42].getHKL();
    TS_ASSERT_DELTA( error.norm(), 0.0, 1e-10 );

                                     // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(WSName);
  }

};


#endif /* MANTID_CRYSTAL_INDEX_PEAKS_TEST_H_ */

