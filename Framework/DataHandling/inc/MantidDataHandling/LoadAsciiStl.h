#ifndef MANTID_DATAHANDLING_LOADASCIISTL_H_
#define MANTID_DATAHANDLING_LOADASCIISTL_H_
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/V3D.h"
#include "MantidDataHandling/LoadStl.h"
#include <fstream>
namespace Mantid {
namespace DataHandling {

class DLLExport LoadAsciiStl :LoadStl{
public:
  LoadAsciiStl(std::string filename) : LoadStl(filename) {}
  std::unique_ptr<Geometry::MeshObject> readStl() override;
  bool isAsciiSTL();

private:
  bool readSTLTriangle(std::ifstream &file, Kernel::V3D &v1, Kernel::V3D &v2, Kernel::V3D &v3);
  bool readSTLVertex(std::ifstream &file, Kernel::V3D &vertex);
  bool readSTLLine(std::ifstream &file, std::string const &type);
};


} // namespace DataHandling
} // namespace Mantid
#endif /* MANTID_DATAHANDLING_LOADASCIISTL_H_ */