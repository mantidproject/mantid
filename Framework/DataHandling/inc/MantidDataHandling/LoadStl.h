// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidDataHandling/LoadSampleShape.h"
#include "MantidDataHandling/LoadSingleMesh.h"
#include "MantidDataHandling/ReadMaterial.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/V3D.h"

#include <boost/functional/hash.hpp>
#include <functional>
#include <unordered_set>
#include <utility>

#include <utility>

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
    size_t seed = 0;
    boost::hash_combine(seed, v.first.X());
    boost::hash_combine(seed, v.first.Y());
    boost::hash_combine(seed, v.first.Z());
    return seed;
  }
};

struct V3DTrueComparator {
  bool operator()(const std::pair<Kernel::V3D, uint32_t> &v1, const std::pair<Kernel::V3D, uint32_t> &v2) const {
    const Kernel::V3D diff = v1.first - v2.first;
    const double nanoMetre = 1e-9;
    return diff.norm() < nanoMetre;
  }
};

class MANTID_DATAHANDLING_DLL LoadStl : public LoadSingleMesh {
public:
  LoadStl(std::string filename, std::ios_base::openmode mode, ScaleUnits scaleType)
      : LoadSingleMesh(std::move(filename), mode, scaleType), m_setMaterial(false) {}
  LoadStl(std::string filename, std::ios_base::openmode mode, ScaleUnits scaleType,
          ReadMaterial::MaterialParameters params)
      : LoadSingleMesh(std::move(filename), mode, scaleType), m_setMaterial(true), m_params(std::move(params)) {}
  virtual ~LoadStl() override = default;

protected:
  bool areEqualVertices(Kernel::V3D const &v1, Kernel::V3D const &v2) const;
  void changeToVector();
  bool m_setMaterial;
  ReadMaterial::MaterialParameters m_params;
  std::unordered_set<std::pair<Kernel::V3D, uint32_t>, HashV3DPair, V3DTrueComparator> vertexSet;
};

} // namespace DataHandling
} // namespace Mantid
