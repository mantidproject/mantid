#include <fstream>
#include <iostream>
#include "MantidDataHandling/LoadBinStl.h"
#include "MantidKernel/BinaryStreamReader.h"


namespace Mantid {
namespace DataHandling {

void LoadBinStl::readStl(std::string filename) {
  std::ifstream myFile(filename.c_str(), std::ios::in | std::ios::binary);
  uint32_t numberTrianglesLong;
  Kernel::BinaryStreamReader streamReader = Kernel::BinaryStreamReader(myFile);
  // skip header
  streamReader.moveStreamToPosition(80);
  // Read the number of triangles
  streamReader >> numberTrianglesLong;
  uint32_t next = 96;
  const uint32_t STEPSIZE = 50;
  // now read in all the triangles
  for (uint32_t i = 0; i < numberTrianglesLong; i++) {
    next = next + i * STEPSIZE;
    readTriangle(streamReader);
  }
  myFile.close();
  return;
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
    verticies.push_back(vec);
  }
  // add index of new verticies to triangle
  size_t newIndex = triangle.size();
  triangle.push_back(newIndex);
  triangle.push_back(newIndex + 1);
  triangle.push_back(newIndex + 2);
}

} // namespace DataHandling
} // namespace Mantid