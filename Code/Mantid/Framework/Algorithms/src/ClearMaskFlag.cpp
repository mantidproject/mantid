#include "MantidAlgorithms/ClearMaskFlag.h"

namespace Mantid {
namespace Algorithms {

using namespace API;
using Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ClearMaskFlag)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ClearMaskFlag::ClearMaskFlag() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ClearMaskFlag::~ClearMaskFlag() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string ClearMaskFlag::name() const { return "ClearMaskFlag"; };

/// Algorithm's version for identification. @see Algorithm::version
int ClearMaskFlag::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string ClearMaskFlag::category() const { return "Utility"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ClearMaskFlag::init() {
  declareProperty(new WorkspaceProperty<>("Workspace", "", Direction::InOut),
                  "Workspace to clear the mask flag of.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ClearMaskFlag::exec() {
  MatrixWorkspace_sptr ws = getProperty("Workspace");

  // Clear the mask flags
  Geometry::ParameterMap &pmap = ws->instrumentParameters();
  pmap.clearParametersByName("masked");
}

} // namespace Algorithms
} // namespace Mantid
