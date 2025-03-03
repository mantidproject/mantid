// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/ComponentInfoBankHelpers.h"
#include "MantidBeamline/ComponentType.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/EigenConversionHelpers.h"

#include <algorithm>

using Mantid::Beamline::ComponentType;

namespace Mantid::Geometry::ComponentInfoBankHelpers {
/** Tests whether or not the detector is within a fixed bank. If detIndex does
not point to a detector, this will return false. This method only returns true
if the bank which houses the detector is rectangular, a grid or structured.
@param compInfo ComponentInfo which defines instrument tree for testing
@param detIndex Index of the detector to be tested
@returns True if the detector is fixed in a bank, False otherwise.
*/
bool isDetectorFixedInBank(const ComponentInfo &compInfo, const size_t detIndex) {
  auto parent = compInfo.parent(detIndex);
  auto grandParent = compInfo.parent(parent);
  auto grandParentType = compInfo.componentType(grandParent);
  auto greatGrandParent = compInfo.parent(parent); // bank
  auto greatGrandParentType = compInfo.componentType(greatGrandParent);

  if (compInfo.isDetector(detIndex) &&
      (grandParentType == ComponentType::Rectangular || grandParentType == ComponentType::Structured ||
       greatGrandParentType == ComponentType::Grid)) {
    return true;
  }

  return false;
}

/** Function: isSaveableBank. Returns true if the index of the component in the
 * Instrument cache can be represented as an NXdetector, ignoring tubes.
 * otherwise returns false. current implementation can treat root as saveable
 * NXdetector.
 *
 * @param compInfo : Geometry::ComponentInfo flat tree representation of all
 * components
 * @param detInfo : Geometry::DetectorInfo flat tree representation of detectors
 * @param idx : size_t index of component
 * @return true if component at index is bank, false otherwise.
 */
bool isSaveableBank(const ComponentInfo &compInfo, const DetectorInfo &detInfo, const size_t idx) {
  // return false if is a detector.
  if (compInfo.isDetector(idx))
    return false;
  // needs to ignore if index is the sample
  if (compInfo.sample() == idx)
    return false;
  // needs to ignore if index is the source
  if (compInfo.source() == idx)
    return false;
  if (!compInfo.hasDetectors(idx))
    return false;

  // banks containing monitors are not considered saveable as are not
  // NXDetectors
  auto detectors = compInfo.detectorsInSubtree(idx);

  return std::none_of(detectors.cbegin(), detectors.cend(),
                      [&detInfo](const auto &det) { return detInfo.isMonitor(det); });
}

/** Finds all ancestors up to the root of a component
 * index and returns true if the possible ancestor is encountered in the
 * search. The root index is not counted, as the function exits upon reaching
 * the root before further searching.
 *
 * @param compInfo : Geometry::ComponentInfo Instrument cache containing the
 * component info.
 * @param possibleAncestor : the queried ancestor.
 * @param current : the queried descendant.
 */
bool isAncestorOf(const ComponentInfo &compInfo, const size_t possibleAncestor, const size_t current) {
  size_t next = current;
  while (next != compInfo.root()) {
    if (next == possibleAncestor)
      return true;
    next = compInfo.parent(next);
  }
  return false;
}

/** Returns the position of the component at the
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
Eigen::Vector3d offsetFromAncestor(const Mantid::Geometry::ComponentInfo &compInfo, const size_t ancestorIdx,
                                   const size_t currentIdx) {

  if ((ancestorIdx < currentIdx)) {
    throw std::invalid_argument("Index of ancestor component is less than the current Index.");
  }

  if (ancestorIdx == currentIdx) {
    return Mantid::Kernel::toVector3d(compInfo.position(currentIdx));
  } else {
    const auto ancestorPos = Mantid::Kernel::toVector3d(compInfo.position(ancestorIdx));
    auto transformation = Eigen::Affine3d(
        Mantid::Kernel::toQuaterniond(compInfo.rotation(ancestorIdx)).conjugate()); // Inverse ancestor rotation
    transformation.translate(-ancestorPos);
    return transformation * Mantid::Kernel::toVector3d(compInfo.position(currentIdx));
  }
}

} // namespace Mantid::Geometry::ComponentInfoBankHelpers
