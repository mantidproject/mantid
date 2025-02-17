// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/BoostOptionalToAlgorithmProperty.h"

namespace Mantid::API {
template <>
std::string checkForMandatoryInstrumentDefault(Mantid::API::Algorithm *const alg, const std::string &propName,
                                               const Mantid::Geometry::Instrument_const_sptr &instrument,
                                               const std::string &idf_name) {
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
std::optional<std::string> checkForOptionalInstrumentDefault(Mantid::API::Algorithm *const alg,
                                                             const std::string &propName,
                                                             const Mantid::Geometry::Instrument_const_sptr &instrument,
                                                             const std::string &idf_name) {
  auto algProperty = alg->getPointerToProperty(propName);
  if (algProperty->isDefault()) {
    auto defaults = instrument->getStringParameter(idf_name);
    if (!defaults.empty()) {
      return std::optional<std::string>(defaults[0]);
    } else {
      return std::nullopt;
    }
  } else {
    return std::optional<std::string>(algProperty->value());
  }
}
} // namespace Mantid::API
