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
             denom;
    if (d != 0) {
      in_plane = false;
      break;
    }
  }
  return in_plane;
}

void validatePointsCoplanar(const std::vector<Mantid::Kernel::V3D> &vertices) {
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
}
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
  CoplanarChecks::validatePointsCoplanar(m_vertices);
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

double MeshObject2D::volume() const {
  return 0; // Volume is always 0 for a plane
}

} // namespace Geometry
} // namespace Mantid
