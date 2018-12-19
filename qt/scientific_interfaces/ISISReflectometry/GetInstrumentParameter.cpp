// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "GetInstrumentParameter.h"
namespace MantidQt {
namespace CustomInterfaces {

std::vector<std::string> InstrumentParameter<std::string>::get(
    Mantid::Geometry::Instrument_const_sptr instrument,
    std::string const &parameterName) {
  try {
    return instrument->getStringParameter(parameterName);
  } catch (std::runtime_error const &ex) {
    throw InstrumentParameterTypeMissmatch(parameterName, "string", ex);
  }
}

std::vector<int> InstrumentParameter<int>::get(
    Mantid::Geometry::Instrument_const_sptr instrument,
    std::string const &parameterName) {
  try {
    return instrument->getIntParameter(parameterName);
  } catch (std::runtime_error const &ex) {
    throw InstrumentParameterTypeMissmatch(parameterName, "int", ex);
  }
}

std::vector<bool> InstrumentParameter<bool>::get(
    Mantid::Geometry::Instrument_const_sptr instrument,
    std::string const &parameterName) {
  try {
    return instrument->getBoolParameter(parameterName);
  } catch (std::runtime_error const &ex) {
    throw InstrumentParameterTypeMissmatch(parameterName, "bool", ex);
  }
}

std::vector<double> InstrumentParameter<double>::get(
    Mantid::Geometry::Instrument_const_sptr instrument,
    std::string const &parameterName) {
  try {
    return instrument->getNumberParameter(parameterName);
  } catch (std::runtime_error const &ex) {
    throw InstrumentParameterTypeMissmatch(parameterName, "double", ex);
  }
}

InstrumentParameterTypeMissmatch::InstrumentParameterTypeMissmatch(
    std::string const &parameterName, std::string const &expectedType,
    std::runtime_error const &ex)
    : std::runtime_error(std::string("Instrument parameter '") + parameterName +
                         std::string("' does not have the expected type '") +
                         expectedType +
                         std::string("'.\n Original Message: \n") + ex.what()),
      m_parameterName(parameterName), m_expectedType(expectedType),
      m_originalMessage(ex.what()) {}

std::string const &InstrumentParameterTypeMissmatch::parameterName() const {
  return m_parameterName;
}

std::string const &InstrumentParameterTypeMissmatch::expectedType() const {
  return m_expectedType;
}

std::string const &InstrumentParameterTypeMissmatch::originalMessage() const {
  return m_originalMessage;
}
} // namespace CustomInterfaces
} // namespace MantidQt
