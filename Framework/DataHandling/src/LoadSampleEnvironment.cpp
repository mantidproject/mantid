// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadSampleEnvironment.h"
#include "MantidDataHandling/LoadAsciiStl.h"
#include "MantidDataHandling/LoadBinaryStl.h"
#include "MantidGeometry/Instrument/Container.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/MeshObject.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Exception.h"

#include <Poco/File.h>
#include <boost/algorithm/string.hpp>
#include <fstream>


namespace Mantid {
namespace DataHandling {

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadSampleEnvironment)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void LoadSampleEnvironment::init() {
  auto wsValidator = boost::make_shared<API::InstrumentValidator>();
  ;

  // input workspace
  declareProperty(make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "The name of the workspace containing the instrument to add "
                  "the Environment");

  // Environment file
  const std::vector<std::string> extensions{".stl"};
  declareProperty(
      make_unique<FileProperty>("Filename", "", FileProperty::Load, extensions),
      "The path name of the file containing the Environment");

  // Output workspace
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name of the workspace that will contain the loaded "
                  "Environment of the sample");

  // Environment Name
  declareProperty("EnvironmentName", "Environment");

  // New Can or Add
  declareProperty("Add", false);

  // Vector to translate mesh
  declareProperty(
      make_unique<ArrayProperty<double>>("TranslationVector", "0,0,0"),
      "Vector by which to translate the loaded environment");

  // Matrix to rotate mesh
  declareProperty(make_unique<ArrayProperty<double>>(
                      "rotationMatrix", "1.0,0.0,0.0,0.0,1.0,0.0,1.0,0.0,0.0"),
                  "Rotation Matrix in format x1,x2,x3,y1,y2,y3,z1,z2,z3");
}

void LoadSampleEnvironment::exec() {

  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  if (inputWS != outputWS) {
    outputWS = inputWS->clone();
  }

  const std::string filename = getProperty("Filename");
  const std::ifstream file(filename.c_str());
  if (!file) {
    g_log.error("Unable to open file: " + filename);
    throw Exception::FileError("Unable to open file: ", filename);
  }

  boost::shared_ptr<MeshObject> environmentMesh = nullptr;

  auto asciiStlReader = LoadAsciiStl(filename);
  auto binaryStlReader = LoadBinaryStl(filename);
  if (binaryStlReader.isBinarySTL(filename)) {
    environmentMesh = binaryStlReader.readStl();
  } else if (asciiStlReader.isAsciiSTL(filename)) {
    environmentMesh = asciiStlReader.readStl();
  } else {
    throw Kernel::Exception::ParseError(
        "Could not read file, did not match either STL Format", filename, 0);
  }
  environmentMesh = translate(environmentMesh);
  environmentMesh = rotate(environmentMesh);

  std::string name = getProperty("EnvironmentName");
  const bool add = getProperty("Add");
  Sample &sample = outputWS->mutableSample();
  std::unique_ptr<Geometry::SampleEnvironment> environment = nullptr;
  if (add) {
    environment =
        std::make_unique<Geometry::SampleEnvironment>(sample.getEnvironment());
    environment->add(environmentMesh);
  } else {
    auto can = boost::make_shared<Container>(environmentMesh);
    environment = std::make_unique<Geometry::SampleEnvironment>(name, can);
  }
  // Put Environment into sample.

  const std::string debugString =
      "Enviroment has: " + std::to_string(environment->nelements()) +
      " elements.";
  sample.setEnvironment(std::move(environment));

  auto translatedVertices = environmentMesh->getVertices();
  int i = 0;
  for (double vertex : translatedVertices) {
    i++;
    g_log.information(std::to_string(vertex));
    if (i % 3 == 0) {
      g_log.information("\n");
    }
  }
  // Set output workspace

  setProperty("OutputWorkspace", outputWS);
  g_log.debug(debugString);
}

boost::shared_ptr<MeshObject> LoadSampleEnvironment::translate(
    boost::shared_ptr<MeshObject> environmentMesh) {
  const std::vector<double> translationVector =
      getProperty("TranslationVector");
  std::vector<double> checkVector = std::vector<double>(3, 0.0);
  if (translationVector != checkVector) {
    if (translationVector.size() != 3) {
      throw std::runtime_error(
          "Invalid Translation vector, must have exactly 3 dimensions");
    }
    Kernel::V3D translate = Kernel::V3D(
        translationVector[0], translationVector[1], translationVector[2]);
    environmentMesh->translate(translate);
  }
  return environmentMesh;
}

boost::shared_ptr<MeshObject>
LoadSampleEnvironment::rotate(boost::shared_ptr<MeshObject> environmentMesh) {
  const std::vector<double> rotationMatrix = getProperty("RotationMatrix");
  double valueList[] = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0};
  std::vector<double> checkVector1 =
      std::vector<double>(std::begin(valueList), std::end(valueList));
  if (rotationMatrix != checkVector1) {
    if (rotationMatrix.size() != 9) {

      throw std::runtime_error(
          "Invalid Rotation Matrix, must have exactly 9 values, not: " +
          std::to_string(rotationMatrix.size()));
    }
    Kernel::Matrix<double> rotation = Kernel::Matrix<double>(rotationMatrix);
    environmentMesh->rotate(rotation);
  }
  return environmentMesh;
}

} // namespace DataHandling
} // namespace Mantid
