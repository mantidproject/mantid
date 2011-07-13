//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/Math/Vertex2D.h"

namespace Mantid
{
  namespace Geometry
  {
    // Drag in V2D
    using Kernel::V2D;

    //-----------------------------------------------------------------------------
    // Public member functions
    //-----------------------------------------------------------------------------
    /** 
    * Constructor puts a point at the origin
    */
    Vertex2D::Vertex2D() 
      : Kernel::V2D()
    {
      initNeighbours();
    }

    /**
     * Constructor taking a point
     * @param point :: A point for the vertex
     */
    Vertex2D::Vertex2D(const Kernel::V2D & point) 
      : Kernel::V2D(point), m_next(NULL)
    {
      initNeighbours();
    }

    /**
     * Constructor taking X and Y values
     * @param x :: The value of X
     * @param y :: The value of Y
     */
    Vertex2D::Vertex2D(const double x, const double y) 
      : Kernel::V2D(x,y), m_next(NULL)
    {
      initNeighbours();
    }

    /**
     * Insert a vertex in at the next point in the chain
     * @param vertex :: A pointer to the vertex to insert
     * @returns A pointer to the inserted vertex
     */
    Vertex2D * Vertex2D::insert(Vertex2D *vertex)
    {
      Vertex2D *curNext = m_next;
      vertex->m_next = m_next;
      vertex->m_prev = this;
      m_next = vertex;
      curNext->m_prev = vertex;
      return vertex;
    }

    /**
     * Remove this node from the chain and return it
     * @returns This node
     */
      Vertex2D * Vertex2D::remove()
      {
        m_prev->m_next = m_next;
        m_next->m_prev = m_prev;
        //Isolate myself
        m_next = m_prev = this;
        return this;
      }

    //-----------------------------------------------------------------------------
    // Private member functions
    //-----------------------------------------------------------------------------
    /**
     * Initialize the next and previous pointers
     */
    void Vertex2D::initNeighbours()
    {
      m_next = this;
      m_prev = this;
    }

  } // namespace Mantid
} // namespace Geometry

