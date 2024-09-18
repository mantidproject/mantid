// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Objects/MeshObjectCommon.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/RandomPoint.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/vtkGeometryCacheReader.h"
#include "MantidGeometry/Rendering/vtkGeometryCacheWriter.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Material.h"

#include <algorithm>
#include <memory>
#include <utility>

namespace Mantid::Geometry {

MeshObject::MeshObject(std::vector<uint32_t> faces, std::vector<Kernel::V3D> vertices, const Kernel::Material &material)
    : m_boundingBox(), m_id("MeshObject"), m_triangles(std::move(faces)), m_vertices(std::move(vertices)),
      m_material(material) {

  initialize();
}

MeshObject::MeshObject(std::vector<uint32_t> &&faces, std::vector<Kernel::V3D> &&vertices,
                       const Kernel::Material &&material)
    : m_boundingBox(), m_id("MeshObject"), m_triangles(std::move(faces)), m_vertices(std::move(vertices)),
      m_material(material) {

  initialize();
}

// Do things that need to be done in constructor
void MeshObject::initialize() {

  MeshObjectCommon::checkVertexLimit(m_vertices.size());
  m_handler = std::make_shared<GeometryHandler>(*this);
}

/**
 * @return The Material that the object is composed from
 */
const Kernel::Material &MeshObject::material() const { return m_material; }

/**
 * @param material :: material that is being set for the object
 */
void MeshObject::setMaterial(const Kernel::Material &material) { m_material = material; }

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

  Kernel::V3D direction(0.0, 0.0, 1.0); // direction to look for intersections
  std::vector<Kernel::V3D> intersectionPoints;
  std::vector<TrackDirection> entryExitFlags;

  getIntersections(point, direction, intersectionPoints, entryExitFlags);

  if (intersectionPoints.empty()) {
    return false;
  }

  // True if point is on surface
  const auto it = std::find_if(
      intersectionPoints.cbegin(), intersectionPoints.cend(),
      [this, &point](const auto &intersectionPoint) { return point.distance(intersectionPoint) < M_TOLERANCE; });
  if (it != intersectionPoints.cend())
    return true;

  // Look for nearest point then check its entry-exit flag
  double nearestPointDistance = point.distance(intersectionPoints[0]);
  size_t nearestPointIndex = 0;
  for (size_t i = 1; i < intersectionPoints.size(); ++i) {
    if (point.distance(intersectionPoints[i]) < nearestPointDistance) {
      nearestPointDistance = point.distance(intersectionPoints[i]);
      nearestPointIndex = i;
    }
  }
  return (entryExitFlags[nearestPointIndex] == TrackDirection::LEAVING);
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

