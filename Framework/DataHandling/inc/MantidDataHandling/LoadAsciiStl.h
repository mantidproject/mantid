// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADASCIISTL_H_
#define MANTID_DATAHANDLING_LOADASCIISTL_H_
#include "MantidDataHandling/LoadStl.h"
#include <iosfwd>
namespace Mantid {

namespace Kernel {
class V3D;
}

namespace Geometry {
class MeshObject;
}
namespace DataHandling {

class DLLExport LoadAsciiStl : LoadStl {
public:
  LoadAsciiStl(std::string filename, ScaleUnits scaleType)
      : LoadStl(filename, scaleType) {}
  LoadAsciiStl(std::string filename, ScaleUnits scaleType,
               ReadMaterial::MaterialParameters params)
      : LoadStl(filename, scaleType, params) {}
  std::unique_ptr<Geometry::MeshObject> readStl() override;
  static bool isAsciiSTL(std::string filename);

private:
  int m_lineNumber = 0;
  bool readSTLTriangle(std::ifstream &file, Kernel::V3D &v1, Kernel::V3D &v2,
                       Kernel::V3D &v3);
  bool readSTLVertex(std::ifstream &file, Kernel::V3D &vertex);
  bool readSTLLine(std::ifstream &file, std::string const &type);
};

} // namespace DataHandling
} // namespace Mantid
#endif /* MANTID_DATAHANDLING_LOADASCIISTL_H_ */