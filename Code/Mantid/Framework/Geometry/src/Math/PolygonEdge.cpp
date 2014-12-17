//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/Math/PolygonEdge.h"
#include "MantidKernel/FloatingPointComparison.h"
#include <limits>

namespace Mantid {
namespace Geometry {
using Kernel::V2D;

//-----------------------------------------------------------------------------
// Public methods
//-----------------------------------------------------------------------------
/**
 * Contructor taking two points, start and end
 */
PolygonEdge::PolygonEdge(const Kernel::V2D &start, const Kernel::V2D &end)
    : m_start(start), m_end(end) {}

/**
 * Create a point a given fraction along this edge
 * @param fraction :: The fraction of the current edge
 * @returns A point on the edge
 */
Kernel::V2D PolygonEdge::point(const double fraction) const {
  const V2D dir = (end() - start());
  return V2D(start() + dir * fraction);
}

//-------------------------------------------------------------------------
// Non-member functions
//-------------------------------------------------------------------------
/**
 * Classify a point with respect to an edge, i.e. left, right of edge etc.
 * @param pt :: A point as V2D
 * @param edge :: A test edge object
 * @returns The type of the point
 */
PointClassification classify(const V2D &pt, const PolygonEdge &edge) {
  V2D p2 = pt;
  V2D a = edge.end() - edge.start();
  V2D b = p2 - edge.start();
  double sa = a.X() * b.Y() - b.X() * a.Y();
  if (sa > 0.0) {
    return OnLeft;
  }
  if (sa < 0.0) {
    return OnRight;
  }
  if ((a.X() * b.X() < 0.0) || (a.Y() * b.Y() < 0.0)) {
    return Behind;
  }
  if (a.norm() < b.norm()) {
    return Beyond;
  }
  if (edge.start() == p2) {
    return Origin;
  }
  if (edge.end() == p2) {
    return Destination;
  }
  return Between;
}

/**
* Calculate the orientation type of this edge with another.
* @param focusEdge :: A reference to the PolygonEdge to test
* @param refEdge :: A reference to another PolygonEdge object to compare with
* @param t [Out] :: If an intersection is found this value is set to the ratio
* of the dot products between the normal to the other line
* @returns An enumeration denoting the orientation type
*/
PolygonEdge::Orientation orientation(const PolygonEdge &focusEdge,
                                     const PolygonEdge &refEdge, double &t) {
  V2D normalToRef((refEdge.end().Y() - refEdge.start().Y()),
                  (refEdge.start().X() - refEdge.end().X()));
  V2D focusDir = focusEdge.end() - focusEdge.start();
  double denom = normalToRef.scalar_prod(focusDir);
  if (Kernel::equals(denom, 0.0)) {
    PointClassification edgeClass = classify(focusEdge.start(), refEdge);
    if (edgeClass == OnLeft || edgeClass == OnRight) {
      return PolygonEdge::Parallel;
    } else {
      return PolygonEdge::Collinear;
    }
  }
  V2D startDir = focusEdge.start() - refEdge.start();
  double numer = normalToRef.scalar_prod(startDir);
  t = -numer / denom;
  return PolygonEdge::Skew;
}

/**
* Calculate the crossing point of this edge with another
* @param edgeOne :: The first polygon edge
* @param edgeTwo :: The second polygon edge
* @param crossPoint [Out] :: If found the point of intersection is filled here
*/
PolygonEdge::Orientation crossingPoint(const PolygonEdge &edgeOne,
                                       const PolygonEdge &edgeTwo,
                                       V2D &crossPoint) {
  using Kernel::ltEquals;
  using Kernel::gtEquals;

  double s(0.0);
  PolygonEdge::Orientation classe = orientation(edgeOne, edgeTwo, s);
  if (classe == PolygonEdge::Collinear || classe == PolygonEdge::Parallel) {
    return classe;
  }
  const double epsilon(std::numeric_limits<double>::epsilon());
  double lene = (edgeOne.end() - edgeOne.start()).norm();
  if ((s < -epsilon * lene) || (s > 1.0 + epsilon * lene)) {
    return PolygonEdge::SkewNoCross;
  }
  double t(0.0);
  orientation(edgeTwo, edgeOne, t);
  double lenf = (edgeTwo.start() - edgeTwo.end()).norm();
  if (ltEquals(-epsilon * lenf, t) && ltEquals(t, 1.0 + epsilon * lenf)) {
    if (ltEquals(t, epsilon * lenf)) {
      crossPoint = edgeTwo.start();
    } else if (gtEquals(t, 1.0 - epsilon * lenf)) {
      crossPoint = edgeTwo.end();
    } else if (ltEquals(s, epsilon * lene)) {
      crossPoint = edgeOne.start();
    } else if (gtEquals(s, 1.0 - epsilon * lene)) {
      crossPoint = edgeOne.end();
    } else {
      crossPoint = edgeTwo.point(t);
    }
    return PolygonEdge::SkewCross;
  } else {
    return PolygonEdge::SkewNoCross;
  }
}

/**
 * Return if the edges aim at each other
 * @param a :: First edge
 * @param b :: Second edge
 * @param aclass :: The point classification of a point on edge a
 * @param crossType :: The edge orientation classification
 */
bool edgeAimsAt(const PolygonEdge &a, const PolygonEdge &b,
                PointClassification aclass,
                PolygonEdge::Orientation crossType) {
  V2D va = a.direction();
  V2D vb = b.direction();
  if (crossType != PolygonEdge::Collinear) {
    double ca = va.X() * vb.Y();
    double cb = vb.X() * va.Y();
    if (Kernel::gtEquals(ca, cb)) {
      return (aclass != OnRight);
    } else {
      return (aclass != OnLeft);
    }
  } else {
    return (aclass != Beyond);
  }
}

} // namespace Geometry
} // namespace Mantid
