/* File: IndexingUtils.cpp */

#include <MantidGeometry/Crystal/IndexingUtils.h>
#include <MantidKernel/System.h>
#include <MantidGeometry/V3D.h>
#include <MantidGeometry/Math/Matrix.h>

extern "C"
{
#include <gsl/gsl_vector.h> 
#include <gsl/gsl_matrix.h> 
#include <gsl/gsl_linalg.h> 
}


using namespace std;
using namespace Mantid::Geometry;


IndexingUtils::IndexingUtils() 
{ // Nothing to do here 
}

double IndexingUtils::BestFit_UB(      Matrix<double> & UB,
                                 const vector<V3D>    & hkl_vectors, 
                                 const vector<V3D>    & q_vectors )
{
  gsl_matrix *H_transpose = gsl_matrix_alloc( hkl_vectors.size(), 3 );
  gsl_vector *tau         = gsl_vector_alloc( 3 );
  gsl_vector *UB_row      = gsl_vector_alloc( 3 );
  gsl_vector *q           = gsl_vector_alloc( q_vectors.size() );
  gsl_vector *residual    = gsl_vector_alloc( q_vectors.size() );

  double sum_sq_error = 0;

                                      // Make the H-transpose matrix from the
                                      // hkl vectors and form QR factorization
  for ( size_t row = 0; row < hkl_vectors.size(); row++ )
    for ( size_t col = 0; col < 3; col++ )
      gsl_matrix_set( H_transpose, row, col, (hkl_vectors[row])[col] );

  int returned_flag = gsl_linalg_QR_decomp( H_transpose, tau );

                                      // solve for each row of UB, using the 
                                      // QR factorization of and accumulate the 
                                      // sum of the squares of the residuals
  for ( size_t row = 0; row < 3; row++ )
  {
    for ( size_t i = 0; i < q_vectors.size(); i++ )
      gsl_vector_set( q, i, (q_vectors[i])[row] );

    gsl_linalg_QR_lssolve( H_transpose, tau, q, UB_row, residual );

    V3D row_values( gsl_vector_get( UB_row, 0 ), 
                    gsl_vector_get( UB_row, 1 ),
                    gsl_vector_get( UB_row, 2 ) );
    UB.setRow( row, row_values );

    for ( size_t i = 0; i < q_vectors.size(); i++ )
      sum_sq_error += gsl_vector_get(residual, i) * gsl_vector_get(residual, i);
  }

  gsl_matrix_free( H_transpose );
  gsl_vector_free( tau );
  gsl_vector_free( UB_row );
  gsl_vector_free( q );
  gsl_vector_free( residual );

  if ( returned_flag == 0 )
    return sum_sq_error;
  else
    return 0;
}

