#include "MantidAlgorithms/ClearMaskFlag.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace Algorithms {

using namespace Geometry;
using namespace API;
using Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ClearMaskFlag)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string ClearMaskFlag::name() const { return "ClearMaskFlag"; }

/// Algorithm's version for identification. @see Algorithm::version
int ClearMaskFlag::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ClearMaskFlag::category() const {
  return "Transforms\\Masking";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ClearMaskFlag::init() {
  declareProperty(Kernel::make_unique<WorkspaceProperty<>>("Workspace", "",
                                                           Direction::InOut),
                  "Workspace to clear the mask flag of.");
  declareProperty("ComponentName", "",
                  "Specify the instrument component to clear the "
                  "mask. If empty clears mask the flag for "
                  "the whole instrument.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ClearMaskFlag::exec() {
  MatrixWorkspace_sptr ws = getProperty("Workspace");
  std::string componentName = getPropertyValue("ComponentName");

  // Clear the mask flags
  Geometry::ParameterMap &pmap = ws->instrumentParameters();

  if (!componentName.empty()) {
    auto instrument = ws->getInstrument();
    auto component = instrument->getComponentByName(componentName);
    boost::shared_ptr<const Geometry::ICompAssembly> componentAssembly =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(component);

    std::vector<Geometry::IComponent_const_sptr> children;
    componentAssembly->getChildren(children, true);
    for (auto det : children) {
      pmap.addBool(det.get(), "masked", false);
    }
  } else {
    pmap.clearParametersByName("masked");
  }
}

} // namespace Algorithms
} // namespace Mantid
