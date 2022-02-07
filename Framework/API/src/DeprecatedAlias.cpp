// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/DeprecatedAlias.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidKernel/Logger.h"
#include "MantidTypes/Core/DateAndTimeHelpers.h"

namespace Mantid::API {

namespace {
Kernel::Logger g_log("DeprecatedAlias");
} // namespace

/// Constructor to ensure the compiler is happy
DeprecatedAlias::DeprecatedAlias() : m_deprecationDate() {}

/// Destructor to ensure the compiler is happy
DeprecatedAlias::~DeprecatedAlias() = default;

/**
 * @brief Set the deprecation date which will be used to inform the users
 *
 * @param date : deprecation date in ISO8601 format
 */
void DeprecatedAlias::setDeprecationDate(const std::string &date) {
  m_deprecationDate = "";
  if ((!date.empty()) && (Types::Core::DateAndTimeHelpers::stringIsISO8601(date))) {
    m_deprecationDate = date;
  } else {
    throw std::invalid_argument("DeprecatedAlias::DeprecationDate(): deprecation date must follow ISO8601.");
  }
}

/**
 * @brief Construct and return a full deprecation message
 *
 * @param algo
 * @return std::string
 */
std::string DeprecatedAlias::deprecationMessage(const IAlgorithm *algo) {
  std::stringstream msg;

  auto alias = algo->alias();

  if (alias.empty()) {
    throw std::logic_error("Cannot find the deprecated alias for this algorithm.");
  } else {
    msg << "The algorithm '" << alias << "' is deprecated on " << m_deprecationDate << "."
        << "Please use '" << algo->name() << "' instead.";
  }

  return msg.str();
}

} // namespace Mantid::API
