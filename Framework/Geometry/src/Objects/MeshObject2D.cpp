// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Objects/MeshObject2D.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/MeshObjectCommon.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/V3D.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <numeric>
#include <utility>

namespace Mantid::Geometry {

namespace CoplanarChecks {
bool sufficientPoints(const std::vector<Kernel::V3D> &vertices) {
  return vertices.size() >= 3; // Not a plane with < 3 points
}

/**
 * Establish the first surface normal. Tries to establish normal from
 * non-colinear points
 * @param vertices : All vertices
 * @return surface normal, or 0,0,0 if not found
 */
Kernel::V3D surfaceNormal(const std::vector<Kernel::V3D> &vertices) {
  auto v0 = vertices[1] - vertices[0];
  Kernel::V3D normal{0, 0, 0};
  const static double tolerance_sq = std::pow(1e-9, 2);
  // Look for normal amongst first 3 non-colinear points
  for (size_t i = 1; i < vertices.size() - 1; ++i) {
    auto v1 = vertices[i + 1] - vertices[i];
    normal = v0.cross_prod(v1);
    if (normal.norm2() > tolerance_sq) {
      break;
    }
  }
  return normal;
}

/**
 * Establish if all vertices are coplanar
 * @param vertices : All vertices to check
 * @param normal : Surface normal
 * @return True only if all vertices are coplanar
 */
bool allCoplanar(const std::vector<Kernel::V3D> &vertices, const Kernel::V3D &normal) {
  bool in_plane = true;
  auto v0 = vertices[0];
  const auto nx = normal[0];
  const auto ny = normal[1];
  const auto nz = normal[2];
  const auto k = nx * v0.X() + ny * v0.Y() + nz * v0.Z();
  const auto denom = normal.norm();
  const static double tolerance = 1e-9; // Fixed Tolerance. Too expensive to
                                        // calculate based on machine
                                        // uncertaintly for each vertex.

  for (const auto &vertex : vertices) {
    auto d = (nx * vertex.X() + ny * vertex.Y() + nz * vertex.Z() - k) / denom;
    if (d > tolerance || d < -1 * tolerance) {
      in_plane = false;
      break;
    }
  }
  return in_plane;
}

/**
 * Establish the surface normal for a set of vertices. Throw invalid_argument if
 * colinear or non-coplanar vertices found.
 * @param vertices : all vertices to consider
 * @return : normal to surface formed by points
 */
Kernel::V3D validatePointsCoplanar(const std::vector<Kernel::V3D> &vertices) {
  if (!sufficientPoints(vertices))
    throw std::invalid_argument("Insufficient vertices to create a plane");

  const auto normal = CoplanarChecks::surfaceNormal(vertices);
  // Check that a valid normal was found amongst collection of vertices
  if (normal.norm2() == 0) {
    // If all points are colinear. Not a plane.
    throw std::invalid_argument("All vertices are colinear. This does not define a plane");
  }

  if (!allCoplanar(vertices, normal))
    throw std::invalid_argument("Vertices do not define a plane");
  return normal;
}
} // namespace CoplanarChecks
namespace {
/**
 * Get a triangle - For iterating over triangles
 * @param index :: Index of triangle in MeshObject
 * @param triangles :: indices into vertices 3 consecutive form triangle
 * @param vertices :: Vertices to lookup
 * @param vertex1 :: First vertex of triangle
 * @param vertex2 :: Second vertex of triangle
 * @param vertex3 :: Third vertex of triangle
 * @returns true if the specified triangle exists
 */
bool getTriangle(const size_t index, const std::vector<uint32_t> &triangles, const std::vector<Kernel::V3D> &vertices,
                 Kernel::V3D &vertex1, Kernel::V3D &vertex2, Kernel::V3D &vertex3) {
  bool triangleExists = index < triangles.size() / 3;
  if (triangleExists) {
    vertex1 = vertices[triangles[3 * index]];
    vertex2 = vertices[triangles[3 * index + 1]];
    vertex3 = vertices[triangles[3 * index + 2]];
  }
  return triangleExists;
}
} // namespace

const double MeshObject2D::MinThickness = 0.001;
const std::string MeshObject2D::Id = "MeshObject2D";

/**
 * Estalish if points are coplanar.
 * @param vertices : All vertices to consider
 * @return : Return True only if all coplanar
 */
bool MeshObject2D::pointsCoplanar(const std::vector<Kernel::V3D> &vertices) {
  if (!CoplanarChecks::sufficientPoints(vertices))
    return false;

  Kernel::V3D normal = CoplanarChecks::surfaceNormal(vertices);
  // Check that a valid normal was found amongst collection of vertices
  if (normal.norm2() == 0) {
    // If all points are colinear. Not a plane.
    return false;
  }

  return CoplanarChecks::allCoplanar(vertices, normal);
}

/**
 * Constructor
 */
MeshObject2D::MeshObject2D(std::vector<uint32_t> faces, std::vector<Kernel::V3D> vertices,
                           const Kernel::Material &material)
    : m_triangles(std::move(faces)), m_vertices(std::move(vertices)), m_material(material) {
  initialize();
}

/**
 * Move constructor
 */
MeshObject2D::MeshObject2D(std::vector<uint32_t> &&faces, std::vector<Kernel::V3D> &&vertices,
                           const Kernel::Material &&material)
    : m_triangles(std::move(faces)), m_vertices(std::move(vertices)), m_material(material) {
  initialize();
}

/**
 * Common initialization
 */
void MeshObject2D::initialize() {
  auto surfaceNormal = CoplanarChecks::validatePointsCoplanar(m_vertices);
  const auto v0 = m_vertices[0];
  const auto n_mag = surfaceNormal.norm();
  PlaneParameters parameters;
  parameters.a = surfaceNormal.X() / n_mag;
  parameters.b = surfaceNormal.Y() / n_mag;
  parameters.c = surfaceNormal.Z() / n_mag;
  parameters.k = parameters.a * v0.X() + parameters.b * v0.Y() + parameters.c * v0.Z();
  parameters.normal = surfaceNormal;
  parameters.abs_normal = surfaceNormal.norm();
  parameters.p0 = v0;
  m_planeParameters = parameters;

  MeshObjectCommon::checkVertexLimit(m_vertices.size());
  m_handler = std::make_shared<GeometryHandler>(*this);
}
bool MeshObject2D::hasValidShape() const {
  // 3 or more points define a plane.
  return (!m_triangles.empty() && m_vertices.size() >= 3);
}

double MeshObject2D::distanceToPlane(const Kernel::V3D &point) const {
  return ((point.X() * m_planeParameters.a) + (point.Y() * m_planeParameters.b) + (point.Z() * m_planeParameters.c) +
          m_planeParameters.k);
}

/**
 * Check that the point is on the plane AND that it is inside or on@brief
 * MeshObject2D::isOnSide one of the triangles that defines the plane. Both must
 * be true.
 * @param point : Point to test
 * @return : True only if the point is valid
 */
bool MeshObject2D::isValid(const Kernel::V3D &point) const {

  static const double tolerance = 1e-9;
  if (distanceToPlane(point) < tolerance) {
    for (size_t i = 0; i < m_triangles.size(); i += 3) {

      if (MeshObjectCommon::isOnTriangle(point, m_vertices[m_triangles[i]], m_vertices[m_triangles[i + 1]],
                                         m_vertices[m_triangles[i + 2]]))
        return true;
    }
  }
  return false;
}

/**
 * Determine if point is on the side of the object
 * @param point : Point to test
 * @return : True if the point is on the side
 */
bool MeshObject2D::isOnSide(const Kernel::V3D &point) const { return isValid(point); }

/**
 * Given a track, fill the track with valid section
 * @param ut :: Initial track
 * @return Number of segments added
 */
int MeshObject2D::interceptSurface(Geometry::Track &ut) const {
  const int originalCount = ut.count(); // Number of intersections original track

  const auto &norm = m_planeParameters.normal;
  const auto t =
      -(ut.startPoint().scalar_prod(norm) + m_planeParameters.p0.scalar_prod(norm)) / ut.direction().scalar_prod(norm);

  // Intersects infinite plane. No point evaluating individual segements if this
  // fails
  if (t >= 0) {
    Kernel::V3D intersection;
    TrackDirection entryExit;
    for (size_t i = 0; i < m_vertices.size(); i += 3) {
      if (MeshObjectCommon::rayIntersectsTriangle(ut.startPoint(), ut.direction(), m_vertices[i], m_vertices[i + 1],
                                                  m_vertices[i + 2], intersection, entryExit)) {
        ut.addPoint(entryExit, intersection, *this);
        ut.buildLink();
        // All vertices on plane. So only one triangle intersection possible
        break;
      }
    }
  }

  return ut.count() - originalCount;
}

/**
 * Compute the distance to the first point of intersection with the mesh
 * @param ut Track defining start/direction
 * @return The distance to the object
 * @throws std::runtime_error if no intersection was found
 */
double MeshObject2D::distance(const Track &ut) const {
  const auto &norm = m_planeParameters.normal;
  const auto t =
      -(ut.startPoint().scalar_prod(norm) + m_planeParameters.p0.scalar_prod(norm)) / ut.direction().scalar_prod(norm);

  // Intersects infinite plane. No point evaluating individual segements if this
  // fails
  if (t >= 0) {
    Kernel::V3D intersection;
    TrackDirection entryExit;
    for (size_t i = 0; i < m_vertices.size(); i += 3) {
      if (MeshObjectCommon::rayIntersectsTriangle(ut.startPoint(), ut.direction(), m_vertices[i], m_vertices[i + 1],
                                                  m_vertices[i + 2], intersection, entryExit)) {
        // All vertices on plane. So only one triangle intersection possible
        return intersection.distance(ut.startPoint());
      }
    }
  }
  std::ostringstream os;
  os << "Unable to find intersection with object with track starting at " << ut.startPoint() << " in direction "
     << ut.direction() << "\n";
  throw std::runtime_error(os.str());
}

MeshObject2D *MeshObject2D::clone() const {
  return new MeshObject2D(this->m_triangles, this->m_vertices, this->m_material);
}

MeshObject2D *MeshObject2D::cloneWithMaterial(const Kernel::Material &material) const {
  return new MeshObject2D(this->m_triangles, this->m_vertices, material);
}

int MeshObject2D::getName() const {
  return 0; // This is a hack. See how "names" are assigned in
            // InstrumentDefinitionParser. Also see vtkGeometryCacheReader for
  // where this is used.
}

/**
 * Solid angle only considers triangle facing sample. Back faces do NOT
 * contribute.
 *
 * This is tantamount to defining an object that is opaque to neutrons. Note
 * that it is still possible to define a facing surface which is obscured by
 * another. In that case there would still be a solid angle contribution as
 * there is no way of detecting the shadowing.
 *
 * @param params
 * @return
 */
double MeshObject2D::solidAngle(const SolidAngleParams &params) const {
  double solidAngleSum(0);
  Kernel::V3D vertex1, vertex2, vertex3;
  for (size_t i = 0; getTriangle(i, m_triangles, m_vertices, vertex1, vertex2, vertex3); ++i) {
    double sa = MeshObjectCommon::getTriangleSolidAngle(vertex1, vertex2, vertex3, params.observer());
    if (sa > 0) {
      solidAngleSum += sa;
    }
  }
  return solidAngleSum;
}

double MeshObject2D::solidAngle(const SolidAngleParams &params, const Kernel::V3D &scaleFactor) const {
  std::vector<Kernel::V3D> scaledVertices;
  scaledVertices.reserve(m_vertices.size());
  std::transform(m_vertices.cbegin(), m_vertices.cend(), std::back_inserter(scaledVertices),
                 [&scaleFactor](const auto &vertex) { return vertex * scaleFactor; });
  MeshObject2D meshScaled(m_triangles, scaledVertices, m_material);
  return meshScaled.solidAngle(params);
}

bool MeshObject2D::operator==(const MeshObject2D &other) const {
  return m_vertices.size() == other.m_vertices.size() && m_triangles.size() == other.m_triangles.size() &&
         m_planeParameters.a == other.m_planeParameters.a && m_planeParameters.b == other.m_planeParameters.b &&
         m_planeParameters.c == other.m_planeParameters.c && m_planeParameters.k == other.m_planeParameters.k &&
         m_planeParameters.p0 == other.m_planeParameters.p0 && m_material.name() == other.m_material.name();
}

double MeshObject2D::volume() const {
  return 0; // Volume is always 0 for a plane
}

/**
 * Returns an axis-aligned bounding box that will fit the shape
 *
 * This is not threadsafe
 *
 * @returns A reference to a bounding box for this shape.
 */
const BoundingBox &MeshObject2D::getBoundingBox() const {
  return MeshObjectCommon::getBoundingBox(m_vertices, m_boundingBox);
}

void MeshObject2D::getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin,
                                  double &zmin) const {
  return MeshObjectCommon::getBoundingBox(m_vertices, m_boundingBox, xmax, ymax, zmax, xmin, ymin, zmin);
}

