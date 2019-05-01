// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadSampleShape.h"
#include "MantidDataHandling/LoadAsciiStl.h"
#include "MantidDataHandling/LoadBinaryStl.h"
#include "MantidDataHandling/LoadOff.h"
#include "MantidGeometry/Objects/MeshObject.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidKernel/ArrayProperty.h"

#include "MantidGeometry/Instrument.h"

#include <Poco/File.h>

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadSampleShape)

using namespace Kernel;
using namespace API;
using namespace Geometry;

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

  // scale to use for stl
  declareProperty("Scale", "cm", "The scale of the stl: m, cm, or mm");

  // Rotation angles
  declareProperty("XDegrees",0.0,"The degrees to rotate on the x axis by");
  declareProperty("YDegrees",0.0,"The degrees to rotate on the y axis by");
  declareProperty("ZDegrees",0.0,"The degrees to rotate on the z axis by");

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

  const std::string filetype = filename.substr(filename.size() - 3);

  boost::shared_ptr<MeshObject> shape = nullptr;
  const std::string scaleProperty = getPropertyValue("Scale");
  const ScaleUnits scaleType = getScaleType(scaleProperty);

  if (filetype == "off") {
    auto offReader = LoadOff(filename, scaleType);
    shape = offReader.readOFFshape();
  } else /* stl */ {

    auto asciiStlReader = LoadAsciiStl(filename, scaleType);
    auto binaryStlReader = LoadBinaryStl(filename, scaleType);
    if (binaryStlReader.isBinarySTL(filename)) {
      shape = binaryStlReader.readStl();
    } else if (asciiStlReader.isAsciiSTL(filename)) {
      shape = asciiStlReader.readStl();
    } else {
      throw Kernel::Exception::ParseError(
          "Could not read file, did not match either STL Format", filename, 0);
    }
  }
  // rotate shape
  shape = rotate(shape);

  // Put shape into sample.
  Sample &sample = outputWS->mutableSample();
  sample.setShape(shape);

  // Set output workspace
  setProperty("OutputWorkspace", outputWS);
}

/**
 * Rotates the Shape by a provided matrix
 * @param ShapeMesh The Shape to rotate
 * @returns a shared pointer to the newly rotated Shape
 */
boost::shared_ptr<MeshObject>
LoadSampleShape::rotate(boost::shared_ptr<MeshObject> sampleMesh) {
  const std::vector<double> rotationMatrix = generateMatrix();
  sampleMesh->rotate(rotationMatrix);
  
  return sampleMesh;
}

Matrix<double> LoadSampleShape::generateMatrix() {
  Kernel::Matrix<double> xMatrix = generateXRotation();
  Kernel::Matrix<double> yMatrix = generateYRotation();
  Kernel::Matrix<double> zMatrix = generateZRotation();
  xMatrix.print();
  g_log.notice("x");
  yMatrix.print();
  g_log.notice("y");
  zMatrix.print();
  g_log.notice("z");
  (xMatrix * yMatrix * zMatrix).print();
  return xMatrix * yMatrix * zMatrix;
}

Matrix<double> LoadSampleShape::generateXRotation() {
  const double xRotation = getProperty("xDegrees");
  const double sinX = sin(xRotation);
  const double cosX = cos(xRotation);
  std::vector<double> matrixList = {1, 0, 0, 0, cosX, sinX, 0, -sinX, cosX};
  return Kernel::Matrix<double>(matrixList);
}

Matrix<double> LoadSampleShape::generateYRotation() {
  const double yRotation = getProperty("yDegrees");
  const double sinY = sin(yRotation);
  const double cosY = cos(yRotation);
  std::vector<double> matrixList = {cosY, 0, sinY, 0, 1, 0, -sinY, 0, cosY};
  return Kernel::Matrix<double>(matrixList);
}

Matrix<double> LoadSampleShape::generateZRotation() {
  const double zRotation = getProperty("zDegrees");
  const double sinZ = sin(zRotation);
  const double cosZ = cos(zRotation);
  std::vector<double> matrixList = {cosZ, -sinZ, 0, sinZ, cosZ, 0, 0, 0, 1};
  return Kernel::Matrix<double>(matrixList);
}
} // namespace DataHandling
} // namespace Mantid
