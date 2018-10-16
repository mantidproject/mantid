#include "MantidDataHandling/LoadBinaryStl.h"
#include <Poco/File.h>
#include <fstream>
#include <string>
#include "MantidKernel/MultiThreaded.h"
namespace Mantid {
namespace DataHandling {

bool LoadBinaryStl::isBinarySTL() {
  Poco::File stlFile = Poco::File(m_filename);
  auto fileSize = stlFile.getSize();
  if (fileSize < HEADER_SIZE + TRIANGLE_COUNT_DATA_SIZE) {
    // File is smaller than header plus number of triangles, cannot be binary
    // format stl
    return false;
  }
  uint32_t numberTrianglesLong;
  std::ifstream myFile(m_filename.c_str(), std::ios::in | std::ios::binary);
  Kernel::BinaryStreamReader streamReader = Kernel::BinaryStreamReader(myFile);
  numberTrianglesLong = getNumberTriangles(streamReader);
  myFile.close();
  if (!(fileSize == (HEADER_SIZE + TRIANGLE_COUNT_DATA_SIZE +
                     (numberTrianglesLong * TRIANGLE_DATA_SIZE)))) {
    // File is not the Header plus the number of triangles it claims to be long,
    // invalid binary Stl
    return false;
  }
  // if both conditions pass, file is likely binary stl
  return true;
}

uint32_t
LoadBinaryStl::getNumberTriangles(Kernel::BinaryStreamReader streamReader) {
  uint32_t numberTrianglesLong;
  // skip header
  streamReader.moveStreamToPosition(HEADER_SIZE);
  // Read the number of triangles
  streamReader >> numberTrianglesLong;
  return numberTrianglesLong;
}

std::unique_ptr<Geometry::MeshObject> LoadBinaryStl::readStl() {
  std::ifstream myFile(m_filename.c_str(), std::ios::in | std::ios::binary);

  Kernel::BinaryStreamReader streamReader = Kernel::BinaryStreamReader(myFile);
  const auto numberTrianglesLong = getNumberTriangles(streamReader);
  uint32_t nextToRead =
      HEADER_SIZE + TRIANGLE_COUNT_DATA_SIZE + VECTOR_DATA_SIZE;
  // now read in all the triangles
  m_triangle.reserve(3*numberTrianglesLong);
  m_verticies.reserve(3*numberTrianglesLong);
  for (uint32_t i = 0; i < numberTrianglesLong; i++) {
    g_logstl.debug(std::to_string(i+1));
    // find next triangle, skipping the normal and attribute
    streamReader.moveStreamToPosition(nextToRead);
    readTriangle(streamReader);
    nextToRead += TRIANGLE_DATA_SIZE;
  }
  m_verticies.shrink_to_fit();
  m_triangle.shrink_to_fit();
  g_logstl.debug("Read All");
  myFile.close();
  std::unique_ptr<Geometry::MeshObject> retVal =
      std::unique_ptr<Geometry::MeshObject>(new Geometry::MeshObject(
          std::move(m_triangle), std::move(m_verticies),
          Mantid::Kernel::Material()));
  return retVal;
}

void LoadBinaryStl::readTriangle(Kernel::BinaryStreamReader streamReader) {
  // read in the verticies
  //g_logstl.debug(std::to_string(m_triangle.size()));
  //g_logstl.debug(std::to_string(m_triangle.capacity()));
  //g_logstl.debug(std::to_string(m_verticies.capacity()));
  for (int i = 0; i < 3; i++) {
    float xVal;
    float yVal;
    float zVal;
    streamReader >> xVal;
    //g_logstl.debug("Read V1");
    streamReader >> yVal;
    //g_logstl.debug("Read V2");
    streamReader >> zVal;
    //g_logstl.debug("Read V3");
    Kernel::V3D vec = Kernel::V3D(double(xVal), double(yVal), double(zVal));
    // add index of new vertex to triangle
    m_triangle.emplace_back(addSTLVertex(vec));
    //g_logstl.debug("Add");

  }
}

} // namespace DataHandling
} // namespace Mantid