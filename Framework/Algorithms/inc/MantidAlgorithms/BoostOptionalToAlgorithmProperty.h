#ifndef MANTID_ALGORITHMS_BOOSTOPTIONALTOALGORITHMPROPERTY_H_
#define MANTID_ALGORITHMS_BOOSTOPTIONALTOALGORITHMPROPERTY_H_

#include <boost/optional.hpp>

namespace BoostOptionalToAlgorithmProperty{


template <typename T>
T checkForMandatoryDefault(Mantid::API::Algorithm* const alg, std::string propName,
                           Mantid::Geometry::Instrument_const_sptr instrument,
                           std::string idf_name){
  auto algProperty = alg->getPointerToProperty(propName);
  if (algProperty->isDefault()) {
    auto defaults = instrument->getNumberParameter(idf_name);
    if (defaults.size() == 0) {
      throw std::runtime_error("No data could be retrieved from the parameters "
                               "and argument wasn't provided: " +
                               propName);
    }
    return static_cast<T>(defaults[0]);
  } else {
    return static_cast<T>(boost::lexical_cast<double, std::string>(algProperty->value()));
  }
}

template <typename T>
boost::optional<T>
checkForOptionalDefault(Mantid::API::Algorithm* const alg, std::string propName,
                        Mantid::Geometry::Instrument_const_sptr instrument,
                        std::string idf_name){
  auto algProperty = alg->getPointerToProperty(propName);
  if (algProperty->isDefault()) {
    auto defaults = instrument->getNumberParameter(idf_name);
    if (defaults.size() != 0) {
      return boost::optional<T>(static_cast<T>(defaults[0]));
    } else {
      return boost::optional<T>();
    }
  } else {
    double value =
        boost::lexical_cast<double, std::string>(algProperty->value());
    return boost::optional<T>(static_cast<T>(value));
  }
}

} //namespace

#endif // MANTID_ALGORITHMS_BOOSTOPTIONALTOALGORITHMPROPERTY_H_