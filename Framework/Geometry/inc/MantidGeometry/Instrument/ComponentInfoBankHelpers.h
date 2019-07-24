#ifndef MANTID_GEOMETRY_COMPONENTINFOBANKHELPERS_H_
#define MANTID_GEOMETRY_COMPONENTINFOBANKHELPERS_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/EigenConversionHelpers.h"

namespace Mantid {
namespace Geometry {

class ComponentInfo;
namespace ComponentInfoBankHelpers {

MANTID_GEOMETRY_DLL bool isDetectorFixedInBank(const ComponentInfo &compInfo,
                                               const size_t detIndex);
MANTID_GEOMETRY_DLL bool isAnyBank(const ComponentInfo &compInfo,
                                   const size_t &idx);

MANTID_GEOMETRY_DLL Eigen::Vector3d
offsetFromAncestor(const Mantid::Geometry::ComponentInfo &compInfo,
                   const size_t ancestorIdx, const size_t &currentIdx);
} // namespace ComponentInfoBankHelpers

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_COMPONENTINFOBANKHELPERS_H_ */
