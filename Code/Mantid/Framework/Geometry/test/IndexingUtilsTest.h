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

#define PI 3.141592653589793238

  static std::vector<V3D> getNatroliteQs()
  {
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

    return q_vectors;
  } 

  void test_Find_UB_given_lattice_parameters()
  {
    Matrix<double> UB(3,3,false);

    double correct_UB[] = { -0.1015550,  0.0992964, -0.0155078,
                             0.1274830,  0.0150210, -0.0839671,
                            -0.0507717, -0.0432269, -0.0645173 };

    std::vector<V3D> q_vectors = getNatroliteQs();

    double  a     = 6.6;
    double  b     = 9.7;
    double  c     = 9.9;
    double  alpha = 84;
    double  beta  = 71;
    double  gamma = 70;
                                           // test both default case(-1) and
                                           // case with specified base index(4)
    for ( int base_index = -1; base_index < 5 ; base_index += 5 )
    {
      double required_tolerance = 0.2;
      size_t num_initial = 3;
      double degrees_per_step = 3;
    
      double error = IndexingUtils::Find_UB( UB, 
                                             q_vectors, 
                                             a, b, c, 
                                             alpha, beta, gamma,
                                             required_tolerance,
                                             base_index,
                                             num_initial,
                                             degrees_per_step );

      TS_ASSERT_DELTA( error, 0.00671575, 1e-5 );

      std::vector<double> UB_returned = UB.get_vector();
      for ( size_t i = 0; i < 9; i++ )
      {
        TS_ASSERT_DELTA( UB_returned[i], correct_UB[i], 1e-5 );
      }

      int num_indexed = IndexingUtils::NumberIndexed( UB, 
                                                      q_vectors, 
                                                      required_tolerance );
      TS_ASSERT_EQUALS( num_indexed, 12 ); 
    }
  }



  void test_Find_UB_given_d_min_d_max()
  {
    Matrix<double> UB(3,3,false);

    double correct_UB[] = { -0.1015550,  0.0992964, -0.0155078,
                             0.1274830,  0.0150210, -0.0839671,
                            -0.0507717, -0.0432269, -0.0645173 };

    std::vector<V3D> q_vectors = getNatroliteQs();

    double d_min =  6;
    double d_max = 10;
    double required_tolerance = 0.08;
    int    num_initial        = 12;
    double degrees_per_step   = 1; 
                                           // test both default case(-1) and
                                           // case with specified base index(4)
    for ( int base_index = -1; base_index < 5 ; base_index += 5 )
    {

      double error = IndexingUtils::Find_UB( UB,
                                             q_vectors,
                                             d_min,
                                             d_max,
                                             required_tolerance,
                                             base_index,
                                             num_initial,
                                             degrees_per_step );

      int num_indexed = IndexingUtils::NumberIndexed( UB,
                                                      q_vectors,
                                                      required_tolerance );
      TS_ASSERT_EQUALS( num_indexed, 12 );

      TS_ASSERT_DELTA( error, 0.000111616, 1e-5 );

      std::vector<double> UB_returned = UB.get_vector();
      for ( size_t i = 0; i < 9; i++ )
      {
        TS_ASSERT_DELTA( UB_returned[i], correct_UB[i], 1e-5 );
      }
    }
  }


  void test_Optimize_UB_given_indexing() 
  {  
/*
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

     double sum_sq_error = IndexingUtils::Optimize_UB( UB, hkl_list, q_list );

     std::vector<double> UB_returned = UB.get_vector();

     for ( int i = 0; i < 3; i++ )
       TS_ASSERT_DELTA( UB_returned[i], correct_UB[i], 1.e-5 );
 
     TS_ASSERT_DELTA( sum_sq_error, 0.390147, 1e-5 );
*/
  }
  
  void test_Optimize_Direction()
  {
    std::vector<int> index_values;
    int correct_indices[] = { 1, 4, 2, 0, 1, 3, 0, -1, 0, -1, -2, -3 };
    for ( size_t i = 0; i < 12; i++ )
    {
      index_values.push_back( correct_indices[i] );
    }

    std::vector<V3D> q_vectors = getNatroliteQs();

    V3D best_vec;
    double error = IndexingUtils::Optimize_Direction( best_vec, 
                                                      index_values, 
                                                      q_vectors );
    TS_ASSERT_DELTA( error, 0.00218606, 1e-5 );
    TS_ASSERT_DELTA( best_vec[0], -2.58222, 1e-4 );
    TS_ASSERT_DELTA( best_vec[1],  3.97345, 1e-4 );
    TS_ASSERT_DELTA( best_vec[2], -4.55145, 1e-4 );
  }


  void test_ScanFor_UB()
  {
    double correct_UB[] = { -0.102577,  0.0999725, -0.0136353,
                             0.123290,  0.0146148, -0.0851386,
                            -0.055154, -0.0427632, -0.0630785 };

    Matrix<double> UB(3,3,false);
    int     degrees_per_step   = 3;
    double  required_tolerance = 0.2;
    double  a     = 6.6f;
    double  b     = 9.7f;
    double  c     = 9.9f;
    double  alpha = 84;
    double  beta  = 71;
    double  gamma = 70;

    std::vector<V3D> q_vectors = getNatroliteQs();

    double error = IndexingUtils::ScanFor_UB( UB,
                                              q_vectors,
                                              a, b, c, alpha, beta, gamma,
                                              degrees_per_step,
                                              required_tolerance );

    TS_ASSERT_DELTA( error, 0.147397, 1.e-5 );

    std::vector<double> UB_returned = UB.get_vector();
    for ( size_t i = 0; i < 9; i++ )
    {
      TS_ASSERT_DELTA( UB_returned[i], correct_UB[i], 1e-5 );
    }
  }


  void test_ScanFor_Directions()
  {
    double vectors[5][3] = { {  0.08445961, 9.26951000,  3.4138980 },
                             { -2.58222370, 3.97345330, -4.5514464 },
                             {  2.66668320, 5.29605670,  7.9653444 },
                             {  7.01297300, 3.23755380, -5.8988633 },
                             { -9.59519700, 0.73589927,  1.3474168 }  };

    std::vector<V3D> directions;
    std::vector<V3D> q_vectors = getNatroliteQs();
    double d_min = 6;
    double d_max = 10;
    double degrees_per_step = 1.0;
    double required_tolerance = 0.12;

    IndexingUtils::ScanFor_Directions( directions,
                                       q_vectors,
                                       d_min, d_max,
                                       required_tolerance,
                                       degrees_per_step );

    TS_ASSERT_EQUALS( 5, directions.size() );

    for ( size_t i = 0; i < 3; i++ )
    {
       V3D vec = directions[i];
       for ( int j = 0; j < 3; j++ )
       {
         TS_ASSERT_DELTA( vectors[i][j], vec[j], 1.e-5 );
       }
     }
  }


  void test_Make_c_dir()
  {
    V3D a_dir(  1, 2, 3 );
    V3D b_dir( -3, 2, 1 );

    double gamma    = a_dir.angle( b_dir ) * 180.0 / PI;
    double alpha    = 123;
    double beta     = 74;
    double c_length = 10;
    V3D result = IndexingUtils::Make_c_dir( a_dir, b_dir, c_length,
                                            alpha, beta, gamma );

    double alpha_calc = result.angle( b_dir ) * 180 / PI;
    double beta_calc  = result.angle( a_dir ) * 180 / PI;

    TS_ASSERT_DELTA( result.norm(), c_length, 1e-5 );
    TS_ASSERT_DELTA( alpha_calc, alpha, 1e-5 );
    TS_ASSERT_DELTA( beta_calc, beta, 1e-5 );
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


  void test_CalculateMillerIndices()
  {
    Matrix<double> UB(3,3,false);

    UB.setRow( 0, V3D( -0.1015550,  0.0992964, -0.0155078 ) );
    UB.setRow( 1, V3D(  0.1274830,  0.0150210, -0.0839671 ) );
    UB.setRow( 2, V3D( -0.0507717, -0.0432269, -0.0645173 ) );

    std::vector<V3D> q_vectors = getNatroliteQs();
    double           tolerance = 0.08;
    std::vector<V3D> miller_indices;
    double           average_error;

    int num_indexed = IndexingUtils::CalculateMillerIndices( UB,
                                                             q_vectors,
                                                             tolerance,
                                                             miller_indices,
                                                             average_error ); 

    TS_ASSERT_EQUALS( num_indexed, 12 );

    TS_ASSERT_DELTA( average_error, 0.0103505, 1e-5 );

    V3D mi_0  = V3D(  0.992465, -4.00351, 4.997260 );
    V3D mi_1  = V3D(  3.991040, -8.00753, 14.00010 );
    V3D mi_2  = V3D(  2.018340, -7.96556, 8.020210 );
    V3D mi_11 = V3D( -3.006000, -7.99572, 0.980049 );

    double diff;
                                                 // spot check a few indices 
    diff = (mi_0  - miller_indices[0] ).norm();
    TS_ASSERT_DELTA( diff, 0, 1e-5 );

    diff = (mi_1  - miller_indices[1] ).norm();
    TS_ASSERT_DELTA( diff, 0, 1e-5 );

    diff = (mi_2  - miller_indices[2] ).norm();
    TS_ASSERT_DELTA( diff, 0, 1e-5 );

    diff = (mi_11 - miller_indices[11]).norm();
    TS_ASSERT_DELTA( diff, 0, 1e-5 );
  }

  
  void test_GetIndexedPeaks_1D()
  {
    int correct_indices[] = { 1, 4, 2, 0, 1, 3, 0, -1, 0, -1, -2, -3 };

    std::vector<V3D> q_vectors = getNatroliteQs();

    V3D direction( -2.5825930,  3.9741700, -4.5514810 );
    double required_tolerance = 0.1;
    double fit_error = 0;

    std::vector<int> index_vals;
    std::vector<V3D> indexed_qs;

    int num_indexed = IndexingUtils::GetIndexedPeaks_1D( direction,
                                                         q_vectors,
                                                         required_tolerance,
                                                         index_vals,
                                                         indexed_qs,
                                                         fit_error );
    TS_ASSERT_EQUALS( num_indexed, 12 );
    TS_ASSERT_EQUALS( index_vals.size(), 12 );
    TS_ASSERT_EQUALS( indexed_qs.size(), 12 );
    TS_ASSERT_DELTA( fit_error, 0.00218634, 1e-5 );

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

    std::vector<V3D> q_vectors = getNatroliteQs();

    V3D direction_1(  -2.5825930,  3.9741700, -4.5514810 );
    V3D direction_2( -16.6087800, -2.5005515,  7.2465878 );
    V3D direction_3(   2.7502847, 14.5671910, 11.3796620 );

    double required_tolerance = 0.1;
    double fit_error = 0;

    std::vector<V3D> index_vals;
    std::vector<V3D> indexed_qs;

    int num_indexed = IndexingUtils::GetIndexedPeaks_3D( direction_1,
                                                         direction_2,
                                                         direction_3,
                                                         q_vectors,
                                                         required_tolerance,
                                                         index_vals,
                                                         indexed_qs,
                                                         fit_error );
    TS_ASSERT_EQUALS( num_indexed, 12 );
    TS_ASSERT_EQUALS( index_vals.size(), 12 );
    TS_ASSERT_EQUALS( indexed_qs.size(), 12 );
    TS_ASSERT_DELTA( fit_error, 0.023007052, 1e-5 );

    for ( size_t i = 0; i < index_vals.size(); i++ )
    {
      for ( size_t j = 0; j < 3; j++ )
      {
        TS_ASSERT_EQUALS( (index_vals[i])[j], (correct_indices[i])[j] );
      }
    }
  }


  void test_GetIndexedPeaks()
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

    std::vector<V3D> q_vectors = getNatroliteQs();

    V3D row_0( -0.059660400, -0.049648200, 0.0077539105 );
    V3D row_1(  0.093009956, -0.007510495, 0.0419835400 );
    V3D row_2( -0.104643770 , 0.021613428, 0.0322586300 );

    Matrix<double> UB(3,3,false);
    UB.setRow( 0, row_0 );    
    UB.setRow( 1, row_1 );    
    UB.setRow( 2, row_2 );    

    double required_tolerance = 0.1;
    double fit_error = 0;

    std::vector<V3D> index_vals;
    std::vector<V3D> indexed_qs;

    int num_indexed = IndexingUtils::GetIndexedPeaks( UB,
                                                      q_vectors,
                                                      required_tolerance,
                                                      index_vals,
                                                      indexed_qs,
                                                      fit_error );
    TS_ASSERT_EQUALS( num_indexed, 12 );
    TS_ASSERT_EQUALS( index_vals.size(), 12 );
    TS_ASSERT_EQUALS( indexed_qs.size(), 12 );
    TS_ASSERT_DELTA( fit_error, 0.023007052, 1e-5 );

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

    std::vector<V3D> q_vectors = getNatroliteQs();

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
