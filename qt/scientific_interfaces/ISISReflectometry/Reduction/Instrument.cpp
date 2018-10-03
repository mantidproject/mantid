#include "Instrument.h"
namespace MantidQt {
namespace CustomInterfaces {

Instrument::Instrument(boost::optional<RangeInLambda> wavelengthRange,
                       MonitorCorrections monitorCorrections,
                       DetectorCorrections detectorCorrections)
    : m_wavelengthRange(wavelengthRange),
      m_monitorCorrections(monitorCorrections),
      m_detectorCorrections(detectorCorrections) {}

boost::optional<RangeInLambda> const &Instrument::wavelengthRange() const {
  return m_wavelengthRange;
}

bool Instrument::integratedMonitors() const {
  return m_monitorCorrections.integrate();
}

size_t Instrument::monitorIndex() const {
  return m_monitorCorrections.monitorIndex();
}

boost::optional<RangeInLambda> Instrument::monitorIntegralRange() const {
  return m_monitorCorrections.integralRange();
}

boost::optional<RangeInLambda> Instrument::monitorBackgroundRange() const {
  return m_monitorCorrections.backgroundRange();
}

bool Instrument::correctDetectors() const {
  return m_detectorCorrections.correctPositions();
}

DetectorCorrectionType Instrument::detectorCorrectionType() const {
  return m_detectorCorrections.correctionType();
}

} // namespace CustomInterfaces
} // namespace MantidQt
