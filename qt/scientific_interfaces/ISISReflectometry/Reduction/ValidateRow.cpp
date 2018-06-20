#include "ValidateRow.h"
#include "Reduction/WorkspaceNamesFactory.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"
#include <boost/algorithm/string/trim.hpp>
#include <boost/tokenizer.hpp>
#include <boost/variant.hpp>

namespace MantidQt {
namespace CustomInterfaces {

template <typename Param>
bool allInitialized(boost::optional<Param> const &param) {
  return param.is_initialized();
}

template <typename FirstParam, typename SecondParam, typename... Params>
bool allInitialized(boost::optional<FirstParam> const &first,
                    boost::optional<SecondParam> const &second,
                    boost::optional<Params> const &... params) {
  return first.is_initialized() && allInitialized(second, params...);
}

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

boost::optional<std::map<std::string, std::string>>
parseOptions(std::string const &options) {
  try {
    return MantidQt::MantidWidgets::parseKeyValueString(options);
  } catch (std::runtime_error &) {
    return boost::none;
  }
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
boost::optional<std::vector<std::string>>
RowValidator<Row>::parseRunNumbers(std::vector<std::string> const &cellText) {
  auto runNumbers = ::MantidQt::CustomInterfaces::parseRunNumbers(cellText[0]);
  if (!runNumbers.is_initialized())
    m_invalidColumns.emplace_back(0);
  return runNumbers;
}

template <typename Row>
boost::optional<double>
RowValidator<Row>::parseTheta(std::vector<std::string> const &cellText) {
  auto theta = ::MantidQt::CustomInterfaces::parseTheta(cellText[1]);
  if (!theta.is_initialized())
    m_invalidColumns.emplace_back(1);
  return theta;
}

template <typename Row>
boost::optional<TransmissionRunPair> RowValidator<Row>::parseTransmissionRuns(
    std::vector<std::string> const &cellText) {
  auto transmissionRunsOrError =
      ::MantidQt::CustomInterfaces::parseTransmissionRuns(cellText[2],
                                                          cellText[3]);
  return boost::apply_visitor(
      AppendErrorIfNotType<TransmissionRunPair>(m_invalidColumns, 2),
      transmissionRunsOrError);
}

template <typename Row>
boost::optional<boost::optional<RangeInQ>>
RowValidator<Row>::parseQRange(std::vector<std::string> const &cellText) {
  auto qRangeOrError = ::MantidQt::CustomInterfaces::parseQRange(
      cellText[4], cellText[5], cellText[6]);
  return boost::apply_visitor(
      AppendErrorIfNotType<boost::optional<RangeInQ>>(m_invalidColumns, 4),
      qRangeOrError);
}

template <typename Row>
boost::optional<boost::optional<double>>
RowValidator<Row>::parseScaleFactor(std::vector<std::string> const &cellText) {
  auto optionalScaleFactorOrNoneIfError =
      ::MantidQt::CustomInterfaces::parseScaleFactor(cellText[7]);
  if (!optionalScaleFactorOrNoneIfError.is_initialized())
    m_invalidColumns.emplace_back(7);
  return optionalScaleFactorOrNoneIfError;
}

template <typename Row>
boost::optional<std::map<std::string, std::string>>
RowValidator<Row>::parseOptions(std::vector<std::string> const &cellText) {
  auto options = ::MantidQt::CustomInterfaces::parseOptions(cellText[8]);
  if (!options.is_initialized())
    m_invalidColumns.emplace_back(8);
  return options;
}

template <typename Result, typename... Params>
boost::optional<Result>
makeIfAllInitialized(boost::optional<Params> const &... params) {
  if (allInitialized(params...))
    return Result(params.get()...);
  else
    return boost::none;
}

template <typename Row>
// cppcheck-suppress syntaxError
template <typename WorkspaceNamesFactory>
RowValidationResult<boost::variant<SlicedRow, UnslicedRow>> RowValidator<Row>::
operator()(std::vector<std::string> const &cellText,
           WorkspaceNamesFactory const &workspaceNames) {
  using RowVariant = boost::variant<SlicedRow, UnslicedRow>;

  auto maybeRunNumbers = parseRunNumbers(cellText);
  auto maybeTheta = parseTheta(cellText);
  auto maybeTransmissionRuns = parseTransmissionRuns(cellText);
  auto maybeQRange = parseQRange(cellText);
  auto maybeScaleFactor = parseScaleFactor(cellText);
  auto maybeOptions = parseOptions(cellText);

  if (allInitialized(maybeRunNumbers, maybeTransmissionRuns)) {
    auto wsNames =
        workspaceNames(maybeRunNumbers.get(), maybeTransmissionRuns.get());
    auto maybeRow = makeIfAllInitialized<Row>(
        maybeRunNumbers, maybeTheta, maybeTransmissionRuns, maybeQRange,
        maybeScaleFactor, maybeOptions, wsNames);
    if (maybeRow.is_initialized())
      return RowValidationResult<RowVariant>(maybeRow.get());
    else
      return RowValidationResult<RowVariant>(m_invalidColumns);
  } else {
    return RowValidationResult<RowVariant>(m_invalidColumns);
  }
}

class ValidateRowVisitor
    : public boost::static_visitor<RowValidationResult<RowVariant>> {
public:
  ValidateRowVisitor(std::vector<std::string> const &cells,
                     WorkspaceNamesFactory const &workspaceNamesFactory)
      : m_cells(cells), m_workspaceNamesFactory(workspaceNamesFactory) {}

  RowValidationResult<RowVariant>
  operator()(ReductionJobs<SlicedGroup> const &) const {
    auto validate = RowValidator<SlicedRow>();
    return validate(
        m_cells,
        [this](std::vector<std::string> const &runNumbers,
               std::pair<std::string, std::string> const &transmissionRuns)
            -> boost::optional<SlicedReductionWorkspaces> {
              return m_workspaceNamesFactory
                  .makeNames<SlicedReductionWorkspaces>(runNumbers,
                                                        transmissionRuns);
            });
  }

  RowValidationResult<RowVariant>
  operator()(ReductionJobs<UnslicedGroup> const &) const {
    auto validate = RowValidator<UnslicedRow>();
    return validate(
        m_cells,
        [this](std::vector<std::string> const &runNumbers,
               std::pair<std::string, std::string> const &transmissionRuns)
            -> boost::optional<ReductionWorkspaces> {
              return m_workspaceNamesFactory.makeNames<ReductionWorkspaces>(
                  runNumbers, transmissionRuns);
            });
  }

private:
  std::vector<std::string> const &m_cells;
  WorkspaceNamesFactory const &m_workspaceNamesFactory;
};

RowValidationResult<RowVariant>
validateRow(Jobs const &jobs,
            WorkspaceNamesFactory const &workspaceNamesFactory,
            std::vector<std::string> const &cells) {
  return boost::apply_visitor(ValidateRowVisitor(cells, workspaceNamesFactory),
                              jobs);
}

boost::optional<RowVariant>
validateRowFromRunAndTheta(Jobs const &jobs,
                           WorkspaceNamesFactory const &workspaceNamesFactory,
                           std::string const &run, std::string const &theta) {
  return boost::apply_visitor(
             ValidateRowVisitor({run, theta, "", "", "", "", "", "", ""},
                                workspaceNamesFactory),
             jobs).validRowElseNone();
}

template class RowValidationResult<UnslicedRow>;
template class RowValidationResult<SlicedRow>;
template class RowValidationResult<RowVariant>;
}
}
