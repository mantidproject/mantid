#include "InstrumentParameters.h"
namespace MantidQt {
namespace CustomInterfaces {
InstrumentParameters::InstrumentParameters(
    Mantid::Geometry::Instrument_const_sptr instrument)
    : m_instrument(std::move(instrument)) {}

std::vector<InstrumentParameterTypeMissmatch> const &
InstrumentParameters::typeErrors() const {
  return m_typeErrors;
}

bool InstrumentParameters::hasTypeErrors() const {
  return !m_typeErrors.empty();
}

std::vector<MissingInstrumentParameterValue> const &
InstrumentParameters::missingValues() const {
  return m_missingValueErrors;
}

bool InstrumentParameters::hasMissingValues() const {
  return !m_missingValueErrors.empty();
}

std::string const &MissingInstrumentParameterValue::parameterName() const {
  return m_parameterName;
}
} // namespace CustomInterfaces
} // namespace MantidQt
