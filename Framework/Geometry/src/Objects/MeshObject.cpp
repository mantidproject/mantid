#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/vtkGeometryCacheReader.h"
#include "MantidGeometry/Rendering/vtkGeometryCacheWriter.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include "MantidKernel/make_unique.h"

#include <boost/make_shared.hpp>

namespace Mantid {
namespace Geometry {

using Kernel::Material;
using Kernel::V3D;

MeshObject::MeshObject(const std::vector<uint16_t> &faces,
                       const std::vector<V3D> &vertices,
                       const Kernel::Material &material)
    : m_boundingBox(), m_id("MeshObject"), m_triangles(faces),
      m_vertices(vertices), m_material(material) {

  initialize();
}

MeshObject::MeshObject(std::vector<uint16_t> &&faces,
                       std::vector<V3D> &&vertices,
                       const Kernel::Material &&material)
    : m_boundingBox(), m_id("MeshObject"), m_triangles(std::move(faces)),
      m_vertices(std::move(vertices)), m_material(material) {

  initialize();
}

// Do things that need to be done in constructor
void MeshObject::initialize() {

  if (m_vertices.size() > std::numeric_limits<uint16_t>::max()) {
    throw std::invalid_argument(
        "Too many vertices (" + std::to_string(m_vertices.size()) +
        "). MeshObject cannot have more than 65535 vertices.");
  }
  m_handler = boost::make_shared<GeometryHandler>(*this);
}

/**
 * @return The Material that the object is composed from
 */
const Kernel::Material MeshObject::material() const { return m_material; }

/**
 * Returns whether this object has a valid shape
 * @returns True if the entire MeshObject may enclose
 * one or more volumes.
 */
bool MeshObject::hasValidShape() const {
  // May enclose volume if there are at
  // at least 4 triangles and 4 vertices (Tetrahedron)
  return (numberOfTriangles() >= 4 && numberOfVertices() >= 4);
}

/**
 * Determines whether point is within the object or on the surface
 * @param point :: Point to be tested
 * @returns true if point is within object or on surface
 */
bool MeshObject::isValid(const Kernel::V3D &point) const {

  BoundingBox bb = getBoundingBox();
  if (!bb.isPointInside(point)) {
    return false;
  }

  V3D direction(0.0, 0.0, 1.0); // direction to look for intersections
  std::vector<V3D> intersectionPoints;
  std::vector<int> entryExitFlags;

  getIntersections(point, direction, intersectionPoints, entryExitFlags);

  if (intersectionPoints.empty()) {
    return false;
  }

  // True if point is on surface
  for (const auto &intersectionPoint : intersectionPoints) {
    if (point.distance(intersectionPoint) < M_TOLERANCE) {
      return true;
    }
  }

  // Look for nearest point then check its entry-exit flag
  double nearestPointDistance = point.distance(intersectionPoints[0]);
  size_t nearestPointIndex = 0;
  for (size_t i = 1; i < intersectionPoints.size(); ++i) {
    if (point.distance(intersectionPoints[i]) < nearestPointDistance) {
      nearestPointDistance = point.distance(intersectionPoints[i]);
      nearestPointIndex = i;
    }
  }
  return (entryExitFlags[nearestPointIndex] == -1);
}

/**
 * Determines wither point is on the surface.
 * @param point :: Point to check
 * @returns true if the point is on the surface
 */
bool MeshObject::isOnSide(const Kernel::V3D &point) const {

  BoundingBox bb = getBoundingBox();
  if (!bb.isPointInside(point)) {
    return false;
  }

  const std::vector<V3D> directions = {
      V3D{0, 0, 1}, V3D{0, 1, 0},
      V3D{1, 0, 0}}; // directions to look for intersections
  // We have to look in several directions in case a point is on a face
  // or edge parallel to the first direction or also the second direction.
  for (const auto &direction : directions) {
    std::vector<V3D> intersectionPoints;
    std::vector<int> entryExitFlags;

    getIntersections(point, direction, intersectionPoints, entryExitFlags);

    if (intersectionPoints.empty()) {
      return false;
    }

    for (const auto &intersectionPoint : intersectionPoints) {
      if (point.distance(intersectionPoint) < M_TOLERANCE) {
        return true;
      }
    }
  }
  return false;
}

/**
 * Given a track, fill the track with valid section
 * @param UT :: Initial track
 * @return Number of segments added
 */
int MeshObject::interceptSurface(Geometry::Track &UT) const {

  int originalCount = UT.count(); // Number of intersections original track
  BoundingBox bb = getBoundingBox();
  if (!bb.doesLineIntersect(UT)) {
    return 0;
  }

  std::vector<V3D> intersectionPoints;
  std::vector<int> entryExitFlags;

  getIntersections(UT.startPoint(), UT.direction(), intersectionPoints,
                   entryExitFlags);
  if (intersectionPoints.empty())
    return 0; // Quit if no intersections found

  for (size_t i = 0; i < intersectionPoints.size(); ++i) {
    UT.addPoint(entryExitFlags[i], intersectionPoints[i], *this);
  }
  UT.buildLink();

  return UT.count() - originalCount;
}

/**
 * Get intersection points and their in out directions on the given ray
 * @param start :: Start point of ray
 * @param direction :: Direction of ray
 * @param intersectionPoints :: Intersection points (not sorted)
 * @param entryExitFlags :: +1 ray enters -1 ray exits at corresponding point
 */
void MeshObject::getIntersections(const Kernel::V3D &start,
                                  const Kernel::V3D &direction,
                                  std::vector<Kernel::V3D> &intersectionPoints,
                                  std::vector<int> &entryExitFlags) const {

  V3D vertex1, vertex2, vertex3, intersection;
  int entryExit;
  for (size_t i = 0; getTriangle(i, vertex1, vertex2, vertex3); ++i) {
    if (rayIntersectsTriangle(start, direction, vertex1, vertex2, vertex3,
                              intersection, entryExit)) {
      intersectionPoints.push_back(intersection);
      entryExitFlags.push_back(entryExit);
    }
  }
  // still need to deal with edge cases
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
 * @returns true if there is an intersection
 */
bool MeshObject::rayIntersectsTriangle(const Kernel::V3D &start,
                                       const Kernel::V3D &direction,
                                       const V3D &v1, const V3D &v2,
                                       const V3D &v3, V3D &intersection,
                                       int &entryExit) const {
  // Implements Möller–Trumbore intersection algorithm
  V3D edge1, edge2, h, s, q;
  double a, f, u, v;
  edge1 = v2 - v1;
  edge2 = v3 - v1;
  h = direction.cross_prod(edge2);
  a = edge1.scalar_prod(h);

  const double EPSILON = 0.0000001 * edge1.norm();
  if (a > -EPSILON && a < EPSILON)
    return false; // Ray in or parallel to plane of triangle
  f = 1 / a;
  s = start - v1;
  u = f * (s.scalar_prod(h));
  if (u < 0.0 || u > 1.0)
    return false; // Intersection with plane outside triangle
  q = s.cross_prod(edge1);
  v = f * direction.scalar_prod(q);
  if (v < 0.0 || u + v > 1.0)
    return false; // Intersection with plane outside triangle

  // At this stage we can compute t to find out where the intersection point is
  // on the line.
  double t = f * edge2.scalar_prod(q);
  if (t >= -EPSILON) // ray intersection
  {
    intersection = start + direction * t;

    // determine entry exit assuming anticlockwise triangle view from outside
    V3D normalDirection = edge1.cross_prod(edge2);
    if (normalDirection.scalar_prod(direction) > 0.0) {
      entryExit = -1; // exit
    } else {
      entryExit = 1; // entry
    }
    return true;
  }
  // Here the intersection occurs on the line of the ray behind the ray.
  return false;
}

/*
 * Get a triangle - useful for iterating over triangles
 * @param index :: Index of triangle in MeshObject
 * @param v1 :: First vertex of triangle
 * @param v2 :: Second vertex of triangle
 * @param v3 :: Third vertex of triangle
 * @returns true if the specified triangle exists
 */
bool MeshObject::getTriangle(const size_t index, V3D &vertex1, V3D &vertex2,
                             V3D &vertex3) const {
  bool triangleExists = index < m_triangles.size() / 3;
  if (triangleExists) {
    vertex1 = m_vertices[m_triangles[3 * index]];
    vertex2 = m_vertices[m_triangles[3 * index + 1]];
    vertex3 = m_vertices[m_triangles[3 * index + 2]];
  }
  return triangleExists;
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

/**
 * Calculate if a point PT is a valid point on the track
 * @param point :: Point to calculate from.
 * @param uVec :: Unit vector of the track
 * @retval 0 :: Not valid / double valid
 * @retval 1 :: Entry point
 * @retval -1 :: Exit Point
 */
int MeshObject::calcValidType(const Kernel::V3D &point,
                              const Kernel::V3D &uVec) const {
  const Kernel::V3D shift(uVec * Kernel::Tolerance * 25.0);
  const Kernel::V3D testA(point - shift);
  const Kernel::V3D testB(point + shift);
  const int flagA = isValid(testA);
  const int flagB = isValid(testB);
  if (!(flagA ^ flagB))
    return 0;
  return (flagA) ? -1 : 1;
}

/**
 * Takes input axis aligned bounding box max and min points and calculates the
 *bounding box for the
 * object and returns them back in max and min points.
 *
 * @param xmax :: Maximum value for the bounding box in x direction
 * @param ymax :: Maximum value for the bounding box in y direction
 * @param zmax :: Maximum value for the bounding box in z direction
 * @param xmin :: Minimum value for the bounding box in x direction
 * @param ymin :: Minimum value for the bounding box in y direction
 * @param zmin :: Minimum value for the bounding box in z direction
 */
void MeshObject::getBoundingBox(double &xmax, double &ymax, double &zmax,
                                double &xmin, double &ymin,
                                double &zmin) const {
  BoundingBox bb = getBoundingBox();
  xmin = bb.xMin();
  xmax = bb.xMax();
  ymin = bb.yMin();
  ymax = bb.yMax();
  zmin = bb.zMin();
  zmax = bb.zMax();
}

/**
 * Find solid angle of object wrt the observer.
 * @param observer :: point to measure solid angle from
 * @return :: estimate of solid angle of object.
 */
double MeshObject::solidAngle(const Kernel::V3D &observer) const {

  double solidAngleSum(0), solidAngleNegativeSum(0);
  V3D vertex1, vertex2, vertex3;
  for (size_t i = 0; getTriangle(i, vertex1, vertex2, vertex3); ++i) {
    double sa = getTriangleSolidAngle(vertex1, vertex2, vertex3, observer);
    if (sa > 0.0) {
      solidAngleSum += sa;
    } else {
      solidAngleNegativeSum += sa;
    }
  }
  return 0.5 * (solidAngleSum - solidAngleNegativeSum);
}

/**
 * Find solid angle of object wrt the observer with a scaleFactor for the
 * object.
 * @param observer :: point to measure solid angle from
 * @param scaleFactor :: V3D giving scaling of the object
 * @return :: estimate of solid angle of object.
 */
double MeshObject::solidAngle(const Kernel::V3D &observer,
                              const Kernel::V3D &scaleFactor) const

{
  std::vector<V3D> scaledVertices;
  scaledVertices.reserve(m_vertices.size());
  for (const auto &vertex : m_vertices) {
    scaledVertices.emplace_back(scaleFactor.X() * vertex.X(),
                                scaleFactor.Y() * vertex.Y(),
                                scaleFactor.Z() * vertex.Z());
  }
  MeshObject scaledObject(m_triangles, scaledVertices, m_material);
  return scaledObject.solidAngle(observer);
}

/**
 * Calculate volume.
 * @return The volume.
 */
double MeshObject::volume() const {
  // Select centre of bounding box as centre point.
  // For each triangle calculate the signed volume of
  // the tetrahedron formed by the triangle and the
  // centre point. Then add to total.

  BoundingBox bb = getBoundingBox();
  double cX = 0.5 * (bb.xMax() + bb.xMin());
  double cY = 0.5 * (bb.yMax() + bb.yMin());
  double cZ = 0.5 * (bb.zMax() + bb.zMin());
  V3D centre(cX, cY, cZ);

  double volumeTimesSix(0.0);

  V3D vertex1, vertex2, vertex3;
  for (size_t i = 0; getTriangle(i, vertex1, vertex2, vertex3); ++i) {
    V3D a = vertex1 - centre;
    V3D b = vertex2 - centre;
    V3D c = vertex3 - centre;
    volumeTimesSix += a.scalar_prod(b.cross_prod(c));
  }

  return volumeTimesSix / 6.0;
}

/**
 * Returns an axis-aligned bounding box that will fit the shape
 * @returns A reference to a bounding box for this shape.
 */
const BoundingBox &MeshObject::getBoundingBox() const {

  if (m_boundingBox.isNull())
    // As the shape of MeshObject is immutable, we need only calculate
    // bounding box, if the cached bounding box is null.
    if (numberOfVertices() > 0) {
      // Initial extents to be overwritten by loop
      constexpr double huge = 1e10;
      double minX, maxX, minY, maxY, minZ, maxZ;
      minX = minY = minZ = huge;
      maxX = maxY = maxZ = -huge;

      // Loop over all vertices and determine minima and maxima on each axis
      for (const auto &vertex : m_vertices) {
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
      if (minZ == maxZ)
        maxZ += 0.001;

      // Cache bounding box, so we do not need to repeat calculation
      m_boundingBox = BoundingBox(maxX, maxY, maxZ, minX, minY, minZ);
    }

  return m_boundingBox;
}

/**
Try to find a point that lies within (or on) the object
@param[out] point :: on exit set to the point value, if found
@return 1 if point found, 0 otherwise
*/
int MeshObject::getPointInObject(Kernel::V3D &point) const {

  Kernel::V3D testPt(0, 0, 0);
  // Try centre of bounding box as initial guess, if we have one.
  const BoundingBox &boundingBox = getBoundingBox();
  if (boundingBox.isNonNull()) {
    testPt = boundingBox.centrePoint();
    if (searchForObject(testPt)) {
      point = testPt;
      return 1;
    }
  }

  return 0;
}

/**
 * Generate a random point within the object. The method simply generates a
 * point within the bounding box and tests if this is a valid point within
 * the object: if so the point is return otherwise a new point is selected.
 * @param rng  A reference to a PseudoRandomNumberGenerator where
 * nextValue should return a flat random number between 0.0 & 1.0
 * @param maxAttempts The maximum number of attempts at generating a point
 * @return The generated point
 */
V3D MeshObject::generatePointInObject(Kernel::PseudoRandomNumberGenerator &rng,
                                      const size_t maxAttempts) const {
  const auto &bbox = getBoundingBox();
  if (bbox.isNull()) {
    throw std::runtime_error("Object::generatePointInObject() - Invalid "
                             "bounding box. Cannot generate new point.");
  }
  return generatePointInObject(rng, bbox, maxAttempts);
}

/**
 * Generate a random point within the object that is also bound by the
 * activeRegion box.
 * @param rng A reference to a PseudoRandomNumberGenerator where
 * nextValue should return a flat random number between 0.0 & 1.0
 * @param activeRegion Restrict point generation to this sub-region of the
 * object
 * @param maxAttempts The maximum number of attempts at generating a point
 * @return The newly generated point
 */
V3D MeshObject::generatePointInObject(Kernel::PseudoRandomNumberGenerator &rng,
                                      const BoundingBox &activeRegion,
                                      const size_t maxAttempts) const {
  size_t attempts(0);
  while (attempts < maxAttempts) {
    const double r1 = rng.nextValue();
    const double r2 = rng.nextValue();
    const double r3 = rng.nextValue();
    auto pt = activeRegion.generatePointInside(r1, r2, r3);
    if (this->isValid(pt))
      return pt;
    else
      ++attempts;
  };
  throw std::runtime_error("Object::generatePointInObject() - Unable to "
                           "generate point in object after " +
                           std::to_string(maxAttempts) + " attempts");
}

/**
 * Try to find a point that lies within (or on) the object, given a seed point
 * @param point :: on entry the seed point, on exit point in object, if found
 * @return true if point found
 */
bool MeshObject::searchForObject(Kernel::V3D &point) const {
  //
  // Method - check if point in object, if not search directions along
  // principle axes using interceptSurface
  //
  if (isValid(point))
    return true;
  for (const auto &dir :
       {V3D(1., 0., 0.), V3D(-1., 0., 0.), V3D(0., 1., 0.), V3D(0., -1., 0.),
        V3D(0., 0., 1.), V3D(0., 0., -1.)}) {
    Geometry::Track tr(point, dir);
    if (this->interceptSurface(tr) > 0) {
      point = tr.cbegin()->entryPoint;
      return true;
    }
  }
  return false;
}

/**
 * Set the geometry handler for Object
 * @param[in] h is pointer to the geometry handler. don't delete this pointer in
 * the calling function.
 */
void MeshObject::setGeometryHandler(boost::shared_ptr<GeometryHandler> h) {
  if (h == nullptr)
    return;
  m_handler = h;
}

/**
 * Draws the Object using geometry handler, If the handler is not set then this
 * function does nothing.
 */
void MeshObject::draw() const {
  if (m_handler == nullptr)
    return;
  // Render the Object
  m_handler->render();
}

/**
 * Initializes/prepares the object to be rendered, this will generate geometry
 * for object,
 * If the handler is not set then this function does nothing.
 */
void MeshObject::initDraw() const {
  if (m_handler == nullptr)
    return;
  // Render the Object
  m_handler->initialize();
}

/**
 * Returns the geometry handler
 */
boost::shared_ptr<GeometryHandler> MeshObject::getGeometryHandler() const {
  // Check if the geometry handler is upto dated with the cache, if not then
  // cache it now.
  return m_handler;
}

/**
 * Updates the geometry handler if needed
 */
void MeshObject::updateGeometryHandler() {
  return; // Hopefully nothing necessary here
}

/**
 * Output functions for rendering, may also be used internally
 */
size_t MeshObject::numberOfTriangles() const { return m_triangles.size() / 3; }

/**
 * get faces
 */
std::vector<uint32_t> MeshObject::getTriangles() const {
  std::vector<uint32_t> faces;
  size_t nFaceCorners = m_triangles.size();
  if (nFaceCorners > 0) {
    faces.resize(static_cast<std::size_t>(nFaceCorners));
    for (size_t i = 0; i < nFaceCorners; ++i) {
      faces[i] = static_cast<int>(m_triangles[i]);
    }
  }
  return faces;
}

/**
 * get number of points
 */
size_t MeshObject::numberOfVertices() const {
  return static_cast<int>(m_vertices.size());
}

/**
 * get vertices
 */
std::vector<double> MeshObject::getVertices() const {
  std::vector<double> points;
  size_t nPoints = m_vertices.size();
  if (nPoints > 0) {
    points.resize(static_cast<std::size_t>(nPoints) * 3);
    for (size_t i = 0; i < nPoints; ++i) {
      V3D pnt = m_vertices[i];
      points[i * 3 + 0] = pnt.X();
      points[i * 3 + 1] = pnt.Y();
      points[i * 3 + 2] = pnt.Z();
    }
  }
  return points;
}

/**
 * get info on standard shapes (none for Mesh Object)
 */
void MeshObject::GetObjectGeom(detail::ShapeInfo::GeometryShape &type,
                               std::vector<Kernel::V3D> &vectors,
                               double &myradius, double &myheight) const {
  // In practice, this outputs type = -1,
  // to indicate not a "standard" object (cuboid/cone/cyl/sphere).
  // Retained for possible future use.
  type = detail::ShapeInfo::GeometryShape::NOSHAPE;
  if (m_handler == nullptr)
    return;
  m_handler->GetObjectGeom(type, vectors, myradius, myheight);
}

} // NAMESPACE Geometry
} // NAMESPACE Mantid
