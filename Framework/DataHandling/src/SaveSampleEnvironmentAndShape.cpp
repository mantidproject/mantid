// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveSampleEnvironmentAndShape.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/LoadShape.h"
#include "MantidDataHandling/SaveStl.h"
#include "MantidGeometry/Instrument/Container.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/MeshObject.h"

#include <memory>
namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveSampleEnvironmentAndShape)
using namespace Kernel;
using namespace API;
using namespace Geometry;
void SaveSampleEnvironmentAndShape::init() {
  auto wsValidator = boost::make_shared<InstrumentValidator>();

  // input workspace
  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input,
                                       wsValidator),
      "The name of the workspace containing the environment to save ");

  // Environment file
  const std::vector<std::string> extensions{".stl"};
  declareProperty(
      make_unique<FileProperty>("Filename", "", FileProperty::Save, extensions),
      "The path name of the file to save");

  // scale to use for stl
  declareProperty("Scale", "cm", "The scale of the stl: m, cm, or mm");
}

void SaveSampleEnvironmentAndShape::exec() {

  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  try {
    // track the total number of triangles and vertices
    size_t numTriangles = 0;
    size_t numVertices = 0;

    // Get the shape of the sample
    auto &sampleShape =
        dynamic_cast<const MeshObject &>(inputWS->sample().getShape());
    if (!sampleShape.hasValidShape()) {
      throw std::invalid_argument("Sample Shape is not complete");
    }
    numVertices += sampleShape.numberOfVertices();
    numTriangles += (sampleShape.numberOfTriangles() * 3);

    if (inputWS->sample().hasEnvironment()) {

      // get the environment
      auto environment = inputWS->sample().getEnvironment();

      // Setup vector to store the pieces of the environment
      std::vector<const MeshObject *> environmentPieces;
      auto numElements = environment.nelements();
      environmentPieces.reserve(numElements);

      // get the shape the container of the environment and add it to the vector
      bool environmentValid = true;
      environmentPieces.emplace_back(dynamic_cast<const MeshObject *>(
          &environment.getContainer().getShape()));
      environmentValid =
          environmentValid && environmentPieces[0]->hasValidShape();
      numVertices += environmentPieces[0]->numberOfVertices();
      numTriangles += (environmentPieces[0]->numberOfTriangles() * 3);

      // get the shapes of the components and add them to the vector
      for (size_t i = 1; i < numElements;
           ++i) { // start at 1 because element 0 is container
        const MeshObject *temp =
            dynamic_cast<const MeshObject *>(&environment.getComponent(i));
        numVertices += temp->numberOfVertices();
        numTriangles += (temp->numberOfTriangles() * 3);
        environmentValid = environmentValid && temp->hasValidShape();
        environmentPieces.emplace_back(temp);
      }
      if (!environmentValid) {
        throw std::invalid_argument("Environment Shape is not complete");
      }

      // setup vectors to store all triangles and vertices
      m_vertices.reserve(numVertices);
      m_triangle.reserve(numTriangles);

      // get the sample vertices and triangles and add them into the vector
      addMeshToVector(sampleShape);

      // keep track of the current number of vertices added
      size_t offset = sampleShape.numberOfVertices();

      // go through the environment, adding the triangles and vertices to the
      // vector
      for (size_t i = 0; i < environmentPieces.size(); ++i) {
        offset = addMeshToVector(*environmentPieces[i], offset);
      }
    } else {
      // setup vectors to store all triangles and vertices
      m_vertices.reserve(numVertices);
      m_triangle.reserve(numTriangles);

      // get the sample vertices and triangles and add them into the vector
      addMeshToVector(sampleShape);
    }
    // get the scale to use
    auto scale = getPropertyValue("Scale");
    auto scaleType = getScaleType(scale);

    // Save out the shape
    auto filename = getPropertyValue("Filename");
    SaveStl writer = SaveStl(filename, m_triangle, m_vertices, scaleType);
    writer.writeStl();
  } catch (std::bad_cast &) {
    // if bad_cast is thrown the sample or environment is not a mesh_object, and
    // therefore cannot be saved as an STL
    throw std::invalid_argument(
        "Attempted to Save out non mesh based Sample or Environment");
  }
}
void SaveSampleEnvironmentAndShape::addMeshToVector(const MeshObject &mesh) {
  auto vertices = mesh.getV3Ds();
  auto triangles = mesh.getTriangles();
  m_vertices.insert(std::end(m_vertices), std::begin(vertices),
                    std::end(vertices));
  m_triangle.insert(std::end(m_triangle), std::begin(triangles),
                    std::end(triangles));
}

size_t SaveSampleEnvironmentAndShape::addMeshToVector(
    const Mantid::Geometry::MeshObject &mesh, size_t offset) {
  auto vertices = mesh.getV3Ds();
  auto triangles = mesh.getTriangles();

  // increase the triangles by the offset, so they refer to the new index of
  // the vertices
  std::transform(std::begin(triangles), std::end(triangles),
                 std::begin(triangles),
                 [&offset](uint32_t &val) { return val + offset; });
  m_vertices.insert(std::end(m_vertices), std::begin(vertices),
                    std::end(vertices));
  m_triangle.insert(std::end(m_triangle), std::begin(triangles),
                    std::end(triangles));

  // add the newly added vertices to the offset
  return offset += vertices.size();
}
} // namespace DataHandling
} // namespace Mantid