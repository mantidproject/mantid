#include "MantidDataHandling/LoadBinStl.h"
#include <Poco/File.h>
#include <fstream>


namespace Mantid {
namespace DataHandling {

bool LoadBinStl::isBinarySTL() {
  // each triangle is 50 bytes
  const uint32_t SIZE_OF_TRIANGLE = 50;
  Poco::File stlFile = Poco::File(m_filename);
  unsigned long fileSize = stlFile.getSize();
  if (fileSize < 84) {
    // File is smaller than header plus number of triangles, cannot be binary
    // format stl
    return false;
  }
  uint32_t numberTrianglesLong;
  std::ifstream myFile(m_filename.c_str(), std::ios::in | std::ios::binary);
  Kernel::BinaryStreamReader streamReader = Kernel::BinaryStreamReader(myFile);
  numberTrianglesLong = getNumberTriangles(streamReader);
  myFile.close();
  if (!(fileSize == (84 + (numberTrianglesLong * SIZE_OF_TRIANGLE)))) {
    // File is not the Header plus the number of triangles it claims to be long,
    // invalid binary Stl
    return false;
  }
  // if both conditions pass, file is likely binary stl
  return true;
}

uint32_t
LoadBinStl::getNumberTriangles(Kernel::BinaryStreamReader streamReader) {
  uint32_t numberTrianglesLong;
  // skip header
  streamReader.moveStreamToPosition(80);
  // Read the number of triangles
  streamReader >> numberTrianglesLong;
  return numberTrianglesLong;
}

std::unique_ptr<Geometry::MeshObject> LoadBinStl::readStl() {
  std::ifstream myFile(m_filename.c_str(), std::ios::in | std::ios::binary);
  uint32_t numberTrianglesLong;
  Kernel::BinaryStreamReader streamReader = Kernel::BinaryStreamReader(myFile);
  numberTrianglesLong = getNumberTriangles(streamReader);
  uint32_t next = 96;
  const int STEPSIZE = 50;

  // now read in all the triangles
  for (uint32_t i = 0; i < numberTrianglesLong; i++) {
    // find next triangle, skipping the normal and attribute
    streamReader.moveStreamToPosition(next);
    readTriangle(streamReader);
    next += STEPSIZE;
  }
  myFile.close();
  std::unique_ptr<Geometry::MeshObject> retVal =
      std::unique_ptr<Geometry::MeshObject>(new Geometry::MeshObject(
          std::move(m_triangle), std::move(m_verticies),
          Mantid::Kernel::Material()));
  return retVal;
}

void LoadBinStl::readTriangle(Kernel::BinaryStreamReader streamReader) {
  // read in the verticies
  for (int i = 0; i < 3; i++) {
    float xVal;
    float yVal;
    float zVal;
    streamReader >> xVal;
    streamReader >> yVal;
    streamReader >> zVal;
    Kernel::V3D vec = Kernel::V3D(double(xVal), double(yVal), double(zVal));
    // add index of new vertex to triangle
    m_triangle.push_back(addSTLVertex(vec));
  }
}



} // namespace DataHandling
} // namespace Mantid