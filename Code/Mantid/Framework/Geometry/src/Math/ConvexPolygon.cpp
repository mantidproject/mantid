//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/Math/ConvexPolygon.h"
#include <sstream>

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

  } //namespace Geometry
} //namespace Mantid