//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/Math/ConvexPolygon.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Exception.h"
#include <sstream>
#include <iostream>

namespace Mantid
{
  namespace Geometry
  {

    using Kernel::V2D;

    //-----------------------------------------------------------------------------
    // Public functions
    //-----------------------------------------------------------------------------
    /**
     * Constructor with a collection of vertices
     * @param vertices :: The points forming the polygon, must be at least 3
     * @param fullCheck :: If true check the vertices conform to the requirements
     * of a convex polygon: see http://mathworld.wolfram.com/ConvexPolygon.html, otherwise
     * just check the number is greater than 2
     * @throws std::invalid_argument if the vertex list is does not meet
     * the convex polygon requirement
     */
    ConvexPolygon::ConvexPolygon(const Vertex2DList & vertices)
      : m_vertices(vertices)
    {
      validate();
    }

    /**
     * Compute the area of the polygon using triangulation. As this is a 
     * convex polygon the calculation is exact. The algorithm uses one vertex
     * as a common vertex and sums the areas of the triangles formed by this
     * and two other vertices, moving in an anti-clockwise direction.
     * @returns The area of the polygon
     */
    double ConvexPolygon::area() const
    {
      Vertex2DList::const_iterator itr = m_vertices.begin();
      Vertex2DList::const_iterator iend = m_vertices.end();
      const Vertex2D & first = *itr;
      const Vertex2D & last = *(iend-1);
      // Arrange the points in a NX2 matrix where N = numVertices+1
      // and each row is a vertex point with the last row equal to the first.
      // Calculate the "determinant". The matrix class needs and NXN matrix
      // as the correct definition of a determinant only exists for
      // square matrices. We could fool it by putting extra zeroes but this
      // would increase the workload for no gain
      double lhs(0.0), rhs(0.0);
      for( ; itr != iend-1; ++itr )
      {
        const Vertex2D & i = *itr;
        const Vertex2D & j = *(itr+1);
        lhs += i.X()*j.Y();
        rhs += j.X()*i.Y();
      }
      // Now the ends we've missed by exiting the loop early
      lhs += last.X()*first.Y();
      rhs += first.X()*last.Y();
      return 0.5*(lhs - rhs);
    }

    /**
     * Check this is a valid convex polygon
     * @throws std::invalid_argument if it is not
     */
    void ConvexPolygon::validate() const
    {
      if( m_vertices.size() < 3 ) 
      {
        std::ostringstream os;
        os << "Expected greater than 2 vertices when constructing a convex polygon, found "
           << m_vertices.size();
        throw std::invalid_argument(os.str());
      }
    }

    /**
     * Compute the area of a triangle given by 3 vertices (a,b,c) using the
     * convention in "Computational Geometry in C" by J. O'Rourke
     * @param a :: The first vertex in the set
     * @param b :: The second vertex in the set
     * @param c :: The third vertex in the set
     */
    double ConvexPolygon::triangleArea(const Vertex2D & a, 
                                       const Vertex2D & b, 
                                       const Vertex2D & c) const
    {
      return 0.5*(b.X() - a.X())*(c.Y() - a.Y()) - 
        (c.X() - a.X())*(b.Y() - a.Y());
    }

  } //namespace Geometry
} //namespace Mantid
