/* File: IndexingUtils.cpp */

#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidKernel/Quat.h"


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
using Mantid::Kernel::Quat;


#define round(x) ((x)>=0?(int)((x)+0.5):(int)((x)-0.5))
#define PI 3.141592653589793238

/** 
  STATIC method BestFit_UB: Calculates the matrix that most nearly indexes 
  the specified q_vectors, given the lattice parameters.  
  The sum of the squares of the residual errors is returned.
  
  @param  UB                  3x3 matrix that will be set to the UB matrix
  @param  q_vectors           std::vector of V3D objects that contains the 
                              list of q_vectors that are to be indexed
                              NOTE: There must be at least 3 q_vectors.
  @param  required_tolerance  The maximum allowed deviation of Miller indices
                              from integer values for a peak to be indexed.
  @param  a                   First unit cell edge length in Angstroms.  
  @param  b                   Second unit cell edge length in Angstroms.  
  @param  c                   Third unit cell edge length in Angstroms.  
  @param  alpha               First unit cell angle in degrees.
  @param  beta                second unit cell angle in degrees.
  @param  gamma               third unit cell angle in degrees.

  @return  This will return the sum of the squares of the residual errors.
  
  @throws  std::invalid_argument exception if there are not at least 3
                                 q vectors.
   
  @throws  std::runtime_error    exception if the UB matrix can't be found.
                                 This will happen if the q_vectors do not
                                 determine all three directions of the unit
                                 cell, or if they cannot be indexed within
                                 the required tolerance.
*/
double IndexingUtils::BestFit_UB(       DblMatrix        & UB,
                                  const std::vector<V3D> & q_vectors,
                                        double             required_tolerance,
                                        double a, double b, double c,
                                        double alpha, double beta, double gamma)
{
  if ( UB.numRows() != 3 || UB.numCols() != 3 )
  {
   throw std::invalid_argument("UB matrix NULL or not 3X3");
  }

  if ( q_vectors.size() < 3 )
  {
   throw std::invalid_argument("Three or more indexed peaks needed to find UB");
  }

                           // Make a hemisphere of possible direction for
                           // plane normals for the reciprocal space planes
                           // with normals in the direction of "a" in unit cell
  int num_steps   = 180;
  std::vector<V3D> dir_list = MakeHemisphereDirections(num_steps);

  double plane_distance = 1/a;

  V3D a_dir;               // First select the best fitting direction vector
                           // for a_dir from the hemisphere of possiblities.
  int num_indexed = SelectDirection( a_dir,
                                     q_vectors,
                                     dir_list,
                                     plane_distance,
                                     required_tolerance );

  a_dir *= a;              // Adjust the length of a_dir so a_dir "dot" q is 
                           // an integer if q is on the family of planes 
                           // perpendicular to a_dir in reciprocal space.
                           // Next, get the sub-list of q_vectors that are
                           // indexed in this direction, along with the indices.
  double fit_error = 0;
  std::vector<int> index_vals;
  std::vector<V3D> indexed_qs;
  num_indexed = GetIndexedPeaks_1D( q_vectors,
                                    a_dir,
                                    required_tolerance,
                                    index_vals,
                                    indexed_qs,
                                    fit_error  );

                            // Use the 1D indices and qs to optimize the 
                            // plane normal, a_dir.
  fit_error = BestFit_Direction( a_dir, index_vals, indexed_qs );

                            // Now do a similar process for the planes with
                            // normals in the direction of "b" in the unit cell
                            // EXCEPT, choose only from the circle of vectors
                            // that form the correct angle (gamma) with the
                            // previously found a_dir vector.
  num_steps = 10000;
  double angle_degrees = gamma;
  std::vector<V3D> directions =
                      MakeCircleDirections( num_steps, a_dir, angle_degrees );
  V3D b_dir;
  plane_distance = 1/b;
  num_indexed = SelectDirection( b_dir,
                                 q_vectors,
                                 directions,
                                 plane_distance,
                                 required_tolerance );

  b_dir *= b;
  num_indexed = GetIndexedPeaks_1D( q_vectors,
                                    b_dir,
                                    required_tolerance,
                                    index_vals,
                                    indexed_qs,
                                    fit_error  );

  fit_error = BestFit_Direction( b_dir, index_vals, indexed_qs );

  // Now calculate the third direction, for plane normals in the c direction,
  // using the results in UBMatriximplementationnotes.pdf, pg 3, Andre Savici.
  // Get the components of c_dir relative to an orthonormal basis with the 
  // first basis vector in the direction of a_dir and the second basis vector
  // in the (a_dir, b_dir) plane. 

  double cos_alpha = cos(PI/180.0 * alpha);
  double cos_beta  = cos(PI/180.0 * beta);
  double cos_gamma = cos(PI/180.0 * gamma);
  double sin_gamma = sin(PI/180.0 * gamma);

  double c1 = c * cos_beta;
  double c2 = c * ( cos_alpha - cos_gamma * cos_beta )/sin_gamma;
  double V  =  sqrt( 1 - cos_alpha * cos_alpha
                       - cos_beta  * cos_beta
                       - cos_gamma * cos_gamma
                   + 2 * cos_alpha * cos_beta * cos_gamma );
  double c3 = c * V / sin_gamma;

  V3D basis_1( a_dir );
  basis_1.normalize();

  V3D basis_3 = a_dir.cross_prod(b_dir);
  basis_3.normalize();

  V3D basis_2 = basis_3.cross_prod(basis_1);
  basis_2.normalize();

  V3D c_dir = (basis_1*c1) + (basis_2*c2) + (basis_3*c3);

                            // Optimize the c_dir vector as before

  num_indexed = GetIndexedPeaks_1D( q_vectors,
                                    c_dir,
                                    required_tolerance,
                                    index_vals,
                                    indexed_qs,
                                    fit_error  );

  fit_error = BestFit_Direction( c_dir, index_vals, indexed_qs );

                            // Now, using the plane normals for all three
                            // families of planes, get a consistent indexing
                            // discarding any peaks that are not indexed in 
                            // all three directions.
  std::vector<V3D> miller_ind;
  num_indexed = GetIndexedPeaks_3D( q_vectors,
                                    a_dir, b_dir, c_dir,
                                    required_tolerance,
                                    miller_ind,
                                    indexed_qs,
                                    fit_error );

                            // Finally, use the indexed peaks to get an 
                            // optimized UB that matches the indexing
  fit_error = BestFit_UB( UB, miller_ind, indexed_qs );

  return fit_error;
}


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
                                 hkl and q vectors are not the same, or if
                                 the UB matrix is not a 3x3 matrix.
   
  @throws  std::runtime_error    exception if the QR factorization fails or
                                 the UB matrix can't be calculated or if 
                                 UB is a singular matrix.
