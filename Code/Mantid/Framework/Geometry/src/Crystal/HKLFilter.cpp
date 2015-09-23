#include "MantidGeometry/Crystal/HKLFilter.h"
#include <boost/make_shared.hpp>
namespace Mantid {
namespace Geometry {

HKLFilter_const_sptr operator&(const HKLFilter_const_sptr &lhs,
                               const HKLFilter_const_sptr &rhs) {
  return boost::make_shared<const HKLFilterAnd>(lhs, rhs);
}

} // namespace Geometry
} // namespace Mantid
