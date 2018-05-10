//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/Math/ConvexPolygon.h"
#include "MantidGeometry/Math/PolygonEdge.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/V2D.h"
#include <algorithm>
#include <cfloat>
#include <sstream>

namespace Mantid {
namespace Geometry {

using Kernel::V2D;

//-----------------------------------------------------------------------------
// Public member functions
//-----------------------------------------------------------------------------
/**
 * Constructs a 'null' polygon with no points
 */
ConvexPolygon::ConvexPolygon()
    : m_minX(DBL_MAX), m_maxX(-DBL_MAX), m_minY(DBL_MAX), m_maxY(-DBL_MAX),
      m_vertices() {}

/**
 * @param vertices A list of points that form the polygon
 */
ConvexPolygon::ConvexPolygon(const Vertices &vertices) : m_vertices(vertices) {
  setup();
}

/// @return True if polygon has 3 or more points
bool ConvexPolygon::isValid() const { return (npoints() > 2); }

/// Clears all points
void ConvexPolygon::clear() {
  m_vertices.clear();
  m_minX = DBL_MAX;
  m_maxX = -DBL_MAX;
  m_minY = DBL_MAX;
  m_maxY = -DBL_MAX;
}

/**
 * Insert a new vertex.
 * @param pt A new point for the shape
 */
void ConvexPolygon::insert(const V2D &pt) {
  m_vertices.push_back(pt);
  // Update extrema
  m_minX = std::min(m_minX, pt.X());
  m_maxX = std::max(m_maxX, pt.X());
  m_minY = std::min(m_minY, pt.Y());
  m_maxY = std::max(m_maxY, pt.Y());
}

/**
 * @param x X coordinate
 * @param y Y coordinate
 */
void ConvexPolygon::insert(double x, double y) { this->insert(V2D(x, y)); }

/**
 * Return the vertex at the given index. The index is assumed to be valid. See
 * at() for a
 * bounds-checking version. Out-of-bounds access is undefined
 * @param index :: An index, starting at 0
 * @returns A reference to the polygon at that index
 * @throws Exception::IndexError if the index is out of range
 */
const V2D &ConvexPolygon::operator[](const size_t index) const {
  return m_vertices[index];
}

/**
 * Return the vertex at the given index. The index is checked for validity
 * @param index :: An index, starting at 0
 * @returns A reference to the polygon at that index
 * @throws Exception::IndexError if the index is out of range
 */
const Kernel::V2D &ConvexPolygon::at(const size_t index) const {
  if (index < npoints()) {
    return m_vertices[index];
  } else {
    throw Kernel::Exception::IndexError(index, npoints(),
                                        "ConvexPolygon::at()");
  }
}

/// @return the number of vertices
size_t ConvexPolygon::npoints() const { return m_vertices.size(); }

/**
 * Is a point inside this polygon
 * @param point :: The point to test
 * @returns True if the point is inside the polygon
 */
bool ConvexPolygon::contains(const Kernel::V2D &point) const {
  for (size_t i = 0; i < npoints(); ++i) {
    PolygonEdge edge(m_vertices[i], m_vertices[(i + 1) % npoints()]);
    if (classify(point, edge) == OnLeft) {
      return false;
    }
  }
  return true;
}

///
/**
 *  Is the given polygon completely encosed by this one
 * @param poly Another polygon
 * @return True if the given polygon is enclosed by this, false otherwise
 */
bool ConvexPolygon::contains(const ConvexPolygon &poly) const {
  // Basically just have to test if each point is inside us, this could be
  // slow
  for (size_t i = 0; i < poly.npoints(); ++i) {
    if (!this->contains(poly[i]))
      return false;
  }
  return true;
}

/**
 * Compute the area of the polygon using triangulation. As this is a
 * convex polygon the calculation is exact. The algorithm uses one vertex
 * as a common vertex and sums the areas of the triangles formed by this
 * and two other vertices, moving in an anti-clockwise direction.
 * @returns The area of the polygon
 */
double ConvexPolygon::area() const { return 0.5 * this->determinant(); }

/**
 * Compute the determinant of the set of points as if they were contained in
 * an (N+1)x(N+1) matrix where N=number of vertices. Each row contains the
 * [X,Y] values of the vertex padded with zeroes to the column length.
 * @returns The determinant of the set of points
 */
double ConvexPolygon::determinant() const {
  // Arrange the points in a Nx2 matrix where N = npoints+1
  // and each row is a vertex point with the last row equal to the first.
  // Calculate the "determinant". The matrix class needs and NxN matrix
  // as the correct definition of a determinant only exists for
  // square matrices. We could fool it by putting extra zeroes but this
  // would increase the workload for no gain

  // Loop over all and compute the individual elements. We assume
  // that calling next() on the vertex takes us clockwise within
  // the polygon.
  double lhs(0.0), rhs(0.0);
  for (size_t i = 0; i < npoints(); ++i) {
    const V2D &v_i = m_vertices[i];
    const V2D &v_ip1 = m_vertices[(i + 1) % npoints()];

    lhs += v_ip1.X() * v_i.Y();
    rhs += v_i.X() * v_ip1.Y();
  }
  return lhs - rhs;
}

/**
 * Return the lowest X value in the polygon
 * @returns A double indicating the smallest X value in the polygon
 */
double ConvexPolygon::minX() const { return m_minX; }

/**
 * Return the largest X value in the polygon
 * @returns A double indicating the smallest X value in the polygon
 */
double ConvexPolygon::maxX() const { return m_maxX; }

/**
 * Return the lowest X value in the polygon
 * @returns A double indicating the smallest Y value in the polygon
 */
double ConvexPolygon::minY() const { return m_minY; }

/**
 * Return the largest Y value in the polygon
 * @returns A double indicating the smallest Y value in the polygon
 */
double ConvexPolygon::maxY() const { return m_maxY; }

/**
 * @return A copy of the current polygon
 */
ConvexPolygon ConvexPolygon::toPoly() const { return *this; }

/**
 * Setup the meta-data: no of vertices, high/low points
 */
void ConvexPolygon::setup() {
  m_minX = DBL_MAX;
  m_maxX = -DBL_MAX;
  m_minY = DBL_MAX;
  m_maxY = -DBL_MAX;

  for (const auto &vertex : m_vertices) {
    double x{vertex.X()}, y{vertex.Y()};
    m_minX = std::min(m_minX, x);
    m_maxX = std::max(m_maxX, x);
    m_minY = std::min(m_minY, y);
    m_maxY = std::max(m_maxY, y);
  }
}

/**
 * Compute the area of a triangle given by 3 points (a,b,c) using the
 * convention in "Computational Geometry in C" by J. O'Rourke
 * @param a :: The first vertex in the set
 * @param b :: The second vertex in the set
 * @param c :: The third vertex in the set
 */
double ConvexPolygon::triangleArea(const V2D &a, const V2D &b,
                                   const V2D &c) const {
  return 0.5 * (b.X() - a.X()) * (c.Y() - a.Y()) -
         (c.X() - a.X()) * (b.Y() - a.Y());
}

//-----------------------------------------------------------------------------
// Non-member non-friend functions
//-----------------------------------------------------------------------------
/**
 * Print a polygon to a stream. The vertices are output in the order defined by
 * the object
 * @param os :: A reference to an output stream
 * @param polygon :: A reference to the polygon to output to the stream
 * @returns A reference to the input stream
 */
std::ostream &operator<<(std::ostream &os, const ConvexPolygon &polygon) {
  os << "ConvexPolygon(";
  const size_t npoints(polygon.npoints());
  for (size_t i = 0; i < npoints; ++i) {
    os << polygon[i];
    if (i < npoints - 1)
      os << ",";
  }
  os << ")";
  return os;
}

} // namespace Geometry
} // namespace Mantid
