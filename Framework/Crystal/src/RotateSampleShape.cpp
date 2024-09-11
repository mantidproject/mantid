// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/RotateSampleShape.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/CreateSampleShape.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace Mantid::Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RotateSampleShape)

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::API;

/// How many axes (max) to define
const size_t NUM_AXES = 6;
Mantid::Kernel::Logger g_log("RotateSampleShape");

/** Initialize the algorithm's properties.
 */
void RotateSampleShape::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("Workspace", "", Direction::InOut),
                  "The workspace containing the sample whose orientation is to be rotated");

  std::string axisHelp = ": degrees,x,y,z,1/-1 (1 for ccw, -1 for cw rotation).";
  for (size_t i = 0; i < NUM_AXES; i++) {
    std::ostringstream propName;
    propName << "Axis" << i;
    declareProperty(std::make_unique<PropertyWithValue<std::string>>(propName.str(), "", Direction::Input),
                    propName.str() + axisHelp);
  }
}

/** Execute the algorithm.
 */
void RotateSampleShape::exec() {
  Workspace_sptr ws = getProperty("Workspace");
  auto ei = std::dynamic_pointer_cast<ExperimentInfo>(ws);

  if (!ei) {
    // We're dealing with an MD workspace which has multiple experiment infos
    auto infos = std::dynamic_pointer_cast<MultipleExperimentInfos>(ws);
    if (!infos) {
      throw std::invalid_argument("Input workspace does not support RotateSampleShape");
    }
    if (infos->getNumExperimentInfo() < 1) {
      ExperimentInfo_sptr info(new ExperimentInfo());
      infos->addExperimentInfo(info);
    }
    ei = infos->getExperimentInfo(0);
  }

  std::string shapeXML;
  bool isMeshShape = false;
  if (!checkIsValidShape(ei, shapeXML, isMeshShape)) {
    throw std::runtime_error("Input sample does not have a valid shape!");
  }

  // Create a goniometer with provided rotations
  Goniometer gon;
  prepareGoniometerAxes(gon);
  if (gon.getNumberAxes() == 0)
    g_log.warning() << "Empty goniometer created; will always return an "
                       "identity rotation matrix.\n";

  const auto sampleShapeRotation = gon.getR();
  if (sampleShapeRotation == Kernel::Matrix<double>(3, 3, true)) {
    // If the resulting rotationMatrix is Identity, ignore the calculatrion
    g_log.warning("Rotation matrix set via RotateSampleShape is an Identity matrix. Ignored rotating sample shape");
    return;
  }

  const auto oldRotation = ei->run().getGoniometer().getR();
  auto newSampleShapeRot = sampleShapeRotation * oldRotation;
  if (isMeshShape) {
    auto meshShape = std::dynamic_pointer_cast<MeshObject>(ei->sample().getShapePtr());
    meshShape->rotate(newSampleShapeRot);
  } else {
    shapeXML = Geometry::ShapeFactory().addGoniometerTag(newSampleShapeRot, shapeXML);
    Mantid::DataHandling::CreateSampleShape::setSampleShape(*ei, shapeXML, false);
  }
}

bool RotateSampleShape::checkIsValidShape(const API::ExperimentInfo_sptr &ei, std::string &shapeXML,
                                          bool &isMeshShape) {
  if (ei->sample().hasShape()) {
    const auto csgShape = std::dynamic_pointer_cast<CSGObject>(ei->sample().getShapePtr());
    if (csgShape && csgShape->hasValidShape()) {
      shapeXML = csgShape->getShapeXML();
      if (!shapeXML.empty()) {
        return true;
      }
    } else {
      const auto meshShape = std::dynamic_pointer_cast<MeshObject>(ei->sample().getShapePtr());
      if (meshShape && meshShape->hasValidShape()) {
        isMeshShape = true;
        return true;
      }
    }
  }
  return false;
}

void RotateSampleShape::prepareGoniometerAxes(Goniometer &gon) {
  for (size_t i = 0; i < NUM_AXES; i++) {
    std::ostringstream propName;
    propName << "Axis" << i;
    std::string axisDesc = getPropertyValue(propName.str());
    if (!axisDesc.empty()) {
      std::vector<std::string> tokens;
      boost::split(tokens, axisDesc, boost::algorithm::detail::is_any_ofF<char>(","));
      if (tokens.size() != 5)
        throw std::invalid_argument("Wrong number of arguments to parameter " + propName.str() +
                                    ". Expected 5 comma-separated arguments.");

      std::transform(tokens.begin(), tokens.end(), tokens.begin(), [](std::string str) { return Strings::strip(str); });
      if (!std::all_of(tokens.begin(), tokens.end(), [](std::string tokenStr) { return !tokenStr.empty(); })) {
        throw std::invalid_argument("Empty axis parameters found!");
      }

      double angle = 0;
      if (!Strings::convert(tokens[0], angle)) {
        throw std::invalid_argument("Error converting angle string '" + tokens[0] + "' to a number.");
      }

      std::string axisName = "RotateSampleShapeAxis" + Strings::toString(i) + "_FixedValue";
      double x = 0, y = 0, z = 0;
      if (!Strings::convert(tokens[1], x))
        throw std::invalid_argument("Error converting x string '" + tokens[1] + "' to a number.");
      if (!Strings::convert(tokens[2], y))
        throw std::invalid_argument("Error converting y string '" + tokens[2] + "' to a number.");
      if (!Strings::convert(tokens[3], z))
        throw std::invalid_argument("Error converting z string '" + tokens[3] + "' to a number.");
      V3D vec(x, y, z);
      if (vec.norm() < 1e-4)
        throw std::invalid_argument("Rotation axis vector should be non-zero!");

      int ccw = 0;
      if (!Strings::convert(tokens[4], ccw)) {
        throw std::invalid_argument("Error converting sense of roation '" + tokens[4] + "' to a number.");
      }
      if (ccw != 1 && ccw != -1) {
        throw std::invalid_argument("The sense of rotation parameter must only be 1 (ccw) or -1 (cw)");
      }
      // Default to degrees
      gon.pushAxis(axisName, x, y, z, angle, ccw);
    }
  }
}

} // namespace Mantid::Crystal
