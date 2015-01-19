#include "MantidAlgorithms/ResizeRectangularDetector.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ResizeRectangularDetector)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ResizeRectangularDetector::ResizeRectangularDetector() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ResizeRectangularDetector::~ResizeRectangularDetector() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string ResizeRectangularDetector::name() const {
  return "ResizeRectangularDetector";
};

/// Algorithm's version for identification. @see Algorithm::version
int ResizeRectangularDetector::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string ResizeRectangularDetector::category() const {
  return "DataHandling\\Instrument";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ResizeRectangularDetector::init() {
  // When used as a Child Algorithm the workspace name is not used - hence the
  // "Anonymous" to satisfy the validator
  declareProperty(new WorkspaceProperty<MatrixWorkspace>(
      "Workspace", "Anonymous", Direction::InOut));
  declareProperty("ComponentName", "",
                  "The name of the RectangularDetector to resize.");
  declareProperty("ScaleX", 1.0,
                  "The scaling factor in the X direction. Default 1.0");
  declareProperty("ScaleY", 1.0,
                  "The scaling factor in the Y direction. Default 1.0");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ResizeRectangularDetector::exec() {
  MatrixWorkspace_sptr WS = getProperty("Workspace");
  std::string ComponentName = getPropertyValue("ComponentName");
  double ScaleX = getProperty("ScaleX");
  double ScaleY = getProperty("ScaleY");

  if (ComponentName.empty())
    throw std::runtime_error("You must specify a ComponentName.");

  Instrument_const_sptr inst = WS->getInstrument();
  IComponent_const_sptr comp;

  comp = inst->getComponentByName(ComponentName);
  if (!comp)
    throw std::runtime_error("Component with name " + ComponentName +
                             " was not found.");

  RectangularDetector_const_sptr det =
      boost::dynamic_pointer_cast<const RectangularDetector>(comp);
  if (!det)
    throw std::runtime_error("Component with name " + ComponentName +
                             " is not a RectangularDetector.");

  Geometry::ParameterMap &pmap = WS->instrumentParameters();
  // Add a parameter for the new scale factors
  pmap.addDouble(det->getComponentID(), "scalex", ScaleX);
  pmap.addDouble(det->getComponentID(), "scaley", ScaleY);

  pmap.clearPositionSensitiveCaches();
}

} // namespace Mantid
} // namespace Algorithms
