#ifndef MANTID_DATAHANDLING_LOADSTL_H_
#define MANTID_DATAHANDLING_LOADSTL_H_

#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/V3D.h"
#include <boost/functional/hash.hpp>
#include <unordered_set>
namespace {
Mantid::Kernel::Logger g_logstl("LoadStl");
}
namespace Mantid {
namespace DataHandling {

namespace {
struct hashV3D {
  size_t operator()(const std::pair<Kernel::V3D, uint32_t> &v) const {
    size_t seed = std::hash<double>{}(v.first.X());
    boost::hash_combine(seed, v.first.Y());
    boost::hash_combine(seed, v.first.Z());
    return seed;
  }
};
//we only care if the values are hashed to the same place, but unordered set requires a comparator, so always return true;
struct trueComparator {
  bool operator()(const std::pair<Kernel::V3D, uint32_t> &v1,
                 const std::pair<Kernel::V3D, uint32_t> &v2) const {
  (void)v1;
  (void)v2;
  return true;
 }
};

} // namespace
class DLLExport LoadStl {
public:
  LoadStl(std::string filename) : m_filename(filename) {}
  virtual std::unique_ptr<Geometry::MeshObject> readStl() = 0;

protected:
  std::pair<std::__detail::_Node_iterator<std::pair<Mantid::Kernel::V3D, unsigned int>, true, true>, bool> addSTLVertex(std::pair<Kernel::V3D, uint32_t> &vertex);
  bool areEqualVertices(Kernel::V3D const &v1, Kernel::V3D const &v2);
  void changeToVector();
  std::string m_filename;
  std::vector<uint32_t> m_triangle;
  std::vector<Kernel::V3D> m_verticies;
  std::unordered_set<std::pair<Kernel::V3D, uint32_t>, hashV3D, trueComparator> hashmap;
};

} // namespace DataHandling
} // namespace Mantid
#endif /* MANTID_DATAHANDLING_LOADSTL_H_ */