#include "MantidDataHandling/LoadStl.h"
#include <Poco/File.h>


namespace Mantid {
namespace DataHandling {

  // Adds vertex to list if distinct and returns index to vertex added or equal
  uint16_t LoadStl::addSTLVertex(Kernel::V3D &vertex){
  for (uint16_t i = 0; i < m_verticies.size(); ++i) {
    if (areEqualVertices(vertex, m_verticies[i])) {
      return i;
    }
  }
  m_verticies.push_back(vertex);
  uint16_t index = static_cast<uint16_t>(m_verticies.size() - 1);
  if (index != m_verticies.size() - 1) {
    throw std::runtime_error("Too many vertices in solid");
  }
  return index;
}

  
  bool LoadStl::areEqualVertices(Kernel::V3D const &v1, Kernel::V3D const &v2){
    Kernel::V3D diff = v1 - v2;
    return diff.norm() < 1e-9; // This is 1 nanometre for a unit of a metre.
  }
} 
}