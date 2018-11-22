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

/**
 * Find the row/col index of a detector in a Rectangular detector bank
 *
 * @param compInfo :: ComponentInfo which defines instrument tree for searching
 * @param detIndex :: Index of the detector to be searched for
 */
std::pair<size_t, size_t>
findRowColIndexForRectangularBank(const ComponentInfo &compInfo,
                                  const size_t detIndex) {
  if (!compInfo.isDetector(detIndex)) {
    throw std::runtime_error("The component at detIndex is not a detector");
  }

  auto parent = compInfo.parent(detIndex);
  auto grandParent = compInfo.parent(parent);

  if (compInfo.componentType(grandParent) != ComponentType::Rectangular) {
    throw std::runtime_error(
        "Cannot find row/col index for non-rectangular banks");
  }

  const auto numColumns = compInfo.children(grandParent).size();
  const auto numRows = compInfo.children(parent).size();

  const auto row = detIndex / numRows;
  const auto col = detIndex % numColumns;

  return {row, col};
}

} // namespace ComponentInfoBankHelpers
} // namespace Geometry
} // namespace Mantid
