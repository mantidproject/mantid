// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADSTL_H_
#define MANTID_DATAHANDLING_LOADSTL_H_
#include "MantidDataHandling/LoadSampleShape.h"
#include "MantidDataHandling/LoadShape.h"
#include "MantidDataHandling/ReadMaterial.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/V3D.h"

#include <boost/functional/hash.hpp>
#include <functional>
#include <unordered_set>
namespace {
Mantid::Kernel::Logger g_logstl("LoadStl");
}
namespace Mantid {

namespace Geometry {
class MeshObject;
}
namespace DataHandling {

struct HashV3DPair {
  size_t operator()(const std::pair<Kernel::V3D, uint32_t> &v) const {
    size_t seed = std::hash<double>{}(v.first.X());
    boost::hash_combine(seed, v.first.Y());
    boost::hash_combine(seed, v.first.Z());
    return seed;
  }
};

struct V3DTrueComparator {
  bool operator()(const std::pair<Kernel::V3D, uint32_t> &v1,
                  const std::pair<Kernel::V3D, uint32_t> &v2) const {
    const Kernel::V3D diff = v1.first - v2.first;
    const double nanoMetre = 1e-9;
    return diff.norm() < nanoMetre;
  }
};

class DLLExport LoadStl : public LoadShape {
public:
  LoadStl(std::string filename, ScaleUnits scaleType)
      : LoadShape(scaleType), m_filename(filename), m_setMaterial(false) {}
  LoadStl(std::string filename, ScaleUnits scaleType,
          ReadMaterial::MaterialParameters params)
      : LoadShape(scaleType), m_filename(filename), m_setMaterial(true),
        m_params(params) {}
  virtual std::unique_ptr<Geometry::MeshObject> readStl() = 0;
  virtual ~LoadStl() = default;

protected:
  bool areEqualVertices(Kernel::V3D const &v1, Kernel::V3D const &v2) const;
  void changeToVector();
  const std::string m_filename;
  bool m_setMaterial;
  ReadMaterial::MaterialParameters m_params;
  std::unordered_set<std::pair<Kernel::V3D, uint32_t>, HashV3DPair,
                     V3DTrueComparator>
      vertexSet;
};

} // namespace DataHandling
} // namespace Mantid
#endif /* MANTID_DATAHANDLING_LOADSTL_H_ */
