// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveSampleEnvironmentAndShape.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#ifdef ENABLE_LIB3MF
#include "MantidDataHandling/Mantid3MFFileIO.h"
#endif
#include "MantidDataHandling/SaveStl.h"
#include "MantidGeometry/Instrument/Container.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/MeshObject.h"

#include <memory>

#include <Poco/Path.h>

namespace Mantid::DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveSampleEnvironmentAndShape)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void SaveSampleEnvironmentAndShape::init() {
  auto wsValidator = std::make_shared<InstrumentValidator>();

  // input workspace
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, wsValidator),
                  "The name of the workspace containing the environment to save ");

  // Environment file
  const std::vector<std::string> extensions{".stl", ".3mf"};
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Save, extensions),
                  "The path name of the file to save");

  // scale to use for stl
  declareProperty("Scale", "m", "The scale of the stl: m, cm, or mm");
}

void SaveSampleEnvironmentAndShape::mergeSampleEnvironmentIntoSingleMesh(
    const MeshObject &sampleShape, const std::vector<const Geometry::MeshObject *> &environmentPieces) {

  if (environmentPieces.size() > 0) {

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
    // get the sample vertices and triangles and add them into the vector
    addMeshToVector(sampleShape);
  }
}

void SaveSampleEnvironmentAndShape::exec() {
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  // track the total number of triangles and vertices
  size_t numTriangles = 0;
  size_t numVertices = 0;

  // Get the shape of the sample
  const auto &sampleShape = toMeshObject(inputWS->sample().getShape());
  if (!sampleShape.hasValidShape()) {
    throw std::invalid_argument("Sample Shape is not complete");
  }
  numVertices += sampleShape.numberOfVertices();
  numTriangles += (sampleShape.numberOfTriangles() * 3);

  // Setup vector to store the pieces of the environment
  std::vector<const MeshObject *> environmentPieces;

  if (inputWS->sample().hasEnvironment()) {
    // get the environment
    auto environment = inputWS->sample().getEnvironment();

    auto numElements = environment.nelements();
    environmentPieces.reserve(numElements);

    // get the shape the container of the environment and add it to the vector
    bool environmentValid = true;
    environmentPieces.emplace_back(&toMeshObject(environment.getContainer().getShape()));
    environmentValid = environmentValid && environmentPieces[0]->hasValidShape();
    numVertices += environmentPieces[0]->numberOfVertices();
    numTriangles += (environmentPieces[0]->numberOfTriangles() * 3);

    // get the shapes of the components and add them to the vector
    for (size_t i = 1; i < numElements; ++i) { // start at 1 because element 0 is container
      const MeshObject *temp = &toMeshObject(environment.getComponent(i));
      numVertices += temp->numberOfVertices();
      numTriangles += (temp->numberOfTriangles() * 3);
      environmentValid = environmentValid && temp->hasValidShape();
      environmentPieces.emplace_back(temp);
    }
    if (!environmentValid) {
      throw std::invalid_argument("Environment Shape is not complete");
    }
  }
  // get the scale to use
  auto scale = getPropertyValue("Scale");
  auto scaleType = getScaleTypeFromStr(scale);

  // Save out the shape
  auto filename = getPropertyValue("Filename");
  std::string fileExt = Poco::Path(filename).getExtension();
  std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), toupper);

  if (fileExt == "STL") {
    // setup vectors to store all triangles and vertices
    m_vertices.reserve(numVertices);
    m_triangle.reserve(numTriangles);
    mergeSampleEnvironmentIntoSingleMesh(sampleShape, environmentPieces);
    SaveStl writer = SaveStl(filename, m_triangle, m_vertices, scaleType);
    writer.writeStl();
  } else {
#ifdef ENABLE_LIB3MF
    Mantid3MFFileIO Mesh3MF;
    auto samplePtr = MeshObject_const_sptr(&sampleShape);
    Mesh3MF.writeMeshObjects(environmentPieces, samplePtr, scaleType);
    Mesh3MF.saveFile(filename);
#else
    throw std::runtime_error("3MF format not supported on this platform");
#endif
  }
}

/**
 * Function to add the triangles and vertices of a mesh object into a vector to
 * allow combining with multiple meshes.
 *
 * @param mesh The mesh object to get the triangles and vertices from.
 */
void SaveSampleEnvironmentAndShape::addMeshToVector(const MeshObject &mesh) {
  auto vertices = mesh.getV3Ds();
  auto triangles = mesh.getTriangles();
  m_vertices.insert(std::end(m_vertices), std::begin(vertices), std::end(vertices));
  m_triangle.insert(std::end(m_triangle), std::begin(triangles), std::end(triangles));
}

/**
 * Function to add the triangles and vertices of a mesh object into a vector to
 * allow combining with multiple meshes, with an offset to take into account
 * meshes that are already added.
 *
 * @param mesh The mesh object to get the triangles and vertices from.
 * @param offset The value to offset the triangles by.
 *
 * @return size_t the new offset value to use if this needs to be used again.
 */
size_t SaveSampleEnvironmentAndShape::addMeshToVector(const Mantid::Geometry::MeshObject &mesh, size_t offset) {
  auto vertices = mesh.getV3Ds();
  auto triangles = mesh.getTriangles();

  // increase the triangles by the offset, so they refer to the new index of
  // the vertices
  std::transform(std::begin(triangles), std::end(triangles), std::begin(triangles),
                 [&offset](const uint32_t &val) { return val + uint32_t(offset); });
  m_vertices.insert(std::end(m_vertices), std::begin(vertices), std::end(vertices));
  m_triangle.insert(std::end(m_triangle), std::begin(triangles), std::end(triangles));

  // add the newly added vertices to the offset
  return offset += vertices.size();
}

/**
 * Function to convert an IObject to a mesh, and throw if this can't be done.
 *
 * @param object The IObject to convert.
 * @return const Geometry::MeshObject& The converted MeshObject.
 */
const Geometry::MeshObject &toMeshObject(const Geometry::IObject &object) {

  try {
    return dynamic_cast<const Geometry::MeshObject &>(object);
  } catch (const std::bad_cast &) {
    // if bad_cast is thrown the sample or environment is not a mesh_object, and
    // therefore cannot be saved as an STL
    throw std::invalid_argument("Attempted to Save out non mesh based Sample or Environment");
  }
}
} // namespace Mantid::DataHandling
