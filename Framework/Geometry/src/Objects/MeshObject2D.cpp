#include "MantidGeometry/Objects/MeshObject2D.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Material.h"

namespace Mantid {
namespace Geometry {

namespace CoplanarChecks {
bool sufficientPoints(const std::vector<Mantid::Kernel::V3D> &vertices) {
  return vertices.size() >= 3; // Not a plane with < 3 points
}

Kernel::V3D surfaceNormal(const std::vector<Kernel::V3D> &vertices) {
  auto v0 = vertices[1] - vertices[0];
  Kernel::V3D normal{0, 0, 0};
  // Look for normal amongst first 3 non-colinear points
  auto v1 = vertices[2] - vertices[1];
  for (size_t i = 1; i < vertices.size() - 1; ++i) {
    v1 = vertices[i + 1] - vertices[i];
    normal = v0.cross_prod(v1);
    if (normal.norm2() != 0) {
      break;
    }
  }
  return normal;
}

bool allCoplanar(const std::vector<Kernel::V3D> &vertices,
                 const Kernel::V3D normal) {
  bool in_plane = true;
  auto v0 = vertices[1] - vertices[0];
  const auto nx = normal[0];
  const auto ny = normal[1];
  const auto nz = normal[2];
  auto denom = normal.norm2();
  auto k = nx * v0.X() + ny * v0.Y() + nz * v0.Z();

  for (size_t i = 0; i < vertices.size(); ++i) {
    auto d = (nx * vertices[i].X() + ny * vertices[i].Y() +
              nz * vertices[i].Z() - k) /
             denom; // TODO, division not necessary
    if (d != 0) {
      in_plane = false;
      break;
    }
  }
  return in_plane;
}

Kernel::V3D
validatePointsCoplanar(const std::vector<Mantid::Kernel::V3D> &vertices) {
  if (!sufficientPoints(vertices))
    throw std::invalid_argument("Insufficient vertices to create a plane");

  Mantid::Kernel::V3D normal = CoplanarChecks::surfaceNormal(vertices);
  // Check that a valid normal was found amonst collection of vertices
  if (normal.norm2() == 0) {
    // If all points are colinear. Not a plane.
    throw std::invalid_argument(
        "All vertices are colinear. This does not define a plane");
  }

  if (!allCoplanar(vertices, normal))
    throw std::invalid_argument("Vertices do not define a plane");
  return normal;
}
}

/**
 * @brief isOnTriangle
 * @param p : point to test
 * @param a : first vertex of triangle
 * @param b : second vertex of triangle
 * @param c : thrid vertex of triangle
 * @return
 */
bool MeshObject2D::isOnTriangle(const Kernel::V3D &point, const Kernel::V3D &a,
                                const Kernel::V3D &b, const Kernel::V3D &c) {

  // in change of basis, barycentric coordinates p = A + u*v0 + v*v1. v0 and
  // v1 are basis vectors
  // rewrite as v0 = u*v0 + v*v1
  // i) v0.v0 = u*v0.v0 + v*v1.v0
  // ii) v0.v1 = u*v0.v1 + v*v1.v1
  // solve for u, v and check u and v >= 0 and u+v <=1

  auto v0 = c - a;
  auto v1 = b - a;
  auto v2 = point - a;

  // Compute dot products
  auto dot00 = v0.scalar_prod(v0);
  auto dot01 = v0.scalar_prod(v1);
  auto dot02 = v0.scalar_prod(v2);
  auto dot11 = v1.scalar_prod(v1);
  auto dot12 = v1.scalar_prod(v2);

  // Compute barycentric coordinates
  auto invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
  auto u = (dot11 * dot02 - dot01 * dot12) * invDenom;
  auto v = (dot00 * dot12 - dot01 * dot02) * invDenom;

  // Check if point is in or on triangle
  return (u >= 0) && (v >= 0) && (u + v <= 1);
}

bool MeshObject2D::pointsCoplanar(
    const std::vector<Mantid::Kernel::V3D> &vertices) {
  if (!CoplanarChecks::sufficientPoints(vertices))
    return false;

  Mantid::Kernel::V3D normal = CoplanarChecks::surfaceNormal(vertices);
  // Check that a valid normal was found amonst collection of vertices
  if (normal.norm2() == 0) {
    // If all points are colinear. Not a plane.
    return false;
  }

  return CoplanarChecks::allCoplanar(vertices, normal);
}

MeshObject2D::MeshObject2D(const std::vector<uint16_t> &faces,
                           const std::vector<Kernel::V3D> &vertices,
                           const Kernel::Material &material)
    : m_triangles(faces), m_vertices(vertices), m_material(material) {
  initialize();
}

MeshObject2D::MeshObject2D(std::vector<uint16_t> &&faces,
                           std::vector<Kernel::V3D> &&vertices,
                           const Kernel::Material &&material)
    : m_triangles(std::move(faces)), m_vertices(std::move(vertices)),
      m_material(std::move(material)) {
  initialize();
}

void MeshObject2D::initialize() {
  auto surfaceNormal = CoplanarChecks::validatePointsCoplanar(m_vertices);
  const auto v0 = m_vertices[0];
  const auto n_mag = surfaceNormal.norm();
  PlaneParameters parameters;
  parameters.a = surfaceNormal.X() / n_mag;
  parameters.b = surfaceNormal.Y() / n_mag;
  parameters.c = surfaceNormal.Z() / n_mag;
  parameters.k =
      parameters.a * v0.X() + parameters.b * v0.Y() + parameters.c * v0.Z();
  m_planeParameters = parameters;

  if (m_vertices.size() > std::numeric_limits<uint16_t>::max()) {
    throw std::invalid_argument(
        "Too many vertices (" + std::to_string(m_vertices.size()) +
        "). MeshObject cannot have more than 65535 vertices.");
  }
  // m_handler = boost::make_shared<GeometryHandler>(this);
}
bool MeshObject2D::hasValidShape() const {
  // 3 or more points define a plane.
  return (m_triangles.size() >= 1 && m_vertices.size() >= 3);
}

double MeshObject2D::distanceToPlane(const Kernel::V3D &point) const {
  return (point.X() * m_planeParameters.a) + (point.Y() * m_planeParameters.b) +
         (point.Z() * m_planeParameters.c) + m_planeParameters.k;
}

/**
 * Check that the point is on the plane AND that it is inside or on one of the
 * triangles that defines the plane. Both must be true.
 * @param point : Point to test
 * @return : True only if the point is valid
 */
bool MeshObject2D::isValid(const Kernel::V3D &point) const {

  if (distanceToPlane(point) == 0) {
    for (size_t i = 0; i < m_vertices.size(); i += 3) {
      if (isOnTriangle(point, m_vertices[i], m_vertices[i + 1],
                       m_vertices[i + 2]))
        return true;
    }
  }
  return false;
}

bool MeshObject2D::isOnSide(const Kernel::V3D &point) const {
  return isValid(point);
}

double MeshObject2D::volume() const {
  return 0; // Volume is always 0 for a plane
}

} // namespace Geometry
} // namespace Mantid
