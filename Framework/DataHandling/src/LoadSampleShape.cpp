// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadSampleShape.h"
#include "MantidDataHandling/LoadAsciiStl.h"
#include "MantidDataHandling/LoadBinaryStl.h"
#include "MantidGeometry/Objects/MeshObject.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"

#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Exception.h"

#include <Poco/File.h>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <stdio.h>

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadSampleShape)

using namespace Kernel;
using namespace API;
using namespace Geometry;

namespace {

bool getOFFline(std::ifstream &file, std::string &line) {
  // Get line from OFF file ignoring blank lines and comments
  // The line output is ready trimmed
  if (!getline(file, line)) {
    return false;
  }
  boost::trim(line);
  while (line.empty() || line.substr(0, 1) == "#") {
    if (!getline(file, line)) {
      return false;
    }
    boost::trim(line);
  }
  return true;
}

void readOFFVertices(std::ifstream &file, uint32_t nVertices,
                     std::vector<V3D> &vertices) {
  std::string line;
  for (uint32_t i = 0; i < nVertices; i++) {
    if (getOFFline(file, line)) {
      std::vector<std::string> tokens;
      boost::split(tokens, line, boost::is_any_of(" "),
                   boost::token_compress_on);
      if (tokens.size() == 3) {
        vertices.emplace_back(boost::lexical_cast<double>(tokens[0]),  // x
                              boost::lexical_cast<double>(tokens[1]),  // y
                              boost::lexical_cast<double>(tokens[2])); // z
      } else {
        throw std::runtime_error("Error on reading OFF vertex");
      }
    } else {
      throw std::runtime_error(
          "Unexpected end of file, while reading OFF vertices");
    }
  }
}

void readOFFTriangles(std::ifstream &file, uint32_t nTriangles,
                      std::vector<uint32_t> &triangleIndices) {
  std::string line;
  uint32_t t1, t2, t3;
  size_t nFaceVertices;
  for (uint32_t i = 0; i < nTriangles; i++) {
    if (getOFFline(file, line)) {
      std::vector<std::string> tokens;
      boost::split(tokens, line, boost::is_any_of(" "),
                   boost::token_compress_on);
      if (tokens.size() >= 4) {
        nFaceVertices = boost::lexical_cast<size_t>(tokens[0]);
        if (nFaceVertices == 3) {
          t1 = boost::lexical_cast<uint32_t>(tokens[1]);
          t2 = boost::lexical_cast<uint32_t>(tokens[2]);
          t3 = boost::lexical_cast<uint32_t>(tokens[3]);
        } else {
          throw std::runtime_error("OFF face is not a triangle.");
        }
        triangleIndices.emplace_back(t1);
        triangleIndices.emplace_back(t2);
        triangleIndices.emplace_back(t3);
      } else {
        throw std::runtime_error("Error on reading OFF triangle");
      }
    } else {
      throw std::runtime_error(
          "Unexpected end of file, while reading OFF triangles");
    }
  }
}

std::unique_ptr<MeshObject> readOFFMeshObject(std::ifstream &file) {
  std::vector<uint32_t> triangleIndices;
  std::vector<V3D> vertices;
  uint32_t nVertices;
  uint32_t nTriangles;

  std::string line;
  // Get number of vetrtices and faces
  if (getOFFline(file, line)) {
    std::vector<std::string> tokens;
    boost::split(tokens, line, boost::is_any_of(" "), boost::token_compress_on);
    if (tokens.size() == 3) {
      try {
        nVertices = boost::lexical_cast<uint32_t>(tokens[0]);
        nTriangles = boost::lexical_cast<uint32_t>(tokens[1]);
      } catch (...) {
        throw std::runtime_error("Error in reading numbers of OFF vertices and "
                                 "triangles, which may be too large");
      }
      vertices.reserve(nVertices);
      triangleIndices.reserve(3 * nTriangles);
    } else {
      throw std::runtime_error(
          "Error on reading OFF number of vertices, faces & edges");
    }
  } else {
    throw std::runtime_error("Unexpected end of OFF file");
  }
  readOFFVertices(file, nVertices, vertices);
  readOFFTriangles(file, nTriangles, triangleIndices);

  // Use efficient constructor of MeshObject
  std::unique_ptr<MeshObject> retVal = std::unique_ptr<MeshObject>(
      new MeshObject(std::move(triangleIndices), std::move(vertices),
                     Mantid::Kernel::Material()));
  return retVal;
}

std::unique_ptr<Geometry::MeshObject> readOFFshape(std::ifstream &file) {
  std::string line;
  if (getOFFline(file, line)) {
    if (line != "OFF") {
      throw std::runtime_error("Expected first line to be 'OFF' keyword");
    }
    // Read OFF shape
    return readOFFMeshObject(file);
  }
  return nullptr;
}

} // end anonymous namespace

void LoadSampleShape::init() {
  auto wsValidator = boost::make_shared<API::InstrumentValidator>();
  ;

  // input workspace
  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input,
                                       wsValidator),
      "The name of the workspace containing the instrument to add the shape");

  // shape file
  const std::vector<std::string> extensions{".stl", ".off"};
  declareProperty(
      make_unique<FileProperty>("Filename", "", FileProperty::Load, extensions),
      "The path name of the file containing the shape");

  // Output workspace
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name of the workspace that will contain the loaded "
                  "shape of the sample");
}

void LoadSampleShape::exec() {

  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  if (inputWS != outputWS) {
    outputWS = inputWS->clone();
  }

  const std::string filename = getProperty("Filename");
  std::ifstream file(filename.c_str());
  if (!file) {
    g_log.error("Unable to open file: " + filename);
    throw Exception::FileError("Unable to open file: ", filename);
  }

  const std::string filetype = filename.substr(filename.size() - 3);

  boost::shared_ptr<MeshObject> shape = nullptr;
  if (filetype == "off") {
    shape = readOFFshape(file);
  } else /* stl */ {
    auto asciiStlReader = LoadAsciiStl(filename);
    auto binaryStlReader = LoadBinaryStl(filename);
    if (binaryStlReader.isBinarySTL(filename)) {
      shape = binaryStlReader.readStl();
    } else if (asciiStlReader.isAsciiSTL(filename)) {
      shape = asciiStlReader.readStl();
    } else {
      throw Kernel::Exception::ParseError(
          "Could not read file, did not match either STL Format", filename, 0);
    }
  }

  // Put shape into sample.
  Sample &sample = outputWS->mutableSample();
  sample.setShape(shape);

  // Set output workspace
  setProperty("OutputWorkspace", outputWS);
}

} // namespace DataHandling
} // namespace Mantid