/**
Try to find a point that lies within (or on) the object
@param[out] point :: on exit set to the point value, if found
@return 1 if point found, 0 otherwise
*/
int MeshObject2D::getPointInObject(Kernel::V3D &point) const { return this->isValid(point) ? 1 : 0; }

std::optional<Kernel::V3D> MeshObject2D::generatePointInObject(Kernel::PseudoRandomNumberGenerator & /*rng*/,
                                                               const size_t /*unused*/) const {
  // How this would work for a finite plane is not clear. Points within the
  // plane can of course be generated, but most implementations of this method
  // use the bounding box
  throw std::runtime_error("Not implemented.");
}

std::optional<Kernel::V3D> MeshObject2D::generatePointInObject(Kernel::PseudoRandomNumberGenerator & /*rng*/,
                                                               const BoundingBox & /*activeRegion*/,
                                                               const size_t /*unused*/) const {

  // How this would work for a finite plane is not clear. Points within the
  // plane can of course be generated, but most implementations of this method
  // in sibling types use the bounding box
  throw std::runtime_error("Not implemented");
}

const Kernel::Material &MeshObject2D::material() const { return m_material; }

/**
 * @param material :: material that is being set for the object
 */
void MeshObject2D::setMaterial(const Kernel::Material &material) { m_material = material; }

