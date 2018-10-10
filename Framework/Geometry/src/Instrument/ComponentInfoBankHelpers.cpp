#include "MantidGeometry/Instrument/ComponentInfoBankHelpers.h"
#include "MantidBeamline/ComponentType.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"

using Mantid::Beamline::ComponentType;

namespace Mantid {
namespace Geometry {
namespace ComponentInfoBankHelpers {
namespace {
/** Tests whether or not the detector pixel is a GridDetectorPixel. This is true
only if the bank is a Grid.
@param detIndex Index of the detector to be tested
@returns True if the detector is a GridDetectorPixel
*/
bool isGridBankPixel(const ComponentInfo &compInfo, const size_t detIndex) {
  auto parent = compInfo.parent(detIndex);              // column
  auto grandParent = compInfo.parent(parent);           // layer
  auto greatGrandParent = compInfo.parent(grandParent); // bank
  auto greatGrandParentType = compInfo.componentType(greatGrandParent);

  if (compInfo.isDetector(detIndex) &&
      (greatGrandParentType == ComponentType::Grid))
    return true;
  return false;
}

/** Tests whether or not the detector is within a Rectangular bank. Only returns
true only if the bank type is Rectangular.
@param compInfo ComponentInfo which defines instrument tree for testing
@param detIndex Index of the detector to be tested
@returns True if the detector is in a rectangular bank.
*/
bool isRectangularBankPixel(const ComponentInfo &compInfo,
                            const size_t detIndex) {
  auto parent = compInfo.parent(detIndex);
  auto grandParent = compInfo.parent(parent);
  auto grandParentType = compInfo.componentType(grandParent);

  if (compInfo.isDetector(detIndex) &&
      grandParentType == ComponentType::Rectangular)
    return true;
  return false;
}
}
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

/** Tests whether or not the detector is within a Rectangular bank. Only returns
true if the bank type is a Grid or Rectangular.
@param compInfo ComponentInfo which defines instrument tree for testing
@param detIndex Index of the detector to be tested
@returns True if the detector is in a rectangular bank.
*/
bool isGridDetectorPixel(const ComponentInfo &compInfo, const size_t detIndex) {
  return isRectangularBankPixel(compInfo, detIndex) ||
         isGridBankPixel(compInfo, detIndex);
}

} // namespace ComponentInfoBankHelpers
} // namespace Geometry
} // namespace Mantid
