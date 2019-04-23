// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "Instrument.h"
namespace MantidQt {
namespace CustomInterfaces {

Instrument::Instrument(boost::optional<RangeInLambda> wavelengthRange,
                       MonitorCorrections monitorCorrections,
                       DetectorCorrections detectorCorrections)
    : m_wavelengthRange(std::move(wavelengthRange)),
      m_monitorCorrections(std::move(monitorCorrections)),
      m_detectorCorrections(std::move(detectorCorrections)) {}

boost::optional<RangeInLambda> const &Instrument::wavelengthRange() const {
  return m_wavelengthRange;
}

MonitorCorrections const &Instrument::monitorCorrections() const {
  return m_monitorCorrections;
}

DetectorCorrections const &Instrument::detectorCorrections() const {
  return m_detectorCorrections;
}

size_t Instrument::monitorIndex() const {
  return m_monitorCorrections.monitorIndex();
}

bool Instrument::integratedMonitors() const {
  return m_monitorCorrections.integrate();
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

bool operator!=(Instrument const &lhs, Instrument const &rhs) {
  return !(lhs == rhs);
}

bool operator==(Instrument const &lhs, Instrument const &rhs) {
  return lhs.wavelengthRange() == rhs.wavelengthRange() &&
         lhs.monitorCorrections() == rhs.monitorCorrections() &&
         lhs.detectorCorrections() == rhs.detectorCorrections();
}
} // namespace CustomInterfaces
} // namespace MantidQt
