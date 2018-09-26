#include "MantidDataHandling/LoadAsciiStl.h"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <fstream>

namespace Mantid {
namespace DataHandling {

bool LoadAsciiStl::isAsciiSTL(){
  std::ifstream file(m_filename.c_str());
  std::string line;
  getline(file, line);
  boost::trim(line);
  if (line.size() < 5 || line.substr(0, 5) != "solid"){ 
    return false;
  } 
  return true;
}
std::unique_ptr<Geometry::MeshObject> LoadAsciiStl::readStl(){
  std::ifstream file(m_filename.c_str());
  std::string line;
  getline(file, line);
  name = line.substr(6, std::string::npos);
  getline(file, line);
  Kernel::V3D t1, t2, t3;
  while (readSTLTriangle(file, t1, t2, t3)) {
    // Add triangle if all 3 vertices are distinct
    if (!areEqualVertices(t1, t2) && !areEqualVertices(t1, t3) &&
        !areEqualVertices(t2, t3)) {
      m_triangle.push_back(addSTLVertex(t1));
      m_triangle.push_back(addSTLVertex(t2));
      m_triangle.push_back(addSTLVertex(t3));
    }
  }
  // Use efficient constructor of MeshObject
  std::unique_ptr<Geometry::MeshObject> retVal = std::unique_ptr<Geometry::MeshObject>(
      new Geometry::MeshObject(std::move(m_triangle), std::move(m_verticies),
                     Mantid::Kernel::Material()));
  return retVal;

}

bool LoadAsciiStl::readSTLTriangle(std::ifstream &file, Kernel::V3D &v1, Kernel::V3D &v2, Kernel::V3D &v3){
  if (readSTLLine(file, "facet") && readSTLLine(file, "outer loop")) {
    bool ok = (readSTLVertex(file, v1) && readSTLVertex(file, v2) &&
               readSTLVertex(file, v3));
    if (!ok) {
      throw std::runtime_error("Error on reading STL triangle");
    }
  } else {
    return false; // End of file
  }
  return readSTLLine(file, "endloop") && readSTLLine(file, "endfacet");
}

bool LoadAsciiStl::readSTLVertex(std::ifstream &file, Kernel::V3D &vertex) {
  std::string line;
  if (getline(file, line)) {
    boost::trim(line);
    std::vector<std::string> tokens;
    boost::split(tokens, line, boost::is_any_of(" "), boost::token_compress_on);
    if (tokens.size() == 4 && tokens[0] == "vertex") {
      vertex.setX(boost::lexical_cast<double>(tokens[1]));
      vertex.setY(boost::lexical_cast<double>(tokens[2]));
      vertex.setZ(boost::lexical_cast<double>(tokens[3]));
      return true;
    } else {
      throw std::runtime_error("Error on reading STL vertex");
    }
  }
  return false;
}

// Read, check and ignore line in STL file. Return true if line is read
bool LoadAsciiStl::readSTLLine(std::ifstream &file, std::string const &type) {
  std::string line;
  if (getline(file, line)) {
    boost::trim(line);
    if (line.size() < type.size() || line.substr(0, type.size()) != type) {
      // Before throwing, check for endsolid statment
      std::string type2 = "endsolid";
      if (line.size() < type2.size() || line.substr(0, type2.size()) != type2) {
        throw std::runtime_error("Expected STL line begining with " + type +
                                 " or " + type2);
      } else {
        return false; // ends reading at endsolid
      }
    }
    return true; // expected line read, then ignored
  } else {
    return false; // end of file
  }
}

}
}