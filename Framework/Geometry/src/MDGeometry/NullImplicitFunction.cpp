#include "MantidGeometry/MDGeometry/NullImplicitFunction.h"

namespace Mantid {
namespace Geometry {

std::string NullImplicitFunction::getName() const {
  return NullImplicitFunction::functionName();
}

std::string NullImplicitFunction::toXMLString() const { return std::string(); }
} // namespace Geometry
} // namespace Mantid