const std::string &MeshObject2D::id() const { return MeshObject2D::Id; }

std::shared_ptr<GeometryHandler> MeshObject2D::getGeometryHandler() const { return m_handler; }

size_t MeshObject2D::numberOfVertices() const { return m_vertices.size(); }

size_t MeshObject2D::numberOfTriangles() const { return m_triangles.size() / 3; }

std::vector<double> MeshObject2D::getVertices() const { return MeshObjectCommon::getVertices(m_vertices); }

std::vector<uint32_t> MeshObject2D::getTriangles() const { return m_triangles; }

detail::ShapeInfo::GeometryShape MeshObject2D::shape() const {
  // should be consistent with MeshObject2D::GetObjectGeom
  return detail::ShapeInfo::GeometryShape::NOSHAPE;
}

const detail::ShapeInfo &MeshObject2D::shapeInfo() const {
  throw std::runtime_error("MeshObject2D::shapeInfo() is not implemented");
}

void MeshObject2D::GetObjectGeom(detail::ShapeInfo::GeometryShape &, std::vector<Kernel::V3D> &, double &, double &,
                                 double &) const {
  throw std::runtime_error("MeshObject2D::GetObjectGeom is not implemented");
}

void MeshObject2D::draw() const {
  if (m_handler == nullptr)
    return;
  // Render the Object
  m_handler->render();
}

void MeshObject2D::initDraw() const {
  if (m_handler == nullptr)
    return;
  // Render the Object
  m_handler->initialize();
}

} // namespace Mantid::Geometry
