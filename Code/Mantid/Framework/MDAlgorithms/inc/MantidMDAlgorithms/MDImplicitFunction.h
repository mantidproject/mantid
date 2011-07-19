#ifndef MANTID_MDALGORITHMS_MDIMPLICITFUNCTION_H_
#define MANTID_MDALGORITHMS_MDIMPLICITFUNCTION_H_
    
#include "MantidAPI/ImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/System.h"
#include "MantidMDAlgorithms/MDPlane.h"
#include <vector>

namespace Mantid
{
namespace MDAlgorithms
{

  /** An "ImplicitFunction" defining a hyper-cuboid-shaped region in N dimensions.
   * This is to be used in various MD rebinning algorithms to determine
   * e.g, which boxes should be considered to be within the integration volume.
   *
   * This general case would cover boxes that are not aligned with the axes.
   *
   * Various shapes can be built by intersecting 1 or more planes.
   * The Plane, and whether a point is bounded by it, will be the basis
   * of determining whether a point is in a volume.
   *
   * For example, in a 3D space:
   *
   * 1 plane = a half-infinite volume
   * 2 parallel planes = a plane with a thickness
   * 4 aligned planes = an infinite line, rectangular in cross-section
   * 6 planes = a cuboid
   *
   * For most efficiency, each MDImplicitFunction should be built with
   * a given set of dimensions in mind; that is, if it is to be applied on
   * a MDEventWorkspace with say 6 dimensions: X, Y, Z, time, temperature, field;
   * then a mask that only looks at the relevant 3 dimensions is used.
    
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
  class DLLExport MDImplicitFunction // : public Mantid::API::ImplicitFunction
  {
  public:
    MDImplicitFunction();
    ~MDImplicitFunction();

    void addPlane(const MDPlane & plane);

    /// @return the number of dimensions for which this object can be applied
    size_t getNumDims() const { return m_nd; }



    // ==================== Methods that are inline for performance ================================
    //----------------------------------------------------------------------------------------------
    /** Is a point in MDimensions contained by this ImplicitFunction?
     * If the point is bounded by ALL planes contained, then this
     * returns true.
     *
     * @param coords :: nd-sized array of coordinates
     * @return true if it is contained in the implicit function.
     */
    bool isPointContained(const coord_t * coords)
    {
      for (size_t i=0; i<m_numPlanes; i++)
      {
        if (!m_planes[i].isPointBounded(coords))
          return false;
      }
      return true;
    }


    //----------------------------------------------------------------------------------------------
    /** Is there a chance that the box defined by these vertexes touches
     * the implicit function volume?
     *
     * The algorithm operates by the idea that if any point in a volume
     * is contained in the volume, then that means that at least one
     * of the vertexes is within EACH one of the planes that define the
     * volume.
     *
     * That means that if you find a plane for which NO vertex is contained,
     * then the box defined by these vertexes CANNOT touch any part of the
     * volume so it is safe to ignore. (I don't have a rigorous proof for
     * this but it looks right :)
     *
     * There are situations where the condition can be satisfied but the box
     * does not actually touch the volume (false positives) but these should be pretty
     * rare.
     *
     *
     * @param vertexes :: vector of n-dimensional coordinate vertexes.
     * @return true if there is a chance of the box touching. Note that the
     *    algorithm does not guarantee that it touches, but it should never
     *    return false if the box does touch.
     */
    bool doesBoxTouch(const std::vector<std::vector<coord_t> > & vertexes)
    {
      //TODO: Everything
      UNUSED_ARG(vertexes)
      return true;
    }


  protected:
    /// number of dimensions for which this object can be applied
    size_t m_nd;

    /// Vector of all the planes applying for this implict function
    std::vector<MDPlane> m_planes;

    /// Cached number of planes (for a sligh speed-up)
    size_t m_numPlanes;

  };


} // namespace MDAlgorithms
} // namespace Mantid

#endif  /* MANTID_MDALGORITHMS_MDIMPLICITFUNCTION_H_ */
