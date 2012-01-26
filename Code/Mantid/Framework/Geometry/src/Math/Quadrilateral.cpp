//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/Math/Quadrilateral.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
  namespace Geometry
  {
    using Kernel::V2D;

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    Quadrilateral::Quadrilateral(const V2D & lowerLeft, const V2D & lowerRight,
                                 const V2D & upperRight, const V2D & upperLeft) :
      ConvexPolygon(), m_lowerLeft(lowerLeft),
      m_lowerRight(lowerRight), m_upperRight(upperRight), m_upperLeft(upperLeft),
      m_lowestX(0.0), m_highestX(0.0), m_lowestY(0.0), m_highestY(0.0)
    {
      initialize();
    }

    /**
     *  Special constructor for a rectangle
     */
    Quadrilateral::Quadrilateral(const double lowerX, const double upperX, 
                                 const double lowerY, const double upperY)
      : ConvexPolygon(), m_lowerLeft(lowerX,lowerY),
        m_lowerRight(upperX,lowerY), m_upperRight(upperX,upperY), 
        m_upperLeft(lowerX,upperY),
        m_lowestX(0.0), m_highestX(0.0), m_lowestY(0.0), m_highestY(0.0)

    {
      initialize();
    }
    
    /**
     * Copy constructor
     * @param other :: The object to construct this from
     */
    Quadrilateral::Quadrilateral(const Quadrilateral & other)
      : ConvexPolygon(), m_lowerLeft(other.m_lowerLeft),
        m_lowerRight(other.m_lowerRight), m_upperRight(other.m_upperRight), 
        m_upperLeft(other.m_upperLeft)
    {
      initialize();
    }
    
    /// Destructor
    Quadrilateral::~Quadrilateral()
    {
      m_head = NULL; // Important as the base class assumes a heap allocated object
    }


    /** 
     * Return the vertex at the given index
     * @param index :: An index, starting at 0
     * @returns A reference to the polygon at that index
     * @throws Exception::IndexError if the index is out of range
     */
    const V2D& Quadrilateral::operator[](const size_t index) const
    {
      if( index < numVertices() ) 
      {
        switch(index)
        {
        case 0: return m_lowerLeft;
        case 1: return m_upperLeft;
        case 2: return m_upperRight;
        case 3: return m_lowerRight;
        }
      }
      throw Kernel::Exception::IndexError(index, numVertices(), "Quadrilateral::operator[]");
    }

    /**
     * Is a point inside this quadrilateral
     * @param point :: The point to test
     * @returns True if the point is inside the polygon
     */
    bool Quadrilateral::contains(const Kernel::V2D &) const
    {
      throw Kernel::Exception::NotImplementedError("ConvexPolygon::contains");
    }

    /**
     * Compute the area of the polygon using triangulation. As this is a 
     * convex polygon the calculation is exact. The algorithm uses one vertex
     * as a common vertex and sums the areas of the triangles formed by this
     * and two other vertices, moving in an anti-clockwise direction.
     * @returns The area of the polygon
     */
    double Quadrilateral::area() const
    {
      const double lhs = m_lowerLeft.Y()*m_upperLeft.X() + m_upperLeft.Y()*m_upperRight.X() + 
        m_upperRight.Y()*m_lowerRight.X() + m_lowerRight.Y()*m_lowerLeft.X();
      const double rhs = m_lowerLeft.X()*m_upperLeft.Y() + m_upperLeft.X()*m_upperRight.Y() + 
        m_upperRight.X()*m_lowerRight.Y() + m_lowerRight.X()*m_lowerLeft.Y();
      return 0.5*(lhs-rhs);
    }

    /**
     * Compute the determinant of the set of points as if they were contained in
     * an (N+1)x(N+1) matrix where N=number of vertices. Each row contains the 
     * [X,Y] values of the vertex padded with zeroes to the column length.
     * @returns The determinant of the set of points
     */
    double Quadrilateral::determinant() const
    {
      return 2.0*area();
    }

    /**
     * Return the lowest X value in the polygon
     * @returns A double indicating the smallest X value in the polygon
     */
    double Quadrilateral::smallestX() const
    {
      return m_lowestX;
    }

    /**
     * Return the largest X value in the polygon
     * @returns A double indicating the smallest X value in the polygon
     */
    double Quadrilateral::largestX() const
    {
      return m_highestX;
    }

    /**
     * Return the lowest X value in the polygon
     * @returns A double indicating the smallest Y value in the polygon
     */
    double Quadrilateral::smallestY() const
    {
      return m_lowestY;
    }

    /**
     * Return the largest Y value in the polygon
     * @returns A double indicating the smallest Y value in the polygon
     */
    double Quadrilateral::largestY() const
    {
      return m_highestY;
    }

    //-----------------------------------------------------------------------------
    // Private member functions
    //-----------------------------------------------------------------------------
    /**
     * Initalize the object
     */
    void Quadrilateral::initialize()
    {
      m_numVertices = 4;
      m_head = &(m_lowerLeft);
      m_head->insert(&m_lowerRight);
      m_head->insert(&m_upperRight);
      m_head->insert(&m_upperLeft);

      m_lowestX = m_lowerLeft.X();
      m_highestX = m_lowerLeft.X();
      m_lowestY = m_lowerLeft.Y();
      m_highestY = m_lowerLeft.Y();
      for( int i = 1; i < 4; ++i )
      {
        const V2D & pt = (*this)[i];
        if( pt.X() < m_lowestX ) m_lowestX = pt.X();
        else if( pt.X() > m_highestX ) m_highestX = pt.X();

        if( pt.Y() < m_lowestY ) m_lowestY = pt.Y();
        else if( pt.Y() > m_highestY ) m_highestY = pt.Y();
      }

    }


  } // namespace Mantid
} // namespace Geometry

