#include "MantidKernel/InvisibleProperty.h"

namespace Mantid {
namespace Kernel {

/// Is the property to be shown in the GUI? Always false.
bool InvisibleProperty::isVisible(const IPropertyManager *) const {
  return false;
}

/// Make a copy of the present type of IPropertySettings
IPropertySettings *InvisibleProperty::clone() const {
  return new InvisibleProperty(*this);
}

} // namespace Kernel
} // namespace Mantid
