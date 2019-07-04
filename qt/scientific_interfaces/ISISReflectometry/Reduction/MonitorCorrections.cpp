// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MonitorCorrections.h"
namespace MantidQt {
namespace CustomInterfaces {

MonitorCorrections::MonitorCorrections(
    size_t monitorIndex, bool integrate,
    boost::optional<RangeInLambda> backgroundRange,
    boost::optional<RangeInLambda> integralRange)
    : m_monitorIndex(monitorIndex), m_integrate(integrate),
      m_backgroundRange(backgroundRange), m_integralRange(integralRange) {}

size_t MonitorCorrections::monitorIndex() const { return m_monitorIndex; }

bool MonitorCorrections::integrate() const { return m_integrate; }

boost::optional<RangeInLambda> MonitorCorrections::backgroundRange() const {
  return m_backgroundRange;
}

boost::optional<RangeInLambda> MonitorCorrections::integralRange() const {
  return m_integralRange;
}

bool operator!=(MonitorCorrections const &lhs, MonitorCorrections const &rhs) {
  return !(lhs == rhs);
}

bool operator==(MonitorCorrections const &lhs, MonitorCorrections const &rhs) {
  return lhs.monitorIndex() == rhs.monitorIndex() &&
         lhs.integrate() == rhs.integrate() &&
         lhs.backgroundRange() == rhs.backgroundRange() &&
         lhs.integralRange() == rhs.integralRange();
}
} // namespace CustomInterfaces
} // namespace MantidQt
