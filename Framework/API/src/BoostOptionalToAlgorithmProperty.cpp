// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/BoostOptionalToAlgorithmProperty.h"

namespace Mantid {
namespace API {
template <>
std::string checkForMandatoryInstrumentDefault(
    Mantid::API::Algorithm *const alg, std::string propName,
    Mantid::Geometry::Instrument_const_sptr instrument, std::string idf_name) {
  auto algProperty = alg->getPointerToProperty(propName);
  if (algProperty->isDefault()) {
    auto defaults = instrument->getStringParameter(idf_name);
    if (defaults.empty()) {
      throw std::runtime_error("No data could be retrieved from the parameters "
                               "and argument wasn't provided: " +
                               propName);
    }
    return defaults[0];
  } else {
    return algProperty->value();
  }
}

template <>
boost::optional<std::string> checkForOptionalInstrumentDefault(
    Mantid::API::Algorithm *const alg, std::string propName,
    Mantid::Geometry::Instrument_const_sptr instrument, std::string idf_name) {
  auto algProperty = alg->getPointerToProperty(propName);
  if (algProperty->isDefault()) {
    auto defaults = instrument->getStringParameter(idf_name);
    if (!defaults.empty()) {
      return boost::optional<std::string>(defaults[0]);
    } else {
      return boost::optional<std::string>();
    }
  } else {
    return boost::optional<std::string>(algProperty->value());
  }
}
} // namespace API
} // namespace Mantid
