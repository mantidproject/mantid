// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADOFF_H_
#define MANTID_DATAHANDLING_LOADOFF_H_
#include "MantidDataHandling/LoadShape.h"
#include "MantidDataHandling/ReadMaterial.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/V3D.h"

#include <fstream>
#include <functional>
#include <unordered_set>

namespace Mantid {

namespace Geometry {
class MeshObject;
}
namespace DataHandling {
namespace {
Mantid::Kernel::Logger g_log("LoadOff");
}
class DLLExport LoadOff : public LoadShape {
public:
  LoadOff(std::string filename, ScaleUnits scaleType);
  std::unique_ptr<Geometry::MeshObject> readOFFshape();

private:
  std::unique_ptr<Geometry::MeshObject> readOFFMeshObject();
  bool getOFFline(std::string &line);
  void readOFFVertices();
  void readOFFTriangles();

  std::ifstream m_file;
  uint32_t m_nVertices;
  uint32_t m_nTriangles;
};
} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADOFF_H_ */