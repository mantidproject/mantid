/* File: Indexing_Utils.h */

#ifndef MANTID_GEOMETRY_INDEXING_UTILS_H_
#define MANTID_GEOMETRY_INDEXING_UTILS_H_

#include <MantidKernel/System.h>
#include <MantidGeometry/V3D.h>
#include <MantidGeometry/Math/Matrix.h>

namespace Mantid
{
namespace Geometry
{
/** This class contains static utility methods for indexing peaks and
 *  finding the UB matrix.  Currently there is only one method, BestFit_UB
 *  that finds the best UB matrix given some indexed peaks.
 *  @author Dennis Mikkelson 
 *  @date 2011-06-14 
 */

class MANTID_GEOMETRY_DLL IndexingUtils
{
  public:

  // STATIC method BestFit_UB: Calculates the matrix that most nearly maps
  // the specified hkl_vectors to the specified q_vectors.  The calculated
  // UB minimizes the sum squared differences between UB*(h,k,l) and the
  // corresponding (qx,qy,qz) for all of the specified hkl and Q vectors.
  // The sum of the squares of the residual errors is returned.
  //
  // @param  UB           3x3 matrix that will be set to the UB matrix
  // @param  hkl_vectors  std::vector of V3D objects that contains the 
  //                      list of hkl values
  // @param  q_vectors    std::vector of V3D objects that contains the list of 
  //                      q_vectors that are indexed by the corresponding hkl
  //                      vectors.
  // NOTE: The number of hkl_vectors and q_vectors must be the same, and must
  //       be at least 3.
  //
  // @return  This will return the sum of the squares of the residual errors.
  //
  // @throws  std::invalid_argument exception if there are not at least 3
  //                                hkl and q vectors, or if the numbers of
  //                                hkl and q vectors are not the same.
  //
  // @throws  std::runtime_error    exception if the QR factorization fails or
  //                                the UB matrix can't be calculated or if 
  //                                UB is a singular matrix.
  static double BestFit_UB(     Matrix<double>    & UB,
                          const std::vector<V3D>  & hkl_vectors, 
                          const std::vector<V3D>  & q_vectors );
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_INDEXING_UTILS_H_ */

