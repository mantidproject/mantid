// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "OptionDefaults.h"

#include <utility>

#include "MantidAPI/AlgorithmManager.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

OptionDefaults::OptionDefaults(Mantid::Geometry::Instrument_const_sptr instrument)
    : m_instrument(std::move(instrument)) {
  // Get the algorithm for which we'll take defaults if available
  m_algorithm = Mantid::API::AlgorithmManager::Instance().createUnmanaged("ReflectometryReductionOneAuto");
  m_algorithm->initialize();
}

int OptionDefaults::getIntOrZero(std::string const &propertyName, std::string const &parameterName) const {
  return getValueOrDefault<int>(propertyName, parameterName, 0);
}

double OptionDefaults::getDoubleOrZero(std::string const &propertyName, std::string const &parameterName) const {
  return getValueOrDefault<double>(propertyName, parameterName, 0.0);
}

bool OptionDefaults::getBoolOrFalse(std::string const &propertyName, std::string const &parameterName) const {
  return getValueOrDefault<bool>(propertyName, parameterName, false);
}

bool OptionDefaults::getBoolOrTrue(std::string const &propertyName, std::string const &parameterName) const {
  return getValueOrDefault<bool>(propertyName, parameterName, true);
}

std::string OptionDefaults::getStringOrDefault(std::string const &propertyName, std::string const &parameterName,
                                               std::string const &defaultValue) const {
  return getValueOrDefault<std::string>(propertyName, parameterName, defaultValue);
}

std::string OptionDefaults::getStringOrEmpty(std::string const &propertyName, std::string const &parameterName) const {
  return getValueOrDefault<std::string>(propertyName, parameterName, "");
}

std::string OptionDefaults::getString(std::string const &propertyName, std::string const &parameterName) const {
  return getValue<std::string>(propertyName, parameterName);
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
