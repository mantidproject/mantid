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
#include "MantidDataHandling/SaveStl.h"
#include "MantidGeometry/Instrument/Container.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/MeshObject.h"

#include <memory>
namespace Mantid {
namespace DataHandling {
enum class ScaleUnits { metres, centimetres, millimetres };
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
  auto filename = getPropertyValue("Filename");
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  try {
    // track the total number of triangles and vertices
    size_t numTriangles = 0;
    size_t numVertices = 0;

    // Get the shape of the sample
    auto &sampleShape =
        dynamic_cast<const MeshObject &>(inputWS->sample().getShape());
    auto environment = inputWS->sample().getEnvironment();
    auto numElements = environment.nelements();
    numVertices += sampleShape.numberOfVertices();
    numTriangles += (sampleShape.numberOfTriangles() * 3);

    // Setup vector to store the pieves of the environment
    std::vector<const MeshObject *> environmentPieces;
    environmentPieces.reserve(numElements);

    // get the shape the container of the environment and add it to the vector
    environmentPieces.emplace_back(dynamic_cast<const MeshObject *>(
        &environment.getContainer().getShape()));
    numVertices += environmentPieces[0]->numberOfVertices();
    numTriangles += (environmentPieces[0]->numberOfTriangles() * 3);

    // get the shapes of the components and add them to the vector
    for (size_t i = 1; i < numElements;
         ++i) { // start at 1 because element 0 is container
      const MeshObject *temp =
          dynamic_cast<const MeshObject *>(&environment.getComponent(i));
      numVertices += temp->numberOfVertices();
      numTriangles += (temp->numberOfTriangles() * 3);
      environmentPieces.emplace_back(temp);
    }

    // setup vectors to store all triangles and vertices
    std::vector<V3D> vertices;
    std::vector<uint32_t> triangle;
    vertices.reserve(numVertices);
    triangle.reserve(numTriangles);

    // get the sample vertices and triangles and add them into the vector
    auto shapeVertices = sampleShape.getV3Ds();
    auto shapeTriangles = sampleShape.getTriangles();
    vertices.insert(std::end(vertices), std::begin(shapeVertices),
                    std::end(shapeVertices));
    triangle.insert(std::end(triangle), std::begin(shapeTriangles),
                    std::end(shapeTriangles));

    // keep track of the current number of vertices added
    size_t offset = sampleShape.numberOfVertices();

    // go through the environment, adding the triangles and vertices to the
    // vector
    for (size_t i = 0; i < environmentPieces.size(); ++i) {
      std::vector<Kernel::V3D> tempVertices = environmentPieces[i]->getV3Ds();
      std::vector<uint32_t> tempTriangles =
          environmentPieces[i]->getTriangles();

      // increase the triangles by the offset, so they refer to the new index of
      // the vertices
      std::transform(std::begin(tempTriangles), std::end(tempTriangles),
                     std::begin(tempTriangles),
                     [&offset](uint32_t &val) { return val + offset; });
      vertices.insert(std::end(vertices), std::begin(tempVertices),
                      std::end(tempVertices));
      triangle.insert(std::end(triangle), std::begin(tempTriangles),
                      std::end(tempTriangles));

      // add the newly added vertices to the offset
      offset += tempVertices.size();
    }

    // Save out the shape
    SaveStl writer = SaveStl(filename, triangle, vertices, ScaleUnits::metres);
    writer.writeStl();
  } catch (std::bad_cast &) {
    // if bad_cast is thrown the sample or environment is not a mesh_object, and
    // therefore cannot be saved as an STL
    throw std::invalid_argument(
        "Attempted to Save out non-mesh based Sample Shape");
  }
}
} // namespace DataHandling
} // namespace Mantid