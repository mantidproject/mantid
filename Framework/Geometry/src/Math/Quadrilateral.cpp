// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/Math/Quadrilateral.h"
#include "MantidKernel/Exception.h"
#include <algorithm>

namespace Mantid::Geometry {
using Kernel::V2D;

/** Constructor
 */
Quadrilateral::Quadrilateral(const V2D &lowerLeft, const V2D &lowerRight, const V2D &upperRight, const V2D &upperLeft)
    : ConvexPolygon(), m_vertices{{lowerLeft, upperLeft, upperRight, lowerRight}} {}

/**
 *  Special constructor for a rectangle
 */
Quadrilateral::Quadrilateral(const double lowerX, const double upperX, const double lowerY, const double upperY)
    : ConvexPolygon(),
      m_vertices{{V2D(lowerX, lowerY), V2D(lowerX, upperY), V2D(upperX, upperY), V2D(upperX, lowerY)}} {}

/**
 * Return the vertex at the given index
 * @param index :: An index, starting at 0
 * @returns A reference to the polygon at that index
 * @throws Exception::IndexError if the index is out of range
 */
const Kernel::V2D &Quadrilateral::at(const size_t index) const {
  if (index > 3)
    throw Kernel::Exception::IndexError(index, npoints(), "Quadrilateral::at()");
  return (*this)[index];
}

/**
 * @return True if the point is inside the polygon or on the edge
 */
bool Quadrilateral::contains(const Kernel::V2D &point) const {
  ConvexPolygon quadAsPoly = this->toPoly();
  return quadAsPoly.contains(point);
}

/**
 * @return True if the given polygon is completely encosed by this one
 */
bool Quadrilateral::contains(const ConvexPolygon &poly) const {
  ConvexPolygon quadAsPoly = this->toPoly();
  return quadAsPoly.contains(poly);
}

/// @return A new polygon based on the current Quadrilateral
ConvexPolygon Quadrilateral::toPoly() const {
  ConvexPolygon::Vertices points(4);
  points[0] = lowerLeft();
  points[1] = upperLeft();
  points[2] = upperRight();
  points[3] = lowerRight();
  return ConvexPolygon(points);
}

/// Shifts the vertexes in a clockwise manner
void Quadrilateral::shiftVertexesClockwise() {
  V2D temp = m_vertices[0];
  m_vertices[0] = m_vertices[1];
  m_vertices[1] = m_vertices[2];
  m_vertices[2] = m_vertices[3];
  m_vertices[3] = temp;
}

} // namespace Mantid::Geometry
