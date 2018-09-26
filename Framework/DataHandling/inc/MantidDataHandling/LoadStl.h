#ifndef MANTID_DATAHANDLING_LOADSTL_H_
#define MANTID_DATAHANDLING_LOADSTL_H_
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/BinaryStreamReader.h"
#include <MantidKernel/V3D.h>

namespace Mantid {
namespace DataHandling {

class DLLExport LoadStl {
public:
  LoadStl(std::string filename) : m_filename(filename) {}
  virtual std::unique_ptr<Geometry::MeshObject> readStl()=0;


private:
  uint32_t getNumberTriangles(Kernel::BinaryStreamReader);
  void readTriangle(Kernel::BinaryStreamReader);
  std::string m_filename;

  std::vector<uint16_t> m_triangle;
  std::vector<Kernel::V3D> m_verticies;
};
uint16_t addSTLVertex(Kernel::V3D &vertex, std::vector<Kernel::V3D> &vertices);
bool areEqualVertices(Kernel::V3D const &v1, Kernel::V3D const &v2);

} // namespace DataHandling
} // namespace Mantid
#endif /* MANTID_DATAHANDLING_LOADSTL_H_ */