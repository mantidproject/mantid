#include "MantidKernel/OptionalBool.h"

namespace Mantid {
namespace Kernel {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
OptionalBool::OptionalBool() : m_arg(Unset) {}

OptionalBool::OptionalBool(bool arg) : m_arg(m_arg = arg ? True : False) {}

OptionalBool::OptionalBool(Value arg) : m_arg(arg) {}

OptionalBool::~OptionalBool() {}

OptionalBool::OptionalBool(const OptionalBool &other) : m_arg(other.m_arg) {}

OptionalBool &OptionalBool::operator=(const OptionalBool &other) {
  if (this != &other) {
    m_arg = other.m_arg;
  }
  return *this;
}

bool OptionalBool::operator==(const OptionalBool &other) const {
  return m_arg == other.getValue();
}

OptionalBool::Value OptionalBool::getValue() const { return m_arg; }

} // namespace Kernel
} // namespace Mantid
