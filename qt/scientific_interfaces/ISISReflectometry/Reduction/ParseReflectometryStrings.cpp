#include "ParseReflectometryStrings.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"
#include "AllInitialized.h"
#include "../Parse.h"
namespace MantidQt {
namespace CustomInterfaces {

boost::optional<std::string>
parseRunNumber(std::string const &runNumberString) {
  auto asInt = parseNonNegativeInt(std::move(runNumberString));
  if (asInt.is_initialized()) {
    auto withoutWhitespaceAndDefinatelyPositive = std::to_string(asInt.get());
    return withoutWhitespaceAndDefinatelyPositive;
  } else {
    return boost::none;
  }
}

boost::optional<std::string>
parseRunNumberOrWhitespace(std::string const &runNumberString) {
  auto maybeRunNumber = parseRunNumber(runNumberString);
  if (maybeRunNumber.is_initialized())
    return maybeRunNumber;
  else if (isEntirelyWhitespace(runNumberString))
    return std::string();
  else
    return boost::none;
}

boost::optional<double> parseTheta(std::string const &theta) {
  auto maybeTheta = parseNonNegativeDouble(std::move(theta));
  if (maybeTheta.is_initialized() && maybeTheta.get() > 0.0)
    return maybeTheta;
  else
    return boost::none;
}

boost::optional<std::map<std::string, std::string>>
parseOptions(std::string const &options) {
  try {
    return MantidQt::MantidWidgets::parseKeyValueString(options);
  } catch (std::runtime_error &) {
    return boost::none;
  }
}

boost::optional<boost::optional<double>>
parseScaleFactor(std::string const &scaleFactor) {
  auto value = parseDouble(scaleFactor);
  if (value.is_initialized()) {
    return value;
  } else if (isEntirelyWhitespace(scaleFactor)) {
    return boost::optional<double>(boost::none);
  } else {
    return boost::none;
  }
}

boost::variant<boost::optional<RangeInQ>, std::vector<int>>
parseQRange(std::string const &min, std::string const &max,
            std::string const &step) {
  if (!(isEntirelyWhitespace(min) && isEntirelyWhitespace(max) &&
        isEntirelyWhitespace(step))) {
    auto minimum = parseNonNegativeDouble(min);
    auto maximum = parseNonNegativeNonZeroDouble(max);
    auto stepValue = parseDouble(step);

    auto invalidParams = std::vector<int>();
    if (allInitialized(minimum, maximum, stepValue)) {
      if (maximum.get() > minimum.get()) {
        return boost::optional<RangeInQ>(
            RangeInQ(minimum.get(), stepValue.get(), maximum.get()));
      } else {
        invalidParams.emplace_back(0);
        invalidParams.emplace_back(1);
        return invalidParams;
      }
    } else {
      if (!minimum.is_initialized())
        invalidParams.emplace_back(0);

      if (!maximum.is_initialized())
        invalidParams.emplace_back(1);

      if (!stepValue.is_initialized())
        invalidParams.emplace_back(2);

      return invalidParams;
    }
  } else {
    return boost::optional<RangeInQ>();
  }
}

boost::optional<std::vector<std::string>>
parseRunNumbers(std::string const &runNumberString) {
  auto runNumbers = std::vector<std::string>();
  auto runNumberCandidates =
      boost::tokenizer<boost::escaped_list_separator<char>>(
          runNumberString,
          boost::escaped_list_separator<char>("\\", ",+", "\"'"));

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

boost::variant<std::pair<std::string, std::string>, std::vector<int>>
parseTransmissionRuns(std::string const &firstTransmissionRun,
                      std::string const &secondTransmissionRun) {
  auto errorColumns = std::vector<int>();
  auto first = parseRunNumberOrWhitespace(firstTransmissionRun);
  auto second = parseRunNumberOrWhitespace(secondTransmissionRun);

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

}
}
