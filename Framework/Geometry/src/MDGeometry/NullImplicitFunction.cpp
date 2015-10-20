#include "MantidGeometry/MDGeometry/NullImplicitFunction.h"

namespace Mantid {
namespace Geometry {

NullImplicitFunction::NullImplicitFunction() {}

NullImplicitFunction::~NullImplicitFunction() {}

std::string NullImplicitFunction::getName() const {
  return NullImplicitFunction::functionName();
}

std::string NullImplicitFunction::toXMLString() const { return std::string(); }
}
}
