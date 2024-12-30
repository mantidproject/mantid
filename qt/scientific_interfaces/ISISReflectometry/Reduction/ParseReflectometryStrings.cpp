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
boost::optional<std::vector<std::string>> parseRunNumbersOrWhitespace(std::string const &runNumberString) {
  auto runNumbers = std::vector<std::string>();
  auto runNumberCandidates = boost::tokenizer<boost::escaped_list_separator<char>>(
      runNumberString, boost::escaped_list_separator<char>("\\", ",+", "\"'"));

  for (auto const &runNumberCandidate : runNumberCandidates) {
    auto const maybeRunNumber = parseRunNumberOrWhitespace(runNumberCandidate);
    if (maybeRunNumber.is_initialized()) {
      runNumbers.emplace_back(maybeRunNumber.get());
    } else {
      return boost::none;
    }
  }

  return runNumbers;
}
} // unnamed namespace

boost::optional<std::string> parseRunNumber(std::string const &runNumberString) {
  // We support any workspace name, as well as run numbers, so for just return
  // the input string, but trimmed of whitespace (or none if the result is
  // empty)
  auto result = std::string(runNumberString);
  boost::trim(result);

  if (result.empty())
    return boost::none;

  return result;
}

boost::optional<std::string> parseRunNumberOrWhitespace(std::string const &runNumberString) {
  if (isEntirelyWhitespace(runNumberString)) {
    return std::string();
  } else {
    auto maybeRunNumber = parseRunNumber(runNumberString);
    if (maybeRunNumber.is_initialized())
      return maybeRunNumber;
  }
  return boost::none;
}

boost::optional<double> parseTheta(std::string const &theta) {
  auto maybeTheta = parseNonNegativeDouble(theta);
  if (maybeTheta.has_value() && maybeTheta.value() > 0.0)
    return maybeTheta.value();
  else
    return boost::none;
}

boost::optional<boost::regex> parseTitleMatcher(std::string const &titleMatcher) {
  if (isEntirelyWhitespace(titleMatcher)) {
    return boost::none;
  }
  try {
    return boost::regex(titleMatcher);
  } catch (boost::regex_error const &) {
    return boost::none;
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

boost::optional<std::map<std::string, std::string>> parseOptions(std::string const &options) {
  try {
    return replaceBoolTextWithBoolValue(MantidQt::MantidWidgets::parseKeyValueString(options));
  } catch (std::runtime_error &) {
    return boost::none;
  }
}

boost::optional<boost::optional<std::string>> parseProcessingInstructions(std::string const &instructions) {
  if (isEntirelyWhitespace(instructions)) {
    return boost::optional<std::string>(boost::none);
  } else {
    try {
      auto const groups = Mantid::Kernel::Strings::parseGroups<size_t>(instructions);
      return boost::optional<std::string>(instructions);
    } catch (std::runtime_error &) {
      return boost::none;
    }
  }
  return boost::none;
}

boost::optional<boost::optional<double>> parseScaleFactor(std::string const &scaleFactor) {
  if (isEntirelyWhitespace(scaleFactor)) {
    return boost::optional<double>(boost::none);
  }

  auto value = parseDouble(scaleFactor);
  if (value.has_value() && value != 0.0)
    return boost::optional<double>(value.value());
  return boost::none;
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
  if (!invalidParams.empty())
    return invalidParams;
  else
    return RangeInQ(minimum, stepValue, maximum);
}

boost::optional<std::vector<std::string>> parseRunNumbers(std::string const &runNumberString) {
  auto runNumbers = std::vector<std::string>();
  auto runNumberCandidates = boost::tokenizer<boost::escaped_list_separator<char>>(
      runNumberString, boost::escaped_list_separator<char>("\\", ",+", "\"'"));

  for (auto const &runNumberCandidate : runNumberCandidates) {
    auto maybeRunNumber = parseRunNumber(runNumberCandidate);
    if (maybeRunNumber.is_initialized()) {
      runNumbers.emplace_back(maybeRunNumber.get());
    } else {
      return boost::none;
    }
  }

  if (runNumbers.empty())
    return boost::none;
  else
    return runNumbers;
}

boost::variant<TransmissionRunPair, std::vector<int>> parseTransmissionRuns(std::string const &firstTransmissionRun,
                                                                            std::string const &secondTransmissionRun) {
  auto errorColumns = std::vector<int>();
  auto first = parseRunNumbersOrWhitespace(firstTransmissionRun);
  auto second = parseRunNumbersOrWhitespace(secondTransmissionRun);

  if (allInitialized(first, second)) {
    if (first.get().empty() && !second.get().empty()) {
      errorColumns.emplace_back(0);
      return errorColumns;
    } else {
      return TransmissionRunPair(first.get(), second.get());
    }
  } else {
    if (!first.is_initialized())
      errorColumns.emplace_back(0);
    if (!second.is_initialized())
      errorColumns.emplace_back(1);
    return errorColumns;
  }
}

/** Extract the group name and angle from the run title. Expects the title to
 * be in the format: "group_name th=angle".
 * If it is not in this format then boost::none is returned.
 * If the format matches then the first element of the vector is the title and the second is theta.
 */
boost::optional<std::vector<std::string>> parseTitleAndThetaFromRunTitle(std::string const &runTitle) {
  boost::smatch matches;
  static const boost::regex runTitleFormatRegex("(.*)(th[:=]\\s*([0-9.\\-]+))(.*)");

  if (!boost::regex_search(runTitle, matches, runTitleFormatRegex)) {
    return boost::none;
  }

  std::vector<std::string> parsedResult;

  constexpr auto preThetaGroup = 1;
  constexpr auto thetaValueGroup = 3;
  parsedResult.push_back(matches[preThetaGroup].str());
  parsedResult.push_back(matches[thetaValueGroup].str());

  return parsedResult;
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
