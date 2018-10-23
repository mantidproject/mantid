// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadBinaryStl.h"
#include "MantidKernel/MultiThreaded.h"
#include <Poco/File.h>
#include <fstream>
#include <string>
#include <vector>
#include <string>
#include <boost/make_shared.hpp>
#include <chrono>
#include <boost/functional/hash.hpp>
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


// std::unique_ptr<Geometry::MeshObject> LoadBinaryStl::readStl() {
//   std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
//   std::ifstream myFile(m_filename.c_str(), std::ios::in | std::ios::binary);
//   myFile.seekg(0, std::ios_base::end);
//   const std::size_t size=myFile.tellg();
//   myFile.seekg(0, std::ios_base::beg);
//   char Header[80];
//   std::unique_ptr<char[]> buffer(new char[size-84]);
//   char temp[4];
//   myFile.read(Header, 80);
//   myFile.read(temp,4);
//   myFile.read(buffer.get(), size);
//   const uint32_t nTriLong = *((uint32_t*)temp);
//   g_logstl.debug(std::to_string(nTriLong) + " Triangles to read");
//   m_triangle.reserve(3*nTriLong);
//   m_verticies.reserve(3*nTriLong);
//   std::chrono::high_resolution_clock::time_point t2 ;
//   std::chrono::high_resolution_clock::time_point t3 ;
//   std::chrono::high_resolution_clock::time_point t4 ;
//   PARALLEL_FOR_NO_WSP_CHECK()
//   for(uint32_t i = 0;i<nTriLong;i++){
//       t2 = std::chrono::high_resolution_clock::now();
//       auto vec1 = makeV3D(buffer.get(),((i*TRIANGLE_DATA_SIZE)+(VECTOR_DATA_SIZE)));
//       auto vec2 = makeV3D(buffer.get(),((i*TRIANGLE_DATA_SIZE)+(2*VECTOR_DATA_SIZE)));
//       auto vec3 = makeV3D(buffer.get(),((i*TRIANGLE_DATA_SIZE)+(3*VECTOR_DATA_SIZE)));
//       t3 = std::chrono::high_resolution_clock::now();
//       #pragma omp critical
//       add3Vertex(vec1, vec2, vec3);    
//       t4 = std::chrono::high_resolution_clock::now();
//   }
  
//   auto duration2 = std::chrono::duration_cast<std::chrono::seconds>( t3 - t2 ).count();
//   auto duration3 = std::chrono::duration_cast<std::chrono::seconds>( t4 - t3 ).count();
//   g_logstl.information("Part 2 Took "+ std::to_string(duration2)+" seconds");
//   g_logstl.information("Part 3 Took "+ std::to_string(duration3)+" seconds");
//   m_verticies.shrink_to_fit();
//   m_triangle.shrink_to_fit();
//   // Close the file
//   myFile.close();
//   std::unique_ptr<Geometry::MeshObject> retVal =
//       std::unique_ptr<Geometry::MeshObject>(new Geometry::MeshObject(
//           std::move(m_triangle), std::move(m_verticies),
//           Mantid::Kernel::Material()));
//   std::chrono::high_resolution_clock::time_point t5 = std::chrono::high_resolution_clock::now();
//   auto duration = std::chrono::duration_cast<std::chrono::minutes>( t5 - t1 ).count();
//   g_logstl.information("Took "+ std::to_string(duration)+" minutes");
//   return retVal;
// }
// void LoadBinaryStl::add3Vertex(Kernel::V3D vec1, Kernel::V3D vec2, Kernel::V3D vec3){
//   m_triangle.emplace_back(addSTLVertex(vec1));
//   m_triangle.emplace_back(addSTLVertex(vec2));
//   m_triangle.emplace_back(addSTLVertex(vec3));
// }


std::unique_ptr<Geometry::MeshObject> LoadBinaryStl::readStl() {
std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
  std::ifstream myFile(m_filename.c_str(), std::ios::in | std::ios::binary);

  Kernel::BinaryStreamReader streamReader = Kernel::BinaryStreamReader(myFile);
  const auto numberTrianglesLong = getNumberTriangles(streamReader);
  uint32_t nextToRead =
      HEADER_SIZE + TRIANGLE_COUNT_DATA_SIZE + VECTOR_DATA_SIZE;
  // now read in all the triangles
  m_triangle.reserve(3*numberTrianglesLong);
  m_verticies.reserve(3*numberTrianglesLong);
  uint32_t vertexCount = 0;
  for (uint32_t i = 0; i < numberTrianglesLong; i++) {
    g_logstl.debug(std::to_string(i+1));
    // find next triangle, skipping the normal and attribute
    streamReader.moveStreamToPosition(nextToRead);
    readTriangle(streamReader,vertexCount);
    nextToRead += TRIANGLE_DATA_SIZE;
  }
  changeToVector();
  m_verticies.shrink_to_fit();
  m_triangle.shrink_to_fit();
  g_logstl.debug("Read All");
  myFile.close();
  std::unique_ptr<Geometry::MeshObject> retVal =
      std::unique_ptr<Geometry::MeshObject>(new Geometry::MeshObject(
          std::move(m_triangle), std::move(m_verticies),
          Mantid::Kernel::Material()));
  std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::minutes>( t2 - t1 ).count();
  g_logstl.information("Took "+ std::to_string(duration)+" minutes");
  return retVal;
}

void LoadBinaryStl::readTriangle(Kernel::BinaryStreamReader streamReader,uint32_t &vertexCount) {
  // read in the verticies
  for (int i = 0; i < 3; i++) {
    float xVal;
    float yVal;
    float zVal;
    streamReader >> xVal;
    streamReader >> yVal;
    streamReader >> zVal;
    Kernel::V3D vec = Kernel::V3D(double(xVal), double(yVal), double(zVal));
    auto vertexPair = std::pair<Kernel::V3D,uint32_t>(vec,vertexCount);
    auto emplacementResult = addSTLVertex(vertexPair);
    if(emplacementResult.second){
      vertexCount++;
    }
    m_triangle.emplace_back(emplacementResult.first->second);
    
  }
}



// Kernel::V3D LoadBinaryStl::makeV3D(char* facet, int index)
// {

//     char f1[4] = {facet[index],
//         facet[index+1],facet[index+2],facet[index+3]};

//     char f2[4] = {facet[index+4],
//         facet[index+5],facet[index+6],facet[index+7]};

//     char f3[4] = {facet[index+8],
//         facet[index+9],facet[index+10],facet[index+11]};

//     float xx = *((float*) f1 );
//     float yy = *((float*) f2 );
//     float zz = *((float*) f3 );
//     Kernel::V3D vec = Kernel::V3D(double(xx), double(yy), double(zz));
//     return vec;
// }

} // namespace DataHandling
} // namespace Mantid