// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "RangeInQ.h"
#include <cassert>
#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

RangeInQ::RangeInQ(std::optional<double> min, std::optional<double> step, std::optional<double> max)
    : m_min(std::move(min)), m_step(std::move(step)), m_max(std::move(max)) {
  // only executes in DEBUG builds
  assert(!(m_min.has_value() && m_max.has_value() && m_max < m_min));
}

std::optional<double> RangeInQ::min() const { return m_min; }

std::optional<double> RangeInQ::max() const { return m_max; }

std::optional<double> RangeInQ::step() const { return m_step; }

bool operator==(RangeInQ const &lhs, RangeInQ const &rhs) {
  return lhs.min() == rhs.min() && lhs.max() == rhs.max() && lhs.step() == rhs.step();
}

bool operator!=(RangeInQ const &lhs, RangeInQ const &rhs) { return !(lhs == rhs); }
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
