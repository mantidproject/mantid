// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AlgorithmHasProperty.h"
#include "MantidAPI/IAlgorithm.h"

#include <memory>
#include <utility>

namespace Mantid::API {

/// Constructor
AlgorithmHasProperty::AlgorithmHasProperty(std::string propName) : m_propName(std::move(propName)) {}

/**
 * Get a string representation of the type
 * @returns A string containing the validator type
 */
std::string AlgorithmHasProperty::getType() const { return "AlgorithmHasProperty"; }

/// Make a copy of the present type of validator
Kernel::IValidator_sptr AlgorithmHasProperty::clone() const { return std::make_shared<AlgorithmHasProperty>(*this); }

/**
 * Checks the value based on the validator's rules
 * @param value :: The input algorithm to check
 * @returns An error message to display to users or an empty string on no
 * error
 */
std::string AlgorithmHasProperty::checkValidity(const std::shared_ptr<IAlgorithm> &value) const {
  std::string message;
  if (value->existsProperty(m_propName)) {
    Kernel::Property const *p = value->getProperty(m_propName);
    if (!p->isValid().empty()) {
      message = "Algorithm object contains the required property \"" + m_propName +
                "\" but it has an invalid value: " + p->value();
    }
  } else {
    message = "Algorithm object does not have the required property \"" + m_propName + "\"";
  }

  return message;
}

} // namespace Mantid::API
