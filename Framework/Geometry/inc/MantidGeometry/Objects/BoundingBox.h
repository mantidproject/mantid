#ifndef MANTIDGEOMETRY_BOUNDINGBOX_H_
#define MANTIDGEOMETRY_BOUNDINGBOX_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Tolerance.h"
#include "MantidKernel/V3D.h"
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif
#include <sstream>

namespace Mantid {
namespace Geometry {

//-------------------------------------------------------------------------
// Forward declarations
//-------------------------------------------------------------------------
class Track;

/**
A simple structure that defines an axis-aligned cuboid shaped bounding box for a
geometrical object.
It is a thin structure containing the 6 points that define the corners of the
cuboid.

@author Martyn Gigg
@date 01/10/2010

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_GEOMETRY_DLL BoundingBox {
public:
  /// Default constructor constructs a zero-sized box
  BoundingBox()
      : m_minPoint(), m_maxPoint(), m_null(true), is_axis_aligned(true) {}

  /** Constructor taking six points. If inconsistent points are defined, i.e.
  * xmin > xmax, then an error is thrown
  * @param xmax :: Value of maximum in X. It must be greater than xmin.
  * @param ymax :: Value of maximum in Y. It must be greater than ymin.
  * @param zmax :: Value of maximum in Z. It must be greater than zmin.
  * @param xmin :: Value of minimum in X. It must be less than xmax.
  * @param ymin :: Value of minimum in Y. It must be less than ymax.
  * @param zmin :: Value of minimum in Z. It must be less than zmax.
  */
  BoundingBox(double xmax, double ymax, double zmax, double xmin, double ymin,
              double zmin)
      : m_minPoint(xmin, ymin, zmin), m_maxPoint(xmax, ymax, zmax),
        m_null(false), is_axis_aligned(true) {
    // Sanity check
    checkValid(xmax, ymax, zmax, xmin, ymin, zmin);
  }

  /**
  * Do the given arguments form a valid bounding box, throws std::invalid
  * argument if not
  * @param xmax :: Value of maximum in X. It must be greater than xmin.
  * @param ymax :: Value of maximum in Y. It must be greater than ymin.
  * @param zmax :: Value of maximum in Z. It must be greater than zmin.
  * @param xmin :: Value of minimum in X. It must be less than xmax.
  * @param ymin :: Value of minimum in Y. It must be less than ymax.
  * @param zmin :: Value of minimum in Z. It must be less than zmax.
  */
  static void checkValid(double xmax, double ymax, double zmax, double xmin,
                         double ymin, double zmin) {
    if (xmax < xmin || ymax < ymin || zmax < zmin) {
      std::ostringstream error;
      error << "Error creating bounding box, inconsistent values given:\n"
            << "\txmin=" << xmin << ", xmax=" << xmax << "\n"
            << "\tymin=" << ymin << ", ymax=" << ymax << "\n"
            << "\tzmin=" << zmin << ", zmax=" << zmax << "\n";
      throw std::invalid_argument(error.str());
    }
  }

  /** @name Point access */
  //@{
  /// Return the minimum value of X
  inline const double &xMin() const { return m_minPoint.X(); }
  /// Return the maximum value of X
  inline const double &xMax() const { return m_maxPoint.X(); }
  /// Return the minimum value of Y
  inline const double &yMin() const { return m_minPoint.Y(); }
  /// Return the maximum value of Y
  inline const double &yMax() const { return m_maxPoint.Y(); }
  /// Return the minimum value of Z
  inline const double &zMin() const { return m_minPoint.Z(); }
  /// Return the maximum value of Z
  inline const double &zMax() const { return m_maxPoint.Z(); }
  /// Returns the min point of the box
  inline const Kernel::V3D &minPoint() const { return m_minPoint; }
  /// Returns the min point of the box
  inline const Kernel::V3D &maxPoint() const { return m_maxPoint; }
  /// Returns the centre of the bounding box
  inline Kernel::V3D centrePoint() const {
    return Kernel::V3D(0.5 * (xMax() + xMin()), 0.5 * (yMax() + yMin()),
                       0.5 * (zMax() + zMin()));
  }
  /// Returns the width of the box
  inline Kernel::V3D width() const {
    return Kernel::V3D(m_maxPoint - m_minPoint);
  }
  //@}

