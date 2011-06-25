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

  
  void test_BestFit_Direction()
  {
    std::vector<int> index_values;
    int correct_indices[] = { 1, 4, 2, 0, 1, 3, 0, -1, 0, -1, -2, -3 };
    for ( size_t i = 0; i < 12; i++ )
    {
      index_values.push_back( correct_indices[i] );
    }

    std::vector<V3D> q_vectors;
    q_vectors.push_back( V3D(-0.57582, -0.35322, -0.19974 ));
    q_vectors.push_back( V3D(-1.41754, -0.78704, -0.75974 ));
    q_vectors.push_back( V3D(-1.12030, -0.53578, -0.27559 ));
    q_vectors.push_back( V3D(-0.68911, -0.59397, -0.12716 ));
    q_vectors.push_back( V3D(-1.06863, -0.43255,  0.01688 ));
    q_vectors.push_back( V3D(-1.82007, -0.49671, -0.06266 ));
    q_vectors.push_back( V3D(-1.10465, -0.73708, -0.01939 ));
    q_vectors.push_back( V3D(-0.12747, -0.32380,  0.00821 ));
    q_vectors.push_back( V3D(-0.84210, -0.37038,  0.15403 ));
    q_vectors.push_back( V3D(-0.54099, -0.46900,  0.11535 ));
    q_vectors.push_back( V3D(-0.90478, -0.50667,  0.51072 ));
    q_vectors.push_back( V3D(-0.50387, -0.58561,  0.43502 ));

    V3D best_vec;
    double error = IndexingUtils::BestFit_Direction( best_vec, 
                                                     index_values, 
                                                     q_vectors );
    TS_ASSERT_DELTA( error, 0.00218606, 1e-5 );
    TS_ASSERT_DELTA( best_vec[0], -2.58222, 1e-4 );
    TS_ASSERT_DELTA( best_vec[1],  3.97345, 1e-4 );
    TS_ASSERT_DELTA( best_vec[2], -4.55145, 1e-4 );
  }


  void test_ValidIndex()
  {
    V3D hkl(0,0,0);
    TS_ASSERT_EQUALS( IndexingUtils::ValidIndex(hkl,0.1), false );

    hkl( 2.09, -3.09, -2.91 );
    TS_ASSERT_EQUALS( IndexingUtils::ValidIndex(hkl,0.1), true );

    hkl( 2.11, -3.09, -2.91 );
    TS_ASSERT_EQUALS( IndexingUtils::ValidIndex(hkl,0.1), false );

    hkl( 2.09, -3.11, -2.91 );
    TS_ASSERT_EQUALS( IndexingUtils::ValidIndex(hkl,0.1), false );

    hkl( 2.09, -3.09, -2.89 );
    TS_ASSERT_EQUALS( IndexingUtils::ValidIndex(hkl,0.1), false );
  }


  void test_NumberIndexed()
  {
    Matrix<double> UB(3,3,false);

    V3D row_values( -0.141251, 0.3042650, -0.147160 );
    UB.setRow( 0, row_values );    

    row_values( 0.120633,  0.0907082,  0.106323 );
    UB.setRow( 1, row_values );    

    row_values( 0.258332, -0.0062807, -0.261151 );
    UB.setRow( 2, row_values );    

    std::vector<V3D> q_list( 5 );

    V3D qxyz( -1.02753, 0.47106, -0.25957 );
    q_list[0] = qxyz;

    qxyz( -2.05753, 0.93893, -0.51988 );
    q_list[1] = qxyz;

    qxyz( -2.19878, 1.05926, -0.27486 );
    q_list[2] = qxyz;

    qxyz( -2.63576, 1.39119, -0.53007 );
    q_list[3] = qxyz;

    qxyz( -1.75324, 1.02999, -0.52537 );
    q_list[4] = qxyz;

    TS_ASSERT_EQUALS( IndexingUtils::NumberIndexed( UB, q_list, 0.017 ), 4 );
  }

  
  void test_GetIndexedPeaks_1D()
  {

    int correct_indices[] = { 1, 4, 2, 0, 1, 3, 0, -1, 0, -1, -2, -3 };

    std::vector<V3D> q_vectors;
    q_vectors.push_back( V3D(-0.57582, -0.35322, -0.19974 ));
    q_vectors.push_back( V3D(-1.41754, -0.78704, -0.75974 ));
    q_vectors.push_back( V3D(-1.12030, -0.53578, -0.27559 ));
    q_vectors.push_back( V3D(-0.68911, -0.59397, -0.12716 ));
    q_vectors.push_back( V3D(-1.06863, -0.43255,  0.01688 ));
    q_vectors.push_back( V3D(-1.82007, -0.49671, -0.06266 ));
    q_vectors.push_back( V3D(-1.10465, -0.73708, -0.01939 ));
    q_vectors.push_back( V3D(-0.12747, -0.32380,  0.00821 ));
    q_vectors.push_back( V3D(-0.84210, -0.37038,  0.15403 ));
    q_vectors.push_back( V3D(-0.54099, -0.46900,  0.11535 ));
    q_vectors.push_back( V3D(-0.90478, -0.50667,  0.51072 ));
    q_vectors.push_back( V3D(-0.50387, -0.58561,  0.43502 ));

    V3D direction(-2.62484,4.04988,-4.46991);
    double required_tolerance = 0.1;
    double fit_error = 0;

    std::vector<int> index_vals;
    std::vector<V3D> indexed_qs;

    int num_indexed = IndexingUtils::GetIndexedPeaks_1D( q_vectors,
                                                         direction,
                                                         required_tolerance,
                                                         index_vals,
                                                         indexed_qs,
                                                         fit_error );

    TS_ASSERT_EQUALS( num_indexed, 12 );
    TS_ASSERT_EQUALS( index_vals.size(), 12 );
    TS_ASSERT_EQUALS( indexed_qs.size(), 12 );
    TS_ASSERT_DELTA( fit_error, 0.011419, 1e-5 );

    for ( size_t i = 0; i < index_vals.size(); i++ )
    {
      TS_ASSERT_EQUALS( index_vals[i], correct_indices[i] );
    }
  }


  void test_GetIndexedPeaks_3D()
  {
    std::vector<V3D> correct_indices;
    correct_indices.push_back( V3D( 1,  9, -9) );
    correct_indices.push_back( V3D( 4, 20,-24) );
    correct_indices.push_back( V3D( 2, 18,-14) );
    correct_indices.push_back( V3D( 0, 12,-12) );
    correct_indices.push_back( V3D( 1, 19, -9) );
    correct_indices.push_back( V3D( 3, 31,-13) );
    correct_indices.push_back( V3D( 0, 20,-14) );
    correct_indices.push_back( V3D(-1,  3, -5) );
    correct_indices.push_back( V3D( 0, 16, -6) );
    correct_indices.push_back( V3D(-1, 11, -7) );
    correct_indices.push_back( V3D(-2, 20, -4) );
    correct_indices.push_back( V3D(-3, 13, -5) );

    std::vector<V3D> q_vectors;
    q_vectors.push_back( V3D(-0.57582, -0.35322, -0.19974 ));
    q_vectors.push_back( V3D(-1.41754, -0.78704, -0.75974 ));
    q_vectors.push_back( V3D(-1.12030, -0.53578, -0.27559 ));
    q_vectors.push_back( V3D(-0.68911, -0.59397, -0.12716 ));
    q_vectors.push_back( V3D(-1.06863, -0.43255,  0.01688 ));
    q_vectors.push_back( V3D(-1.82007, -0.49671, -0.06266 ));
    q_vectors.push_back( V3D(-1.10465, -0.73708, -0.01939 ));
    q_vectors.push_back( V3D(-0.12747, -0.32380,  0.00821 ));
    q_vectors.push_back( V3D(-0.84210, -0.37038,  0.15403 ));
    q_vectors.push_back( V3D(-0.54099, -0.46900,  0.11535 ));
    q_vectors.push_back( V3D(-0.90478, -0.50667,  0.51072 ));
    q_vectors.push_back( V3D(-0.50387, -0.58561,  0.43502 ));

    V3D direction_1(-2.58222,3.97345,-4.55145);
    V3D direction_2(-16.6082,-2.50165,7.24628);
    V3D direction_3(2.7609,14.5661,11.3343);

    double required_tolerance = 0.1;
    double fit_error = 0;

    std::vector<V3D> index_vals;
    std::vector<V3D> indexed_qs;

    int num_indexed = IndexingUtils::GetIndexedPeaks_3D( q_vectors,
                                                         direction_1,
                                                         direction_2,
                                                         direction_3,
                                                         required_tolerance,
                                                         index_vals,
                                                         indexed_qs,
                                                         fit_error );
    TS_ASSERT_EQUALS( num_indexed, 12 );
    TS_ASSERT_EQUALS( index_vals.size(), 12 );
    TS_ASSERT_EQUALS( indexed_qs.size(), 12 );
    TS_ASSERT_DELTA( fit_error, 0.0258739, 1e-5 );

    for ( size_t i = 0; i < index_vals.size(); i++ )
    {
      for ( size_t j = 0; j < 3; j++ )
      {
        TS_ASSERT_EQUALS( (index_vals[i])[j], (correct_indices[i])[j] );
      }
    }

  }


  void test_MakeHemisphereDirections()
  {
    std::vector<V3D> direction_list=IndexingUtils::MakeHemisphereDirections(5);

    TS_ASSERT_EQUALS( direction_list.size(), 64 );

    // check some random entries
    TS_ASSERT_DELTA( direction_list[0].X(), 0, 1e-5 );
    TS_ASSERT_DELTA( direction_list[0].Y(), 1, 1e-5 );
    TS_ASSERT_DELTA( direction_list[0].Z(), 0, 1e-5 );

    TS_ASSERT_DELTA( direction_list[5].X(), -0.154508, 1e-5 );
    TS_ASSERT_DELTA( direction_list[5].Y(),  0.951057, 1e-5 );
    TS_ASSERT_DELTA( direction_list[5].Z(), -0.267617, 1e-5 );
  
    TS_ASSERT_DELTA( direction_list[10].X(), 0, 1e-5 );
    TS_ASSERT_DELTA( direction_list[10].Y(), 0.809017, 1e-5 );
    TS_ASSERT_DELTA( direction_list[10].Z(), 0.587785, 1e-5 );

    TS_ASSERT_DELTA( direction_list[63].X(), -0.951057, 1e-5 );
    TS_ASSERT_DELTA( direction_list[63].Y(),  0, 1e-5 );
    TS_ASSERT_DELTA( direction_list[63].Z(),  0.309017, 1e-5 );
  }


  void test_MakeCircleDirections()
  {
    int num_steps = 8;
    V3D axis( 1, 1, 1 );
    double angle_degrees = 90;

    std::vector<V3D> direction_list
        = IndexingUtils::MakeCircleDirections( num_steps, axis, angle_degrees);

    TS_ASSERT_EQUALS( direction_list.size(), 8 );

    TS_ASSERT_DELTA( direction_list[0].X(), -0.816497, 1e-5 );
    TS_ASSERT_DELTA( direction_list[0].Y(),  0.408248, 1e-5 );
    TS_ASSERT_DELTA( direction_list[0].Z(),  0.408248, 1e-5 );

    TS_ASSERT_DELTA( direction_list[1].X(), -0.577350, 1e-5 );
    TS_ASSERT_DELTA( direction_list[1].Y(), -0.211325, 1e-5 );
    TS_ASSERT_DELTA( direction_list[1].Z(),  0.788675, 1e-5 );

    TS_ASSERT_DELTA( direction_list[7].X(), -0.577350, 1e-5 );
    TS_ASSERT_DELTA( direction_list[7].Y(),  0.788675, 1e-5 );
    TS_ASSERT_DELTA( direction_list[7].Z(), -0.211325, 1e-5 );

    double dot_prod;
    for ( size_t i = 0; i < direction_list.size(); i++ )
    {
      dot_prod = axis.scalar_prod( direction_list[i] );
      TS_ASSERT_DELTA( dot_prod, 0, 1e-10 );
    }
  }


  void test_SelectDirection()
  {
    V3D best_direction;

    std::vector<V3D> q_vectors;
    q_vectors.push_back( V3D(-0.57582, -0.35322, -0.19974 ));
    q_vectors.push_back( V3D(-1.41754, -0.78704, -0.75974 ));
    q_vectors.push_back( V3D(-1.12030, -0.53578, -0.27559 ));
    q_vectors.push_back( V3D(-0.68911, -0.59397, -0.12716 ));
    q_vectors.push_back( V3D(-1.06863, -0.43255,  0.01688 ));
    q_vectors.push_back( V3D(-1.82007, -0.49671, -0.06266 ));
    q_vectors.push_back( V3D(-1.10465, -0.73708, -0.01939 ));
    q_vectors.push_back( V3D(-0.12747, -0.32380,  0.00821 ));
    q_vectors.push_back( V3D(-0.84210, -0.37038,  0.15403 ));
    q_vectors.push_back( V3D(-0.54099, -0.46900,  0.11535 ));
    q_vectors.push_back( V3D(-0.90478, -0.50667,  0.51072 ));
    q_vectors.push_back( V3D(-0.50387, -0.58561,  0.43502 ));

    std::vector<V3D> directions = IndexingUtils::MakeHemisphereDirections(90);

    double plane_spacing = 1.0/6.5781;
    double required_tolerance = 0.1;

    int num_indexed = IndexingUtils::SelectDirection( best_direction,
                                                      q_vectors,
                                                      directions,
                                                      plane_spacing,
                                                      required_tolerance );

    TS_ASSERT_DELTA( best_direction[0], -0.399027, 1e-5 );
    TS_ASSERT_DELTA( best_direction[1],  0.615661, 1e-5 );
    TS_ASSERT_DELTA( best_direction[2], -0.679513, 1e-5 );

    TS_ASSERT_EQUALS( num_indexed, 12 );
  }


};

#endif  /* MANTID_GEOMETRY_INDEXING_UTILS_TEST_H_ */
