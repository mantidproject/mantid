#ifndef MANTID_GEOMETRY_INDEXING_UTILS_TEST_H_
#define MANTID_GEOMETRY_INDEXING_UTILS_TEST_H_

#include <cxxtest/TestSuite.h>
#include <MantidKernel/Timer.h>
#include <MantidKernel/System.h>
#include <iostream>
#include <iomanip>
#include <MantidKernel/V3D.h>
#include <MantidKernel/Matrix.h>

#include <MantidGeometry/Crystal/IndexingUtils.h>

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;
using Mantid::Kernel::Matrix;

class IndexingUtilsTest : public CxxTest::TestSuite
{
public:
  void test_BestFit_UB() 
  {  
     double h_vals[]  = {  1,  0,  0, -1,  0,  0, 1, 1 };
     double k_vals[]  = { .1,  1,  0,  0, -1,  0, 1, 2 };
     double l_vals[]  = {-.1,  0,  1,  0,  0, -1, 1, 3 }; 

     double qx_vals[]  = {  2,  0,  0, -2,  0,  0, 2,  2 };
     double qy_vals[]  = {  1,  3,  0,  0, -3,  0, 3,  6 };
     double qz_vals[]  = {  0,  0,  4,  0,  0, -4, 4, 12 }; 

     double correct_UB[] = { 2.000000e+00,  0.000000e+00, -0.000000e+00,
                             2.766704e-01,  2.959570e+00, -7.214043e-02,  
                             1.580974e-01, -2.310306e-02,  3.958777e+00 };

     size_t N_INDEXED_PEAKS = 8;

     Matrix<double> UB(3,3,false);
     std::vector<V3D> q_list( N_INDEXED_PEAKS );
     for ( size_t row = 0; row < N_INDEXED_PEAKS; row++ )
     {
       V3D qxyz( qx_vals[row], qy_vals[row], qz_vals[row] );
       q_list[row] = qxyz;
     }   

     std::vector<V3D> hkl_list( N_INDEXED_PEAKS );
     for ( size_t row = 0; row < N_INDEXED_PEAKS; row++ )
     {
       V3D hkl( h_vals[row], k_vals[row], l_vals[row] );
       hkl_list[row] = hkl;
     }

     double sum_sq_error = IndexingUtils::BestFit_UB( UB, hkl_list, q_list );

     std::vector<double> UB_returned = UB.get_vector();

     for ( int i = 0; i < 3; i++ )
       TS_ASSERT_DELTA( UB_returned[i], correct_UB[i], 1.e-5 );
 
     TS_ASSERT_DELTA( sum_sq_error, 0.390147, 1e-5 );
  }

};

#endif  /* MANTID_GEOMETRY_INDEXING_UTILS_TEST_H_ */
