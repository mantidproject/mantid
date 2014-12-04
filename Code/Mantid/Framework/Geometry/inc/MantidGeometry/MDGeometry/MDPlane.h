#ifndef MANTID_MDALGORITHMS_MDPLANE_H_
#define MANTID_MDALGORITHMS_MDPLANE_H_
    
#include "MantidKernel/System.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include <vector>
#include "MantidKernel/VMD.h"


namespace Mantid
{
namespace Geometry
{

  /** A generalized description of a N-dimensional hyperplane.
    To be used in MDImplicitFunction.

    This has be to fully general, with:
      nd : number of dimensions of space

    The general equation for a hyperplane is:
      a1*x1 + a2*x2 + ... > b

    where  x1, x2, .. are the n-th coordinate of the point.
           a1, a2, .. are coefficients (can be 0).

    Any plane can be defined with:
      - A vector that is normal to its surface. The components
        of the vector becomes the coefficents in the equation
      - A point that is on the plane, which gives the RHS
        of the equation b = point . normal

    Points that are in the direction of the normal of the plane
    are considered to be bounded by it.

    @author Janik Zikovsky
    @date 2011-07-15

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport MDPlane 
  {
  public:
    MDPlane(const Mantid::Kernel::VMD & normal, const Mantid::Kernel::VMD & point);
    MDPlane(const std::vector<coord_t> & normal, const std::vector<coord_t> & point);
    MDPlane(const size_t nd, const float * normal, const float * point);
    MDPlane(const size_t nd, const double * normal, const double * point);
    MDPlane(const std::vector<Mantid::Kernel::VMD> & vectors, const Mantid::Kernel::VMD & origin, const Mantid::Kernel::VMD & insidePoint);
    MDPlane(const MDPlane & other);
    MDPlane & operator=(const MDPlane & other);
    ~MDPlane();

    /// @return the number of dimensions for which this object can be applied
    size_t getNumDims() const { return m_nd; }

    /// @return the normal to the plane. For debugging mostly.
    const coord_t * getNormal() const { return m_normal; }

    /// @return the RHS of the inequality equation. For debugging mostly.
    coord_t getInequality() const { return m_inequality; }


    // ==================== Methods that are inline for performance ================================
    //----------------------------------------------------------------------------------------------
    /** Is a point in MDimensions bounded by this hyperplane, that is,
     * is (a1*x1 + a2*x2 + ... >= b)?
     *
     * @param coords :: nd-sized array of coordinates
     * @return true if it is bounded by the plane
     */
    inline bool isPointBounded(const coord_t * coords) const
    {
      coord_t total = 0;
      for (size_t d=0; d<m_nd; d++)
      {
        total += m_normal[d] * coords[d];
      }
      return (total >= m_inequality);
    }

    //----------------------------------------------------------------------------------------------
    /** Is a point in MDimensions bounded by this hyperplane, that is,
     * is (a1*x1 + a2*x2 + ... >= b)?
     *
     * @param coords :: VMD vector giving the point of the right size
     * @return true if it is bounded by the plane
     */
    inline bool isPointBounded(const Mantid::Kernel::VMD & coords) const
    {
      coord_t total = 0;
      for (size_t d=0; d<m_nd; d++)
      {
        total += m_normal[d] * static_cast<coord_t>(coords[d]);
      }
      return (total >= m_inequality);
    }

    //----------------------------------------------------------------------------------------------
    /** Is a point in MDimensions bounded by this hyperplane, that is,
     * is (a1*x1 + a2*x2 + ... >= b)?
     *
     * @param coords :: nd-sized vector of coordinates. No size check is made!
     * @return true if it is bounded by the plane
     */
    inline bool isPointBounded(const std::vector<coord_t> & coords) const
    {
      coord_t total = 0;
      for (size_t d=0; d<m_nd; d++)
      {
        total += m_normal[d] * coords[d];
      }
      return (total >= m_inequality);
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



  private:
    //----------------------------------------------------------------------------------------------
    /** Templated method for constructing the MDPlane.
     * m_nd must have been set before this call!
     *
     * @param normal :: vector giving the normal to the surface
     * @param point :: vector giving one point on the surface
     */
    template <typename IterableType>
    void construct(IterableType normal, IterableType point)
    {
      m_normal = new coord_t[m_nd];
      m_inequality = 0;
      for (size_t d=0; d<m_nd; d++)
      {
        m_normal[d] = static_cast<coord_t>(normal[d]);
        m_inequality += static_cast<coord_t>(point[d]) * m_normal[d];
      }
    }



  protected:
    /// Number of dimensions in the MDEventWorkspace
    size_t m_nd;

    /** Coefficients to multiply each coordinate, sized m_nd. This
     * is the normal to the plane.
     */
    coord_t * m_normal;

    /// Right-hand side of the linear equation. aka b in : a1*x1 + a2*x2 + ... < b
    coord_t m_inequality;

  };


} // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_MDALGORITHMS_MDPLANE_H_ */
