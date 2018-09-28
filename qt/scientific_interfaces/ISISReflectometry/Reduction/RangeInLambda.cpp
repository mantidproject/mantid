#include "RangeInLambda.h"
#include "MantidKernel/Tolerance.h"
namespace MantidQt {
namespace CustomInterfaces {

RangeInLambda::RangeInLambda(double min, double max) : m_min(min), m_max(max) {}

bool RangeInLambda::isValid() const {
  return m_min <= m_max + Mantid::Kernel::Tolerance;
}

double RangeInLambda::min() const { return m_min; }

double RangeInLambda::max() const { return m_max; }

bool operator==(RangeInLambda const &lhs, RangeInLambda const &rhs) {
  return lhs.min() == rhs.min() && lhs.max() == rhs.max();
}

bool operator!=(RangeInLambda const &lhs, RangeInLambda const &rhs) {
  return !(lhs == rhs);
}
} // namespace CustomInterfaces
} // namespace MantidQt
