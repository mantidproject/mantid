// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SetBeam.h"

#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerProperty.h"

namespace {
/// Name of slit geometry
constexpr const char *SLIT_TYPE_NAME = "Slit";
/// Name of width parameter in map
constexpr const char *WIDTH_PARAM_NAME = "beam-width";
/// Name of height parameter in map
constexpr const char *HEIGHT_PARAM_NAME = "beam-height";
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
const std::string SetBeam::summary() const {
  return "Set properties of the beam such as size and shape";
}

/// @return A dict of any errors in parameters
std::map<std::string, std::string> SetBeam::validateInputs() {
  using Kernel::PropertyManager_sptr;

  std::map<std::string, std::string> errors;
  PropertyManager_sptr geometryArgs = getProperty("Geometry");
  if (geometryArgs) {
    if (!geometryArgs->existsProperty("Shape") ||
        !geometryArgs->existsProperty("Width") ||
        !geometryArgs->existsProperty("Height")) {
      errors["Geometry"] =
          "'Geometry' flags missing. Required flags: Shape, Width, Height";
    } else {
      std::string shape = geometryArgs->getProperty("Shape");
      if (shape != SLIT_TYPE_NAME) {
        errors["Geometry"] = "Only 'Slit' shape is supported.";
      }
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
  auto validator = boost::make_shared<InstrumentValidator>(
      InstrumentValidator::SourcePosition);
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input, validator),
                  "An input workspace with an attached instrument.");
  declareProperty(std::make_unique<PropertyManagerProperty>(
                      "Geometry", Direction::Input),
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
  double width = geometryArgs->getProperty("Width");
  double height = geometryArgs->getProperty("Height");
  // convert to metres
  width *= 0.01;
  height *= 0.01;

  // Add the values as parameters on the source object
  auto instrument = inputWS->getInstrument();
  auto source = instrument->getSource();
  auto &pmap = inputWS->instrumentParameters();
  pmap.addDouble(source->getComponentID(), WIDTH_PARAM_NAME, width);
  pmap.addDouble(source->getComponentID(), HEIGHT_PARAM_NAME, height);
}

} // namespace DataHandling
} // namespace Mantid
