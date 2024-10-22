// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidDataHandling/LoadStl.h"
#include <iosfwd>
#include <utility>

namespace Mantid {

namespace Kernel {
class V3D;
}

namespace Geometry {
class MeshObject;
}
namespace DataHandling {

class MANTID_DATAHANDLING_DLL LoadAsciiStl : public LoadStl {
public:
  LoadAsciiStl(std::string filename, ScaleUnits scaleType)
      : LoadStl(std::move(filename), std::ios_base::in, scaleType) {}
  LoadAsciiStl(std::string filename, ScaleUnits scaleType, ReadMaterial::MaterialParameters params)
      : LoadStl(std::move(filename), std::ios_base::in, scaleType, std::move(params)) {}
  std::unique_ptr<Geometry::MeshObject> readShape() override;
  static bool isAsciiSTL(const std::string &filename);

private:
  int m_lineNumber = 0;
  bool readSTLTriangle(std::ifstream &file, Kernel::V3D &v1, Kernel::V3D &v2, Kernel::V3D &v3);
  bool readSTLVertex(std::ifstream &file, Kernel::V3D &vertex);
  bool readSTLLine(std::ifstream &file, std::string const &type);
};

} // namespace DataHandling
} // namespace Mantid
