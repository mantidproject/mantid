#include "RangeInQ.h"
#include <cassert>
namespace MantidQt {
namespace CustomInterfaces {

RangeInQ::RangeInQ(double min, double step, double max)
    : m_min(min), m_step(step), m_max(max) {
  assert(min < max);
}

double RangeInQ::min() const { return m_min; }

double RangeInQ::max() const { return m_max; }

double RangeInQ::step() const { return m_step; }

bool operator==(RangeInQ const &lhs, RangeInQ const &rhs) {
  return lhs.min() == rhs.min() && lhs.max() == rhs.max() &&
         lhs.step() == rhs.step();
}

bool operator!=(RangeInQ const &lhs, RangeInQ const &rhs) {
  return !(lhs == rhs);
}
} // namespace CustomInterfaces
} // namespace MantidQt
