// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveStl.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/DateAndTime.h"

#include <Poco/File.h>
#include <boost/functional/hash.hpp>
#include <fstream>
#include <vector>
namespace Mantid {
namespace DataHandling {

namespace {
void writeHeader(Kernel::BinaryStreamWriter streamWriter) {
  const std::string header =
      "Binary STL File created using Mantid Environment:";
  streamWriter << header;
  const auto time =
      Types::Core::DateAndTime::getCurrentTime().toISO8601String() + ":";
  streamWriter << time;
  // TODO add scale type
  const size_t emptySize = 80 - size_t(header.size() + time.size());
  streamWriter << std::string(emptySize, ' ');
}

void writeNumberTriangles(Kernel::BinaryStreamWriter streamWriter,
                          uint32_t numberTrianglesLong) {

  // Write the number of triangles
  streamWriter << numberTrianglesLong;
}

void writeNormal(Kernel::BinaryStreamWriter StreamWriter) {
  float normal = 0;
  StreamWriter << normal;
  StreamWriter << normal;
  StreamWriter << normal;
}

} // namespace

void SaveStl::writeStl() {
  std::ofstream myFile(m_filename.c_str(), std::ios::out | std::ios::binary);
  const uint32_t numberOfTriangles = uint32_t(m_triangle.size() / 3);
  Kernel::BinaryStreamWriter streamWriter = Kernel::BinaryStreamWriter(myFile);
  writeHeader(streamWriter);
  writeNumberTriangles(streamWriter, numberOfTriangles);
  const uint16_t attributeByte = 0;
  for (uint32_t i = 0; i < numberOfTriangles; ++i) {
    // Write out 0, 0, 0 to act as default normal vector
    writeNormal(streamWriter);
    // write out vertices of triangle
    writeTriangle(streamWriter, i * 3);

    streamWriter << attributeByte;
  }
}

void SaveStl::writeTriangle(Kernel::BinaryStreamWriter streamWriter,
                            uint32_t triangle) {
  // get each vertex
  for (int i = 0; i < 3; ++i) {
    uint32_t index = m_triangle[triangle + i];
    Kernel::V3D vertex = m_vertices[index];
    float xVal = removeScale(vertex.X());
    float yVal = removeScale(vertex.Y());
    float zVal = removeScale(vertex.Z());
    streamWriter << xVal;
    streamWriter << yVal;
    streamWriter << zVal;
  }
}

float SaveStl::removeScale(double value) {
  switch (m_units) {
  case ScaleUnits::centimetres:
    value = value * 100;
    break;
  case ScaleUnits::millimetres:
    value = value * 1000;
    break;
  case ScaleUnits::metres:
    break;
  }
  return float(value);
}

} // namespace DataHandling
} // namespace Mantid