#include "MantidDataHandling/LoadBinStl.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/BinaryStreamReader.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include <fstream>
#include <iostream>

namespace Mantid {
namespace DataHandling {

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadBinStl)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadBinStl::name() const { return "LoadBinStl"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadBinStl::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadBinStl::category() const {
  return "DataHandling\\LoadBinStl";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadBinStl::summary() const {
  return "Load mesh from binary stl file for diffraction";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadBinStl::init() {
  const std::vector<std::string> exts{".stl"};
  declareProperty(Kernel::make_unique<Mantid::API::FileProperty>(
                      "Filename", "", Mantid::API::FileProperty::Load, exts),
                  "The path name of the file containing the shape");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadBinStl::exec() {
  auto filename = getProperty("Filename");
  readStl(filename);  
}

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
    //find next triangle, skipping the normal and attribute
    next += (i * STEPSIZE);
    streamReader.moveStreamToPosition(next);
    readTriangle(streamReader);
  }
  myFile.close();
  std::unique_ptr<Geometry::MeshObject> retVal = std::unique_ptr<Geometry::MeshObject>(
      new Geometry::MeshObject(std::move(m_triangle), std::move(m_verticies),
                     Mantid::Kernel::Material()));
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
    // add index of new vertex to triangle
    m_triangle.push_back(addSTLVertex(vec,m_verticies));
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