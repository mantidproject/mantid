#include "GetInstrumentParameter.h"
namespace MantidQt {
namespace CustomInterfaces {

std::vector<std::string> InstrumentParameter<std::string>::get(
    Mantid::Geometry::Instrument_const_sptr instrument,
    std::string const &parameterName) {
  return instrument->getStringParameter(parameterName);
}

std::vector<int> InstrumentParameter<int>::get(
    Mantid::Geometry::Instrument_const_sptr instrument,
    std::string const &parameterName) {
  return instrument->getIntParameter(parameterName);
}

std::vector<bool> InstrumentParameter<bool>::get(
    Mantid::Geometry::Instrument_const_sptr instrument,
    std::string const &parameterName) {
  return instrument->getBoolParameter(parameterName);
}

std::vector<double> InstrumentParameter<double>::get(
    Mantid::Geometry::Instrument_const_sptr instrument,
    std::string const &parameterName) {
  return instrument->getNumberParameter(parameterName);
}
}
}
