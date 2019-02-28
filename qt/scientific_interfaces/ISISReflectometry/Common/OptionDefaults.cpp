// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "OptionDefaults.h"
#include "MantidAPI/AlgorithmManager.h"

namespace MantidQt {
namespace CustomInterfaces {

OptionDefaults::OptionDefaults(
    Mantid::Geometry::Instrument_const_sptr instrument)
    : m_instrument(instrument) {
  // Get the algorithm for which we'll take defaults if available
  m_algorithm = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
      "ReflectometryReductionOneAuto");
  m_algorithm->initialize();
}

int OptionDefaults::getIntOrZero(std::string const &propertyName,
                                 std::string const &parameterName) const {
  return getValueOrDefault<int>(propertyName, parameterName, 0);
}

double OptionDefaults::getDoubleOrZero(std::string const &propertyName,
                                       std::string const &parameterName) const {
  return getValueOrDefault<double>(propertyName, parameterName, 0.0);
}

bool OptionDefaults::getBoolOrFalse(std::string const &propertyName,
                                    std::string const &parameterName) const {
  return getValueOrDefault<bool>(propertyName, parameterName, false);
}

std::string
OptionDefaults::getStringOrDefault(std::string const &propertyName,
                                   std::string const &parameterName,
                                   std::string const &defaultValue) const {
  return getValueOrDefault<std::string>(propertyName, parameterName,
                                        defaultValue);
}

std::string OptionDefaults::getString(std::string const &propertyName,
                                      std::string const &parameterName) const {
  return getValue<std::string>(propertyName, parameterName);
}
} // namespace CustomInterfaces
} // namespace MantidQt
