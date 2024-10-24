// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "MantidAPI/BoostOptionalToAlgorithmProperty.h"
#include "MantidGeometry/Instrument_fwd.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class OptionDefaults

    A helper class to get defaults from an algorithm or parameters file
*/
class OptionDefaults {
public:
  explicit OptionDefaults(Mantid::Geometry::Instrument_const_sptr instrument, std::string const &algorithmName);

  template <typename T>
  T getValueOrDefault(std::string const &propertyName, std::string const &parameterName, T defaultValue) const;
  template <typename T>
  boost::optional<T> getOptionalValue(std::string const &propertyName, std::string const &parameterName) const;
  template <typename T> T getValue(std::string const &propertyName, std::string const &parameterName) const;

  int getIntOrZero(std::string const &propertyName, std::string const &parameterName) const;
  double getDoubleOrZero(std::string const &propertyName, std::string const &parameterName) const;
  bool getBoolOrFalse(std::string const &propertyName, std::string const &parameterName) const;
  bool getBoolOrTrue(std::string const &propertyName, std::string const &parameterName) const;
  std::string getStringOrDefault(std::string const &propertyName, std::string const &parameterName,
                                 std::string const &defaultValue) const;
  std::string getStringOrEmpty(std::string const &propertyName, std::string const &parameterName) const;
  std::string getString(std::string const &propertyName, std::string const &parameterName) const;

private:
  Mantid::API::Algorithm_sptr m_algorithm;
  Mantid::Geometry::Instrument_const_sptr m_instrument;
};

template <typename T>
T OptionDefaults::getValueOrDefault(std::string const &propertyName, std::string const &parameterName,
                                    T defaultValue) const {
  auto maybeValue =
      Mantid::API::checkForOptionalInstrumentDefault<T>(m_algorithm.get(), propertyName, m_instrument, parameterName);
  if (maybeValue.is_initialized())
    return maybeValue.get();
  return defaultValue;
}

template <typename T>
boost::optional<T> OptionDefaults::getOptionalValue(std::string const &propertyName,
                                                    std::string const &parameterName) const {
  return Mantid::API::checkForOptionalInstrumentDefault<T>(m_algorithm.get(), propertyName, m_instrument,
                                                           parameterName);
}

template <typename T>
T OptionDefaults::getValue(std::string const &propertyName, std::string const &parameterName) const {
  return Mantid::API::checkForMandatoryInstrumentDefault<T>(m_algorithm.get(), propertyName, m_instrument,
                                                            parameterName);
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
