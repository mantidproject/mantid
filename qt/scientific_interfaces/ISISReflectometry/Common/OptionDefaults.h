// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_OPTIONDEFAULTS_H
#define MANTID_ISISREFLECTOMETRY_OPTIONDEFAULTS_H
#include "Common/DllConfig.h"
#include "MantidAPI/BoostOptionalToAlgorithmProperty.h"
#include "MantidGeometry/Instrument_fwd.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** @class OptionDefaults

    A helper class to get defaults from an algorithm or parameters file
*/
class OptionDefaults {
public:
  OptionDefaults(Mantid::Geometry::Instrument_const_sptr instrument);

  template <typename T>
  T getValueOrDefault(std::string const &propertyName,
                      std::string const &parameterName, T defaultValue) const;
  template <typename T>
  T getValue(std::string const &propertyName,
             std::string const &parameterName) const;

  int getIntOrZero(std::string const &propertyName,
                   std::string const &parameterName) const;
  double getDoubleOrZero(std::string const &propertyName,
                         std::string const &parameterName) const;
  bool getBoolOrFalse(std::string const &propertyName,
                      std::string const &parameterName) const;
  std::string getStringOrDefault(std::string const &propertyName,
                                 std::string const &parameterName,
                                 std::string const &defaultValue) const;
  std::string getString(std::string const &propertyName,
                        std::string const &parameterName) const;

private:
  Mantid::API::Algorithm_sptr m_algorithm;
  Mantid::Geometry::Instrument_const_sptr m_instrument;
};

template <typename T>
T OptionDefaults::getValueOrDefault(std::string const &propertyName,
                                    std::string const &parameterName,
                                    T defaultValue) const {
  auto maybeValue = Mantid::API::checkForOptionalInstrumentDefault<T>(
      m_algorithm.get(), propertyName, m_instrument, parameterName);
  if (maybeValue.is_initialized())
    return maybeValue.get();
  return defaultValue;
}

template <typename T>
T OptionDefaults::getValue(std::string const &propertyName,
                           std::string const &parameterName) const {
  return Mantid::API::checkForMandatoryInstrumentDefault<T>(
      m_algorithm.get(), propertyName, m_instrument, parameterName);
}
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_OPTIONDEFAULTS_H
