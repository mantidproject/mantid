/* File: Indexing_Utils.h */

#ifndef MANTID_GEOMETRY_INDEXING_UTILS_H_
#define MANTID_GEOMETRY_INDEXING_UTILS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Matrix.h"

namespace Mantid
{
namespace Geometry
{
/**
    @class IndexingUtils
  
    This class contains static utility methods for indexing peaks and
    finding the UB matrix.  Currently there is only one method, BestFit_UB
    that finds the best UB matrix given some indexed peaks.
  
    @author Dennis Mikkelson 
    @date   2011-06-14 
     
    Copyright © 2011 ORNL, STFC Rutherford Appleton Laboratories
  
    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
    File change history is stored at: 
                 <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>

    Code Documentation is available at 
                 <http://doxygen.mantidproject.org>
 */

class MANTID_GEOMETRY_DLL IndexingUtils
{
  public:

  /// Find the UB matrix that most nearly indexes the specified qxyz values 
  /// given the lattice parameters 
  static double BestFit_UB(       Kernel::DblMatrix        & UB,
                            const std::vector<Kernel::V3D> & q_vectors,
                                  double                     required_tolerance,
                                  double a, double b, double c,
                                  double alpha, double beta, double gamma );

  /// Find the UB matrix that most nearly maps hkl to qxyz for 3 or more peaks
  static double BestFit_UB(Kernel::DblMatrix               & UB,
                           const std::vector<Kernel::V3D>  & hkl_vectors, 
                           const std::vector<Kernel::V3D>  & q_vectors   );

  /// Find the vector that best corresponds to plane normal, given 1-D indices
  static double BestFit_Direction(     Kernel::V3D       & best_vec,
                                 const std::vector<int>  & index_values,
                                 const std::vector<Kernel::V3D>  & q_vectors );

  /// Get the vector in the direction of "c" given other unit cell information
  static Kernel::V3D Make_c_dir( const Kernel::V3D  & a_dir,
                                 const Kernel::V3D  & b_dir,
                                       double c,
                                       double alpha, double beta, double gamma);

  /// Check is hkl is within tolerance of integer (h,k,l) non-zero values
  static bool ValidIndex( const Kernel::V3D  & hkl,
                                double         tolerance );

  /// Calculate the number of Q vectors that are mapped to integer indices by UB
  static int NumberIndexed( const Kernel::DblMatrix & UB,
                            const std::vector<Kernel::V3D>  & q_vectors,
                                  double                      tolerance   );

  /// Get lists of indices and Qs for peaks indexed in the specified direction 
  static int GetIndexedPeaks_1D( 
                          const std::vector<Kernel::V3D> & q_vectors,
                          const Kernel::V3D              & direction,
                                double                     required_tolerance,
                                std::vector<int>         & index_vals,
                                std::vector<Kernel::V3D> & indexed_qs,
                                double      & fit_error );

  /// Get lists of indices and Qs for peaks indexed in three given directions
  static int GetIndexedPeaks_3D( 
                          const std::vector<Kernel::V3D> & q_vectors,
                          const Kernel::V3D              & direction_1,
                          const Kernel::V3D              & direction_2,
                          const Kernel::V3D              & direction_3,
                                double                     required_tolerance,
                                std::vector<Kernel::V3D> & miller_indices,
                                std::vector<Kernel::V3D> & indexed_qs,
                                double                   & fit_error );

  /// Get lists of indices and Qs for peaks indexed by the specified UB matrix 
  static int GetIndexedPeaks( 
                          const std::vector<Kernel::V3D> & q_vectors,
                          const Kernel::DblMatrix        & UB,
                                double                     required_tolerance,
                                std::vector<Kernel::V3D> & miller_indices,
                                std::vector<Kernel::V3D> & indexed_qs,
                                double                   & fit_error );

  /// Make list of direction vectors uniformly distributed over a hemisphere
  static std::vector<Kernel::V3D> MakeHemisphereDirections( int n_steps );

  /// Make list of the circle of direction vectors that form a fixed angle
  /// with the specified axis 
  static std::vector<Kernel::V3D> MakeCircleDirections( int         n_steps,
                                                  const Kernel::V3D axis,
                                                  double      angle_degrees );


  /// Choose the direction in a list of directions, that is most nearly 
  //  perpendicular to planes with the specified spacing in reciprocal space.
  static int SelectDirection(        Kernel::V3D & best_direction,
                              const  std::vector<Kernel::V3D> q_vectors,
                              const  std::vector<Kernel::V3D> direction_list,
                                     double plane_spacing,
                                     double required_tolerance );
};


} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_INDEXING_UTILS_H_ */