*/  
double IndexingUtils::BestFit_UB(      DblMatrix         & UB,
                                 const std::vector<V3D>  & hkl_vectors, 
                                 const std::vector<V3D>  & q_vectors )
{
  if ( UB.numRows() != 3 || UB.numCols() != 3 )
  {
   throw std::invalid_argument("UB matrix NULL or not 3X3");
  }

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
  STATIC method BestFit_Direction: Calculates the vector for which the
  dot product of the the vector with each of the specified Qxyz vectors 
  is most nearly the corresponding integer index.  The calculated best_vec
  minimizes the sum squared differences between best_vec dot (qx,qy,z) 
  and the corresponding index for all of the specified Q vectors and 
  indices.  The sum of the squares of the residual errors is returned.
  NOTE: This method is similar the BestFit_UB method, but this method only
        optimizes the plane normal in one direction.  Also, this optimizes
        the mapping from (qx,qy,qz) to one index (Q to index), while the 
        BestFit_UB method optimizes the mapping from three (h,k,l) to
        (qx,qy,qz) (3 indices to Q).
  
  @param  best_vec     V3D vector that will be set to a vector whose 
                       direction most nearly corresponds to the plane
                       normal direction and whose magnitude is d.  The 
                       corresponding plane spacing in reciprocal space 
                       is 1/d.
  @param  index_values std::vector of ints that contains the list of indices 
  @param  q_vectors    std::vector of V3D objects that contains the list of 
                       q_vectors that are indexed in one direction by the 
                       corresponding index values.
  NOTE: The number of index_values and q_vectors must be the same, and must
        be at least 3.
  
  @return  This will return the sum of the squares of the residual errors.
  
  @throws  std::invalid_argument exception if there are not at least 3
                                 indices and q vectors, or if the numbers of
                                 indices and q vectors are not the same.
   
  @throws  std::runtime_error    exception if the QR factorization fails or
                                 the best direction can't be calculated.
*/

double IndexingUtils::BestFit_Direction(           V3D          & best_vec,
                                        const std::vector<int>  & index_values,
                                        const std::vector<V3D>  & q_vectors )
{
  if ( index_values.size() < 3 )
  {
   throw std::invalid_argument("Three or more indexed values needed");
  }

  if ( index_values.size() != q_vectors.size() )
  {
   throw std::invalid_argument("Number of index_values != number of q_vectors");
  }

  gsl_matrix *H_transpose = gsl_matrix_alloc( q_vectors.size(), 3 );
  gsl_vector *tau         = gsl_vector_alloc( 3 );

  double sum_sq_error = 0;
                                     // Make the H-transpose matrix from the
                                     // q vectors and form QR factorization

  for ( size_t row = 0; row < q_vectors.size(); row++ )
  {
    for ( size_t col = 0; col < 3; col++ )
    {
      gsl_matrix_set( H_transpose, row, col, (q_vectors[row])[col] );
    }
  }
  int returned_flag = gsl_linalg_QR_decomp( H_transpose, tau );

  if ( returned_flag != 0 )
  {
    gsl_matrix_free( H_transpose );
    gsl_vector_free( tau );
    throw std::runtime_error("gsl QR_decomp failed, invalid hkl values");
  }
                                      // solve for the best_vec, using the
                                      // QR factorization and accumulate the 
                                      // sum of the squares of the residuals
  gsl_vector *x        = gsl_vector_alloc( 3 );
  gsl_vector *indices  = gsl_vector_alloc( index_values.size() );
  gsl_vector *residual = gsl_vector_alloc( index_values.size() );

  bool found_best_vec = true;

  for ( size_t i = 0; i < index_values.size(); i++ )
  {
    gsl_vector_set( indices, i, index_values[i] );
  }

  returned_flag = gsl_linalg_QR_lssolve( H_transpose,
                                         tau,
                                         indices,
                                         x,
                                         residual);
  if ( returned_flag != 0 )
  {
    found_best_vec = false;
  }

  for ( size_t i = 0; i < 3; i++ )
  {
    double value = gsl_vector_get( x, i );
    if ( gsl_isnan( value ) || gsl_isinf( value) )
      found_best_vec = false;
  }

  best_vec( gsl_vector_get( x, 0 ),
            gsl_vector_get( x, 1 ),
            gsl_vector_get( x, 2 ) );

  for ( size_t i = 0; i < index_values.size(); i++ )
  {
    sum_sq_error += gsl_vector_get(residual, i) * gsl_vector_get(residual, i);
  }

  gsl_matrix_free( H_transpose );
  gsl_vector_free( tau );
  gsl_vector_free( x );
  gsl_vector_free( indices );
  gsl_vector_free( residual );

  if ( !found_best_vec )
  {
    throw std::runtime_error(
                      "Failed to find best_vec, invalid indexes or Q values");
  }

  return sum_sq_error;
}


/**
    For a rotated unit cell, calculate the vector in the direction of edge
    "c" given two vectors a_dir and b_dir in the directions of edges "a" 
    and "b", with lengths a and b, and the cell angles.
    @param  a_dir   V3D object with length "a" in the direction of the rotated 
                    cell edge "a"
    @param  b_dir   V3D object with length "b" in the direction of the rotated 
                    cell edge "b"
    @param  c       The length of the third cell edge, c.
    @param  alpha   angle between edges b and c. 
    @param  beta    angle between edges c and a. 
    @param  gamma   angle between edges a and b. 

    @return A new V3D object with length "c", in the direction of the third
            rotated unit cell edge, "c".
 */
V3D IndexingUtils::Make_c_dir( const V3D  & a_dir,
                               const V3D  & b_dir,
                                     double c,
                                     double alpha, double beta, double gamma )
{
  double cos_alpha = cos( PI/180.0 * alpha );
  double cos_beta  = cos( PI/180.0 * beta  );
  double cos_gamma = cos( PI/180.0 * gamma );
  double sin_gamma = sin( PI/180.0 * gamma );

  double c1 = c * cos_beta;
  double c2 = c * ( cos_alpha - cos_gamma * cos_beta )/sin_gamma;
  double V  = sqrt( 1 - cos_alpha * cos_alpha
                      - cos_beta  * cos_beta
                      - cos_gamma * cos_gamma
                      + 2 * cos_alpha * cos_beta * cos_gamma );
  double c3 = c * V / sin_gamma;

  V3D basis_1(a_dir);
  basis_1.normalize();

  V3D basis_3(a_dir);
  basis_3 = basis_3.cross_prod(b_dir);
  basis_3.normalize();

  V3D basis_2( basis_3 );
  basis_2 = basis_2.cross_prod(basis_1);
  basis_2.normalize();

  V3D c_dir = basis_1 * c1 + basis_2 * c2 + basis_3 * c3;

  return c_dir;
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
  h = round( hkl[0] );
  k = round( hkl[1] );
  l = round( hkl[2] );

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
  
  @throws std::invalid_argument exception if the UB matrix is not a 3X3 matrix.
 */
int IndexingUtils::NumberIndexed( const DblMatrix         & UB,
                                  const std::vector<V3D>  & q_vectors,
                                        double              tolerance )
{
  int count = 0;

  if ( UB.numRows() != 3 || UB.numCols() != 3 )
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
  Given one plane normal direction for a family of parallel planes in 
  reciprocal space, find the peaks that lie on these planes to within the 
  specified tolerance.  The direction is specified as a vector with length 
  "a" if the plane spacing in reciprocal space is 1/a.  In that way, the 
  dot product of a peak Qxyz with the direction vector will be an integer 
  if the peak lies on one of the planes.   

  @param q_vectors           List of V3D peaks in reciprocal space
  @param direction           Direction vector in the direction of the 
                             normal vector for a family of parallel planes
                             in reciprocal space.  The length of this vector 
                             must be the reciprocal of the plane spacing.
  @param required_tolerance  The maximum allowed error (as a faction of
                             the corresponding Miller index) for a peak
                             q_vector to be counted as indexed.
  @param index_vals          List of the one-dimensional Miller indices peaks
                             that were indexed in the specified direction.
  @param indexed_qs          List of Qxyz value for the peaks that were
                             indexed indexed in the specified direction.
  @param fit_error           The sum of the squares of the distances from
                             integer values for the projections of the 
                             indexed q_vectors on the specified direction.

  @return The number of q_vectors that are indexed to within the specified
          tolerance, in the specified direction.
 */
int IndexingUtils::GetIndexedPeaks_1D( const std::vector<V3D> & q_vectors,
                                       const V3D              & direction,
                                             double        required_tolerance,
                                             std::vector<int> & index_vals,
                                             std::vector<V3D> & indexed_qs,
                                             double           & fit_error )
{
  int     nearest_int;
  double  proj_value;
  double  error;
  int     num_indexed = 0;
  index_vals.clear();
  indexed_qs.clear();
  fit_error = 0;

  for ( size_t q_num = 0; q_num < q_vectors.size(); q_num++ )
  {
    proj_value = direction.scalar_prod( q_vectors[ q_num ] );
    nearest_int = round( proj_value );
    error = fabs( proj_value - nearest_int );
    if ( error < required_tolerance )
    {
      fit_error += error * error;
      indexed_qs.push_back( q_vectors[q_num] );
      index_vals.push_back( nearest_int );
      num_indexed++;
    }
  }

  return num_indexed;
}


/**
  Given three plane normal directions for three families of parallel planes in 
  reciprocal space, find the peaks that lie on these planes to within the 
  specified tolerance.  The three directions are specified as vectors with
  lengths that are the reciprocals of the corresponding plane spacings.  In
  that way, the dot product of a peak Qxyz with one of the direction vectors
  will be an integer if the peak lies on one of the planes corresponding to
  that direction.  If the three directions are properly chosen to correspond
  to the unit cell edges, then the resulting indices will be proper Miller
  indices for the peaks.  This method is similar to GetIndexedPeaks_1D, but
  checks three directions simultaneously and requires that the peak lies
  on all three families of planes simultaneously and does NOT index as (0,0,0).

  @param q_vectors           List of V3D peaks in reciprocal space
  @param direction_1         Direction vector in the direction of the normal
                             vector for the first family of parallel planes.
  @param direction_2         Direction vector in the direction of the normal
                             vector for the second family of parallel planes.
  @param direction_3         Direction vector in the direction of the normal
                             vector for the third family of parallel planes.
  @param required_tolerance  The maximum allowed error (as a faction of
                             the corresponding Miller index) for a peak
                             q_vector to be counted as indexed.
  @param index_vals          List of the Miller indices (h,k,l) of peaks
                             that were indexed in all specified directions.
  @param indexed_qs          List of Qxyz value for the peaks that were
                             indexed indexed in all specified directions.
  @param fit_error           The sum of the squares of the distances from
                             integer values for the projections of the 
                             indexed q_vectors on the specified directions.

  @return The number of q_vectors that are indexed to within the specified
          tolerance, in the specified direction.
 */
int IndexingUtils::GetIndexedPeaks_3D( const std::vector<V3D> & q_vectors,
                                       const V3D              & direction_1,
                                       const V3D              & direction_2,
                                       const V3D              & direction_3,
                                             double        required_tolerance,
                                             std::vector<V3D> & miller_indices,
                                             std::vector<V3D> & indexed_qs,
                                             double           & fit_error )
{
  double  projected_h;
  double  projected_k;
  double  projected_l;
  double  h_error;
  double  k_error;
  double  l_error;
  int     h_int;
  int     k_int;
  int     l_int;
  V3D     hkl;
  int     num_indexed = 0;
  miller_indices.clear();
  indexed_qs.clear();
  fit_error = 0;

  for ( size_t q_num = 0; q_num < q_vectors.size(); q_num++ )
  {
    projected_h = direction_1.scalar_prod( q_vectors[ q_num ] );
    projected_k = direction_2.scalar_prod( q_vectors[ q_num ] );
    projected_l = direction_3.scalar_prod( q_vectors[ q_num ] );

    hkl( projected_h, projected_k, projected_l );

    if ( ValidIndex( hkl, required_tolerance ) )
    {
      h_int = round( projected_h );
      k_int = round( projected_k );
      l_int = round( projected_l );

      h_error = fabs( projected_h - h_int );
      k_error = fabs( projected_k - k_int );
      l_error = fabs( projected_l - l_int );

      fit_error += h_error*h_error + k_error*k_error + l_error*l_error;

      indexed_qs.push_back( q_vectors[q_num] );

      V3D miller_ind( h_int, k_int, l_int );
      miller_indices.push_back( miller_ind );

      num_indexed++;
    }
  }

  return num_indexed;
}


/**
  Given a list of peak positions and a UB matrix, get the list of Miller
  indices and corresponding peak positions for the peaks that are indexed
  to within a specified tolerance, by the UB matrix.  This method is similar
  to GetIndexedPeaks_3D, but directly uses the inverse of the UB matrix to
  map Q -> hkl.

  @param q_vectors           List of V3D peaks in reciprocal space
  @param UB                  The UB matrix that determines the indexing of
                             the peaks.
  @param required_tolerance  The maximum allowed error (as a faction of
                             the corresponding Miller index) for a peak
                             q_vector to be counted as indexed.
  @param index_vals          List of the Miller indices (h,k,l) of peaks
                             that were indexed in all specified directions.
  @param indexed_qs          List of Qxyz value for the peaks that were
                             indexed indexed in all specified directions.
  @param fit_error           The sum of the squares of the distances from
                             integer values for the projections of the 
                             indexed q_vectors on the specified directions.

  @return The number of q_vectors that are indexed to within the specified
          tolerance, in the specified direction.
 */
int IndexingUtils::GetIndexedPeaks( const std::vector<V3D>  & q_vectors,
                                    const DblMatrix         & UB,
                                          double            required_tolerance,
                                          std::vector<V3D>  & miller_indices,
                                          std::vector<V3D>  & indexed_qs,
                                          double            & fit_error )
{
  double  error;
  int     num_indexed = 0;
  V3D     hkl;

  miller_indices.clear();
  indexed_qs.clear();
  fit_error = 0;

  DblMatrix UB_inverse( UB );
  UB_inverse.Invert();

  for ( size_t q_num = 0; q_num < q_vectors.size(); q_num++ )
  {
    hkl = UB_inverse * q_vectors[q_num];

    if ( ValidIndex( hkl, required_tolerance ) )
    {
      for ( int i = 0; i < 3; i++ )
      {
        error = hkl[i] - round(hkl[i]);
        fit_error += error * error; 
      }

      indexed_qs.push_back( q_vectors[q_num] );

      V3D miller_ind( round(hkl[0]), round(hkl[1]), round(hkl[2]) );
      miller_indices.push_back( miller_ind );

      num_indexed++;
    }
  }

  return num_indexed;
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

  @return A std::vector containing directions distributed over the hemisphere
          with y-coordinate at least zero.

  @throws std::invalid_argument exception if the number of steps is <= 0.
 */
std::vector<V3D> IndexingUtils::MakeHemisphereDirections( int n_steps )
{
  if ( n_steps <= 0 )
  {
    throw std::invalid_argument("n_steps must be greater than 0");
  }

  std::vector<V3D> direction_list;

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
  Make a list of directions, uniformly distributed around a circle, all of
  which form the specified angle with the specified axis. 

  @param n_steps   The number of vectors to generate around the circle. 

  @return A std::vector containing direction vectors forming the same angle
          with the axis.

  @throws std::invalid_argument exception if the number of steps is <= 0, or 
                                if the axix length is 0.
 */
std::vector<V3D> IndexingUtils::MakeCircleDirections(        int    n_steps,
                                                       const V3D    axis,
                                                       double angle_degrees )
{
  if ( n_steps <= 0 )
  {
    throw std::invalid_argument("n_steps must be greater than 0");
  }
                               // first get a vector perpendicular to axis
  double max_component = fabs( axis[0] );
  double min_component = fabs( axis[0] );
  size_t min_index = 0;
  for ( size_t i = 1; i < 3; i++ )
  {
    if ( fabs( axis[i] ) < min_component )
    {
      min_component = fabs( axis[i] );
      min_index = i;
    }
    if ( fabs( axis[i] ) > max_component )
    {
      max_component = fabs( axis[i] );
    }
  }

  if ( max_component == 0 )
  {
    throw std::invalid_argument("Axis vector must be non-zero!");
  }

  V3D second_vec( 0, 0, 0 );
  second_vec[min_index] = 1;

  V3D perp_vec = second_vec.cross_prod( axis );
  perp_vec.normalize();

                                // next get a vector that is the specified 
                                // number of degrees away from the axis
  Quat rotation_1( angle_degrees, perp_vec );
  V3D  vector_at_angle( axis );
  rotation_1.rotate( vector_at_angle );
  vector_at_angle.normalize();

                                // finally, form the circle of directions 
                                // consisting of vectors that are at the 
                                // specified angle from the original axis
  double angle_step = 360.0 / n_steps;
  Quat rotation_2( 0, axis );
  std::vector<V3D> directions;
  for ( int i = 0; i < n_steps; i++ )
  {
    V3D vec( vector_at_angle );
    rotation_2.setAngleAxis( i*angle_step, axis );
    rotation_2.rotate( vec );
    directions.push_back( vec );
  }

  return directions;
}


/**
  Choose the direction vector that most nearly corresponds to a family of
  planes in the list of q_vectors, with spacing equal to the specified
  plane_spacing.  The direction is chosen from the specified direction_list.

  @param  best_direction      This will be set to the direction that minimizes
                              the sum squared distances of projections of peaks
                              from integer multiples of the specified plane
                              spacing.
  @param  q_vectors           List of peak positions, specified according to
                              the convention that |q| = 1/d.  (i.e. Q/2PI)
  @param  direction_list      List of possible directions for plane normals.
                              Initially, this will be a long list of possible
                              directions from MakeHemisphereDirections().
  @param  plane_spacing       The required spacing between planes in reciprocal
                              space.
  @param  required_tolerance  The maximum deviation of the component of a
                              peak Qxyz in the direction of the best_direction
                              vector for that peak to count as being indexed. 
                              NOTE: The tolerance is specified in terms of
                              Miller Index.  That is, the distance between 
                              adjacent planes is effectively normalized to one
                              for measuring the error in the computed index.

  @return The number of peaks that lie within the specified tolerance of the
          family of planes with normal direction = best_direction and with 
          spacing given by plane_spacing.

  @throws invalid_argument exception of no Q vectors or directions are 
                           specified.
 */
int IndexingUtils::SelectDirection(       V3D & best_direction,
                                   const  std::vector<V3D> q_vectors,
                                   const  std::vector<V3D> direction_list,
                                   double plane_spacing,
                                   double required_tolerance )
{
  if ( q_vectors.size() == 0 )
  {
    throw std::invalid_argument("No Q vectors specified");
  }

  if ( direction_list.size() == 0 )
  {
    throw std::invalid_argument("List of possible directions has zero length");
  }

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
    for ( size_t q_num = 0; q_num < q_vectors.size(); q_num++ )
    {
      dot_product = direction.scalar_prod( q_vectors[ q_num ] );
      nearest_int = round( dot_product );
      error = fabs( dot_product - nearest_int );
      sum_sq_error += error * error;
    }

    if ( sum_sq_error < min_sum_sq_error + DBL_EPSILON )
    {
      min_sum_sq_error = sum_sq_error;
      best_direction = direction;
    }
  }

  double proj_value  = 0;
  int    num_indexed = 0;
  for ( size_t q_num = 0; q_num < q_vectors.size(); q_num++ )
  {
    proj_value = best_direction.scalar_prod( q_vectors[ q_num ] );
    nearest_int = round( proj_value );
    error = fabs( proj_value - nearest_int );
    if ( error < required_tolerance )
      num_indexed++;
  }

  best_direction.normalize();

  return num_indexed;
}

