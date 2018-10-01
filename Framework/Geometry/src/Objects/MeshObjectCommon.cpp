#include "MantidGeometry/Objects/MeshObjectCommon.h"

namespace Mantid {
namespace Geometry {

using Mantid::Kernel::V3D;

namespace MeshObjectCommon {
/**
 * getVertices converts vector V3D to vector doubles. 3x size of input. ordered
 * x,y,z,x,y,z...
 * @param vertices : input vector of V3D
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
double getTriangleSolidAngle(const V3D &a, const V3D &b, const V3D &c,
                             const V3D &observer) {
  const V3D ao = a - observer;
  const V3D bo = b - observer;
  const V3D co = c - observer;
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

/*
 * Get a triangle - For iterating over triangles
 * @param index :: Index of triangle in MeshObject
 * @param triangles :: indices into vertices 3 consecutive form triangle
 * @param vertices :: Vertices to lookup
 * @param v1 :: First vertex of triangle
 * @param v2 :: Second vertex of triangle
 * @param v3 :: Third vertex of triangle
 * @returns true if the specified triangle exists
 */
bool getTriangle(const size_t index, const std::vector<uint16_t> &triangles,
                 const std::vector<Kernel::V3D> &vertices, V3D &vertex1,
                 V3D &vertex2, V3D &vertex3) {
  bool triangleExists = index < triangles.size() / 3;
  if (triangleExists) {
    vertex1 = vertices[triangles[3 * index]];
    vertex2 = vertices[triangles[3 * index + 1]];
    vertex3 = vertices[triangles[3 * index + 2]];
  }
  return triangleExists;
}

double solidAngle(const Kernel::V3D &observer,
                  const std::vector<uint16_t> &triangles,
                  const std::vector<Kernel::V3D> &vertices) {

  double solidAngleSum(0), solidAngleNegativeSum(0);
  V3D vertex1, vertex2, vertex3;
  for (size_t i = 0;
       getTriangle(i, triangles, vertices, vertex1, vertex2, vertex3); ++i) {
    double sa = getTriangleSolidAngle(vertex1, vertex2, vertex3, observer);
    if (sa > 0.0) {
      solidAngleSum += sa;
    } else {
      solidAngleNegativeSum += sa;
    }
  }
  return 0.5 * (solidAngleSum - solidAngleNegativeSum);
}
} // namespace MeshObjectCommon
} // namespace Geometry
} // namespace Mantid
