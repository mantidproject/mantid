#include "MantidDataHandling/LoadBinStl.h"
#include "MantidAPI/FileProperty.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/BinaryStreamReader.h"
#include <Poco/File.h>
#include <fstream>
#include <iostream>

namespace Mantid {
namespace DataHandling {

bool LoadBinStl::isBinarySTL() {
  // each triangle is 50 bytes
  const uint32_t SIZE_OF_TRIANGLE = 50;
  Poco::File stlFile = Poco::File(m_filename);
  auto fileSize = stlFile.getSize();
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
    m_triangle.push_back(addSTLVertex(vec, m_verticies));
  }
}

bool areEqualVertices(Kernel::V3D const &v1, Kernel::V3D const &v2) {
  Kernel::V3D diff = v1 - v2;
  return diff.norm() < 1e-9; // This is 1 nanometre for a unit of a metre.
}

// Adds vertex to list if distinct and returns index to vertex added or equal
uint16_t addSTLVertex(Kernel::V3D &vertex, std::vector<Kernel::V3D> &vertices) {
  for (uint16_t i = 0; i < vertices.size(); ++i) {
    if (areEqualVertices(vertex, vertices[i])) {
      return i;
    }
  }
  vertices.push_back(vertex);
  uint16_t index = static_cast<uint16_t>(vertices.size() - 1);
  if (index != vertices.size() - 1) {
    throw std::runtime_error("Too many vertices in solid");
  }
  return index;
}

} // namespace DataHandling
} // namespace Mantid