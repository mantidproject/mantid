// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/DefineGaugeVolume.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/MandatoryValidator.h"

namespace Mantid::DataHandling {

using namespace Mantid::API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DefineGaugeVolume)

/**
 * Initialize the algorithm
 */
void DefineGaugeVolume::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("Workspace", "", Kernel::Direction::InOut),
                  "The workspace with which to associate the defined gauge volume");
  declareProperty("ShapeXML", "", std::make_shared<Kernel::MandatoryValidator<std::string>>(),
                  "The XML that describes the shape of the gauge volume");
}

/**
 * Execute the algorithm
 */
void DefineGaugeVolume::exec() {
  // Get the XML definition
  const std::string shapeXML = getProperty("ShapeXML");
  // Try creating the shape to make sure the input's valid
  auto shape_sptr = Geometry::ShapeFactory().createShape(shapeXML);
  if (!shape_sptr->hasValidShape()) {
    g_log.error("Invalid shape definition provided. Gauge Volume NOT added to "
                "workspace.");
    throw std::invalid_argument("Invalid shape definition provided.");
  }

  // Should we check that the volume defined is within the sample? Is this
  // necessary?
  // Do we even have a way to do this?

  progress(0.5);

  // Add as an entry in the workspace's Run object, just as text. Overwrite if
  // already present.
  const MatrixWorkspace_sptr workspace = getProperty("Workspace");
  workspace->mutableRun().addProperty("GaugeVolume", shapeXML, true);

  progress(1);
}

} // namespace Mantid::DataHandling
