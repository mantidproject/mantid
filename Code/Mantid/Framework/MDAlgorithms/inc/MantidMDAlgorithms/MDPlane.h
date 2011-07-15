#ifndef MANTID_MDALGORITHMS_MDPLANE_H_
#define MANTID_MDALGORITHMS_MDPLANE_H_
    
#include "MantidKernel/System.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include <vector>


namespace Mantid
{
namespace MDAlgorithms
{

  /** A generalized description of a N-dimensional hyperplane.
    To be used in MDImplicitFunction.

    This has be to fully general, with:
      nd : number of dimensions of space

    The general equation for a hyperplane is:
      a1*x1 + a2*x2 + ... < b

    where  x1, x2, .. are the n-th coordinate of the point.
           a1, a2, .. are coefficients (can be 0).

    @author Janik Zikovsky
    @date 2011-07-15

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport MDPlane 
  {
  public:
    MDPlane(const std::vector<coord_t> & coeff, const coord_t inequality);
    MDPlane(const size_t nd, const coord_t * coeff, const coord_t inequality);
    virtual ~MDPlane();

    /// @return the number of dimensions
    size_t getNumDims() const { return m_nd; }

    /// @return the coefficients. For debugging mostly.
    const coord_t * getCoeff() const { return m_coeff; }


    // ==================== Methods that are inline for performance ================================
    //----------------------------------------------------------------------------------------------
    /** Is a point in MDimensions bounded by this hyperplane, that is,
     * is (a1*x1 + a2*x2 + ... < b)?
     *
     * @param coords :: nd-sized array of coordinates
     * @return true if it is bounded by the plane
     */
    bool isPointBounded(const coord_t * coords) const
    {
      coord_t total = 0;
      for (size_t d=0; d<m_nd; d++)
      {
        total += m_coeff[d] * coords[d];
      }
      return (total < m_inequality);
    }


    //----------------------------------------------------------------------------------------------
    /** Given two points defining the start and end point of a line,
     * is there an intersection between the hyperplane and the line
     * defined by the points?
     *
     * @param pointA :: first point/vertex; nd-sized array of coordinates
     * @param pointB :: last point/vertex; nd-sized array of coordinates
     * @return true if the line DOES intersect.
     */
    bool doesLineIntersect(const coord_t * pointA, const coord_t * pointB) const
    {
      bool AisBounded = isPointBounded(pointA);
      bool BisBounded = isPointBounded(pointB);
      // The line crosses the plane if one point is bounded and not the other. Simple! :)
      return (AisBounded != BisBounded);
    }



  protected:
    /// Number of dimensions in the MDEventWorkspace
    size_t m_nd;

    /// Coefficients to multiply each coordinate. Of size nd;
    coord_t * m_coeff;

    /// Right-hand side of the linear equation. aka b in : a1*x1 + a2*x2 + ... < b
    coord_t m_inequality;

  };


} // namespace MDAlgorithms
} // namespace Mantid

#endif  /* MANTID_MDALGORITHMS_MDPLANE_H_ */
