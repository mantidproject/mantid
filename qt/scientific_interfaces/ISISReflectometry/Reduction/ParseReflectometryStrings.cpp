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
#include <boost/regex.hpp>
namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace { // unnamed
std::optional<std::vector<std::string>> parseRunNumbersOrWhitespace(std::string const &runNumberString) {
  auto runNumbers = std::vector<std::string>();
  auto runNumberCandidates = boost::tokenizer<boost::escaped_list_separator<char>>(
      runNumberString, boost::escaped_list_separator<char>("\\", ",+", "\"'"));

  for (auto const &runNumberCandidate : runNumberCandidates) {
    auto const maybeRunNumber = parseRunNumberOrWhitespace(runNumberCandidate);
    if (!maybeRunNumber.has_value()) {
      return std::nullopt;
    }
    runNumbers.emplace_back(maybeRunNumber.value());
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
  if (result.empty()) {
    return std::nullopt;
  }
  return result;
}

std::optional<std::string> parseRunNumberOrWhitespace(std::string const &runNumberString) {
  if (isEntirelyWhitespace(runNumberString)) {
    return std::string();
  }
  auto maybeRunNumber = parseRunNumber(runNumberString);
  if (maybeRunNumber.has_value()) {
    return maybeRunNumber;
  }
  return std::nullopt;
}

std::optional<double> parseTheta(std::string const &theta) {
  auto maybeTheta = parseNonNegativeDouble(theta);
  if (maybeTheta.has_value() && maybeTheta.value() > 0.0) {
    return maybeTheta.value();
  }
  return std::nullopt;
}

std::optional<boost::regex> parseTitleMatcher(std::string const &titleMatcher) {
  if (isEntirelyWhitespace(titleMatcher)) {
    return std::nullopt;
  }
  try {
    return boost::regex(titleMatcher);
  } catch (boost::regex_error const &) {
    return std::nullopt;
  }
}

namespace {
std::map<std::string, std::string> replaceBoolTextWithBoolValue(std::map<std::string, std::string> stitchParams) {
  for (auto &paramPair : stitchParams) {
    auto lower_value = boost::algorithm::to_lower_copy(paramPair.second); // Avoid changing the original value.
    if (lower_value == "true") {
      paramPair.second = "1";
      continue;
    }
    if (lower_value == "false") {
      paramPair.second = "0";
    }
  }
  return stitchParams;
}
} // namespace

std::optional<std::map<std::string, std::string>> parseOptions(std::string const &options) {
  try {
    return replaceBoolTextWithBoolValue(MantidQt::MantidWidgets::parseKeyValueString(options));
  } catch (std::runtime_error &) {
    return std::nullopt;
  }
}

TaggedOptional<std::string> parseProcessingInstructions(std::string const &instructions) {
  if (isEntirelyWhitespace(instructions)) {
    return std::make_pair(std::optional<std::string>(), true);
  }
  try {
    auto const groups = Mantid::Kernel::Strings::parseGroups<size_t>(instructions);
    return std::make_pair(std::optional(instructions), true);
  } catch (std::runtime_error &) {
    return std::make_pair(std::nullopt, false);
  }
}

TaggedOptional<double> parseScaleFactor(std::string const &scaleFactor) {
  if (isEntirelyWhitespace(scaleFactor)) {
    return std::make_pair(std::optional<double>(), true);
  }
  auto scaleFactorNum = parseDouble(scaleFactor);
  if (scaleFactorNum.has_value() && scaleFactorNum != 0.0) {
    return std::make_pair(std::optional<double>(scaleFactorNum.value()), true);
  }
  return std::make_pair(std::nullopt, false);
}

boost::variant<RangeInQ, std::vector<int>> parseQRange(std::string const &min, std::string const &max,
                                                       std::string const &step) {
  auto invalidParams = std::vector<int>();
  std::optional<double> minimum = std::nullopt;
  std::optional<double> maximum = std::nullopt;
  std::optional<double> stepValue = std::nullopt;

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
    stepValue = parseDouble(step);
    if (!stepValue.has_value())
      invalidParams.emplace_back(2);
  }

  // Check max is not less than min
  if (maximum.has_value() && minimum.has_value() && maximum.value() < minimum.value()) {
    invalidParams.emplace_back(0);
    invalidParams.emplace_back(1);
  }

  // Return errors, valid range, or unset if nothing was specified
  if (!invalidParams.empty()) {
    return invalidParams;
  }
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

  if (runNumbers.empty()) {
    return std::nullopt;
  }
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
    }
    return TransmissionRunPair(first.value(), second.value());
  }

  if (!first.has_value())
    errorColumns.emplace_back(0);
  if (!second.has_value())
    errorColumns.emplace_back(1);
  return errorColumns;
}

/** Extract the group name and angle from the run title. Expects the title to
 * be in the format: "group_name th=angle".
 * If it is not in this format then std::nullopt is returned.
 * If the format matches then the first element of the vector is the title and the second is theta.
 */
std::optional<std::vector<std::string>> parseTitleAndThetaFromRunTitle(std::string const &runTitle) {
  boost::smatch matches;
  static const boost::regex runTitleFormatRegex("(.*)(th[:=]\\s*([0-9.\\-]+))(.*)");

  if (!boost::regex_search(runTitle, matches, runTitleFormatRegex)) {
    return std::nullopt;
  }

  std::vector<std::string> parsedResult;

  constexpr auto preThetaGroup = 1;
  constexpr auto thetaValueGroup = 3;
  parsedResult.push_back(matches[preThetaGroup].str());
  parsedResult.push_back(matches[thetaValueGroup].str());

  return parsedResult;
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
