/* File: IndexingUtils.cpp */

#include "MantidGeometry/Crystal/IndexingUtils.h"

#include <iostream>
#include <stdexcept>

extern "C"
{
#include <gsl/gsl_sys.h> 
#include <gsl/gsl_vector.h> 
#include <gsl/gsl_matrix.h> 
#include <gsl/gsl_linalg.h> 
}

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;
using Mantid::Kernel::DblMatrix;

/** 
    STATIC method BestFit_UB: Calculates the matrix that most nearly maps
    the specified hkl_vectors to the specified q_vectors.  The calculated
    UB minimizes the sum squared differences between UB*(h,k,l) and the
    corresponding (qx,qy,qz) for all of the specified hkl and Q vectors.
    The sum of the squares of the residual errors is returned.
  
    @param  UB           3x3 matrix that will be set to the UB matrix
    @param  hkl_vectors  std::vector of V3D objects that contains the 
                         list of hkl values
    @param  q_vectors    std::vector of V3D objects that contains the list of 
                         q_vectors that are indexed by the corresponding hkl
                         vectors.
    NOTE: The number of hkl_vectors and q_vectors must be the same, and must
          be at least 3.
  
    @return  This will return the sum of the squares of the residual errors.
  
    @throws  std::invalid_argument exception if there are not at least 3
                                   hkl and q vectors, or if the numbers of
                                   hkl and q vectors are not the same.
   
    @throws  std::runtime_error    exception if the QR factorization fails or
                                   the UB matrix can't be calculated or if 
                                   UB is a singular matrix.
*/  
double IndexingUtils::BestFit_UB(    DblMatrix & UB,
                                 const std::vector<V3D>    & hkl_vectors, 
                                 const std::vector<V3D>    & q_vectors )
{
  if ( hkl_vectors.size() < 3 ) 
  {
   throw std::invalid_argument("Three or more indexed peaks needed to find UB");
  }

  if ( hkl_vectors.size() != q_vectors.size() )
  {
   throw std::invalid_argument("Number of hkl_vectors != number of q_vectors");
  }

  gsl_matrix *H_transpose = gsl_matrix_alloc( hkl_vectors.size(), 3 );
  gsl_vector *tau         = gsl_vector_alloc( 3 );

  double sum_sq_error = 0;
                                      // Make the H-transpose matrix from the
                                      // hkl vectors and form QR factorization
  for ( size_t row = 0; row < hkl_vectors.size(); row++ )
  {
    for ( size_t col = 0; col < 3; col++ )
    {
      gsl_matrix_set( H_transpose, row, col, (hkl_vectors[row])[col] );
    }
  }

  int returned_flag = gsl_linalg_QR_decomp( H_transpose, tau );

  if ( returned_flag != 0 )
  {
    gsl_matrix_free( H_transpose );
    gsl_vector_free( tau );
    throw std::runtime_error("gsl QR_decomp failed, invalid hkl values");
  }
                                      // solve for each row of UB, using the 
                                      // QR factorization of and accumulate the 
                                      // sum of the squares of the residuals
  gsl_vector *UB_row      = gsl_vector_alloc( 3 );
  gsl_vector *q           = gsl_vector_alloc( q_vectors.size() );
  gsl_vector *residual    = gsl_vector_alloc( q_vectors.size() );

  bool found_UB = true;

  for ( size_t row = 0; row < 3; row++ )
  {
    for ( size_t i = 0; i < q_vectors.size(); i++ )
    {
      gsl_vector_set( q, i, (q_vectors[i])[row] );
    }

    returned_flag = gsl_linalg_QR_lssolve(H_transpose,tau,q,UB_row,residual);
    if ( returned_flag != 0 )
    {
      found_UB = false;
    }

    for ( size_t i = 0; i < 3; i++ )
    {
      double value = gsl_vector_get( UB_row, i );      
      if ( gsl_isnan( value ) || gsl_isinf( value) )
        found_UB = false;
    }

    V3D row_values( gsl_vector_get( UB_row, 0 ), 
                    gsl_vector_get( UB_row, 1 ),
                    gsl_vector_get( UB_row, 2 ) );
    UB.setRow( row, row_values );

    for ( size_t i = 0; i < q_vectors.size(); i++ )
    {
      sum_sq_error += gsl_vector_get(residual, i) * gsl_vector_get(residual, i);
    }
  }

  gsl_matrix_free( H_transpose );
  gsl_vector_free( tau );
  gsl_vector_free( UB_row );
  gsl_vector_free( q );
  gsl_vector_free( residual );

  if ( !found_UB )
  {
    throw std::runtime_error("Failed to find UB, invalid hkl or Q values");
  }
 
  if ( fabs ( UB.determinant() ) < 1.0e-10 )
  {
    throw std::runtime_error("UB is singular, invalid hkl or Q values");
  }

  return sum_sq_error;
}
