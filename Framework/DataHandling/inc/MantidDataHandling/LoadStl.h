// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADSTL_H_
#define MANTID_DATAHANDLING_LOADSTL_H_

#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/V3D.h"
#include <boost/functional/hash.hpp>
#include <functional>
#include <unordered_set>
namespace {
Mantid::Kernel::Logger g_logstl("LoadStl");
}
namespace Mantid {
namespace DataHandling {

struct HashV3DPair {
  size_t operator()(const std::pair<Kernel::V3D, uint32_t> &v) const {
    size_t seed = std::hash<double>{}(v.first.X());
    boost::hash_combine(seed, v.first.Y());
    boost::hash_combine(seed, v.first.Z());
    return seed;
  }
};
// we only care if the values are hashed to the same place, but unordered set
// requires a comparator, so always return true;
struct V3DTrueComparator {
  bool operator()(const std::pair<Kernel::V3D, uint32_t> &v1,
                  const std::pair<Kernel::V3D, uint32_t> &v2) const {
    (void)v1;
    (void)v2;
    return true;
  }
};

class DLLExport LoadStl {
public:
  LoadStl(std::string filename) : m_filename(filename) {}
  virtual std::unique_ptr<Geometry::MeshObject> readStl() = 0;

protected:
  bool areEqualVertices(Kernel::V3D const &v1, Kernel::V3D const &v2);
  void changeToVector();
  std::string m_filename;
  std::vector<uint32_t> m_triangle;
  std::vector<Kernel::V3D> m_verticies;
  std::unordered_set<std::pair<Kernel::V3D, uint32_t>, HashV3DPair,
                     V3DTrueComparator>
      vertexSet;
};

} // namespace DataHandling
} // namespace Mantid
#endif /* MANTID_DATAHANDLING_LOADSTL_H_ */