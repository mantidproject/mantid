#include "MantidDataHandling/LoadBinStl.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/BinaryStreamReader.h"
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
    m_verticies.push_back(vec);
  }
  // add index of new verticies to triangle
  size_t newIndex = m_triangle.size();
  m_triangle.push_back(newIndex);
  m_triangle.push_back(newIndex + 1);
  m_triangle.push_back(newIndex + 2);
}

} // namespace DataHandling
} // namespace Mantid