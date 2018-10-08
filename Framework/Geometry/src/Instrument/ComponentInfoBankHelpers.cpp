#include "MantidGeometry/Instrument/ComponentInfoBankHelpers.h"
#include "MantidBeamline/ComponentType.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"

using Mantid::Beamline::ComponentType;

namespace Mantid {
namespace Geometry {
namespace ComponentInfoBankHelpers {
bool isDetectorFixedInBank(const ComponentInfo &compInfo,
                           const size_t detIndex) {
  auto parent = compInfo.parent(detIndex);
  auto grandParent = compInfo.parent(parent);
  auto grandParentType = compInfo.componentType(grandParent);

  if (compInfo.isDetector(detIndex) &&
      (grandParentType == ComponentType::Rectangular ||
       grandParentType == ComponentType::Structured ||
       grandParentType == ComponentType::Grid)) {
    return true;
  }

  return false;
}
} // namespace ComponentInfoBankHelpers
} // namespace Geometry
} // namespace Mantid
