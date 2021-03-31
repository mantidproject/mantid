// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "RangeInQ.h"
#include <cassert>
#include <utility>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

RangeInQ::RangeInQ(boost::optional<double> min, boost::optional<double> step, boost::optional<double> max)
    : m_min(std::move(min)), m_step(std::move(step)), m_max(std::move(max)) {
  assert(!(m_min.is_initialized() && m_max.is_initialized() && m_max < m_min));
}

boost::optional<double> RangeInQ::min() const { return m_min; }

boost::optional<double> RangeInQ::max() const { return m_max; }

boost::optional<double> RangeInQ::step() const { return m_step; }

bool operator==(RangeInQ const &lhs, RangeInQ const &rhs) {
  return lhs.min() == rhs.min() && lhs.max() == rhs.max() && lhs.step() == rhs.step();
}

bool operator!=(RangeInQ const &lhs, RangeInQ const &rhs) { return !(lhs == rhs); }
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
