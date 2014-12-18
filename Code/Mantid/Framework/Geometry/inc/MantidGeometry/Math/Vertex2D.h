#ifndef MANTID_GEOMETRY_VERTEX2D_H_
#define MANTID_GEOMETRY_VERTEX2D_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Math/PolygonEdge.h"
#include "MantidKernel/V2D.h"

namespace Mantid {
namespace Geometry {

/**
Implements a vertex in two-dimensional space

@author Martyn Gigg
@date 2011-07-12

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
class MANTID_GEOMETRY_DLL Vertex2D : public Kernel::V2D {
public:
  /// Helper to delete a chain of vertices
  static void deleteChain(Vertex2D *startNode);

  /// Default constructor (a point at the origin)
  Vertex2D();
  /// Constructor with X and Y values
  Vertex2D(const double x, const double y);
  /// Constructor with a point
  Vertex2D(const Kernel::V2D &point);
  /// Copy constructor
  Vertex2D(const Vertex2D &other);
  /// Copy-assignment operator
  Vertex2D &operator=(const Vertex2D &rhs);

  /// Return the vertex as a point
  inline const Kernel::V2D &point() const { return *this; }
  /// Insert a vertex so that it is next
  Vertex2D *insert(Vertex2D *vertex);
  /// Remove this node from the chain
  Vertex2D *remove();

  /**
   * Returns the next in the chain (non-const version)
   * @returns The next vertex
   */
  inline Vertex2D *next() { return m_next; }
  /**
    * Returns the previous in the chain (non-const version)
    * @returns The previous vertex
    */
  inline Vertex2D *previous() { return m_prev; }

  /**
   * Returns the next in the chain (const version)
   * @returns The next vertex
   */
  inline const Vertex2D *next() const { return m_next; }
  /**
    * Returns the previous in the chain (const version)
    * @returns The previous vertex
    */
  inline const Vertex2D *previous() const { return m_prev; }

private:
  /// Initialize the neighbour pointers
  void initNeighbours();

  /// Pointer to the "next" in the chain
  Vertex2D *m_next;
  /// Pointer to the "previous" in the chain
  Vertex2D *m_prev;
};

/**
 * A small iterator type structure
 */
class Vertex2DIterator {
public:
  /// Constructor
  Vertex2DIterator(const Vertex2D *start) : m_vertex(start) {}
  /// Advance the iterator
  void advance() { m_vertex = m_vertex->next(); }
  /// Get the point
  const Kernel::V2D &point() { return m_vertex->point(); }
  /// Get an edge between this and the next
  PolygonEdge edge() { return PolygonEdge(*m_vertex, *(m_vertex->next())); }

private:
  /// A pointer to the current vertex
  Vertex2D const *m_vertex;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_VERTEX2D_H_ */
