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
#include <array>

namespace Mantid {
namespace Geometry {

/**
 *
 * A ConvexPolygon with only 4 vertices. Better performance as no dynamic
 * allocation
 */
class DLLExport Quadrilateral final : public ConvexPolygon {
public:
  /// Constructor with the four vertices
  Quadrilateral(const Kernel::V2D &lowerLeft, const Kernel::V2D &lowerRight,
                const Kernel::V2D &upperRight, const Kernel::V2D &upperLeft);
  /// Special constructor for a rectangle
  Quadrilateral(const double lowerX, const double upperX, const double lowerY,
                const double upperY);
  Quadrilateral(Quadrilateral &&) = default;
  Quadrilateral &operator=(Quadrilateral &&) = default;
  Quadrilateral(const Quadrilateral &) = default;
  Quadrilateral &operator=(const Quadrilateral &) = default;

  /// Index access.
  inline const Kernel::V2D &operator[](const size_t index) const override {
    return m_vertices[index];
  };
  /// Bounds-checked index access
  const Kernel::V2D &at(const size_t index) const override;
  /// Return the number of vertices
  inline size_t npoints() const override { return 4; }
  /// Is a point inside this polygon
  bool contains(const Kernel::V2D &point) const override;
  /// Is a the given polygon completely encosed by this one
  bool contains(const ConvexPolygon &poly) const override;
  /**
   * Compute the area of the polygon using triangulation. As this is a
   * convex polygon the calculation is exact. The algorithm uses one vertex
   * as a common vertex and sums the areas of the triangles formed by this
   * and two other vertices, moving in an anti-clockwise direction.
   * @returns The area of the polygon
   */
  inline double area() const override {
    const double lhs = lowerLeft().Y() * upperLeft().X() +
                       upperLeft().Y() * upperRight().X() +
                       upperRight().Y() * lowerRight().X() +
                       lowerRight().Y() * lowerLeft().X();
    const double rhs = lowerLeft().X() * upperLeft().Y() +
                       upperLeft().X() * upperRight().Y() +
                       upperRight().X() * lowerRight().Y() +
                       lowerRight().X() * lowerLeft().Y();
    return 0.5 * (lhs - rhs);
  }

  /**
   * Compute the determinant of the set of points as if they were contained in
   * an (N+1)x(N+1) matrix where N=number of vertices. Each row contains the
   * [X,Y] values of the vertex padded with zeroes to the column length.
   * @returns The determinant of the set of points
   */
  inline double determinant() const override { return 2.0 * area(); }
  /// Return the lowest X value in the polygon
  inline double minX() const override {
    return std::min(lowerLeft().X(), upperLeft().X());
  }
  /// Return the max X value in the polygon
  inline double maxX() const override {
    return std::max(lowerRight().X(), upperRight().X());
  }
  /// Return the lowest Y value in the polygon
  inline double minY() const override {
    return std::min(lowerLeft().Y(), lowerRight().Y());
  }
  /// Return the max Y value in the polygon
  inline double maxY() const override {
    return std::max(upperLeft().Y(), upperRight().Y());
  }
  /// Return a new Polygon based on the current Quadrilateral
  ConvexPolygon toPoly() const override;
  /// Shifts the vertexes in a clockwise manner
  virtual void shiftVertexesClockwise();

private:
  inline const Kernel::V2D &lowerLeft() const { return m_vertices[0]; }
  inline const Kernel::V2D &upperLeft() const { return m_vertices[1]; }
  inline const Kernel::V2D &upperRight() const { return m_vertices[2]; }
  inline const Kernel::V2D &lowerRight() const { return m_vertices[3]; }

  // Order=ll,ul,ur,lr
  std::array<Kernel::V2D, 4> m_vertices;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_QUADRILATERAL_H_ */
