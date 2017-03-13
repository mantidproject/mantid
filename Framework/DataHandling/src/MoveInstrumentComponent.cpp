#include "MantidDataHandling/MoveInstrumentComponent.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Exception.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidGeometry/Instrument/RectangularDetectorPixel.h"

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(MoveInstrumentComponent)

using namespace Kernel;
using namespace Geometry;
using namespace API;

/// Empty default constructor
MoveInstrumentComponent::MoveInstrumentComponent() {}

/// Initialisation method.
void MoveInstrumentComponent::init() {
  // When used as a Child Algorithm the workspace name is not used - hence the
  // "Anonymous" to satisfy the validator
  declareProperty(make_unique<WorkspaceProperty<Workspace>>(
                      "Workspace", "Anonymous", Direction::InOut),
                  "The name of the workspace for which the new instrument "
                  "configuration will have an effect. Any other workspaces "
                  "stored in the analysis data service will be unaffected.");
  declareProperty("ComponentName", "",
                  "The name of the component to move. Component names are "
                  "defined in the instrument definition files. A pathname "
                  "delited by '/' may be used for non-unique name.");
  declareProperty("DetectorID", -1, "The ID of the detector to move. If both "
                                    "the component name and the detector ID "
                                    "are set the latter will be used.");
  declareProperty("X", 0.0, "The x-part of the new location vector.");
  declareProperty("Y", 0.0, "The y-part of the new location vector.");
  declareProperty("Z", 0.0, "The z-part of the new location vector.");
  declareProperty("RelativePosition", true,
                  "The property defining how the (X,Y,Z) vector should be "
                  "interpreted. If true it is a vector relative to the initial "
                  "component's position. Otherwise it is a new position in the "
                  "absolute co-ordinates.");
}

/** Executes the algorithm.
 *
 *  @throw std::runtime_error Thrown with Workspace problems
 */
void MoveInstrumentComponent::exec() {
  // Get the input workspace
  Workspace_sptr ws = getProperty("Workspace");
  MatrixWorkspace_sptr inputW =
      boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
  DataObjects::PeaksWorkspace_sptr inputP =
      boost::dynamic_pointer_cast<DataObjects::PeaksWorkspace>(ws);

  // Get some stuff from the input workspace
  Instrument_const_sptr inst;
  if (inputW) {
    inst = inputW->getInstrument();
    if (!inst)
      throw std::runtime_error("Could not get a valid instrument from the "
                               "MatrixWorkspace provided as input");
  } else if (inputP) {
    inst = inputP->getInstrument();
    if (!inst)
      throw std::runtime_error("Could not get a valid instrument from the "
                               "PeaksWorkspace provided as input");
  } else {
    if (!inst)
      throw std::runtime_error("Could not get a valid instrument from the "
                               "workspace and it does not seem to be valid as "
                               "input (must be either MatrixWorkspace or "
                               "PeaksWorkspace");
  }
  const std::string ComponentName = getProperty("ComponentName");
  const int DetID = getProperty("DetectorID");
  const double X = getProperty("X");
  const double Y = getProperty("Y");
  const double Z = getProperty("Z");
  const bool relativePosition = getProperty("RelativePosition");

  IComponent_const_sptr comp;
  // Find the component to move
  if (DetID != -1) {
    comp = inst->getDetector(DetID);
    if (comp == nullptr) {
      std::ostringstream mess;
      mess << "Detector with ID " << DetID << " was not found.";
      g_log.error(mess.str());
      throw std::runtime_error(mess.str());
    }
  } else if (!ComponentName.empty()) {
    comp = inst->getComponentByName(ComponentName);
    if (comp == nullptr) {
      std::ostringstream mess;
      mess << "Component with name " << ComponentName << " was not found.";
      g_log.error(mess.str());
      throw std::runtime_error(mess.str());
    }
  } else {
    g_log.error("DetectorID or ComponentName must be given.");
    throw std::invalid_argument("DetectorID or ComponentName must be given.");
  }

  if (dynamic_cast<const Geometry::RectangularDetectorPixel *>(comp.get())) {
    // DetectorInfo makes changing positions possible but we keep the old
    // behavior of ignoring position changes for RectangularDetectorPixel.
    g_log.warning("Component is a RectangularDetectorPixel, moving is not "
                  "possible, doing nothing.");
    return;
  }

  // Do the move
  V3D position(X, Y, Z);
  if (relativePosition)
    position += comp->getPos();

  if (inputW) {
    inputW->mutableDetectorInfo().setPosition(*comp, position);
  } else if (inputP) {
    inputP->mutableDetectorInfo().setPosition(*comp, position);
  }
}

} // namespace DataHandling
} // namespace Mantid
