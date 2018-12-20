//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/Math/ConvexPolygon.h"
#include "MantidGeometry/Math/PolygonEdge.h"
#include "MantidKernel/Exception.h"

namespace Mantid {
namespace Geometry {

//-----------------------------------------------------------------------------
// Public member functions
//-----------------------------------------------------------------------------
/**
 * @param polygon A reference to the initializing polygon
 */
ConvexPolygon::Iterator::Iterator(const ConvexPolygon &polygon)
    : m_polygon(polygon), m_index(0) {
  if (!polygon.isValid()) {
    throw std::invalid_argument("Cannot create iterator for invalid polygon.");
  }
}

/**
 * @return Dereference the iterator and return the current value
 */
const Kernel::V2D &ConvexPolygon::Iterator::operator*() const {
  return m_polygon[m_index];
}

/**
 * Advance the iterator to the next point. When the iterator points to the
 * last point the next increment will take it back to the "first" point.
 */
void ConvexPolygon::Iterator::operator++() { m_index = nextIndex(); }

/**
 * @return A PolygonEdge defining the edge with the current point and the next
 */
PolygonEdge ConvexPolygon::Iterator::edge() const {
  return PolygonEdge(**this, m_polygon[nextIndex()]);
}

//-----------------------------------------------------------------------------
// Private member functions
//-----------------------------------------------------------------------------
/// @return The next index, taking into account cycling back after npoints()
size_t ConvexPolygon::Iterator::nextIndex() const {
  return (m_index + 1) % m_polygon.npoints();
}

} // namespace Geometry
} // namespace Mantid
