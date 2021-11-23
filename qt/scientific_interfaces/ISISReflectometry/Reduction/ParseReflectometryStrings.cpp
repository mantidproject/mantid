// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ParseReflectometryStrings.h"
#include "AllInitialized.h"
#include "Common/Parse.h"
#include "MantidKernel/Strings.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"
#include <boost/algorithm/string.hpp>
#include <optional>
#include <set>
namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace { // unnamed
std::optional<std::vector<std::string>> parseRunNumbersOrWhitespace(std::string const &runNumberString) {
  auto runNumbers = std::vector<std::string>();
  auto runNumberCandidates = boost::tokenizer<boost::escaped_list_separator<char>>(
      runNumberString, boost::escaped_list_separator<char>("\\", ",+", "\"'"));

  for (auto const &runNumberCandidate : runNumberCandidates) {
    auto const maybeRunNumber = parseRunNumberOrWhitespace(runNumberCandidate);
    if (maybeRunNumber.has_value()) {
      runNumbers.emplace_back(maybeRunNumber.value());
    } else {
      return std::nullopt;
    }
  }

  return runNumbers;
}
} // unnamed namespace

std::optional<std::string> parseRunNumber(std::string const &runNumberString) {
  // We support any workspace name, as well as run numbers, so for just return
  // the input string, but trimmed of whitespace (or none if the result is
  // empty)
  auto result = std::string(runNumberString);
  boost::trim(result);

  if (result.empty())
    return std::nullopt;

  return result;
}

std::optional<std::string> parseRunNumberOrWhitespace(std::string const &runNumberString) {
  if (isEntirelyWhitespace(runNumberString)) {
    return std::string();
  } else {
    auto maybeRunNumber = parseRunNumber(runNumberString);
    if (maybeRunNumber.has_value())
      return maybeRunNumber;
  }
  return std::nullopt;
}

std::optional<double> parseTheta(std::string const &theta) {
  auto maybeTheta = parseNonNegativeDouble(theta);
  if (maybeTheta.has_value() && maybeTheta.value() > 0.0)
    return maybeTheta;
  else
    return std::nullopt;
}

std::optional<std::map<std::string, std::string>> parseOptions(std::string const &options) {
  try {
    return MantidQt::MantidWidgets::parseKeyValueString(options);
  } catch (std::runtime_error &) {
    return std::nullopt;
  }
}

std::optional<std::optional<std::string>> parseProcessingInstructions(std::string const &instructions) {
  if (isEntirelyWhitespace(instructions)) {
    return std::optional<std::string>(std::nullopt);
  } else {
    try {
      auto const groups = Mantid::Kernel::Strings::parseGroups<size_t>(instructions);
      return std::optional<std::string>(instructions);
    } catch (std::runtime_error &) {
      return std::nullopt;
    }
  }
  return std::nullopt;
}

std::optional<std::optional<double>> parseScaleFactor(std::string const &scaleFactor) {
  if (isEntirelyWhitespace(scaleFactor)) {
    return std::optional<double>(std::nullopt);
  }

  auto value = parseDouble(scaleFactor);
  if (value.has_value() && value != 0.0)
    return value;
  return std::nullopt;
}

boost::variant<RangeInQ, std::vector<int>> parseQRange(std::string const &min, std::string const &max,
                                                       std::string const &step) {
  auto invalidParams = std::vector<int>();
  std::optional<double> minimum;
  std::optional<double> maximum;
  std::optional<double> stepValue;

  // If any values are set, check they parse ok
  if (!isEntirelyWhitespace(min)) {
    minimum = parseNonNegativeDouble(min);
    if (!minimum.has_value())
      invalidParams.emplace_back(0);
  }

  if (!isEntirelyWhitespace(max)) {
    maximum = parseNonNegativeDouble(max);
    if (!maximum.has_value())
      invalidParams.emplace_back(1);
  }

  if (!isEntirelyWhitespace(step)) {
    stepValue = parseNonNegativeDouble(step);
    if (!stepValue.has_value())
      invalidParams.emplace_back(2);
  }

  // Check max is not less than min
  if (maximum.has_value() && minimum.has_value() && maximum.value() < minimum.value()) {
    invalidParams.emplace_back(0);
    invalidParams.emplace_back(1);
  }

  // Return errors, valid range, or unset if nothing was specified
  if (!invalidParams.empty())
    return invalidParams;
  else
    return RangeInQ(minimum, stepValue, maximum);
}

std::optional<std::vector<std::string>> parseRunNumbers(std::string const &runNumberString) {
  auto runNumbers = std::vector<std::string>();
  auto runNumberCandidates = boost::tokenizer<boost::escaped_list_separator<char>>(
      runNumberString, boost::escaped_list_separator<char>("\\", ",+", "\"'"));

  for (auto const &runNumberCandidate : runNumberCandidates) {
    auto maybeRunNumber = parseRunNumber(runNumberCandidate);
    if (maybeRunNumber.has_value()) {
      runNumbers.emplace_back(maybeRunNumber.value());
    } else {
      return std::nullopt;
    }
  }

  if (runNumbers.empty())
    return std::nullopt;
  else
    return runNumbers;
}

boost::variant<TransmissionRunPair, std::vector<int>> parseTransmissionRuns(std::string const &firstTransmissionRun,
                                                                            std::string const &secondTransmissionRun) {
  auto errorColumns = std::vector<int>();
  auto first = parseRunNumbersOrWhitespace(firstTransmissionRun);
  auto second = parseRunNumbersOrWhitespace(secondTransmissionRun);

  if (allInitialized(first, second)) {
    if (first.value().empty() && !second.value().empty()) {
      errorColumns.emplace_back(0);
      return errorColumns;
    } else {
      return TransmissionRunPair(first.value(), second.value());
    }
  } else {
    if (!first.has_value())
      errorColumns.emplace_back(0);
    if (!second.has_value())
      errorColumns.emplace_back(1);
    return errorColumns;
  }
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
