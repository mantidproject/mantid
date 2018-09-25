#include "MonitorCorrections.h"
namespace MantidQt {
namespace CustomInterfaces {

MonitorCorrections::MonitorCorrections(size_t monitorIndex, bool integrate,
                                       RangeInLambda backgroundRange,
                                       RangeInLambda integralRange)
    : m_monitorIndex(monitorIndex), m_integrate(integrate),
      m_backgroundRange(backgroundRange), m_integralRange(integralRange) {}

size_t MonitorCorrections::monitorIndex() const { return m_monitorIndex; }

bool MonitorCorrections::integrate() const { return m_integrate; }

RangeInLambda MonitorCorrections::backgroundRange() const { return m_backgroundRange; }

RangeInLambda MonitorCorrections::integralRange() const { return m_integralRange; }
}
}
