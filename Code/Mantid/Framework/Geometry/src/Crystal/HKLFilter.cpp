#include "MantidGeometry/Crystal/HKLFilter.h"
#include <boost/make_shared.hpp>
namespace Mantid {
namespace Geometry {

const HKLFilter_const_sptr operator&(const HKLFilter_const_sptr &lhs,
                                     const HKLFilter_const_sptr &rhs) {
  return boost::make_shared<const HKLFilterAnd>(lhs, rhs);
}

const HKLFilter_const_sptr operator~(const HKLFilter_const_sptr &filter) {
  return boost::make_shared<const HKLFilterNot>(filter);
}

} // namespace Geometry
} // namespace Mantid
