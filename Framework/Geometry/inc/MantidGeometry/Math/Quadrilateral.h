// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
*/
class DLLExport Quadrilateral : public ConvexPolygon {
public:
  /// Constructor with the four vertices
  Quadrilateral(const Kernel::V2D &lowerLeft, const Kernel::V2D &lowerRight,
                const Kernel::V2D &upperRight, const Kernel::V2D &upperLeft);
  /// Special constructor for a rectangle
  Quadrilateral(const double lowerX, const double upperX, const double lowerY,
                const double upperY);
  /// Index access.
  const Kernel::V2D &operator[](const size_t index) const override;
  /// Bounds-checked index access
  const Kernel::V2D &at(const size_t index) const override;
  /// Return the number of vertices
  size_t npoints() const override { return 4; }
  /// Is a point inside this polygon
  bool contains(const Kernel::V2D &point) const override;
  /// Is a the given polygon completely encosed by this one
  bool contains(const ConvexPolygon &poly) const override;
  /// Compute the area of the polygon using triangulation
  double area() const override;
  /// Compute the 'determinant' of the points
  double determinant() const override;
  /// Return the lowest X value in the polygon
  double minX() const override;
  /// Return the max X value in the polygon
  double maxX() const override;
  /// Return the lowest Y value in the polygon
  double minY() const override;
  /// Return the max Y value in the polygon
  double maxY() const override;
  /// Return a new Polygon based on the current Quadrilateral
  ConvexPolygon toPoly() const override;
  /// Shifts the vertexes in a clockwise manner
  virtual void shiftVertexesClockwise();

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
