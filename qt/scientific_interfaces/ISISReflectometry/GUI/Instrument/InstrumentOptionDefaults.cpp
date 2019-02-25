// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "InstrumentOptionDefaults.h"
#include "MantidAPI/AlgorithmManager.h"
#include "Reduction/Instrument.h"

namespace MantidQt {
namespace CustomInterfaces {

InstrumentOptionDefaults::InstrumentOptionDefaults(
    Mantid::Geometry::Instrument_const_sptr instrument)
    : m_instrument(instrument) {
  // Get the algorithm for which we'll take defaults if available
  m_algorithm = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
      "ReflectometryReductionOneAuto");
  m_algorithm->initialize();
}

int InstrumentOptionDefaults::getIntOrZero(
    std::string const &propertyName, std::string const &parameterName) const {
  return getValueOrDefault<int>(propertyName, parameterName, 0);
}

double InstrumentOptionDefaults::getDoubleOrZero(
    std::string const &propertyName, std::string const &parameterName) const {
  return getValueOrDefault<double>(propertyName, parameterName, 0.0);
}

bool InstrumentOptionDefaults::getBoolOrFalse(
    std::string const &propertyName, std::string const &parameterName) const {
  return getValueOrDefault<bool>(propertyName, parameterName, false);
}

std::string InstrumentOptionDefaults::getStringOrDefault(
    std::string const &propertyName, std::string const &parameterName,
    std::string const &defaultValue) const {
  return getValueOrDefault<std::string>(propertyName, parameterName,
                                        defaultValue);
}

std::string
InstrumentOptionDefaults::getString(std::string const &propertyName,
                                    std::string const &parameterName) const {
  return getValue<std::string>(propertyName, parameterName);
}

Instrument InstrumentOptionDefaults::operator()() const {

  auto wavelengthRange =
      RangeInLambda(getValue<double>("WavelengthMin", "LambdaMin"),
                    getValue<double>("WavelengthMax", "LambdaMax"));

  auto monitorIndex = getIntOrZero("I0MonitorIndex", "I0MonitorIndex");
  auto integrate = getBoolOrFalse("NormalizeByIntegratedMonitors",
                                  "NormalizeByIntegratedMonitors");
  auto backgroundRange = RangeInLambda(
      getDoubleOrZero("MonitorBackgroundWavelengthMin", "MonitorBackgroundMin"),
      getDoubleOrZero("MonitorBackgroundWavelengthMax",
                      "MonitorBackgroundMax"));
  auto integralRange = RangeInLambda(
      getDoubleOrZero("MonitorIntegrationWavelengthMin", "MonitorIntegralMin"),
      getDoubleOrZero("MonitorIntegrationWavelengthMax", "MonitorIntegralMax"));
  auto monitorCorrections = MonitorCorrections(monitorIndex, integrate,
                                               backgroundRange, integralRange);

  auto detectorCorrectionString = getStringOrDefault(
      "DetectorCorrectionType", "DetectorCorrectionType", "VerticalShift");
  auto detectorCorrections = DetectorCorrections(
      getBoolOrFalse("CorrectDetectors", "CorrectDetectors"),
      detectorCorrectionTypeFromString(detectorCorrectionString));

  return Instrument(std::move(wavelengthRange), std::move(monitorCorrections),
                    std::move(detectorCorrections));
}
} // namespace CustomInterfaces
} // namespace MantidQt
