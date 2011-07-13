//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/Math/PolygonIntersection.h"
#include "MantidGeometry/Math/ORourkeIntersection.h"
//#include "MantidGeometry/Math/LaszloIntersection.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
  namespace Geometry
  {
    /**
     * Compute the intersection of two polygons via the chosen method
     * @param p :: The first polygon
     * @param q :: The second polygon
     * @param method ::  The chosen method (Default = Laszlo)
     * @returns The overlap of the two polygons as a ConvexPolygon object
     */
    ConvexPolygon intersection(const ConvexPolygon &p, const ConvexPolygon &q, 
      PolygonIntersection::Method method)
    {
      switch(method)
      {
      //case PolygonIntersection::Laszlo: return intersectionByLaszlo(p,q);
      case PolygonIntersection::ORourke: return intersectionByORourke(p,q);
      default: throw std::invalid_argument("Unknown type of polygon intersection algorithm requested.");
      };
    }
    

  } //namespace Geometry
} //namespace Mantid
