#include "MantidDataHandling/RotateSource.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace DataHandling {

using Mantid::Kernel::Direction;
using Mantid::Kernel::Quat;
using Mantid::Kernel::V3D;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RotateSource)

using namespace Geometry;
using namespace API;

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void RotateSource::init() {

  // When used as a Child Algorithm the workspace name is not used - hence the
  // "Anonymous" to satisfy the validator
  declareProperty(new WorkspaceProperty<Workspace>("Workspace", "Anonymous",
                                                   Direction::InOut),
                  "The name of the workspace for which the new instrument "
                  "configuration will have an effect. Any other workspaces "
                  "stored in the analysis data service will be unaffected.");
  declareProperty("Angle", 0.0, "The angle of rotation in degrees.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void RotateSource::exec() {

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
    throw std::runtime_error("Input workspaces does not seem to be valid (must "
                             "be either MatrixWorkspace or PeaksWorkspace)");
  }

  const double angle = getProperty("Angle");

  if (angle != 0.) {

    // Need the reference frame to decide around which axis to rotate
    auto refFrame = inst->getReferenceFrame();

    if (!refFrame) {
      throw std::runtime_error("Could not get a valid reference frame");
    }

    auto pointingAlong = refFrame->pointingHorizontal();

    // (x, y, z) -> the rotation axis
    double x = 0.;
    double y = 0.;
    double z = 0.;

    if (pointingAlong == X) {
      x = 1.;
    } else if (pointingAlong == Y) {
      y = 1.;
    } else if (pointingAlong == Z) {
      z = 1.;
    } else {
      throw std::runtime_error("Could not get a valid rotation axis");
    }

    // Get the source's position
    auto source = inst->getSource();
    if (!source) {
      throw std::runtime_error("Could not get the source's position");
    }
    // Get the sample's position
    auto sample = inst->getSample();
    if (!sample) {
      throw std::runtime_error("Could not get the sample's position");
    }
    auto samplePos = sample->getPos();
    // The vector we want to rotate
    auto sourcePos = source->getPos() - samplePos;

    // The new position
    Quat quat(angle, V3D(x, y, z));
    quat.rotate(sourcePos);
    sourcePos += samplePos;

    // The source's name
    std::string sourceName = source->getName();

    // Move the source to the new location
    auto move = this->createChildAlgorithm("MoveInstrumentComponent");
    move->initialize();
    move->setProperty("Workspace", ws);
    move->setProperty("ComponentName", sourceName);
    move->setProperty("X", sourcePos.X());
    move->setProperty("Y", sourcePos.Y());
    move->setProperty("Z", sourcePos.Z());
    move->setProperty("RelativePosition", false);
    move->execute();
  }
}

} // namespace DataHandling
} // namespace Mantid
