#ifndef MANTID_MDALGORITHMS_MDIMPLICITFUNCTION_H_
#define MANTID_MDALGORITHMS_MDIMPLICITFUNCTION_H_

#include "MantidGeometry/MDGeometry/MDPlane.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Exception.h"
#include <vector>
#include <string>

namespace Mantid {
namespace Geometry {

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

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

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
class DLLExport MDImplicitFunction {
public:
  MDImplicitFunction();
  MDImplicitFunction(const MDImplicitFunction &other);
  virtual ~MDImplicitFunction();

  void addPlane(const MDPlane &plane);

  /** @return the MDPlane contained
   * @param index :: which plane to return */
  const MDPlane &getPlane(size_t index) const { return m_planes[index]; }

  /// @return the number of dimensions for which this object can be applied
  size_t getNumDims() const { return m_nd; }

  /// @return the number of dimensions for which this object can be applied
  size_t getNumPlanes() const { return m_planes.size(); }

  /// @return the MDImplicitFunction type name.
  virtual std::string getName() const {
    throw std::runtime_error(
        "Cannot call MDImplicitFunction does not implement getName()");
  }

  /// @return the XML string.
  virtual std::string toXMLString() const {
    throw std::runtime_error(
        "Cannot call MDImplicitFunction does not implement toXMLString()");
  }

  //----------------------------------------------------------------------------------------------
  /** Enum for describing the contact between a box and an
   * implicit function.
   */
  enum eContact {
    /// No part of the box touches the implicit function
    NOT_TOUCHING = 0,
    /// Box is partly touching the implicit function region
    TOUCHING = 1,
    /// Box is fully contained by the implicit function
    CONTAINED = 2
  };

  // ==================== Methods that are inline for performance
  // ================================
  //----------------------------------------------------------------------------------------------
  /** Is a point in MDimensions contained by this ImplicitFunction?
   * If the point is bounded by ALL planes contained, then this
   * returns true.
   *
   * @param coords :: nd-sized array of coordinates
   * @return true if it is contained in the implicit function.
   */
  virtual bool isPointContained(const coord_t *coords) {
    for (size_t i = 0; i < m_numPlanes; i++) {
      if (!m_planes[i].isPointBounded(coords))
        return false;
    }
    return true;
  }

  //----------------------------------------------------------------------------------------------
  /** Is a point in MDimensions contained by this ImplicitFunction?
   * If the point is bounded by ALL planes contained, then this
   * returns true.
   *
   * @param coords :: nd-sized array of coordinates
   * @return true if it is contained in the implicit function.
   */
  virtual bool isPointContained(const Mantid::Kernel::VMD &coords) {
    for (size_t i = 0; i < m_numPlanes; i++) {
      if (!m_planes[i].isPointBounded(coords))
        return false;
    }
    return true;
  }

  //----------------------------------------------------------------------------------------------
  /** Is a point in MDimensions contained by this ImplicitFunction?
   * If the point is bounded by ALL planes contained, then this
   * returns true.
   *
   * @param coords :: nd-sized vector of coordinates. No size-check is made!
   * @return true if it is contained in the implicit function.
   */
  virtual bool isPointContained(const std::vector<coord_t> &coords) {
    for (size_t i = 0; i < m_numPlanes; i++) {
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
   * does not actually touch the volume (false positives) but these should be
   *pretty
   * rare.
   *
   *
   * @param vertexes :: vector of n-dimensional coordinate vertexes.
   *        NOTE: no size check is done! Each vertex must be length m_nd!
   *
   * @return true if there is a chance of the box touching. Note that the
   *    algorithm does not guarantee that it touches, but it should never
   *    return false if the box does touch.
   */
  bool isBoxTouching(const std::vector<std::vector<coord_t>> &vertexes) {
    size_t numPoints = vertexes.size();

    // As the description states, the first plane with NO points inside it
    // means the box does NOT touch. So iterate by planes
    for (size_t i = 0; i < m_numPlanes; i++) {
      size_t numBounded = 0;
      for (size_t j = 0; j < numPoints; j++) {
        if (m_planes[i].isPointBounded(vertexes[j])) {
          numBounded++;
          // No need to evaluate any more points.
          break;
        }
      }
      // Not a single point is in this plane
      if (numBounded == 0)
        // That means the box CANNOT touch the implicit function
        return false;
    }

    return true;
  }

  //----------------------------------------------------------------------------------------------
  /** Same as isBoxTouching(vector), except that
   * it takes a bare array of coordinates. This is for max. performance.
   *
   * The array is to be filled with numPoints sets of
   * coordinates, each of m_nd in length.
   *
   * @param vertexes :: bare array of length numPoints * m_nd
   * @param numPoints :: number of vertexes in the array.
   * @return true if there is a chance of the box touching.
   */
  bool isBoxTouching(const coord_t *vertexes, const size_t numPoints) {
    // As the description states, the first plane with NO points inside it
    // means the box does NOT touch. So iterate by planes
    for (size_t i = 0; i < m_numPlanes; i++) {
      size_t numBounded = 0;
      for (size_t j = 0; j < numPoints; j++) {
        if (m_planes[i].isPointBounded(vertexes + j * m_nd)) {
          numBounded++;
          // No need to evaluate any more points.
          break;
        }
      }
      // Not a single point is in this plane
      if (numBounded == 0)
        // That means the box CANNOT touch the implicit function
        return false;
    }
    return true;
  }

  //----------------------------------------------------------------------------------------------
  /** Determine how a box (consisting of a number of vertexes)
   * is in contact with the implicit function.
   *
   * Returns:
   *  NOT_TOUCHING :  if any of the planes has no vertex in it.
   *  CONTAINED :     if all of the vertexes are in all of the planes.
   *  TOUCHING :      if there is a chance of the box touching the volume.
   *                  (there can sometimes be false positives)
   *
   * @param vertexes :: bare array of length numPoints * m_nd
   * @param numPoints :: number of vertexes in the array.
   * @return eContact enum value
   */
  eContact boxContact(const coord_t *vertexes, const size_t numPoints) const {
    // For speed, we can stop looking when we know the box CANNOT be fully
    // contained.
    bool lookForFullyContained = true;

    // As the description states, the first plane with NO points inside it
    // means the box does NOT touch. So iterate by planes
    for (size_t i = 0; i < m_numPlanes; i++) {
      size_t numBounded = 0;
      for (size_t j = 0; j < numPoints; j++) {
        if (m_planes[i].isPointBounded(vertexes + j * m_nd)) {
          numBounded++;
          // No need to evaluate any more points, unless we look for fully
          // contained
          if (!lookForFullyContained)
            break;
        } else
          // One of the vertexes is not contained by one of the planes.
          // This means that the box CANNOT be fully contained.
          lookForFullyContained = false;
      }
      // Not a single point is in this plane
      if (numBounded == 0)
        // That means the box CANNOT touch the implicit function
        return NOT_TOUCHING;
      // If all points were within this plane, then there is still a chance that
      // the box is fully contained
      if (numBounded != numPoints)
        lookForFullyContained = false;
    }
    // If nothing said that the box might not be fully contained, then it is
    // fully contained!
    if (lookForFullyContained)
      return CONTAINED;
    else
      return TOUCHING;
  }

protected:
  /// number of dimensions for which this object can be applied
  size_t m_nd;

  /// Vector of all the planes applying for this implict function
  std::vector<MDPlane> m_planes;

  /// Cached number of planes (for a sligh speed-up)
  size_t m_numPlanes;
};

typedef boost::shared_ptr<MDImplicitFunction> MDImplicitFunction_sptr;

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_MDIMPLICITFUNCTION_H_ */
