#ifndef MANTID_GEOMETRY_QUADRILATERAL_H_
#define MANTID_GEOMETRY_QUADRILATERAL_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/Math/ConvexPolygon.h"
#include "MantidKernel/V2D.h"

namespace Mantid {
namespace Geometry {

/** Quadrilateral

    A ConvexPolygon with only 4 vertices. Better performance as no dynamic
   allocation

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
class DLLExport Quadrilateral : public ConvexPolygon {
public:
  /// Constructor with the four vertices
  Quadrilateral(const Kernel::V2D &lowerLeft, const Kernel::V2D &lowerRight,
                const Kernel::V2D &upperRight, const Kernel::V2D &upperLeft);
  /// Special constructor for a rectangle
  Quadrilateral(const double lowerX, const double upperX, const double lowerY,
                const double upperY);
  /// Copy constructor
  Quadrilateral(const Quadrilateral &other);
  /// Copy-assignment operator
  Quadrilateral &operator=(const Quadrilateral &rhs);

  /// Index access.
  virtual const Kernel::V2D &operator[](const size_t index) const;
  /// Bounds-checked index access
  virtual const Kernel::V2D &at(const size_t index) const;
  /// Return the number of vertices
  virtual size_t npoints() const { return 4; }
  /// Is a point inside this polygon
  virtual bool contains(const Kernel::V2D &point) const;
  /// Is a the given polygon completely encosed by this one
  virtual bool contains(const ConvexPolygon &poly) const;
  /// Compute the area of the polygon using triangulation
  virtual double area() const;
  /// Compute the 'determinant' of the points
  virtual double determinant() const;
  /// Return the lowest X value in the polygon
  virtual double minX() const;
  /// Return the max X value in the polygon
  virtual double maxX() const;
  /// Return the lowest Y value in the polygon
  virtual double minY() const;
  /// Return the max Y value in the polygon
  virtual double maxY() const;
  /// Return a new Polygon based on the current Quadrilateral
  virtual ConvexPolygon toPoly() const;

private:
  /// Lower left
  Kernel::V2D m_lowerLeft;
  /// Lower right
  Kernel::V2D m_lowerRight;
  /// Upper right
  Kernel::V2D m_upperRight;
  /// Upper left
  Kernel::V2D m_upperLeft;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_QUADRILATERAL_H_ */
