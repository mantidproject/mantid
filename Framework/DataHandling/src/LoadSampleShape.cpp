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
  ScaleUnits scaleType;
  std::string scaleProperty = getPropertyValue("Scale");
  if (scaleProperty == "m") {
    scaleType = metres;
  } else if (scaleProperty == "cm") {
    scaleType = centimetres;
  } else if (scaleProperty == "mm") {
    scaleType = milimetres;
  } else {
    throw std::invalid_argument(scaleProperty +
                                " is not an accepted scale of stl file.");
  }
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

  // Put shape into sample.
  Sample &sample = outputWS->mutableSample();
  sample.setShape(shape);

  // Set output workspace
  setProperty("OutputWorkspace", outputWS);
}

} // namespace DataHandling
} // namespace Mantid
