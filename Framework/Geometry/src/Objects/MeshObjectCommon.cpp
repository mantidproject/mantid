#include "MantidGeometry/Objects/MeshObjectCommon.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include <limits>
#include <string>

namespace Mantid {
namespace Geometry {

namespace MeshObjectCommon {
/**
 * getVertices converts vector Kernel::V3D to vector doubles. 3x size of input.
 * ordered x,y,z,x,y,z...
 * @param vertices : input vector of Kernel::V3D
 * @return: vector of doubles. Elements of input.
 */
std::vector<double> getVertices(const std::vector<Kernel::V3D> &vertices) {
  std::vector<double> points;
  size_t nPoints = vertices.size();
  if (nPoints > 0) {
    points.resize(static_cast<std::size_t>(nPoints) * 3);
    for (size_t i = 0; i < nPoints; ++i) {
      const auto &pnt = vertices[i];
      points[i * 3] = pnt.X();
      points[i * 3 + 1] = pnt.Y();
      points[i * 3 + 2] = pnt.Z();
    }
  }
  return points;
}
/**
 * Find the solid angle of a triangle defined by vectors a,b,c from point
 *"observer"
 *
 * formula (Oosterom) O=2atan([a,b,c]/(abc+(a.b)c+(a.c)b+(b.c)a))
 *
 * @param a :: first point of triangle
 * @param b :: second point of triangle
 * @param c :: third point of triangle
 * @param observer :: point from which solid angle is required
 * @return :: solid angle of triangle in Steradians.
 *
 * This duplicates code in CSGOjbect both need a place to be merged.
 * To aid this, this function has been defined as a non-member.
 */
double getTriangleSolidAngle(const Kernel::V3D &a, const Kernel::V3D &b,
                             const Kernel::V3D &c,
                             const Kernel::V3D &observer) {
  const Kernel::V3D ao = a - observer;
  const Kernel::V3D bo = b - observer;
  const Kernel::V3D co = c - observer;
  const double modao = ao.norm();
  const double modbo = bo.norm();
  const double modco = co.norm();
  const double aobo = ao.scalar_prod(bo);
  const double aoco = ao.scalar_prod(co);
  const double boco = bo.scalar_prod(co);
  const double scalTripProd = ao.scalar_prod(bo.cross_prod(co));
  const double denom =
      modao * modbo * modco + modco * aobo + modbo * aoco + modao * boco;
  if (denom != 0.0)
    return 2.0 * atan2(scalTripProd, denom);
  else
    return 0.0; // not certain this is correct
}

/**
 * @brief isOnTriangle
 * @param point : point to test
 * @param v1 : first vertex of triangle
 * @param v2 : second vertex of triangle
 * @param v3 : thrid vertex of triangle
 * @return True only point if is on triangle
 */
bool isOnTriangle(const Kernel::V3D &point, const Kernel::V3D &v1,
                  const Kernel::V3D &v2, const Kernel::V3D &v3) {

  // p = w*p0 + u*p1 + v*p2, where numbered p refers to vertices of triangle
  // w+u+v == 1, so w = 1-u-v
  // p = (1-u-v)p0 + u*p1 + v*p2, rearranging ...
  // p = u(p1 - p0) + v(p2 - p0) + p0
  // in change of basis, barycentric coordinates p = p0 + u*e0 + v*e1. e0 and
  // e1 are basis vectors. e2 = (p - p0)
  // rewrite as e2 = u*e0 + v*e1
  // i) e2.e0 = u*e0.e0 + v*e1.e0
  // ii) e2.e1 = u*e0.e1 + v*e1.e1
  // solve for u, v and check u and v >= 0 and u+v <=1

  auto e0 = v3 - v1;
  auto e1 = v2 - v1;
  auto e2 = point - v1;

  // Compute dot products
  auto dot00 = e0.scalar_prod(e0);
  auto dot01 = e0.scalar_prod(e1);
  auto dot02 = e0.scalar_prod(e2);
  auto dot11 = e1.scalar_prod(e1);
  auto dot12 = e1.scalar_prod(e2);

  /* in matrix form
   M = e0.e0 e1.e0
       e0.e1 e1.e1
   U = u
       v
   R = e2.e0
       e2.e1
   U = R*(M^-1)
   */

  // Compute barycentric coordinates
  auto invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
  auto u = (dot11 * dot02 - dot01 * dot12) * invDenom;
  auto v = (dot00 * dot12 - dot01 * dot02) * invDenom;

  // Check if point is in or on triangle
  return (u >= 0) && (v >= 0) && (u + v <= 1);
}

/**
 * Get intersection points and their in out directions on the given ray
 * @param start :: Start point of ray
 * @param direction :: Direction of ray
 * @param v1 :: First vertex of triangle
 * @param v2 :: Second vertex of triangle
 * @param v3 :: Third vertex of triangle
 * @param intersection :: Intersection point
 * @param entryExit :: 1 if intersection is entry, -1 if exit
 * intersection
 * @returns true if there is an intersection
 */
bool rayIntersectsTriangle(const Kernel::V3D &start,
                           const Kernel::V3D &direction, const Kernel::V3D &v1,
                           const Kernel::V3D &v2, const Kernel::V3D &v3,
                           Kernel::V3D &intersection,
                           TrackDirection &entryExit) {
  // Implements Moller Trumbore intersection algorithm

  // Eq line x = x0 + tV
  //
  // p = w*p0 + u*p1 + v*p2, where numbered p refers to vertices of triangle
  // w+u+v == 1, so w = 1-u-v
  // p = (1-u-v)p0 + u*p1 + v*p2, rearranging ...
  // p = u(p1 - p0) + v(p2 - p0) + p0
  // in change of basis, barycentric coordinates p = p0 + u*v0 + v*v1. v0 and
  // v1 are basis vectors.

  // For line to pass through triangle...
  // (x0 + tV) = u(p1 - p0) + v(p2 - p0) + p0, yields
  // (x0 - p0) = -tV + u(p1 - p0) + v(p2 - p0)

  // rest is just to solve for u, v, t and check u and v are both >= 0 and <= 1
  // and u+v <=1

  auto edge1 = v2 - v1;
  auto edge2 = v3 - v1;
  auto h = direction.cross_prod(edge2);
  auto a = edge1.scalar_prod(h);

  const double EPSILON = 0.0000001 * edge1.norm();
  if (a > -EPSILON && a < EPSILON)
    return false; // Ray in or parallel to plane of triangle
  auto f = 1 / a;
  auto s = start - v1;
  // Barycentric coordinate offset u
  auto u = f * (s.scalar_prod(h));
  if (u < 0.0 || u > 1.0)
    return false; // Intersection with plane outside triangle
  auto q = s.cross_prod(edge1);
  // Barycentric coordinate offset v
  auto v = f * direction.scalar_prod(q);
  if (v < 0.0 || u + v > 1.0)
    return false; // Intersection with plane outside triangle

  // At this stage we can compute t to find out where the intersection point is
  // on the line.
  auto t = f * edge2.scalar_prod(q);
  if (t >= -EPSILON) // ray intersection
  {
    intersection = start + direction * t;

    // determine entry exit assuming anticlockwise triangle view from outside
    Kernel::V3D normalDirection = edge1.cross_prod(edge2);
    const auto scalar_prod = normalDirection.scalar_prod(direction);
    if (scalar_prod > 0.) {
      entryExit = TrackDirection::LEAVING; // exit
    } else if (scalar_prod < 0.) {
      entryExit = TrackDirection::ENTERING; // entry
    } else {
      throw std::domain_error("Track is in same direction as surface");
    }
    return true;
  }
  // The triangle is behind the start point. Forward ray does not intersect.
  return false;
}

void checkVertexLimit(size_t nVertices) {
  if (nVertices >= std::numeric_limits<uint32_t>::max()) {
    throw std::invalid_argument(
        "Too many vertices (" + std::to_string(nVertices) +
        "). MeshObject cannot have more than 2^32 vertices.");
  }
}

/**
 * Takes input vertices and calculates bounding box. Returns the bounding box.
 *
 * @param vertices :: vertices to create BB from
 * @param cacheBB :: mutable BB object to write to.
 */
const BoundingBox &getBoundingBox(const std::vector<Kernel::V3D> &vertices,
                                  BoundingBox &cacheBB) {

  if (cacheBB.isNull()) {
    static const double MinThickness = 0.001;
    double minX, maxX, minY, maxY, minZ, maxZ;
    minX = minY = minZ = std::numeric_limits<double>::max();
    maxX = maxY = maxZ = std::numeric_limits<double>::min();

    // Loop over all vertices and determine minima and maxima on each axis
    for (const auto &vertex : vertices) {
      auto vx = vertex.X();
      auto vy = vertex.Y();
      auto vz = vertex.Z();

      minX = std::min(minX, vx);
      maxX = std::max(maxX, vx);
      minY = std::min(minY, vy);
      maxY = std::max(maxY, vy);
      minZ = std::min(minZ, vz);
      maxZ = std::max(maxZ, vz);
    }
    if (minX == maxX)
      maxX += MinThickness;
    if (minY == maxY)
      maxY += MinThickness;
    if (minZ == maxZ)
      maxZ += MinThickness;

    // Cache bounding box, so we do not need to repeat calculation
    cacheBB = BoundingBox(maxX, maxY, maxZ, minX, minY, minZ);
  }

  return cacheBB;
}

/**
 * From vertices calculates the bounding box. Returns them back in max and min
 * points as well as mutated BB object.
 *
 * @param vertices :: vertices to create BB from
 * @param cacheBB :: mutable BB object to write to.
 * @param xmax :: Maximum value for the bounding box in x direction
 * @param ymax :: Maximum value for the bounding box in y direction
 * @param zmax :: Maximum value for the bounding box in z direction
 * @param xmin :: Minimum value for the bounding box in x direction
 * @param ymin :: Minimum value for the bounding box in y direction
 * @param zmin :: Minimum value for the bounding box in z direction
 */
void getBoundingBox(const std::vector<Kernel::V3D> &vertices,
                    BoundingBox &cacheBB, double &xmax, double &ymax,
                    double &zmax, double &xmin, double &ymin, double &zmin) {
  auto bb = getBoundingBox(vertices, cacheBB);
  xmax = bb.xMax();
  xmin = bb.xMin();
  ymax = bb.yMax();
  ymin = bb.yMin();
  zmax = bb.zMax();
  zmin = bb.zMin();
}

} // namespace MeshObjectCommon
} // namespace Geometry
} // namespace Mantid
