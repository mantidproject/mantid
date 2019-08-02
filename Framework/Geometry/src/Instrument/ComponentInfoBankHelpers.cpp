#include "MantidGeometry/Instrument/ComponentInfoBankHelpers.h"
#include "MantidBeamline/ComponentType.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"

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

/*
* Function: isSaveableBank. Returns true if the index in the Instrument cache is
* a detector bank, ignoring tubes. otherwise returns false. Used by
* SaveInstrument to find and save NXdetectors from memory to file.
*
* @param compInfo : Geometry::ComponentInfo Instrument cache containing the
* component info.
* @param detInfo :  Geometry::DetectorInfo Instrument cache containing the
* detector info.
* @param idx : size_t index of component
* @return true if component at index is bank, false otherwise.

*/
bool isSaveableBank(const ComponentInfo &compInfo, const DetectorInfo &detInfo,
                    const size_t idx) {
  // return false if is a detector.
  if (compInfo.isDetector(idx))
    return false;
  // needs to ignore if index is the sample
  if (compInfo.sample() == idx)
    return false;
  // needs to ignore if index is the source
  if (compInfo.source() == idx)
    return false;
  // a bank must have a parent, skip this block if the component does not.
  if (compInfo.hasParent(idx)) {
    size_t parent = compInfo.parent(idx);
    auto parentType = compInfo.componentType(parent);
    auto childType = compInfo.componentType(idx);
    if (detInfo.size() != 0) {
      // if parent is not a detector bank
      if (parentType != Beamline::ComponentType::Rectangular &&
          parentType != Beamline::ComponentType::Structured &&
          parentType != Beamline::ComponentType::Grid) {
        // if component at index is not a tube
        if (childType != Beamline::ComponentType::OutlineComposite) {
          return true;
        }
      }
    }
  }
  return false;
}

/*
 * Function: offsetFromAncestor. Returns the position of the component at the
 * current index relative to the ancestor component at the ancestor index. Used
 * by saveInstrument to get the pixel offsets relative directly to the bank,
 * ignoring any intermediate assembly types.
 *
 * @param compInfo : Geometry::ComponentInfo Instrument cache containing the
 * component info.
 * @param ancestorIdx : size_t index of ancestor component which the offset is
 * relative to
 * @param currentIdx : size_t index of current component
 * @return Eigen::vector3d offset of the component at the current index relative
 * to the ancestor component
 */
Eigen::Vector3d
offsetFromAncestor(const Mantid::Geometry::ComponentInfo &compInfo,
                   const size_t ancestorIdx, const size_t currentIdx) {

  if (ancestorIdx == currentIdx) {
    return Mantid::Kernel::toVector3d(compInfo.position(currentIdx));
  } else {
    const auto ancestorPos =
        Mantid::Kernel::toVector3d(compInfo.position(ancestorIdx));
    auto transformation = Eigen::Affine3d(
        Mantid::Kernel::toQuaterniond(compInfo.rotation(ancestorIdx))
            .conjugate()); // Inverse ancestor rotation
    transformation.translate(-ancestorPos);
    return transformation *
           Mantid::Kernel::toVector3d(compInfo.position(currentIdx));
  }
}

} // namespace ComponentInfoBankHelpers
} // namespace Geometry
} // namespace Mantid
