#ifndef MANTID_GEOMETRY_COMPONENTINFOBANKHELPERS_H_
#define MANTID_GEOMETRY_COMPONENTINFOBANKHELPERS_H_

#include "MantidGeometry/DllConfig.h"
#include <utility>

namespace Mantid {
namespace Geometry {

class ComponentInfo;
namespace ComponentInfoBankHelpers {

MANTID_GEOMETRY_DLL bool isDetectorFixedInBank(const ComponentInfo &compInfo,
                                               const size_t detIndex);

MANTID_GEOMETRY_DLL std::pair<size_t, size_t>
findRowColIndexForRectangularBank(const ComponentInfo &compInfo,
                                  const size_t detIndex);
} // namespace ComponentInfoBankHelpers

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_COMPONENTINFOBANKHELPERS_H_ */
