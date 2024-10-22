// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadOff.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/V3D.h"

#include <boost/algorithm/string.hpp>

namespace Mantid::DataHandling {

LoadOff::LoadOff(const std::string &filename, ScaleUnits scaleType)
    : LoadSingleMesh(filename, std::ios_base::in, scaleType) {}

bool LoadOff::getOFFline(std::string &line) {
  // Get line from OFF file ignoring blank lines and comments
  // The line output is ready trimmed
  if (!getline(m_file, line)) {
    return false;
  }
  boost::trim(line);
  while (line.empty() || line.substr(0, 1) == "#") {
    if (!getline(m_file, line)) {
      return false;
    }
    boost::trim(line);
  }
  return true;
}

void LoadOff::readOFFVertices() {
  std::string line;
  for (uint32_t i = 0; i < m_nVertices; i++) {
    if (getOFFline(line)) {
      std::vector<std::string> tokens;
      boost::split(tokens, line, boost::is_any_of(" "), boost::token_compress_on);
      if (tokens.size() == 3) {
        auto vertex = createScaledV3D(boost::lexical_cast<double>(tokens[0]),  // x
                                      boost::lexical_cast<double>(tokens[1]),  // y
                                      boost::lexical_cast<double>(tokens[2])); // z
        m_vertices.emplace_back(vertex);
      } else {
        throw std::runtime_error("Error on reading OFF vertex");
      }
    } else {
      throw std::runtime_error("Unexpected end of file, while reading OFF m_vertices");
    }
  }
}

void LoadOff::readOFFTriangles() {
  std::string line;
  uint32_t t1, t2, t3;
  size_t nFaceVertices;
  for (uint32_t i = 0; i < m_nTriangles; i++) {
    if (getOFFline(line)) {
      std::vector<std::string> tokens;
      boost::split(tokens, line, boost::is_any_of(" "), boost::token_compress_on);
      if (tokens.size() >= 4) {
        nFaceVertices = boost::lexical_cast<size_t>(tokens[0]);
        if (nFaceVertices == 3) {
          t1 = boost::lexical_cast<uint32_t>(tokens[1]);
          t2 = boost::lexical_cast<uint32_t>(tokens[2]);
          t3 = boost::lexical_cast<uint32_t>(tokens[3]);
        } else {
          throw std::runtime_error("OFF face is not a triangle.");
        }
        m_triangle.emplace_back(t1);
        m_triangle.emplace_back(t2);
        m_triangle.emplace_back(t3);
      } else {
        throw std::runtime_error("Error on reading OFF triangle");
      }
    } else {
      throw std::runtime_error("Unexpected end of file, while reading OFF triangles");
    }
  }
}

std::unique_ptr<Geometry::MeshObject> LoadOff::readOFFMeshObject() {

  std::string line;
  // Get number of vetrtices and faces
  if (getOFFline(line)) {
    std::vector<std::string> tokens;
    boost::split(tokens, line, boost::is_any_of(" "), boost::token_compress_on);
    if (tokens.size() == 3) {
      try {
        m_nVertices = boost::lexical_cast<uint32_t>(tokens[0]);
        m_nTriangles = boost::lexical_cast<uint32_t>(tokens[1]);
      } catch (...) {
        throw std::runtime_error("Error in reading numbers of OFF m_vertices and "
                                 "triangles, which may be too large");
      }
      m_vertices.reserve(m_nVertices);
      m_triangle.reserve(3 * m_nTriangles);
    } else {
      throw std::runtime_error("Error on reading OFF number of m_vertices, faces & edges");
    }
  } else {
    throw std::runtime_error("Unexpected end of OFF file");
  }
  readOFFVertices();
  readOFFTriangles();

  // Use efficient constructor of MeshObject
  std::unique_ptr<Geometry::MeshObject> retVal =
      std::make_unique<Geometry::MeshObject>(std::move(m_triangle), std::move(m_vertices), Mantid::Kernel::Material());
  return retVal;
}

std::unique_ptr<Geometry::MeshObject> LoadOff::readShape() {
  std::string line;
  if (getOFFline(line)) {
    if (line != "OFF") {
      throw std::runtime_error("Expected first line to be 'OFF' keyword");
    }
    // Read OFF shape
    return readOFFMeshObject();
  }
  return nullptr;
}
} // namespace Mantid::DataHandling
