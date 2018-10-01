#ifndef MANTID_DATAHANDLING_LOADBinaryStl_H_
#define MANTID_DATAHANDLING_LOADBinaryStl_H_
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/BinaryStreamReader.h"
#include <MantidKernel/V3D.h>

namespace Mantid {
namespace DataHandling {

class DLLExport LoadBinaryStl {
public:
  LoadBinaryStl(std::string filename)
      : m_filename(filename), M_HEADER_SIZE(80), M_SIZE_OF_TRIANGLE(50),
        M_NUM_OF_TRIANGLES(4) {}
  std::unique_ptr<Geometry::MeshObject> readStl();
  bool isBinarySTL();

private:
  uint32_t getNumberTriangles(Kernel::BinaryStreamReader);
  void readTriangle(Kernel::BinaryStreamReader);
  std::string m_filename;

  std::vector<uint16_t> m_triangle;
  std::vector<Kernel::V3D> m_verticies;
  const int M_HEADER_SIZE;
  const uint32_t M_SIZE_OF_TRIANGLE;
  const uint32_t M_NUM_OF_TRIANGLES;
};
uint16_t addSTLVertex(Kernel::V3D &vertex, std::vector<Kernel::V3D> &vertices);
bool areEqualVertices(Kernel::V3D const &v1, Kernel::V3D const &v2);

} // namespace DataHandling
} // namespace Mantid
#endif /* MANTID_DATAHANDLING_LOADBinaryStl_H_ */