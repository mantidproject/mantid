// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SetBeam.h"

#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerProperty.h"

namespace {
/// Names of possible slit geometries
constexpr const char *SHAPE_NAME_SLIT = "Slit";
constexpr const char *SHAPE_NAME_CIRCLE = "Circle";
/// Name of width parameter in map
constexpr const char *WIDTH_PARAM_NAME = "beam-width";
/// Name of height parameter in map
constexpr const char *HEIGHT_PARAM_NAME = "beam-height";
/// Name of radius parameter in map
constexpr const char *RADIUS_PARAM_NAME = "beam-radius";
/// Name of shape parameter in map
constexpr const char *SHAPE_PARAM_NAME = "beam-shape";
} // namespace

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SetBeam)

/// Algorithms name for identification. @see Algorithm::name
const std::string SetBeam::name() const { return "SetBeam"; }

/// Algorithm's version for identification. @see Algorithm::version
int SetBeam::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SetBeam::category() const { return "Sample"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string SetBeam::summary() const { return "Set properties of the beam such as size and shape"; }

/// @return A dict of any errors in parameters
std::map<std::string, std::string> SetBeam::validateInputs() {
  using Kernel::PropertyManager_sptr;

  std::map<std::string, std::string> errors;
  PropertyManager_sptr geometryArgs = getProperty("Geometry");
  if (geometryArgs) {
    bool s = geometryArgs->existsProperty("Shape");
    bool w = geometryArgs->existsProperty("Width");
    bool h = geometryArgs->existsProperty("Height");
    bool r = geometryArgs->existsProperty("Radius");
    if (s && ((w && h) != r)) {
      std::string shape = geometryArgs->getProperty("Shape");
      if (shape != SHAPE_NAME_SLIT && shape != SHAPE_NAME_CIRCLE) {
        errors["Geometry"] = "Only 'Slit' and 'Circle' shapes are supported.";
      }
    } else {
      errors["Geometry"] = "'Geometry' flags missing or incorrect. Required flags: Shape, "
                           "plus Width and Height, or Radius";
    }
  } else {
    errors["Geometry"] = "No 'Geometry' flags given.";
  }
  return errors;
}

/** Initialize the algorithm's properties.
 */
void SetBeam::init() {
  using API::InstrumentValidator;
  using API::MatrixWorkspace;
  using API::WorkspaceProperty;
  using Kernel::Direction;
  using Kernel::PropertyManagerProperty;

  // In/out
  auto validator = std::make_shared<InstrumentValidator>(InstrumentValidator::SourcePosition);
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input, validator),
      "An input workspace with an attached instrument.");
  declareProperty(std::make_unique<PropertyManagerProperty>("Geometry", Direction::Input),
                  "A dictionary of geometry parameters for the beam");
}

/**
 * Execute
 */
void SetBeam::exec() {

  using API::MatrixWorkspace_sptr;
  using Kernel::PropertyManager_sptr;

  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  PropertyManager_sptr geometryArgs = getProperty("Geometry");

  auto instrument = inputWS->getInstrument();
  auto source = instrument->getSource();
  auto &pmap = inputWS->instrumentParameters();

  std::string shape = geometryArgs->getProperty("Shape");

  if (shape.compare("Circle") == 0) {
    double radius = geometryArgs->getProperty("Radius");
    // convert to metres
    radius *= 0.01;

    // Add the values as parameters on the source object
    pmap.addDouble(source->getComponentID(), RADIUS_PARAM_NAME, radius);
    pmap.addString(source->getComponentID(), SHAPE_PARAM_NAME, SHAPE_NAME_CIRCLE);
  } else {
    double width = geometryArgs->getProperty("Width");
    double height = geometryArgs->getProperty("Height");
    // convert to metres
    width *= 0.01;
    height *= 0.01;

    // Add the values as parameters on the source object
    pmap.addDouble(source->getComponentID(), WIDTH_PARAM_NAME, width);
    pmap.addDouble(source->getComponentID(), HEIGHT_PARAM_NAME, height);
    pmap.addString(source->getComponentID(), SHAPE_PARAM_NAME, SHAPE_NAME_SLIT);
  }
}

} // namespace DataHandling
} // namespace Mantid
