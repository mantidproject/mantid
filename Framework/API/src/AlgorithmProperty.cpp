// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/Algorithm.h"

#include <json/value.h>

namespace Mantid {
namespace API {

//-----------------------------------------------------------------------------
// Public member functions
//-----------------------------------------------------------------------------

/** Constructor
 *  @param propName :: The name to assign to the property
 *  @param validator :: The validator to use for this property (this class will
 * take ownership of the validator)
 *  @param direction :: Whether this is a Direction::Input, Direction::Output or
 * Direction::InOut (Input & Output) property
 */
AlgorithmProperty::AlgorithmProperty(const std::string &propName,
                                     Kernel::IValidator_sptr validator,
                                     unsigned int direction)
    : Kernel::PropertyWithValue<HeldType>(propName, HeldType(), validator,
                                          direction),
      m_algmStr() {}

/**
 * Return the algorithm as string
 * @returns The algorithm serialized as a string
 */
std::string AlgorithmProperty::value() const { return m_algmStr; }

/**
 * @return A Json::Value objectValue encoding the algorithm
 */
Json::Value AlgorithmProperty::valueAsJson() const {
  return (*this)()->toJson();
}

/**
 * Get the default
 * @returns An empty string
 */
std::string AlgorithmProperty::getDefault() const { return ""; }

/**
 * Set value of the algorithm
 * Attempts to create an Algorithm object
 * @param value :: The string format for an algorithm, @see Algorithm::toString
 * @return An empty string if the value is valid, otherwise the string will
 * contain the error
 */
std::string AlgorithmProperty::setValue(const std::string &value) {
  try {
    return setBaseValue(Algorithm::fromString(value));
  } catch (std::exception &exc) {
    return exc.what();
  }
}

/**
 * Set the value of the algorithm property from a Json value
 * @param value A reference
 * @return An empty string if the value is valid, otherwise the string will
 * contain the error
 */
std::string AlgorithmProperty::setValueFromJson(const Json::Value &value) {
  try {
    return setBaseValue(Algorithm::fromJson(value));
  } catch (std::exception &exc) {
    return exc.what();
  }
}

/**
 * Set the value from the algorithm pointer
 * @param algm
 * @return An empty string if the value is valid, otherwise the string will
 * contain the error
 */
std::string
AlgorithmProperty::setBaseValue(const AlgorithmProperty::HeldType &algm) {
  std::string message;
  try {
    Kernel::PropertyWithValue<IAlgorithm_sptr>::m_value = algm;
  } catch (std::exception &e) {
    message = e.what();
  }

  if (message.empty()) {
    m_algmStr = algm->toString();
    return isValid();
  } else
    return message;
}

} // namespace API
} // namespace Mantid
