//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/Math/ORourkeIntersection.h"
#include "MantidKernel/V2D.h"
#include "MantidKernel/Exception.h"
#include <iostream>

namespace Mantid {
namespace Geometry {
using Kernel::V2D;

namespace {
//---------------------------------------------------------
// Utility function declarations
//---------------------------------------------------------
///@cond
enum InFlag { Pin, Qin, Unknown };
/// Calculate the intersection type for the line segments
unsigned int intersection(const V2D &a, const V2D &b, const V2D &c,
                          const V2D &d, Kernel::V2D &crossPoint);
/// Calculate intersection type for parallel lines
unsigned int parallelIntersect(const V2D &a, const V2D &b, const V2D &c,
                               const V2D &d, Kernel::V2D &crossPoint);
/// Are the 3 points collinear
bool collinear(const V2D &a, const V2D &b, const V2D &c);
/// Is point C between point A and point B
bool isBetween(const V2D &a, const V2D &b, const V2D &c);
/// Advance the vertex index
size_t advanceVertex(const size_t vi, size_t &vertex_count, const size_t nverts,
                     const bool inside, const V2D &currentVertex,
                     Vertex2DList &intersections);
///@endcond
}

/**
* Compute the polygon that defines the intersection between two
* other concex polygons using the method of chasing edges
* @param P :: A reference to the first polygon
* @param Q :: A reference to the second polygon
* @returns A new polygon defining the region of intersection
*/
ConvexPolygon intersectionByORourke(const ConvexPolygon &P,
                                    const ConvexPolygon &Q) {
  const size_t nverts_p(P.numVertices()), nverts_q(Q.numVertices());
  const V2D origin(0.0, 0.0);

  size_t count_p(0), count_q(0); // Number of vertices visited
  unsigned int inflag(Unknown);
  bool firstPoint(true);
  // Avoid vertex list reallocations
  Vertex2DList originAB(3);
  originAB[0] = origin;
  Vertex2DList aHB(3);
  Vertex2DList bHA(3);

  // Final list
  Vertex2DList intersectList;
  do {
    size_t pi(0), pim1(0), qi(0), qim1(0);

    // Compute a vector between the previous point in the direction of the next
    pim1 = (pi + nverts_p - 1) % nverts_p;
    qim1 = (qi + nverts_q - 1) % nverts_q;
    V2D edge_p = P[pi] - P[pim1];
    V2D edge_q = Q[qi] - Q[qim1];

    // Orientations
    originAB[1] = edge_p;
    originAB[2] = edge_q;
    int cross = ConvexPolygon(originAB).orientation();
    aHB[0] = Q[qim1];
    aHB[1] = Q[qi];
    aHB[2] = P[pi];
    int aHB_dir = ConvexPolygon(aHB).orientation();
    bHA[0] = P[pim1];
    bHA[1] = P[pi];
    bHA[2] = Q[qi];
    int bHA_dir = ConvexPolygon(bHA).orientation();
    // Test for line intersection
    V2D intersect;
    unsigned int type = intersection(P[pim1], P[pi], Q[qim1], Q[qi], intersect);
    if (type == 1 || type == 2) {
      if (inflag == Unknown && firstPoint) {
        count_p = count_q = 0;
        firstPoint = false;
      }
      if (aHB_dir > 0)
        inflag = Pin;
      else if (bHA_dir > 0)
        inflag = Qin;
      else {
      };
      intersectList.insert(intersect);
    }

    // Deal with advance of indices
    /* Special case: A & B overlap and oppositely oriented. */
    if (type == 3 && edge_p.scalar_prod(edge_q) < 0.0) {
      throw std::runtime_error("Single segment intersection");
    }
    /* Special case: A & B parallel and separated. */
    else if (cross == 0 && (aHB_dir < 0) && (bHA_dir < 0)) {
      throw std::runtime_error(
          "AxB=0 and both are left-hand oriented so no intersection");
    }
    /* Special case: A & B collinear. */
    else if (cross == 0 && (aHB_dir == 0) && (bHA_dir == 0)) {
      /* Advance but do not output point. */
      if (inflag == Pin)
        qi = advanceVertex(qi, count_q, nverts_q, false, Q[qi], intersectList);
      else
        pi = advanceVertex(pi, count_p, nverts_p, false, P[pi], intersectList);
    }
    /* Generic cases. */
    else if (cross >= 0) {
      if (bHA_dir > 0)
        pi = advanceVertex(pi, count_p, nverts_p, inflag == Pin, P[pi],
                           intersectList);
      else
        qi = advanceVertex(qi, count_q, nverts_q, inflag == Qin, Q[qi],
                           intersectList);
    } else /* if ( cross < 0 ) */
    {
      if (aHB_dir > 0)
        qi = advanceVertex(qi, count_q, nverts_q, inflag == Qin, Q[qi],
                           intersectList);
      else
        pi = advanceVertex(pi, count_p, nverts_p, inflag == Pin, P[pi],
                           intersectList);
    }
  } while ((count_p < nverts_p || count_q < nverts_q) &&
           (count_p < 2 * nverts_p) && (count_q < 2 * nverts_q));

  if (intersectList.size() < 3) {
    throw std::runtime_error(
        "Intersection points do not form a bounded polygon.");
  }

  return ConvexPolygon(intersectList);
}

//--------------------------------------------------------------------------
// Utility functions for this algorithm
//--------------------------------------------------------------------------
namespace {
/**
* Calculate the point of intersection for the given line segments, ab-cd
* @param a :: Starting point of the first segment
* @param b :: Starting point of the first segment
* @param c :: Starting point of the second segment
* @param d :: Starting point of the second segment
* @param crossPoint :: Output the crossing point
* @returns An flag marking the type of intersection
* see http://mathworld.wolfram.com/Line-LineIntersection.html
*/
unsigned int intersection(const V2D &a, const V2D &b, const V2D &c,
                          const V2D &d, V2D &crossPoint) {
  double denominator = (a[0] * (d[1] - c[1]) + b[0] * (c[1] - d[1]) +
                        d[0] * (b[1] - a[1]) + c[0] * (a[1] - b[1]));
  // Denominator=0.0, parallel lines
  if (denominator == 0.0) {
    return parallelIntersect(a, b, c, d, crossPoint);
  }
  double numerator =
      (a[0] * (d[1] - c[1]) + c[0] * (a[1] - d[1]) + d[0] * (c[1] - a[1]));
  unsigned int code(0);
  if (numerator == 0.0 || numerator == denominator)
    code = 2;
  double s = numerator / denominator;
  numerator =
      -(a[0] * (c[1] - b[1]) + b[0] * (a[1] - c[1]) + c[0] * (b[1] - a[1]));
  if (numerator == 0.0 || numerator == denominator)
    code = 2;
  double t = numerator / denominator;
  if ((s > 0.0) && (s < 1.0) && (t > 0.0) && (t < 1.0)) {
    code = 1;
  } else if ((s < 0.0) || (s > 1.0) || (t < 0.0) || (t > 1.0)) {
    code = 0;
  } else {
  }
  crossPoint = V2D(a[0] + s * (b[0] - a[0]), a[1] + s * (b[1] - a[1]));
  return code;
}

/**
* Calculate the point of intersection for the given parallel line segments,
* ab-cd
* @param a :: Starting point of the first segment
* @param b :: Starting point of the first segment
* @param c :: Starting point of the second segment
* @param d :: Starting point of the second segment
* @param crossPoint :: Output the crossing point
* @returns An flag marking the type of intersection
* see http://mathworld.wolfram.com/Line-LineIntersection.html
*/
unsigned int parallelIntersect(const V2D &a, const V2D &b, const V2D &c,
                               const V2D &d, V2D &crossPoint) {
  unsigned int type(0);
  if (!collinear(a, b, c))
    type = 0;
  else {
    type = 3;
    if (isBetween(a, b, c) && isBetween(a, b, d)) {
      crossPoint = c;
    } else if (isBetween(c, d, a) && isBetween(c, d, b)) {
      crossPoint = a;
    } else if (isBetween(a, b, c) && isBetween(c, d, b)) {
      crossPoint = c;
    } else if (isBetween(a, b, c) && isBetween(c, d, a)) {
      crossPoint = c;
    } else if (isBetween(a, b, d) && isBetween(c, d, b)) {
      crossPoint = d;
    } else if (isBetween(a, b, d) && isBetween(c, d, a)) {
      crossPoint = d;
    } else
      type = 0;
  }
  return type;
}

/**
* Are the 3 points collinear
* @param a :: Point A
* @param b :: Point B
* @param c :: Point C
*/
bool collinear(const V2D &a, const V2D &b, const V2D &c) {
  Vertex2DList vertices(3);
  vertices[0] = a;
  vertices[1] = b;
  vertices[2] = c;
  return (ConvexPolygon(vertices).determinant() == 0.0);
}

/// Is point C between point A and point B
bool isBetween(const V2D &a, const V2D &b, const V2D &c) {
  if (a[0] != b[0])
    return ((a[0] <= c[0]) && (c[0] <= b[0])) ||
           ((a[0] >= c[0]) && (c[0] >= b[0]));
  else
    return ((a[1] <= c[1]) && (c[1] <= b[1])) ||
           ((a[1] >= c[1]) && (c[1] >= b[1]));
}

/**
* Advance the given vertex index while keeping a count of the number used
* @param vi :: Index to be cycled
* @param vertex_count :: Keep track of the total number visited
* @param nverts :: The total number of vertices
*/
size_t advanceVertex(const size_t vi, size_t &vertex_count, const size_t nverts,
                     const bool inside, const V2D &currentVertex,
                     Vertex2DList &intersections) {
  if (inside) {
    intersections.insert(currentVertex);
  }
  ++vertex_count;
  return (vi + 1) % nverts;
}
}

} // namespace Geometry
} // namespace Mantid
