//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/Math/ConvexPolygon.h"
#include "MantidGeometry/Math/Vertex2D.h"
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
     * Constructor with a head vertex. It must be linked to at least 2 others to be considered valid
     * @param head :: A reference to the head vertex
     * @throws std::invalid_argument If the vertex list is invalid
     */
    ConvexPolygon::ConvexPolygon(Vertex2D & head)
    {
      validate(head);
      m_currentVertex = m_head = &head;
      m_numVertices = 1;
      Vertex2D *nextVertex = m_currentVertex->next();
      while( nextVertex != m_head)
      {
        ++m_numVertices;
        nextVertex = nextVertex->next();
      }
    }

    /**
     * Constructor with a collection of vertices
     * @param points :: The points forming the polygon, must be at least 3
     * @throws std::invalid_argument if the vertex list is does not meet
     * the convex polygon requirement
     */
    ConvexPolygon::ConvexPolygon(const Vertex2DList & points)
      : m_currentVertex(NULL), m_numVertices(0)
    {
      validate(points);
      m_numVertices = points.size();
      m_currentVertex = m_head = new Vertex2D(points[0]);
      for( size_t i = 1; i < m_numVertices; ++i )
      {
        m_currentVertex = m_currentVertex->insert(new Vertex2D(points[i]));
      }
      // Point it to the first one
      m_currentVertex = m_currentVertex->next();
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
      : m_currentVertex(new Vertex2D(x_lower, y_lower)), m_numVertices(4), m_head(m_currentVertex)
    {
      m_currentVertex = m_currentVertex->insert(new Vertex2D(x_upper, y_lower)); // Bottom right
      m_currentVertex = m_currentVertex->insert(new Vertex2D(x_upper, y_upper)); // Top right
      m_currentVertex = m_currentVertex->insert(new Vertex2D(x_lower, y_upper)); // Top left
      // Point it to the first one
      m_currentVertex = m_currentVertex->next();
    }

    /**
     * Copy constructor
     * @param rhs :: The object to copy from
     */
    ConvexPolygon::ConvexPolygon(const ConvexPolygon & rhs)
    {
      if( this != &rhs )
      {
        m_numVertices = rhs.m_numVertices;
        Vertex2D *rhsVertex = rhs.m_currentVertex;
        m_currentVertex = new Vertex2D(rhsVertex->X(), rhsVertex->Y());
        for(int i = 1; i < m_numVertices; ++i)
        {
          rhsVertex = rhsVertex->next();
          m_currentVertex = m_currentVertex->insert(new Vertex2D(rhsVertex->X(), rhsVertex->Y()));
          if( rhsVertex == rhs.m_head )
          {
            m_head = m_currentVertex;
          }
        }
        m_currentVertex = m_currentVertex->next();
      }
    }

    /**
     * Destructor
     */
    ConvexPolygon::~ConvexPolygon()
    {
      // Ensure we delete the vertices
      if( m_currentVertex )
      {
        Vertex2D *nextVertex = m_currentVertex->next();
        while(m_currentVertex != nextVertex)
        {
          delete nextVertex->remove();
          nextVertex = m_currentVertex->next();
        }
        delete m_currentVertex;
      }
    }

    /**
     * Returns the current point
     */
    const Kernel::V2D & ConvexPolygon::point() const
    {
      return *m_currentVertex;
    }

    /**
     * Returns an edge on the polygon between the current vertex and the next
     * @returns A PolygonEdge object
     */
    PolygonEdge ConvexPolygon::edge() const
    {
      return PolygonEdge(*m_currentVertex, *(m_currentVertex->next()));
    }
     
    /**
     * Return the vertex at the given index
     * @param index :: An index, starting at 0
     * @returns A reference to the polygon at that index
     * @throws Exception::IndexError if the index is out of range
     */
    const V2D& ConvexPolygon::operator[](const size_t index) const
    {
      if( index < numVertices() ) 
      {
        size_t count(0);
        Vertex2D *p = m_head;
        while( count != index )
        {
          ++count;
          p = p->next();
        }
        return static_cast<const V2D&>(*p);
      }
      throw Kernel::Exception::IndexError(index, numVertices(), "ConvexPolygon::operator[]");
    }

    /**
     * Advance the current vertex to the next in the chain and return it
     * @returns The next point in the chain
     */
    const V2D & ConvexPolygon::advance()
    {
      m_currentVertex = m_currentVertex->next();
      return *m_currentVertex;
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

      // Loop over all and compute the individual elements
      // The ends are handled by the vertex pointer linkage
      double lhs(0.0), rhs(0.0);
      const Vertex2D *start = m_currentVertex;
      Vertex2D *v_i = m_currentVertex;
      Vertex2D *v_ip1 = v_i->next();
      do
      {
        lhs += v_i->X()*v_ip1->Y();
        rhs += v_ip1->X()*v_i->Y();
        v_i = v_i->next();
        v_ip1 = v_i->next();
      }
      while( v_i != start );
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
     * @param points :: Validate a list of points
     * @throws std::invalid_argument if it is not
     */
    void ConvexPolygon::validate(const Vertex2DList & points) const
    {
      if( points.size() < 3 ) 
      {
        std::ostringstream os;
        os << "Expected greater than 2 vertices when constructing a convex polygon, found "
           << points.size();
        throw std::invalid_argument(os.str());
      }
    }

    /**
     * Check this is a valid polygon
     * @param head :: A pointer to the head vertex
     * @throws std::invalid_argument if it is not
     */
    void ConvexPolygon::validate(const Vertex2D & head) const
    {
      // Must have at least two neighbours
      if( head.next() == head.previous() )
      {
        std::ostringstream os;
        os << "Expected 3 or more vertices when constructing a convex polygon, found ";
        if( head.next() == &head ) os << "1";
        else os << "2";
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
    std::ostream & operator<<(std::ostream & os, const ConvexPolygon & polygon)
    {
      os << "ConvexPolygon(";
      const size_t nverts(polygon.numVertices());
      for( size_t i = 0; i < nverts; ++i )
      {
        os << polygon[i];
        if( i < nverts - 1 ) os << ",";
      }
      os << ")";
      return os;
    }

  } //namespace Geometry
} //namespace Mantid
