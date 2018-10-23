#include "MantidDataHandling/LoadStl.h"
#include "MantidKernel/V3D.h"
#include <Poco/File.h>

#include <functional>

namespace Mantid {
namespace DataHandling {

// Adds vertex to list if distinct and returns index to vertex added or equal
std::pair<std::__detail::_Node_iterator<
              std::pair<Mantid::Kernel::V3D, unsigned int>, true, true>,
          bool>
LoadStl::addSTLVertex(std::pair<Kernel::V3D, uint32_t> &vertex) {
  auto inserted = vertexSet.insert(vertex);
  return inserted;
}

bool LoadStl::areEqualVertices(Kernel::V3D const &v1, Kernel::V3D const &v2) {
  Kernel::V3D diff = v1 - v2;
  return diff.norm() < 1e-9; // This is 1 nanometre for a unit of a metre.
}

void LoadStl::changeToVector() {
  m_verticies.resize(vertexSet.size());
  for (auto const &mapValue : vertexSet) {
    m_verticies[mapValue.second] = mapValue.first;
  }
}
} // namespace DataHandling
} // namespace Mantid