#ifndef MANTID_ALGORITHMS_BOOSTOPTIONALTOALGORITHMPROPERTY_H_
#define MANTID_ALGORITHMS_BOOSTOPTIONALTOALGORITHMPROPERTY_H_

#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument.h"
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <string>

namespace Mantid {
namespace Algorithms {
/** BoostOptionalToAlgorithmProperty : Checks for default values of an algorithm
property
if the user has not supplied the value. If it is a mandatory property then the
value will be
returned, if the property is optional then a value of type boost::optional<T>
will be returned.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
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
T checkForMandatoryInstrumentDefault(
    Mantid::API::Algorithm *const alg, std::string propName,
    Mantid::Geometry::Instrument_const_sptr instrument, std::string idf_name) {
  auto algProperty = alg->getPointerToProperty(propName);
  if (algProperty->isDefault()) {
    auto defaults = instrument->getNumberParameter(idf_name);
    if (defaults.empty()) {
      throw std::runtime_error("No data could be retrieved from the parameters "
                               "and argument wasn't provided: " +
                               propName);
    }
    return static_cast<T>(defaults[0]);
  } else {
    return static_cast<T>(
        boost::lexical_cast<double, std::string>(algProperty->value()));
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
 * the user supplied value or an uninitialized boost::optional.
 *
 */
template <typename T>
boost::optional<T> checkForOptionalInstrumentDefault(
    Mantid::API::Algorithm *const alg, std::string propName,
    Mantid::Geometry::Instrument_const_sptr instrument, std::string idf_name) {
  auto algProperty = alg->getPointerToProperty(propName);
  if (algProperty->isDefault()) {
    auto defaults = instrument->getNumberParameter(idf_name);
    if (!defaults.empty()) {
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
} // namespace Algorithms
} // namespace Mantid

#endif // MANTID_ALGORITHMS_BOOSTOPTIONALTOALGORITHMPROPERTY_H_