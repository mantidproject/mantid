// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <utility>

#include "MantidMDAlgorithms/InvalidParameter.h"

namespace Mantid {
namespace MDAlgorithms {
InvalidParameter::InvalidParameter() {}

InvalidParameter::InvalidParameter(std::string value) : m_value(std::move(value)) {}

std::string InvalidParameter::getName() const { return parameterName(); }

std::string InvalidParameter::getValue() const { return m_value; }

bool InvalidParameter::isValid() const { return false; }

InvalidParameter *InvalidParameter::clone() const { return new InvalidParameter(m_value); }

std::string InvalidParameter::toXMLString() const {
  throw std::runtime_error("Invalid parameters cannot be represented in xml.");
}
} // namespace MDAlgorithms
} // namespace Mantid
