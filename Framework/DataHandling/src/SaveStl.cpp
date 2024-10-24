// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveStl.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/DateAndTime.h"

#include <Poco/File.h>
#include <fstream>
#include <vector>
namespace Mantid::DataHandling {

namespace {

/**
 * Function to write out number of triangles to files.
 *
 * @param streamWriter The binary stream to write to.
 * @param numberTrianglesLong The number of triangles to write out.
 */
void writeNumberTriangles(Kernel::BinaryStreamWriter streamWriter, uint32_t numberTrianglesLong) {

  // Write the number of triangles.
  streamWriter << numberTrianglesLong;
}

/**
 * Function to write out a default normal, as we don't save this information in
 * mantid.
 *
 * @param streamWriter The binary stream to write to.
 */
void writeNormal(Kernel::BinaryStreamWriter StreamWriter) {
  float normal = 0;
  StreamWriter << normal;
  StreamWriter << normal;
  StreamWriter << normal;
}

} // namespace

/**
 * Function to write the header of the STL file, the header has no requirements
 * other than it being 80 bytes long and not being a valid ascii stl header, so
 * store creation info and scale here.
 *
 * @param streamWriter The binary stream to write to.
 */
void SaveStl::writeHeader(Kernel::BinaryStreamWriter streamWriter) {
  const std::string headerStart = "Binary STL File created using Mantid Environment:";
  const auto timeString = Types::Core::DateAndTime::getCurrentTime().toFormattedString("%Y-%b-%dT%H:%M:%S") + ":";
  std::string unitString;
  switch (m_scaleType) {
  case ScaleUnits::centimetres:
    unitString = "cm:";
    break;
  case ScaleUnits::millimetres:
    unitString = "mm:";
    break;
  case ScaleUnits::metres:
    unitString = "m:";
    break;
  default:
    // not mandatory to have units in header so just output blank
    unitString = ":";
  }
  const size_t emptySize = 80 - size_t(headerStart.size() + timeString.size() + 4 + unitString.size());
  streamWriter << headerStart + timeString + unitString + std::string(emptySize, ' ');
}

/**
 * Function to write out the full mesh to an stl binary file.
 * Handles the format of the file e.g.
 * Header->NumberOfTriangles->Normal->Triangle->attribute->Next Normal...
 */
void SaveStl::writeStl() {
  if (m_triangle.size() % 3 != 0) {
    throw std::runtime_error("Invalid mesh, could not save.");
  }
  std::ofstream myFile(m_filename.c_str(), std::ios::out | std::ios::binary);
  const auto numberOfTriangles = uint32_t(m_triangle.size() / 3);
  Kernel::BinaryStreamWriter streamWriter = Kernel::BinaryStreamWriter(myFile);
  writeHeader(streamWriter);
  writeNumberTriangles(streamWriter, numberOfTriangles);
  const uint16_t attributeByte = 0;
  for (uint32_t i = 0; i < numberOfTriangles; ++i) {
    writeNormal(streamWriter);
    writeTriangle(streamWriter, i * 3);
    // write out attribute, currently always zero
    streamWriter << attributeByte;
  }
  myFile.close();
}

/**
 * Function to write out an individual triangle with the scale removed.
 *
 * @param streamWriter The binary stream to write to.
 * @param triangle the first index of the triangles in the m_triangles vector.
 */
void SaveStl::writeTriangle(Kernel::BinaryStreamWriter streamWriter, uint32_t triangle) {
  // for each vertex
  for (int i = 0; i < 3; ++i) {
    // write out the coords
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

} // namespace Mantid::DataHandling
