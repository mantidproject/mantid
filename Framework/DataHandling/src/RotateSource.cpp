#include "MantidDataHandling/RotateSource.h"

namespace Mantid {
namespace DataHandling {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RotateSource)


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
	declareProperty("X", 0.0, "The x-part of the rotation axis.");
	declareProperty("Y", 0.0, "The y-part of the rotation axis.");
	declareProperty("Z", 0.0, "The z-part of the rotation axis.");
	declareProperty("Angle", 0.0, "The angle of rotation in degrees.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void RotateSource::exec() {
  // TODO Auto-generated execute stub
}

} // namespace DataHandling
} // namespace Mantid
