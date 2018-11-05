#ifndef MANTID_DATAHANDLING_LOADBINARYSTL_H_
#define MANTID_DATAHANDLING_LOADBINARYSTL_H_
#include "MantidDataHandling/LoadStl.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/BinaryStreamReader.h"

namespace Mantid {
namespace DataHandling {

class DLLExport LoadBinaryStl : LoadStl {
public:
  static constexpr int HEADER_SIZE = 80;
  static constexpr uint32_t TRIANGLE_DATA_SIZE = 50;
  static constexpr uint32_t TRIANGLE_COUNT_DATA_SIZE = 4;
  static constexpr uint32_t VECTOR_DATA_SIZE = 12;
  LoadBinaryStl(std::string filename) : LoadStl(filename) {}
  std::unique_ptr<Geometry::MeshObject> readStl() override;
  bool isBinarySTL();

private:
  uint32_t getNumberTriangles(Kernel::BinaryStreamReader);
  void readTriangle(Kernel::BinaryStreamReader);
};

} // namespace DataHandling
} // namespace Mantid
#endif /* MANTID_DATAHANDLING_LOADBINARYSTL_H_ */