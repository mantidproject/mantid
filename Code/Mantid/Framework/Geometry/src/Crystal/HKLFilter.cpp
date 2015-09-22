#include "MantidGeometry/Crystal/HKLFilter.h"

namespace Mantid {
namespace Geometry {

HKLFilterAnd operator&(const HKLFilter &lhs, const HKLFilter &rhs) {
    return HKLFilterAnd(lhs, rhs);
}

} // namespace Geometry
} // namespace Mantid
