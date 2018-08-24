#include "MantidGeometry/Objects/MeshObjectCommon.h"

namespace Mantid {
namespace Geometry {

namespace MeshObjectCommon {
std::vector<double> getVertices(const std::vector<Kernel::V3D> &vertices) {
  using Mantid::Kernel::V3D;
  std::vector<double> points;
  size_t nPoints = vertices.size();
  if (nPoints > 0) {
    points.resize(static_cast<std::size_t>(nPoints) * 3);
    for (size_t i = 0; i < nPoints; ++i) {
      V3D pnt = vertices[i];
      points[i * 3] = pnt.X();
      points[i * 3 + 1] = pnt.Y();
      points[i * 3 + 2] = pnt.Z();
    }
  }
  return points;
}
} // namespace MeshObjectCommon
} // namespace Geometry
} // namespace Mantid
