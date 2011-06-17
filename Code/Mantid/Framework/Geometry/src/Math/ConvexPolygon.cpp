//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/Math/ConvexPolygon.h"
#include "MantidKernel/Exception.h"
#include <sstream>

#include <iostream>

namespace Mantid
{
  namespace Geometry
  {

    //-----------------------------------------------------------------------------
    // Public functions
    //-----------------------------------------------------------------------------
    /**
     * Constructor with a collection of vertices
     * @param vertices :: The points forming the polygon, must be at least 3
     * @param check :: If true check the vertices conform to the requirements
     * of a convex polygon: see http://mathworld.wolfram.com/ConvexPolygon.html
     * @throws std::invalid_argument if the vertex list is does not meet
     * the convex polygon requirement
     */
    ConvexPolygon::ConvexPolygon(const Vertex2DList & vertices, const bool check)
      : m_vertices(vertices)
    {
      UNUSED_ARG(check);
      if( vertices.size() < 3 )
      {
        std::ostringstream os;
        os << "At least 3 vertices are required to build a convex polygon, only " 
           << vertices.size() << " supplied";
        throw std::invalid_argument(os.str());
      }
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
      const Vertex2D & vertex_0 = *itr;
      // Skip vertex 1 has we need a min of 3 points
      itr += 2;
      double area(0.0);
      while( itr != iend )
      {
        const Vertex2D & vertex_j = *itr;
        const Vertex2D & vertex_i = *(itr-1);
        area += triangleArea(vertex_0, vertex_i, vertex_j);
        ++itr;
      }
      return area;
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