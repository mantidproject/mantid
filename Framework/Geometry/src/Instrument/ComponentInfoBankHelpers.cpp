#include "MantidGeometry/Instrument/ComponentInfoBankHelpers.h"
#include "MantidBeamline/ComponentType.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"

using Mantid::Beamline::ComponentType;

namespace Mantid {
namespace Geometry {
namespace ComponentInfoBankHelpers {
/** Tests whether or not the detector is within a fixed bank. If detIndex does
not point to a detector, this will return false. This method only returns true
if the bank which houses the detector is rectangular, a grid or structured.
@param compInfo ComponentInfo which defines instrument tree for testing
@param detIndex Index of the detector to be tested
@returns True if the detector is fixed in a bank, False otherwise.
*/
bool isDetectorFixedInBank(const ComponentInfo &compInfo,
                           const size_t detIndex) {
  auto parent = compInfo.parent(detIndex);
  auto grandParent = compInfo.parent(parent);
  auto grandParentType = compInfo.componentType(grandParent);
  auto greatGrandParent = compInfo.parent(parent); // bank
  auto greatGrandParentType = compInfo.componentType(greatGrandParent);

  if (compInfo.isDetector(detIndex) &&
      (grandParentType == ComponentType::Rectangular ||
       grandParentType == ComponentType::Structured ||
       greatGrandParentType == ComponentType::Grid)) {
    return true;
  }

  return false;
}

bool isAnyBank(const ComponentInfo &compInfo, const size_t &idx) {
  // if component at index is not a detector
  if (compInfo.isDetector(idx))
    return false;
  if (compInfo.hasParent(idx)) {
    size_t parent = compInfo.parent(idx);
    auto parentType = compInfo.componentType(parent);
    auto childType = compInfo.componentType(idx);
    if (compInfo.detectorsInSubtree(idx).size() != 0) {
      // if parent is not a detector bank
      if (parentType != Beamline::ComponentType::Rectangular &&
          parentType != Beamline::ComponentType::Structured &&
          parentType != Beamline::ComponentType::Grid) {

        // if component at index is not a tube
        if (childType != Beamline::ComponentType::Unstructured) {
          return true;
        }
        return false;
      }
      return false;
    }
    return false;
  }
  return false;
}

} // namespace ComponentInfoBankHelpers
} // namespace Geometry
} // namespace Mantid
