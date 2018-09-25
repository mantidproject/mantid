#include "Instrument.h"
namespace MantidQt {
namespace CustomInterfaces {

Instrument::Instrument(RangeInLambda wavelengthRange,
                       MonitorCorrections monitorCorrections,
                       DetectorCorrections detectorCorrections)
    : m_wavelengthRange(wavelengthRange),
      m_monitorCorrections(monitorCorrections),
      m_detectorCorrections(detectorCorrections) {}

RangeInLambda const &Instrument::wavelengthRange() const {
  return m_wavelengthRange;
}

MonitorCorrections Instrument::monitorCorrections() const {
  return m_monitorCorrections;
}

DetectorCorrections Instrument::detectorCorrections() const {
  return m_detectorCorrections;
}
}
}
