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
double IndexingUtils::BestFit_UB(      DblMatrix         & UB,
                                 const std::vector<V3D>  & hkl_vectors, 
                                 const std::vector<V3D>  & q_vectors )
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

/**
   Calculate the number of Q vectors that are mapped to integer h,k,l 
   values by UB.  Each of the Miller indexes, h, k and l must be within
   the specified tolerance of an integer, in order to count the peak
   as indexed.  Also, if (h,k,l) = (0,0,0) the peak will NOT be counted
   as indexed, since (0,0,0) is not a valid index of any peak.
  
   @param UB           A 3x3 matrix of doubles holding the UB matrix
   @param q_vectors    std::vector of V3D objects that contains the list of 
                       q_vectors that are indexed by the corresponding hkl
                       vectors.
   @param tolerance    The maximum allowed distance between each component
                       of UB*Q and the nearest integer value, required to
                       to count the peak as indexed by UB.
   @return A non-negative integer giving the number of peaks indexed by UB. 
  
   @throws 
 */
int IndexingUtils::NumberIndexed( const DblMatrix         & UB,
                                  const std::vector<V3D>  & q_vectors,
                                        double              tolerance )
{
  int count = 0;

  if ( UB != 0 || UB.numRows() != 3 || UB.numCols() != 3 )
  {
   throw std::invalid_argument("UB matrix NULL or not 3X3");
  }

  DblMatrix UB_inverse( UB );
  double determinant = UB_inverse.Invert();
  if ( fabs( determinant ) < 1.0e-10 )
  {
   throw std::invalid_argument("UB is singular (det < 1e-10)");
  } 

  V3D hkl;
  for ( size_t i = 0; i < q_vectors.size(); i++ )
  {
    hkl = UB_inverse * q_vectors[i];
    if ( ValidIndex( hkl, tolerance ) )
    {
      count++;
    }
  }

  return count;
}

/**
  Check whether or not the components of the specified vector are within
  the specified tolerance of integer values, other than (0,0,0).
  @param hkl        A V3D object containing what may be valid Miller indices
                    for a peak.
  @param tolerance  The maximum acceptable deviation from integer values for
                    the Miller indices.
  @return true if all components of the vector are within the tolerance of
               integer values (h,k,l) and (h,k,l) is NOT (0,0,0)
 */

bool IndexingUtils::ValidIndex( const V3D & hkl, double tolerance )
{
  bool valid_index = false;

  int h,k,l;
                                        // since C++ lacks a round() we need
                                        // to do it ourselves!
  h = (int)(hkl[0] + (hkl[0] < 0? -0.5 : +0.5));
  k = (int)(hkl[1] + (hkl[1] < 0? -0.5 : +0.5));
  l = (int)(hkl[2] + (hkl[2] < 0? -0.5 : +0.5));

  if ( h != 0 || k != 0 || l != 0 )   // check if indexed, but not as (0,0,0)
  {
    if ( (fabs( hkl[0] - h ) <= tolerance) &&
         (fabs( hkl[1] - k ) <= tolerance) &&
         (fabs( hkl[2] - l ) <= tolerance) )
    {
      valid_index = true; 
    }
  }

  return valid_index;
}


/**
  Make a list of directions, approximately uniformly distributed over a
  hemisphere, with the angular separation between direction vectors 
  approximately 90 degrees/n_steps.
  NOTE: This method provides a list of possible directions for plane 
        normals for reciprocal lattice planes.  This facilitates a 
        brute force search for lattice planes with a specific spacing
        between planes.  This will be used for finding the UB matrix, 
        given the lattice parameters.
  @param n_steps   The number of subdivisions in latitude in the upper
                   hemisphere.
  @retrun A std::vector containing directions distributed over the hemisphere
          with y-coordinate at least zero.
 */
std::vector<V3D> IndexingUtils::MakeHemisphereDirections( int n_steps )
{
  std::vector<V3D> direction_list;

  double PI = 3.14159265358979323846;
  double angle_step = PI / (2*n_steps);

  for ( double phi = 0; phi <= (1.0001)*PI/2; phi += angle_step )
  {
    double r = sin(phi);

    int n_theta = (int)( 2 * PI * r / angle_step + 0.5 );
     
    double theta_step;
      
    if ( n_theta == 0 )                     // n = ( 0, 1, 0 ).  Just
      theta_step = 2 * PI + 1;              // use one vector at the pole

    else
      theta_step = 2 * PI / n_theta;
      
    double last_theta = 2 * PI - theta_step / 2;

    if ( fabs(phi - PI/2) < angle_step/2 )  // use half the equator to avoid
     last_theta = PI - theta_step/2;        // vectors that are the negatives
                                            // of other vectors in the list.

    for ( double theta = 0; theta < last_theta; theta += theta_step )
    {
      V3D direction( r*cos(theta), cos(phi), r*sin(theta) );
      direction_list.push_back( direction );
    }
  }

  return direction_list;
}

/**
  Choose the direction vector that most nearly corresponds to a family of
  planes in the list of qxyz vectors, with spacing equal to the specified
  plane_spacing.  The direction is chosen from the direction_list.

  @param  best_direction      This will be set to the direction that minimizes
                              the sum squared distances of projections of peaks
                              from integer multiples of the specified plane
                              spacing.
  @param  qxyz_vals           List of peak positions, specified according to
                              the convention that |q| = 1/d.  (i.e. Q/2PI)
  @param  direction_list      List of possible directions for plane normals.
                              Initially, this will be a long list of possible
                              directions from MakeHemisphereDirections().
  @param  plane_spacing       The required spacing between planes in reciprocal
                              space.
  @param  required_tolerance  The maximum deviation of the component of a
                              peak qxyz in the direction of the best_direction
                              vector for that peak to count as being indexed. 
                              NOTE: The tolerance is specified in terms of
                              Miller Index.  That is, the distance between 
                              adjacent planes is normalized one for computing
                              the error.
  @return The number of peaks that lie within the specified tolerance of the
          family of planes with normal direction = best_direction and with 
          spacing given by plane_spacing.
 */
size_t IndexingUtils::BestFit_Direction(       V3D & best_direction,
                                        const  std::vector<V3D> qxyz_vals,
                                        const  std::vector<V3D> direction_list,
                                        double plane_spacing,
                                        double required_tolerance )
{
    double dot_product;
    int    nearest_int;
    double error;
    double sum_sq_error;
    double min_sum_sq_error = 1.0e100;

    for ( size_t dir_num = 0; dir_num < direction_list.size(); dir_num++ )
    {
      sum_sq_error = 0;
      V3D direction = direction_list[ dir_num ];
      direction/=plane_spacing;
      for ( size_t q_num = 0; q_num < qxyz_vals.size(); q_num++ )
      {
        dot_product = direction.scalar_prod( qxyz_vals[ q_num ] );
        nearest_int = (int)(dot_product + (dot_product < 0? -0.5 : +0.5));
        error = fabs( dot_product - nearest_int );
        sum_sq_error += error * error;
      }

      if ( sum_sq_error < min_sum_sq_error )
      {
        min_sum_sq_error = sum_sq_error;
        best_direction = direction;
      }
    }

    double proj_value  = 0;
    size_t num_indexed = 0;
    for ( size_t q_num = 0; q_num < qxyz_vals.size(); q_num++ )
    {
      proj_value = best_direction.scalar_prod( qxyz_vals[ q_num ] );
      nearest_int = (int)(proj_value + (proj_value < 0? -0.5 : +0.5));
      error = fabs( proj_value - nearest_int );
      if ( error < required_tolerance )
        num_indexed++;
    }

  best_direction.normalize();

  return num_indexed;
}