  /** @name Querying */
  //@{
  /// Is this a default constructed box?
  inline bool isNull() const { return m_null; }
  /// Is the box considered valid. Convenience for !isNull()
  inline bool isNonNull() const { return !m_null; }
  /// Is the given point within the bounding box?
  bool isPointInside(const Kernel::V3D &point) const;
  /// Does a specified track intersect the bounding box
  bool doesLineIntersect(const Track &track) const;
  /// Does a line intersect the bounding box
  bool doesLineIntersect(const Kernel::V3D &startPoint,
                         const Kernel::V3D &lineDir) const;
  /// Calculate the angular half width from the given point
  double angularWidth(const Kernel::V3D &observer) const;
  /// Check if it is normal axis aligned bounding box or not.
  inline bool isAxisAligned() const { return is_axis_aligned; }
  /// returns the coordinate system to which BB is alighned to;
  std::vector<Kernel::V3D> const &getCoordSystem() const {
    return coord_system;
  }

  //@}

  /** returns the expanded box consisting of all 8 box points,
    * shifted into the coordinate system with the observer centre; */
  void getFullBox(std::vector<Kernel::V3D> &box,
                  const Kernel::V3D &observer) const;
  /** @name Box mutation functions*/
  //@{
  /// Return the minimum value of X (non-const)
  inline double &xMin() {
    m_null = false;
    return m_minPoint[0];
  }
  /// Return the maximum value of X  (non-const)
  inline double &xMax() {
    m_null = false;
    return m_maxPoint[0];
  }
  /// Return the minimum value of Y  (non-const)
  inline double &yMin() {
    m_null = false;
    return m_minPoint[1];
  }
  /// Return the maximum value of Y  (non-const)
  inline double &yMax() {
    m_null = false;
    return m_maxPoint[1];
  }
  /// Return the minimum value of Z  (non-const)
  inline double &zMin() {
    m_null = false;
    return m_minPoint[2];
  }
  /// Return the maximum value of Z  (non-const)
  inline double &zMax() {
    m_null = false;
    return m_maxPoint[2];
  }
  /// Grow the bounding box so that it also encompasses the given box
  void grow(const BoundingBox &other);
  /// change the BB alighnment, providing new coordinate system to alighn it to.
  void setBoxAlignment(const Kernel::V3D &R0,
                       const std::vector<Kernel::V3D> &orts);
  /// set BB in to undefined state with min=FLT_MAX>max=-FLT_MAX
  void nullify();
  /// reallign the BB according to new coordinate system, provided earlier or
  /// specified as parameter;
  void realign(std::vector<Kernel::V3D> const *const pCS = NULL);
  /// move the BB by a vector
  void moveBy(const Kernel::V3D &v) {
    m_minPoint += v;
    m_maxPoint += v;
  }
  //@}

private:
  /// The minimum point of the axis-aligned box
  Kernel::V3D m_minPoint;
  /// The maximum point of the axis-aligned box
  Kernel::V3D m_maxPoint;
  /// Flag marking if we've been initialized using the default constructor,
  /// with values or default values and user-set points
  bool m_null;
  /// the parameter which describe if the bounding box is axis aligned or not
  bool is_axis_aligned;
  /** if the bounding box is not axis aligned, the vector below describes the
   coordinate system,
   to which the bounding box is alighned to. The vector has 4 members, with
   first describing
   new coordinate system center and three others -- orts of this system */
  std::vector<Kernel::V3D> coord_system;
};

/// A shared pointer to a BoundingBox
typedef boost::shared_ptr<BoundingBox> BoundingBox_sptr;
/// A shared pointer to a const BoundingBox
typedef boost::shared_ptr<const BoundingBox> BoundingBox_const_sptr;

/// Print out the bounding box values to a stream.
std::ostream &operator<<(std::ostream &os, const BoundingBox &box);
}
}

#endif // MANTIDGEOMETRY_BOUNDINGBOX_H_
