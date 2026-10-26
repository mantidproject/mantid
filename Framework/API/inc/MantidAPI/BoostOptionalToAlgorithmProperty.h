// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include <boost/lexical_cast.hpp>
#include <optional>
#include <string>
#include <type_traits>

namespace Mantid {
namespace API {
/** BoostOptionalToAlgorithmProperty : Checks for default values of an
algorithm property if the user has not supplied the value. If it is a mandatory
property then the value will be returned, if the property is optional then a
value of type std::optional<T> will be returned.
*/

/**
 * Checks for the default values of a mandatory algorithm property associated
 * with
 * an instrument component. i.e MonitorIndex
 *
 * @param alg : A pointer to the algorithm to which the property belongs
 * @param propName : The name of the property in the algorithm
 * @param instrument : A pointer to the instrument
 * @param idf_name : The name of the property in the Instrument Defintion
 * @return A value of type T that is either the default value or the user
 * supplied value.
 *
 */
template <typename T>
T checkForMandatoryInstrumentDefault(const Mantid::API::Algorithm *const alg, const std::string &propName,
                                     const Mantid::Geometry::Instrument_const_sptr &instrument,
                                     const std::string &idf_name) {
  const auto *algProperty = alg->getPointerToProperty(propName);
  if (algProperty->isDefault()) {
    auto defaults = instrument->getNumberParameter(idf_name);
    if (defaults.empty()) {
      throw std::runtime_error("No data could be retrieved from the parameters "
                               "and argument wasn't provided: " +
                               propName);
    }
    return static_cast<T>(defaults[0]);
  } else {
    return static_cast<T>(boost::lexical_cast<double, std::string>(algProperty->value()));
  }
}

/**
 * Checks for the default values of an optional algorithm property associated
 * with
 * an instrument component. i.e MonitorIndex
 *
 * @param alg : A pointer to the algorithm to which the property belongs
 * @param propName : The name of the property in the algorithm
 * @param instrument : A pointer to the instrument
 * @param idf_name : The name of the property in the Instrument Defintion
 * @return A boost optional value of type T that is either the default value,
 * the user supplied value or an uninitialized std::optional.
 *
 */
template <typename T>
std::optional<T> checkForOptionalInstrumentDefault(const Mantid::API::Algorithm *const alg, const std::string &propName,
                                                   const Mantid::Geometry::Instrument_const_sptr &instrument,
                                                   const std::string &idf_name) {
  const auto *algProperty = alg->getPointerToProperty(propName);
  if (algProperty->isDefault()) {
    auto defaults = instrument->getNumberParameter(idf_name);
    if (!defaults.empty()) {
      return std::optional<T>(static_cast<T>(defaults[0]));
    } else {
      return std::nullopt;
    }
  } else {
    double value = boost::lexical_cast<double, std::string>(algProperty->value());
    return std::optional<T>(static_cast<T>(value));
  }
}

/**
 * Checks for the default values of a mandatory algorithm property associated
 * with the instrument, read from the root component's parameters.
 *
 * @param alg : A pointer to the algorithm to which the property belongs
 * @param propName : The name of the property in the algorithm
 * @param componentInfo : The instrument's component info
 * @param idf_name : The name of the property in the Instrument Defintion
 * @return A value of type T that is either the default value or the user
 * supplied value.
 */
template <typename T>
T checkForMandatoryInstrumentDefault(Mantid::API::Algorithm const *const alg, std::string const &propName,
                                     Mantid::Geometry::ComponentInfo const &componentInfo,
                                     std::string const &idf_name) {
  auto const *algProperty = alg->getPointerToProperty(propName);
  if constexpr (std::is_same_v<T, std::string>) {
    if (!algProperty->isDefault()) {
      return algProperty->value();
    }
    auto const defaults = componentInfo.getStringParameter(componentInfo.root(), idf_name);
    if (defaults.empty()) {
      throw std::runtime_error("No data could be retrieved from the parameters "
                               "and argument wasn't provided: " +
                               propName);
    }
    return defaults[0];
  } else {
    if (!algProperty->isDefault()) {
      return static_cast<T>(boost::lexical_cast<double, std::string>(algProperty->value()));
    }
    auto const defaults = componentInfo.getNumberParameter(componentInfo.root(), idf_name);
    if (defaults.empty()) {
      throw std::runtime_error("No data could be retrieved from the parameters "
                               "and argument wasn't provided: " +
                               propName);
    }
    return static_cast<T>(defaults[0]);
  }
}

/**
 * Checks for the default values of an optional algorithm property associated
 * with the instrument, read from the root component's parameters.
 *
 * @param alg : A pointer to the algorithm to which the property belongs
 * @param propName : The name of the property in the algorithm
 * @param componentInfo : The instrument's component info
 * @param idf_name : The name of the property in the Instrument Defintion
 * @return A std::optional value of type T that is either the default value,
 * the user supplied value or an uninitialized std::optional.
 */
template <typename T>
std::optional<T> checkForOptionalInstrumentDefault(Mantid::API::Algorithm const *const alg, std::string const &propName,
                                                   Mantid::Geometry::ComponentInfo const &componentInfo,
                                                   std::string const &idf_name) {
  auto const *algProperty = alg->getPointerToProperty(propName);
  if constexpr (std::is_same_v<T, std::string>) {
    if (!algProperty->isDefault()) {
      return algProperty->value();
    }
    auto const defaults = componentInfo.getStringParameter(componentInfo.root(), idf_name);
    if (defaults.empty()) {
      return std::nullopt;
    }
    return defaults[0];
  } else {
    if (!algProperty->isDefault()) {
      return static_cast<T>(boost::lexical_cast<double, std::string>(algProperty->value()));
    }
    auto const defaults = componentInfo.getNumberParameter(componentInfo.root(), idf_name);
    if (defaults.empty()) {
      return std::nullopt;
    }
    return static_cast<T>(defaults[0]);
  }
}

/**
 * Specializations for std::string
 */
template <>
MANTID_API_DLL std::string checkForMandatoryInstrumentDefault(const Mantid::API::Algorithm *const alg,
                                                              const std::string &propName,
                                                              const Mantid::Geometry::Instrument_const_sptr &instrument,
                                                              const std::string &idf_name);

template <>
MANTID_API_DLL std::optional<std::string>
checkForOptionalInstrumentDefault(const Mantid::API::Algorithm *const alg, const std::string &propName,
                                  const Mantid::Geometry::Instrument_const_sptr &instrument,
                                  const std::string &idf_name);
} // namespace API
} // namespace Mantid
