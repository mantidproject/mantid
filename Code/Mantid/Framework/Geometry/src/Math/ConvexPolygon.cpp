//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/Math/ConvexPolygon.h"
#include "MantidGeometry/Math/Vertex2D.h"
#include "MantidKernel/Exception.h"
#include <sstream>
#include <iostream>

namespace Mantid {
namespace Geometry {

using Kernel::V2D;

//-----------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------
/**
 * Constructor with a head vertex.
 * @param head :: A pointer the head vertex
 * @throws std::invalid_argument If the vertex list is invalid
 */
ConvexPolygon::ConvexPolygon(Vertex2D *head) {
  validate(head);
  m_head = head;
  setup();
}

/**
 * Construct a rectange
 * @param x_lower :: Lower x coordinate
 * @param x_upper :: Upper x coordinate
 * @param y_lower :: Lower y coordinate
 * @param y_upper :: Upper y coordinate
 */
ConvexPolygon::ConvexPolygon(const double x_lower, const double x_upper,
                             const double y_lower, const double y_upper)
    : m_numVertices(4), m_head(new Vertex2D(x_lower, y_lower)) {
  // Iterating from the head now produces a clockwise pattern
  m_head->insert(new Vertex2D(x_upper, y_lower)); // Bottom right
  m_head->insert(new Vertex2D(x_upper, y_upper)); // Top right
  m_head->insert(new Vertex2D(x_lower, y_upper)); // Top left

  m_lowestX = x_lower;
  m_highestX = x_upper;
  m_lowestY = y_lower;
  m_highestY = y_upper;
}

/**
 * Copy constructor
 * @param rhs :: The object to copy from
 */
ConvexPolygon::ConvexPolygon(const ConvexPolygon &rhs) {
  if (this != &rhs) {
    m_head = new Vertex2D(rhs.m_head->X(), rhs.m_head->Y());
    /// Iterating next() around the other polygon produces the points in
    /// clockwise order
    /// so we need to go backwards here to ensure that they remain in the same
    /// order
    Vertex2D *rhsVertex = rhs.m_head->previous();
    // Count the vertices
    while (rhsVertex != rhs.m_head) {
      m_head->insert(new Vertex2D(rhsVertex->X(), rhsVertex->Y()));
      rhsVertex = rhsVertex->previous();
    }
    setup();
  }
}

/**
 * Destructor
 */
ConvexPolygon::~ConvexPolygon() {
  if (m_head)
    Vertex2D::deleteChain(m_head);
}

/**
 * Return the vertex at the given index
 * @param index :: An index, starting at 0
 * @returns A reference to the polygon at that index
 * @throws Exception::IndexError if the index is out of range
 */
const V2D &ConvexPolygon::operator[](const size_t index) const {
  if (index < numVertices()) {
    size_t count(0);
    Vertex2D *p = m_head;
    while (count != index) {
      ++count;
      p = p->next();
    }
    return static_cast<const V2D &>(*p);
  }
  throw Kernel::Exception::IndexError(index, numVertices(),
                                      "ConvexPolygon::operator[]");
}

/**
 * Is a point inside this polygon
 * @param point :: The point to test
 * @returns True if the point is inside the polygon
 */
bool ConvexPolygon::contains(const Kernel::V2D &point) const {
  Vertex2D *v = m_head;
  do {
    PolygonEdge edge(v->point(), v->next()->point());
    if (classify(point, edge) == OnLeft)
      return false;
    v = v->next();
  } while (v != m_head);

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
  const Vertex2D *current = poly.head();
  for (size_t i = 0; i < poly.numVertices(); ++i) {
    if (!this->contains(*current))
      return false;
    current = current->next();
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
  // Arrange the points in a NX2 matrix where N = numVertices+1
  // and each row is a vertex point with the last row equal to the first.
  // Calculate the "determinant". The matrix class needs and NXN matrix
  // as the correct definition of a determinant only exists for
  // square matrices. We could fool it by putting extra zeroes but this
  // would increase the workload for no gain

  // Loop over all and compute the individual elements. We assume
  // that calling next() on the vertex takes us clockwise within
  // the polygon.
  double lhs(0.0), rhs(0.0);
  Vertex2D *v_i = m_head;
  Vertex2D *v_ip1 = v_i->next();
  do {
    lhs += v_ip1->X() * v_i->Y();
    rhs += v_i->X() * v_ip1->Y();
    v_i = v_i->next();
    v_ip1 = v_i->next();
  } while (v_i != m_head);
  return lhs - rhs;
}

/**
 * Return the lowest X value in the polygon
 * @returns A double indicating the smallest X value in the polygon
 */
double ConvexPolygon::smallestX() const { return m_lowestX; }

/**
 * Return the largest X value in the polygon
 * @returns A double indicating the smallest X value in the polygon
 */
double ConvexPolygon::largestX() const { return m_highestX; }

/**
 * Return the lowest X value in the polygon
 * @returns A double indicating the smallest Y value in the polygon
 */
double ConvexPolygon::smallestY() const { return m_lowestY; }

/**
 * Return the largest Y value in the polygon
 * @returns A double indicating the smallest Y value in the polygon
 */
double ConvexPolygon::largestY() const { return m_highestY; }

/**
 * Setup the meta-data: no of vertices, high/low points
 */
void ConvexPolygon::setup() {
  m_numVertices = 0;

  m_lowestX = m_head->X();
  m_highestX = m_head->X();
  m_lowestY = m_head->Y();
  m_highestY = m_head->Y();

  // Count the vertices
  Vertex2D *current = m_head;
  do {
    ++m_numVertices;
    if (current->X() < m_lowestX)
      m_lowestX = current->X();
    else if (current->X() > m_highestX)
      m_highestX = current->X();

    if (current->Y() < m_lowestY)
      m_lowestY = current->Y();
    else if (current->Y() > m_highestY)
      m_highestY = current->Y();
    current = current->next();
  } while (current != m_head);
}

/**
 * Check this is a valid polygon
 * @param head :: A pointer to the head vertex
 * @throws std::invalid_argument if it is not
 */
void ConvexPolygon::validate(const Vertex2D *head) const {
  if (!head) {
    throw std::invalid_argument("ConvexPolygon::validate - NULL pointer is an "
                                "invalid head for a convex polygon");
  }
  // Must have at least two neighbours
  if (head->next() == head->previous()) {
    std::ostringstream os;
    os << "ConvexPolygon::validate - Expected 3 or more vertices when "
          "constructing a convex polygon, found ";
    if (head->next() == head)
      os << "1";
    else
      os << "2";
    throw std::invalid_argument(os.str());
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
  const size_t numVertices(polygon.numVertices());
  Vertex2DIterator pIter(polygon.head());
  for (size_t i = 0; i < numVertices; ++i) {
    os << pIter.point();
    if (i < numVertices - 1)
      os << ",";
    pIter.advance();
  }
  os << ")";
  return os;
}

} // namespace Geometry
} // namespace Mantid
