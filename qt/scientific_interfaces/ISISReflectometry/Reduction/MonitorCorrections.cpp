// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MonitorCorrections.h"

#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

MonitorCorrections::MonitorCorrections(size_t monitorIndex, bool integrate,
                                       std::optional<RangeInLambda> backgroundRange,
                                       std::optional<RangeInLambda> integralRange)
    : m_monitorIndex(monitorIndex), m_integrate(integrate), m_backgroundRange(std::move(backgroundRange)),
      m_integralRange(std::move(integralRange)) {}

size_t MonitorCorrections::monitorIndex() const { return m_monitorIndex; }

bool MonitorCorrections::integrate() const { return m_integrate; }

std::optional<RangeInLambda> MonitorCorrections::backgroundRange() const { return m_backgroundRange; }

std::optional<RangeInLambda> MonitorCorrections::integralRange() const { return m_integralRange; }

bool operator!=(MonitorCorrections const &lhs, MonitorCorrections const &rhs) { return !(lhs == rhs); }

bool operator==(MonitorCorrections const &lhs, MonitorCorrections const &rhs) {
  return lhs.monitorIndex() == rhs.monitorIndex() && lhs.integrate() == rhs.integrate() &&
         lhs.backgroundRange() == rhs.backgroundRange() && lhs.integralRange() == rhs.integralRange();
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
