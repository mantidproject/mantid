// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ResizeRectangularDetector.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ResizeRectangularDetectorHelper.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ResizeRectangularDetector)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string ResizeRectangularDetector::name() const { return "ResizeRectangularDetector"; }

/// Algorithm's version for identification. @see Algorithm::version
int ResizeRectangularDetector::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ResizeRectangularDetector::category() const { return "DataHandling\\Instrument"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ResizeRectangularDetector::init() {
  // When used as a Child Algorithm the workspace name is not used - hence the
  // "Anonymous" to satisfy the validator
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("Workspace", "Anonymous", Direction::InOut));
  declareProperty("ComponentName", "", "The name of the RectangularDetector to resize.");
  declareProperty("ScaleX", 1.0, "The scaling factor in the X direction. Default 1.0");
  declareProperty("ScaleY", 1.0, "The scaling factor in the Y direction. Default 1.0");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ResizeRectangularDetector::exec() {
  // Get the input workspace
  Workspace_sptr ws = getProperty("Workspace");
  MatrixWorkspace_sptr inputW = std::dynamic_pointer_cast<MatrixWorkspace>(ws);
  DataObjects::PeaksWorkspace_sptr inputP = std::dynamic_pointer_cast<DataObjects::PeaksWorkspace>(ws);

  // Get some stuff from the input workspace
  Instrument_sptr inst;
  if (inputW) {
    inst = std::const_pointer_cast<Instrument>(inputW->getInstrument());
    if (!inst)
      throw std::runtime_error("Could not get a valid instrument from the "
                               "MatrixWorkspace provided as input");

  } else if (inputP) {
    inst = std::const_pointer_cast<Instrument>(inputP->getInstrument());
    if (!inst)
      throw std::runtime_error("Could not get a valid instrument from the "
                               "PeaksWorkspace provided as input");
  } else {
    throw std::runtime_error("Could not get a valid instrument from the "
                             "workspace and it does not seem to be valid as "
                             "input (must be either MatrixWorkspace or "
                             "PeaksWorkspace");
  }

  std::string ComponentName = getPropertyValue("ComponentName");
  double ScaleX = getProperty("ScaleX");
  double ScaleY = getProperty("ScaleY");

  if (ComponentName.empty())
    throw std::runtime_error("You must specify a ComponentName.");

  IComponent_const_sptr comp;

  comp = inst->getComponentByName(ComponentName);
  if (!comp)
    throw std::runtime_error("Component with name " + ComponentName + " was not found.");

  RectangularDetector_const_sptr det = std::dynamic_pointer_cast<const RectangularDetector>(comp);
  if (!det)
    throw std::runtime_error("Component with name " + ComponentName + " is not a RectangularDetector.");

  auto input = std::dynamic_pointer_cast<ExperimentInfo>(ws);
  Geometry::ParameterMap &pmap = input->instrumentParameters();
  auto oldscalex = pmap.getDouble(det->getName(), std::string("scalex"));
  auto oldscaley = pmap.getDouble(det->getName(), std::string("scaley"));
  // Add a parameter for the new scale factors
  pmap.addDouble(det->getComponentID(), "scalex", ScaleX);
  pmap.addDouble(det->getComponentID(), "scaley", ScaleY);
  pmap.clearPositionSensitiveCaches();

  // Positions of detectors are now stored in DetectorInfo, so we must update
  // positions there.
  // This algorithm is setting the absolute scale factor. Since there may be a
  // previous scaling we have to factor that out.
  double relscalex = ScaleX;
  double relscaley = ScaleY;
  if (!oldscalex.empty())
    relscalex /= oldscalex[0];
  if (!oldscaley.empty())
    relscaley /= oldscaley[0];
  applyRectangularDetectorScaleToComponentInfo(input->mutableComponentInfo(), comp->getComponentID(), relscalex,
                                               relscaley);
}

} // namespace Mantid::Algorithms
