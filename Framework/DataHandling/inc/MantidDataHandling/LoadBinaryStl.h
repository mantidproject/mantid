// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADBINARYSTL_H_
#define MANTID_DATAHANDLING_LOADBINARYSTL_H_
#include "MantidDataHandling/LoadStl.h"

namespace Mantid {
namespace Kernel {
class BinaryStreamReader;
}

namespace Geometry {
class MeshObject;
}
namespace DataHandling {

class DLLExport LoadBinaryStl : LoadStl {
public:
  static constexpr int HEADER_SIZE = 80;
  static constexpr uint32_t TRIANGLE_DATA_SIZE = 50;
  static constexpr uint32_t TRIANGLE_COUNT_DATA_SIZE = 4;
  static constexpr uint32_t VECTOR_DATA_SIZE = 12;
  LoadBinaryStl(std::string filename) : LoadStl(filename) {}
  LoadBinaryStl(std::string filename, ReadMaterial::MaterialParameters params)
      : LoadStl(filename, params) {}
  std::unique_ptr<Geometry::MeshObject> readStl() override;
  static bool isBinarySTL(std::string filename);

private:
  void readTriangle(Kernel::BinaryStreamReader, uint32_t &vertexCount);
};

} // namespace DataHandling
} // namespace Mantid
#endif /* MANTID_DATAHANDLING_LOADBINARYSTL_H_ */