// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//--------------------------------
// Includes
//--------------------------------
#include "MantidDataHandling/CreateSampleShape.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Material.h"

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateSampleShape)

using namespace Mantid::DataHandling;
using namespace Mantid::API;

/**
 * Initialize the algorithm
 */
void CreateSampleShape::init() {
  using namespace Mantid::Kernel;
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "The workspace with which to associate the sample ");
  declareProperty("ShapeXML", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The XML that describes the shape");
}

/**
 * Execute the algorithm
 */
void CreateSampleShape::exec() {
  // Get the input workspace
  MatrixWorkspace_sptr workspace = getProperty("InputWorkspace");
  // Get the XML definition
  std::string shapeXML = getProperty("ShapeXML");

  Geometry::ShapeFactory sFactory;
  // Create the object
  auto shape = sFactory.createShape(shapeXML);
  // Check it's valid and attach it to the workspace sample but preserve any
  // material
  if (shape->hasValidShape()) {
    const auto mat = workspace->sample().getMaterial();
    shape->setMaterial(mat);
    workspace->mutableSample().setShape(shape);
  } else {
    std::ostringstream msg;
    msg << "Object has invalid shape.";
    if (auto csgShape = dynamic_cast<Geometry::CSGObject *>(shape.get())) {
      msg << " TopRule = " << csgShape->topRule()
          << ", number of surfaces = " << csgShape->getSurfacePtr().size()
          << "\n";
    }
    throw std::runtime_error(msg.str());
  }
  // Done!
  progress(1);
}
} // namespace DataHandling
} // namespace Mantid
