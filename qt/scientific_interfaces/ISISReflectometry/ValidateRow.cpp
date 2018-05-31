#include "ValidateRow.h"
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>

namespace MantidQt {
namespace CustomInterfaces {

boost::optional<double> parseDouble(std::string string) {
  boost::trim(string);
  auto end = std::size_t();
  try {
    auto result = std::stod(string, &end);
    if (end == string.size())
      return result;
    else
      return boost::none;
  } catch (std::invalid_argument &) {
    return boost::none;
  } catch (std::out_of_range &) {
    return boost::none;
  }
}

boost::optional<double> parseNonNegativeDouble(std::string string) {
  auto maybeNegative = parseDouble(std::move(string));
  if (maybeNegative.is_initialized() && maybeNegative.get() >= 0.0)
    return maybeNegative.get();
  else
    return boost::none;
}

boost::optional<double> parseNonNegativeNonZeroDouble(std::string string) {
  auto maybeNegative = parseDouble(std::move(string));
  if (maybeNegative.is_initialized() && maybeNegative.get() > 0.0)
    return maybeNegative.get();
  else
    return boost::none;
}

boost::optional<int> parseInt(std::string string) {
  boost::trim(string);
  auto end = std::size_t();
  try {
    auto result = std::stoi(string, &end);
    if (end == string.size())
      return result;
    else
      return boost::none;
  } catch (std::invalid_argument &) {
    return boost::none;
  } catch (std::out_of_range &) {
    return boost::none;
  }
}

boost::optional<int> parseNonNegativeInt(std::string string) {
  auto maybeNegative = parseInt(std::move(string));
  if (maybeNegative.is_initialized() && maybeNegative.get() >= 0)
    return maybeNegative.get();
  else
    return boost::none;
}

boost::variant<boost::optional<RangeInQ>, std::vector<int>>
parseQRange(std::string const &min, std::string const &max,
            std::string const &step) {
  if (!(isEntirelyWhitespace(min) && isEntirelyWhitespace(max) && isEntirelyWhitespace(step))) {
    auto minimum = parseNonNegativeDouble(min);
    auto maximum = parseNonNegativeNonZeroDouble(max);
    auto stepValue = parseDouble(step);

    auto invalidParams = std::vector<int>();
    if (allInitialized(minimum, maximum, stepValue)) {
      if (maximum.get() > minimum.get()) {
        return boost::optional<RangeInQ>(RangeInQ(minimum.get(), stepValue.get(), maximum.get()));
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

template <typename Row>
RowValidationResult<Row>::RowValidationResult(Row row)
    : m_invalidColumns(), m_validRow(std::move(row)) {}

template <typename Row>
RowValidationResult<Row>::RowValidationResult(std::vector<int> invalidColumns)
    : m_invalidColumns(std::move(invalidColumns)), m_validRow(boost::none) {}

template <typename Row> bool RowValidationResult<Row>::isValid() const {
  return m_validRow.is_initialized();
}

template <typename Row>
std::vector<int> const &RowValidationResult<Row>::invalidColumns() const {
  return m_invalidColumns;
}

template <typename Row>
boost::optional<Row> const &RowValidationResult<Row>::validRowElseNone() const {
  return m_validRow;
}

template class RowValidationResult<SingleRow>;
template class RowValidationResult<SlicedRow>;

boost::optional<std::vector<std::string>>
parseRunNumbers(std::string const &runNumberString) {
  auto runNumberCandidates = std::vector<std::string>();
  boost::split(runNumberCandidates, runNumberString, boost::is_any_of("+"));
  for (auto &runNumberCandidate : runNumberCandidates) {
    auto maybeRunNumber = parseRunNumber(std::move(runNumberCandidate));
    if (maybeRunNumber.is_initialized())
      runNumberCandidate = maybeRunNumber.get();
    else
      return boost::none;
  }
  return runNumberCandidates;
}

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

bool isEntirelyWhitespace(std::string const &string) {
  return std::all_of(string.cbegin(), string.cend(),
                     [](unsigned char c) { return std::isspace(c); });
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

template <typename T>
class AppendErrorIfNotType : public boost::static_visitor<boost::optional<T>> {
public:
  AppendErrorIfNotType(std::vector<int> &invalidParams, int baseColumn)
      : m_invalidParams(invalidParams), m_baseColumn(baseColumn) {}

  boost::optional<T> operator()(T const &result) const { return result; }

  boost::optional<T> operator()(int errorColumn) const {
    m_invalidParams.emplace_back(m_baseColumn + errorColumn);
    return boost::none;
  }

  boost::optional<T> operator()(std::vector<int> errorColumns) const {
    std::transform(errorColumns.cbegin(), errorColumns.cend(),
                   std::back_inserter(m_invalidParams),
                   [this](int column) -> int { return m_baseColumn + column; });
    return boost::none;
  }

private:
  std::vector<int> &m_invalidParams;
  int m_baseColumn;
};

template <typename Row>
RowValidationResult<Row> validateRow(std::vector<std::string> const &cellText) {
  auto invalidColumns = std::vector<int>();

  auto runNumbers = parseRunNumbers(cellText[0]);
  if (!runNumbers.is_initialized())
    invalidColumns.emplace_back(0);

  auto theta = parseTheta(cellText[1]);
  if (!theta.is_initialized())
    invalidColumns.emplace_back(1);

  auto transmissionRunsOrError =
      parseTransmissionRuns(cellText[2], cellText[3]);
  auto transmissionRuns = boost::apply_visitor(
      AppendErrorIfNotType<TransmissionRunPair>(invalidColumns, 2),
      transmissionRunsOrError);

  auto qRangeOrError = parseQRange(cellText[4], cellText[5], cellText[6]);
  auto qRange = boost::apply_visitor(
      AppendErrorIfNotType<boost::optional<RangeInQ>>(invalidColumns, 4), qRangeOrError);

  if (invalidColumns.empty())
    return RowValidationResult<Row>(
        makeUsingValues<Row>(runNumbers, theta, transmissionRuns, qRange,
                             boost::optional<double>(0.0),
                             boost::optional<boost::optional<typename Row::WorkspaceNames>>(boost::optional<typename Row::WorkspaceNames>())));
  else
    return RowValidationResult<Row>(invalidColumns);
}

template RowValidationResult<SingleRow>
validateRow(std::vector<std::string> const &);
template RowValidationResult<SlicedRow>
validateRow(std::vector<std::string> const &);
}
}
