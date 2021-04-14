// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadSampleShape.h"
#include "MantidDataHandling/LoadAsciiStl.h"
#include "MantidDataHandling/LoadBinaryStl.h"
#include "MantidDataHandling/LoadOff.h"
#include "MantidDataHandling/LoadStlFactory.h"
#include "MantidGeometry/Instrument/Goniometer.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/Workspace.h"

#include "MantidKernel/ArrayProperty.h"

#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace DataHandling {

namespace {
double DegreesToRadians(double angle) { return angle * M_PI / 180; }
} // namespace

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadSampleShape)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void LoadSampleShape::init() {
  auto wsValidator = std::make_shared<API::InstrumentValidator>();
  ;

  // input workspace
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("InputWorkspace", "", Direction::Input, wsValidator),
                  "The name of the workspace containing the instrument to add the shape");

  // shape file
  const std::vector<std::string> extensions{".stl", ".off"};
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, extensions),
                  "The path name of the file containing the shape");

  // scale to use for stl
  declareProperty("Scale", "cm", "The scale of the stl: m, cm, or mm");

  // Rotation angles
  declareProperty("XDegrees", 0.0, "The degrees to rotate on the x axis by");
  declareProperty("YDegrees", 0.0, "The degrees to rotate on the y axis by");
  declareProperty("ZDegrees", 0.0, "The degrees to rotate on the z axis by");

  // Vector to translate mesh
  declareProperty(std::make_unique<ArrayProperty<double>>("TranslationVector", "0,0,0"),
                  "Vector by which to translate the loaded sample shape");

  // Output workspace
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace that will contain the loaded "
                  "shape of the sample");
}

void LoadSampleShape::exec() {

  Workspace_const_sptr inputWS = getProperty("InputWorkspace");
  Workspace_sptr outputWS = getProperty("OutputWorkspace");

  if (inputWS != outputWS) {
    outputWS = inputWS->clone();
  }

  auto ei = std::dynamic_pointer_cast<ExperimentInfo>(outputWS);
  if (!ei)
    throw std::invalid_argument("Wrong type of input workspace");

  const std::string filename = getProperty("Filename");

  const std::string filetype = filename.substr(filename.size() - 3);

  std::shared_ptr<MeshObject> shape = nullptr;
  const std::string scaleProperty = getPropertyValue("Scale");
  const ScaleUnits scaleType = getScaleTypeFromStr(scaleProperty);

  std::unique_ptr<LoadSingleMesh> reader = nullptr;
  if (filetype == "off") {
    reader = std::make_unique<LoadOff>(filename, scaleType);
    shape = reader->readShape();
  } else {
    reader = LoadStlFactory::createReader(filename, scaleType);
    shape = reader->readShape();
  }

  const double xRotation = DegreesToRadians(getProperty("xDegrees"));
  const double yRotation = DegreesToRadians(getProperty("yDegrees"));
  const double zRotation = DegreesToRadians(getProperty("zDegrees"));
  shape = reader->rotate(shape, xRotation, yRotation, zRotation);

  const std::vector<double> translationVector = getProperty("TranslationVector");
  shape = reader->translate(shape, translationVector);

  // rotate shape according to goniometer
  shape->rotate(ei->run().getGoniometer().getR());

  // Put shape into sample.
  Sample &sample = ei->mutableSample();
  sample.setShape(shape);

  // Set output workspace
  setProperty("OutputWorkspace", outputWS);
}

} // namespace DataHandling
} // namespace Mantid
