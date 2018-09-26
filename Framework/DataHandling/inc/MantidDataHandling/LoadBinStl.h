#ifndef MANTID_DATAHANDLING_LOADBINSTL_H_
#define MANTID_DATAHANDLING_LOADBINSTL_H_
#include "MantidDataHandling/LoadStl.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/BinaryStreamReader.h"
namespace Mantid {
namespace DataHandling {

class DLLExport LoadBinStl : LoadStl {
public:
  LoadBinStl(std::string filename) : LoadStl(filename) {}
  std::unique_ptr<Geometry::MeshObject> readStl() override;
  bool isBinarySTL();

private:
  uint32_t getNumberTriangles(Kernel::BinaryStreamReader);
  void readTriangle(Kernel::BinaryStreamReader);
};

} // namespace DataHandling
} // namespace Mantid
#endif /* MANTID_DATAHANDLING_LOADBINSTL_H_ */