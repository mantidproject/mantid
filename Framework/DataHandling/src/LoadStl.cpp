#include "MantidDataHandling/LoadStl.h"
#include "MantidKernel/V3D.h"
#include <Poco/File.h>

#include <functional>

namespace Mantid {
namespace DataHandling {

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