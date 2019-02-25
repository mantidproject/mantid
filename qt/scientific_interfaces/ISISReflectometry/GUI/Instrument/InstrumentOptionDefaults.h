// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_INSTRUMENTOPTIONDEFAULTS_H
#define MANTID_ISISREFLECTOMETRY_INSTRUMENTOPTIONDEFAULTS_H
#include "Common/DllConfig.h"
#include "MantidAPI/BoostOptionalToAlgorithmProperty.h"
#include "MantidGeometry/Instrument_fwd.h"
#include "Reduction/Instrument.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** @class InstrumentOptionDefaults

    A class to create an Instrument model with defaults from the parameters file
    or algorithm defaults for a given instrument
*/
class InstrumentOptionDefaults {
public:
  InstrumentOptionDefaults(Mantid::Geometry::Instrument_const_sptr instrument);
  Instrument operator()() const;

private:
  Mantid::API::Algorithm_sptr m_algorithm;
  Mantid::Geometry::Instrument_const_sptr m_instrument;

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
};

template <typename T>
T InstrumentOptionDefaults::getValueOrDefault(std::string const &propertyName,
                                              std::string const &parameterName,
                                              T defaultValue) const {
  auto maybeValue = Mantid::API::checkForOptionalInstrumentDefault<T>(
      m_algorithm.get(), propertyName, m_instrument, parameterName);
  if (maybeValue.is_initialized())
    return maybeValue.get();
  return defaultValue;
}

template <typename T>
T InstrumentOptionDefaults::getValue(std::string const &propertyName,
                                     std::string const &parameterName) const {
  return Mantid::API::checkForMandatoryInstrumentDefault<T>(
      m_algorithm.get(), propertyName, m_instrument, parameterName);
}
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_INSTRUMENTOPTIONDEFAULTS_H
