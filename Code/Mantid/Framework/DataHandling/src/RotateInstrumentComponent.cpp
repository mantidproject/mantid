//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/RotateInstrumentComponent.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(RotateInstrumentComponent)

using namespace Kernel;
using namespace Geometry;
using namespace API;

/// Empty default constructor
RotateInstrumentComponent::RotateInstrumentComponent() {}

/// Initialisation method.
void RotateInstrumentComponent::init() {
  // When used as a Child Algorithm the workspace name is not used - hence the
  // "Anonymous" to satisfy the validator
  declareProperty(new WorkspaceProperty<MatrixWorkspace>(
                      "Workspace", "Anonymous", Direction::InOut),
                  "The name of the workspace for which the new instrument "
                  "configuration will have an effect. Any other workspaces "
                  "stored in the analysis data service will be unaffected.");
  declareProperty("ComponentName", "", "The name of the component to rotate. "
                                       "Component names are defined in the "
                                       "instrument definition files.");
  declareProperty("DetectorID", -1, "The ID of the detector to rotate. If both "
                                    "the component name and the detector ID "
                                    "are set the latter will be used.");
  declareProperty("X", 0.0, "The x-part of the rotation axis.");
  declareProperty("Y", 0.0, "The y-part of the rotation axis.");
  declareProperty("Z", 0.0, "The z-part of the rotation axis.");
  declareProperty("Angle", 0.0, "The angle of rotation in degrees.");
  declareProperty("RelativeRotation", true,
                  "The property defining how the rotation should be "
                  "interpreted. If true it is a relative rotation. Otherwise "
                  "it is an absolute rotation.");
}

/** Executes the algorithm.
 *
 *  @throw std::runtime_error Thrown with Workspace problems
 */
void RotateInstrumentComponent::exec() {
  // Get the workspace
  MatrixWorkspace_sptr WS = getProperty("Workspace");
  const std::string ComponentName = getProperty("ComponentName");
  const int DetID = getProperty("DetectorID");
  const double X = getProperty("X");
  const double Y = getProperty("Y");
  const double Z = getProperty("Z");
  const double angle = getProperty("Angle");
  const bool relativeRotation = getProperty("RelativeRotation");

  if (X + Y + Z == 0.0)
    throw std::invalid_argument("The rotation axis must not be a zero vector");

  // Get the ParameterMap reference before the instrument so that
  // we avoid a copy
  Geometry::ParameterMap &pmap = WS->instrumentParameters();
  Instrument_const_sptr inst = WS->getInstrument();
  IComponent_const_sptr comp;

  // Find the component to move
  if (DetID != -1) {
    comp = inst->getDetector(DetID);
    if (comp == 0) {
      std::ostringstream mess;
      mess << "Detector with ID " << DetID << " was not found.";
      g_log.error(mess.str());
      throw std::runtime_error(mess.str());
    }
  } else if (!ComponentName.empty()) {
    comp = inst->getComponentByName(ComponentName);
    if (comp == 0) {
      std::ostringstream mess;
      mess << "Component with name " << ComponentName << " was not found.";
      g_log.error(mess.str());
      throw std::runtime_error(mess.str());
    }
  } else {
    g_log.error("DetectorID or ComponentName must be given.");
    throw std::invalid_argument("DetectorID or ComponentName must be given.");
  }

  // Do the rotation
  using namespace Geometry::ComponentHelper;
  TransformType rotType = Absolute;
  if (relativeRotation)
    rotType = Relative;
  Geometry::ComponentHelper::rotateComponent(
      *comp, pmap, Quat(angle, V3D(X, Y, Z)), rotType);
}

} // namespace DataHandling
} // namespace Mantid
