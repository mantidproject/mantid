// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadBinaryStl.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/BinaryStreamReader.h"
#include "MantidKernel/MultiThreaded.h"
#include <Poco/File.h>
#include <boost/functional/hash.hpp>
#include <boost/make_shared.hpp>
#include <chrono>
#include <fstream>
#include <string>
#include <vector>
namespace Mantid {
namespace DataHandling {

namespace {
uint32_t getNumberTriangles(Kernel::BinaryStreamReader streamReader,
                            const int header) {
  uint32_t numberTrianglesLong;
  // skip header
  streamReader.moveStreamToPosition(header);
  // Read the number of triangles
  streamReader >> numberTrianglesLong;
  return numberTrianglesLong;
}
} // namespace

bool LoadBinaryStl::isBinarySTL(std::string filename) {
  Poco::File stlFile = Poco::File(filename);
  if (!stlFile.exists()) {
    // if the file cannot be read then it is not a valid binary Stl File
    return false;
  }
  auto fileSize = stlFile.getSize();
  if (fileSize < HEADER_SIZE + TRIANGLE_COUNT_DATA_SIZE) {
    // File is smaller than header plus number of triangles, cannot be binary
    // format stl
    return false;
  }
  uint32_t numberTrianglesLong;
  std::ifstream myFile(filename.c_str(), std::ios::in | std::ios::binary);
  Kernel::BinaryStreamReader streamReader = Kernel::BinaryStreamReader(myFile);
  numberTrianglesLong = getNumberTriangles(streamReader, HEADER_SIZE);
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

std::unique_ptr<Geometry::MeshObject> LoadBinaryStl::readStl() {
  std::ifstream myFile(m_filename.c_str(), std::ios::in | std::ios::binary);

  Kernel::BinaryStreamReader streamReader = Kernel::BinaryStreamReader(myFile);
  const auto numberTrianglesLong =
      getNumberTriangles(streamReader, HEADER_SIZE);
  uint32_t nextToRead =
      HEADER_SIZE + TRIANGLE_COUNT_DATA_SIZE + VECTOR_DATA_SIZE;
  // now read in all the triangles
  m_triangle.reserve(3 * numberTrianglesLong);
  m_vertices.reserve(3 * numberTrianglesLong);
  g_logstl.debug("Began reading " + std::to_string(numberTrianglesLong) +
                 " triangles.");
  uint32_t vertexCount = 0;
  for (uint32_t i = 0; i < numberTrianglesLong; i++) {

    // find next triangle, skipping the normal and attribute
    streamReader.moveStreamToPosition(nextToRead);
    readTriangle(streamReader, vertexCount);
    nextToRead += TRIANGLE_DATA_SIZE;
  }
  changeToVector();
  m_vertices.shrink_to_fit();
  m_triangle.shrink_to_fit();
  g_logstl.debug("Read All");
  myFile.close();
  Mantid::Kernel::Material material;
  if (m_setMaterial) {
    g_logstl.information("Setting Material");
    ReadMaterial reader;
    reader.setMaterialParameters(m_params);
    material = *(reader.buildMaterial());
  } else {
    material = Mantid::Kernel::Material();
  }
  auto retVal = std::make_unique<Geometry::MeshObject>(
      std::move(m_triangle), std::move(m_vertices), material);
  return retVal;
}

void LoadBinaryStl::readTriangle(Kernel::BinaryStreamReader streamReader,
                                 uint32_t &vertexCount) {
  // read in the verticies
  for (int i = 0; i < 3; i++) {
    float xVal;
    float yVal;
    float zVal;
    streamReader >> xVal;
    streamReader >> yVal;
    streamReader >> zVal;
    // mesh objects are stored in metres, so convert to that
    auto vec = createScaledV3D(xVal, yVal, zVal);
    auto vertexPair = std::pair<Kernel::V3D, uint32_t>(vec, vertexCount);
    auto emplacementResult = vertexSet.insert(vertexPair);
    // check if the value was new to the map and increase the value to assign to
    // the next if so
    if (emplacementResult.second) {
      vertexCount++;
    }
    m_triangle.emplace_back(emplacementResult.first->second);
  }
}

} // namespace DataHandling
} // namespace Mantid