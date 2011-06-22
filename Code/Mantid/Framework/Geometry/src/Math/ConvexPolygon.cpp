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
     * Return the vertex at the given index
     * @param index :: An index, starting at 0
     * @returns A reference to the polygon at that index
     * @throws Exception::IndexError if the index is out of range
     */
    const V2D& ConvexPolygon::operator[](const size_t index) const
    {
      if( index < m_vertices.size() ) return m_vertices[index];
      throw Kernel::Exception::IndexError(index, m_vertices.size(), "ConvexPolygon::operator[]");
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
      return 0.5*this->determinant();
    }

    /**
     * Compute the determinant of the set of points as if they were contained in
     * an (N+1)x(N+1) matrix where N=number of vertices. Each row contains the 
     * [X,Y] values of the vertex padded with zeroes to the column length.
     * @returns The determinant of the set of points
     */
    double ConvexPolygon::determinant() const
    {
      // Arrange the points in a NX2 matrix where N = numVertices+1
      // and each row is a vertex point with the last row equal to the first.
      // Calculate the "determinant". The matrix class needs and NXN matrix
      // as the correct definition of a determinant only exists for
      // square matrices. We could fool it by putting extra zeroes but this
      // would increase the workload for no gain
      const size_t nend(m_vertices.size()-1);
      double lhs(0.0), rhs(0.0);
      for(size_t index = 0; index < nend; ++index)
      {
        const V2D & v_i = m_vertices[index];
        const V2D & v_ip1 = m_vertices[index+1];
        lhs += v_i.X()*v_ip1.Y();
        rhs += v_ip1.X()*v_i.Y();
      }
      // Now the ends we've missed by exiting the loop early
      const V2D & first = m_vertices.front();
      const V2D & last = m_vertices.back();
      lhs += last.X()*first.Y();
      rhs += first.X()*last.Y();
      return lhs - rhs;
    }
    
    /**
     * Compute the 'orientation' of the polygon. The possible values are:
     *    1: 2*area > 0.5
     *   -1: 2*area < -0.5 else
     *    0.
     * @returns An integer representing the orientation
     */
    int ConvexPolygon::orientation() const
    {
      const double determinant = this->determinant();
      if( determinant > 0.5 ) return 1;
      else if( determinant < -0.5 ) return -1;
      else return 0;
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
     * Compute the area of a triangle given by 3 points (a,b,c) using the
     * convention in "Computational Geometry in C" by J. O'Rourke
     * @param a :: The first vertex in the set
     * @param b :: The second vertex in the set
     * @param c :: The third vertex in the set
     */
    double ConvexPolygon::triangleArea(const V2D & a, const V2D & b, 
                                       const V2D & c) const
    {
      return 0.5*(b.X() - a.X())*(c.Y() - a.Y()) - 
        (c.X() - a.X())*(b.Y() - a.Y());
    }

  } //namespace Geometry
} //namespace Mantid