  const std::vector<Kernel::V3D> directions = {Kernel::V3D{0, 0, 1}, Kernel::V3D{0, 1, 0},
                                               Kernel::V3D{1, 0, 0}}; // directions to look for intersections
  // We have to look in several directions in case a point is on a face
  // or edge parallel to the first direction or also the second direction.
  for (const auto &direction : directions) {
    std::vector<Kernel::V3D> intersectionPoints;
    std::vector<TrackDirection> entryExitFlags;

    getIntersections(point, direction, intersectionPoints, entryExitFlags);

    if (intersectionPoints.empty()) {
      return false;
    }

    const auto it = std::find_if(
        intersectionPoints.cbegin(), intersectionPoints.cend(),
        [this, &point](const auto &intersectionPoint) { return point.distance(intersectionPoint) < M_TOLERANCE; });
    if (it != intersectionPoints.cend())
      return true;
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

  std::vector<Kernel::V3D> intersectionPoints;
  std::vector<TrackDirection> entryExit;

  getIntersections(UT.startPoint(), UT.direction(), intersectionPoints, entryExit);
  if (intersectionPoints.empty())
    return 0; // Quit if no intersections found

  // For a 3D mesh, a ray may intersect several segments
  for (size_t i = 0; i < intersectionPoints.size(); ++i) {
    UT.addPoint(entryExit[i], intersectionPoints[i], *this);
  }
  UT.buildLink();

  return UT.count() - originalCount;
}

/**
 * Compute the distance to the first point of intersection with the surface
 * @param track Track defining start/direction
 * @return The distance to the object
 * @throws std::runtime_error if no intersection was found
 */
double MeshObject::distance(const Track &track) const {
  Kernel::V3D vertex1, vertex2, vertex3, intersection;
  TrackDirection unused;
  for (size_t i = 0; getTriangle(i, vertex1, vertex2, vertex3); ++i) {
    if (MeshObjectCommon::rayIntersectsTriangle(track.startPoint(), track.direction(), vertex1, vertex2, vertex3,
                                                intersection, unused)) {
      return track.startPoint().distance(intersection);
    }
  }
  std::ostringstream os;
  os << "Unable to find intersection with object with track starting at " << track.startPoint() << " in direction "
     << track.direction() << "\n";
  throw std::runtime_error(os.str());
}

/**
 * Get intersection points and their in out directions on the given ray
 * @param start :: Start point of ray
 * @param direction :: Direction of ray
 * @param intersectionPoints :: Intersection points (not sorted)
 * @param entryExitFlags :: +1 ray enters -1 ray exits at corresponding point
 */
void MeshObject::getIntersections(const Kernel::V3D &start, const Kernel::V3D &direction,
                                  std::vector<Kernel::V3D> &intersectionPoints,
                                  std::vector<TrackDirection> &entryExitFlags) const {

  Kernel::V3D vertex1, vertex2, vertex3, intersection;
  TrackDirection entryExit;
  for (size_t i = 0; getTriangle(i, vertex1, vertex2, vertex3); ++i) {
    if (MeshObjectCommon::rayIntersectsTriangle(start, direction, vertex1, vertex2, vertex3, intersection, entryExit)) {
      intersectionPoints.emplace_back(intersection);
      entryExitFlags.emplace_back(entryExit);
    }
  }
  // still need to deal with edge cases
}

/*
 * Get a triangle - useful for iterating over triangles
 * @param index :: Index of triangle in MeshObject
 * @param v1 :: First vertex of triangle
 * @param v2 :: Second vertex of triangle
 * @param v3 :: Third vertex of triangle
 * @returns true if the specified triangle exists
 */
bool MeshObject::getTriangle(const size_t index, Kernel::V3D &vertex1, Kernel::V3D &vertex2,
                             Kernel::V3D &vertex3) const {
  bool triangleExists = index < m_triangles.size() / 3;
  if (triangleExists) {
    vertex1 = m_vertices[m_triangles[3 * index]];
    vertex2 = m_vertices[m_triangles[3 * index + 1]];
    vertex3 = m_vertices[m_triangles[3 * index + 2]];
  }
  return triangleExists;
}

/**
 * Calculate if a point PT is a valid point on the track
 * @param point :: Point to calculate from.
 * @param uVec :: Unit vector of the track
 * @retval 0 :: Not valid / double valid
 * @retval 1 :: Entry point
 * @retval -1 :: Exit Point
 */
int MeshObject::calcValidType(const Kernel::V3D &point, const Kernel::V3D &uVec) const {
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
void MeshObject::getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin,
                                double &zmin) const {
  return MeshObjectCommon::getBoundingBox(m_vertices, m_boundingBox, xmax, ymax, zmax, xmin, ymin, zmin);
}

/**
 * Find solid angle of object wrt the observer.
 * @param params :: point to measure solid angle from, and number of cylinder slices.
 * @return :: estimate of solid angle of object.
 */
double MeshObject::solidAngle(const SolidAngleParams &params) const {
  double solidAngleSum(0), solidAngleNegativeSum(0);
  Kernel::V3D vertex1, vertex2, vertex3;
  for (size_t i = 0; this->getTriangle(i, vertex1, vertex2, vertex3); ++i) {
    double sa = MeshObjectCommon::getTriangleSolidAngle(vertex1, vertex2, vertex3, params.observer());
    if (sa > 0.0) {
      solidAngleSum += sa;
    } else {
      solidAngleNegativeSum += sa;
    }
  }
  /*
    Same implementation as CSGObject. Assumes a convex closed mesh with
    solidAngleSum == -solidAngleNegativeSum

    Average is used to bypass issues with winding order. Surface normal
    affects magnitude of solid angle. See CSGObject.
  */
  return 0.5 * (solidAngleSum - solidAngleNegativeSum);
}

/**
 * Find solid angle of object wrt the observer with a scaleFactor for the
 * object.
 * @param params :: point to measure solid angle from, and number of cylinder slices
 * @param scaleFactor :: Kernel::V3D giving scaling of the object
 * @return :: estimate of solid angle of object.
 */
double MeshObject::solidAngle(const SolidAngleParams &params, const Kernel::V3D &scaleFactor) const {
  std::vector<Kernel::V3D> scaledVertices;
  scaledVertices.reserve(m_vertices.size());
  std::transform(m_vertices.cbegin(), m_vertices.cend(), std::back_inserter(scaledVertices),
                 [&scaleFactor](const auto &vertex) { return scaleFactor * vertex; });
  MeshObject meshScaled(m_triangles, scaledVertices, m_material);
  return meshScaled.solidAngle(params);
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
  Kernel::V3D centre(cX, cY, cZ);

  double volumeTimesSix(0.0);

  Kernel::V3D vertex1, vertex2, vertex3;
  for (size_t i = 0; getTriangle(i, vertex1, vertex2, vertex3); ++i) {
    Kernel::V3D a = vertex1 - centre;
    Kernel::V3D b = vertex2 - centre;
    Kernel::V3D c = vertex3 - centre;
    volumeTimesSix += a.scalar_prod(b.cross_prod(c));
  }

  return volumeTimesSix / 6.0;
}

/**
 * Returns an axis-aligned bounding box that will fit the shape
 * @returns A reference to a bounding box for this shape.
 */
const BoundingBox &MeshObject::getBoundingBox() const {
  return MeshObjectCommon::getBoundingBox(m_vertices, m_boundingBox);
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
std::optional<Kernel::V3D> MeshObject::generatePointInObject(Kernel::PseudoRandomNumberGenerator &rng,
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
 * @return The generated point
 */
std::optional<Kernel::V3D> MeshObject::generatePointInObject(Kernel::PseudoRandomNumberGenerator &rng,
                                                             const BoundingBox &activeRegion,
                                                             const size_t maxAttempts) const {

  const auto point = RandomPoint::bounded(*this, rng, activeRegion, maxAttempts);

  return point;
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
  for (const auto &dir : {Kernel::V3D(1., 0., 0.), Kernel::V3D(-1., 0., 0.), Kernel::V3D(0., 1., 0.),
                          Kernel::V3D(0., -1., 0.), Kernel::V3D(0., 0., 1.), Kernel::V3D(0., 0., -1.)}) {
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
void MeshObject::setGeometryHandler(const std::shared_ptr<GeometryHandler> &h) {
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
std::shared_ptr<GeometryHandler> MeshObject::getGeometryHandler() const {
  // Check if the geometry handler is upto dated with the cache, if not then
  // cache it now.
  return m_handler;
}

/**
 * Rotate the mesh according to the supplied rotation matrix
 * @param rotationMatrix Rotation matrix to be applied
 */
void MeshObject::rotate(const Kernel::Matrix<double> &rotationMatrix) {
  std::for_each(m_vertices.begin(), m_vertices.end(),
                [&rotationMatrix](auto &vertex) { vertex.rotate(rotationMatrix); });
}

/**
 * Translate the mesh according to the supplied x, y, z vector
 * @param translationVector Translation vector to be applied
 */
void MeshObject::translate(const Kernel::V3D &translationVector) {
  std::transform(m_vertices.cbegin(), m_vertices.cend(), m_vertices.begin(),
                 [&translationVector](const auto &vertex) { return vertex + translationVector; });
}

/**
 * Scale the mesh according to the supplied scale factor
 * @param scaleFactor Scale factor
 */
void MeshObject::scale(const double scaleFactor) {
  std::transform(m_vertices.cbegin(), m_vertices.cend(), m_vertices.begin(),
                 [&scaleFactor](const auto &vertex) { return vertex * scaleFactor; });
}

/**
 * Transform the mesh (scale, translate, rotate) according to the
 * supplied transformation matrix
 * @param matrix 4 x 4 transformation matrix
 */
void MeshObject::multiply(const Kernel::Matrix<double> &matrix) {
  if ((matrix.numCols() != 4) || (matrix.numRows() != 4)) {
    throw "Transformation matrix must be 4 x 4";
  }

  // create homogenous coordinates for the input vector with 4th element
  // equal to 1 (position)
  for (Kernel::V3D &vertex : m_vertices) {
    std::vector<double> vertexin(4);
    vertexin[0] = vertex.X();
    vertexin[1] = vertex.Y();
    vertexin[2] = vertex.Z();
    vertexin[3] = 1;
    std::vector<double> vertexout(4);
    matrix.multiplyPoint(vertexin, vertexout);
    Kernel::V3D newvertex(vertexout[0], vertexout[1], vertexout[2]);
    vertex = newvertex;
  }
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
std::vector<uint32_t> MeshObject::getTriangles() const { return m_triangles; }

/**
 * get number of points
 */
size_t MeshObject::numberOfVertices() const { return m_vertices.size(); }

/**
 * get vertices
 */
std::vector<double> MeshObject::getVertices() const { return MeshObjectCommon::getVertices(m_vertices); }

/**
 * get vertices in V3D form
 */
const std::vector<Kernel::V3D> &MeshObject::getV3Ds() const { return m_vertices; }

detail::ShapeInfo::GeometryShape MeshObject::shape() const { return detail::ShapeInfo::GeometryShape::NOSHAPE; }

const detail::ShapeInfo &MeshObject::shapeInfo() const {
  throw std::runtime_error("MeshObject::shapeInfo() is not implemented");
}

/**
 * get info on standard shapes (none for Mesh Object)
 */
void MeshObject::GetObjectGeom(detail::ShapeInfo::GeometryShape &type, std::vector<Kernel::V3D> &vectors,
                               double &innerRadius, double &radius, double &height) const {
  // In practice, this outputs type = -1,
  // to indicate not a "standard" object (cuboid/cone/cyl/sphere).
  // Retained for possible future use.
  type = detail::ShapeInfo::GeometryShape::NOSHAPE;
  if (m_handler == nullptr)
    return;
  m_handler->GetObjectGeom(type, vectors, innerRadius, radius, height);
}

} // namespace Mantid::Geometry
