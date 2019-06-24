// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ValidateRow.h"
#include "AllInitialized.h"
#include "Common/Parse.h"
#include <boost/algorithm/string/trim.hpp>
#include <boost/tokenizer.hpp>
#include <boost/variant.hpp>

namespace MantidQt {
namespace CustomInterfaces {

namespace {
enum ColumnNumber {
  // 0-based column indices for cells in a row. The Actual values are important
  // here so set them explicitly
  RUNS_COLUMN = 0,
  THETA_COLUMN = 1,
  FIRST_TRANS_COLUMN = 2,
  SECOND_TRANS_COLUMN = 3,
  QMIN_COLUMN = 4,
  QMAX_COLUMN = 5,
  QSTEP_COLUMN = 6,
  SCALE_COLUMN = 7,
  OPTIONS_COLUMN = 8
};
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

boost::optional<std::vector<std::string>>
RowValidator::parseRunNumbers(std::vector<std::string> const &cellText) {
  auto runNumbers =
      ::MantidQt::CustomInterfaces::parseRunNumbers(cellText[RUNS_COLUMN]);
  if (!runNumbers.is_initialized())
    m_invalidColumns.emplace_back(RUNS_COLUMN);
  return runNumbers;
}

boost::optional<double>
RowValidator::parseTheta(std::vector<std::string> const &cellText) {
  auto theta = ::MantidQt::CustomInterfaces::parseTheta(cellText[THETA_COLUMN]);
  if (!theta.is_initialized())
    m_invalidColumns.emplace_back(THETA_COLUMN);
  return theta;
}

boost::optional<TransmissionRunPair>
RowValidator::parseTransmissionRuns(std::vector<std::string> const &cellText) {
  auto transmissionRunsOrError =
      ::MantidQt::CustomInterfaces::parseTransmissionRuns(
          cellText[FIRST_TRANS_COLUMN], cellText[SECOND_TRANS_COLUMN]);
  return boost::apply_visitor(AppendErrorIfNotType<TransmissionRunPair>(
                                  m_invalidColumns, FIRST_TRANS_COLUMN),
                              transmissionRunsOrError);
}

boost::optional<RangeInQ>
RowValidator::parseQRange(std::vector<std::string> const &cellText) {
  auto qRangeOrError = ::MantidQt::CustomInterfaces::parseQRange(
      cellText[QMIN_COLUMN], cellText[QMAX_COLUMN], cellText[QSTEP_COLUMN]);
  return boost::apply_visitor(
      AppendErrorIfNotType<RangeInQ>(m_invalidColumns, QMIN_COLUMN),
      qRangeOrError);
}

boost::optional<boost::optional<double>>
RowValidator::parseScaleFactor(std::vector<std::string> const &cellText) {
  auto optionalScaleFactorOrNoneIfError =
      ::MantidQt::CustomInterfaces::parseScaleFactor(cellText[SCALE_COLUMN]);
  if (!optionalScaleFactorOrNoneIfError.is_initialized())
    m_invalidColumns.emplace_back(SCALE_COLUMN);
  return optionalScaleFactorOrNoneIfError;
}

boost::optional<std::map<std::string, std::string>>
RowValidator::parseOptions(std::vector<std::string> const &cellText) {
  auto options =
      ::MantidQt::CustomInterfaces::parseOptions(cellText[OPTIONS_COLUMN]);
  if (!options.is_initialized())
    m_invalidColumns.emplace_back(OPTIONS_COLUMN);
  return options;
}

ValidationResult<Row, std::vector<int>> RowValidator::
operator()(std::vector<std::string> const &cellText) {
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
        maybeScaleFactor, maybeOptions,
        boost::optional<ReductionWorkspaces>(wsNames));
    if (maybeRow.is_initialized())
      return RowValidationResult(maybeRow.get());
    else
      return RowValidationResult(m_invalidColumns);
  } else {
    return RowValidationResult(m_invalidColumns);
  }
}

RowValidationResult validateRow(std::vector<std::string> const &cells) {
  auto validate = RowValidator();
  RowValidationResult result = validate(cells);
  return result;
}

boost::optional<Row> validateRowFromRunAndTheta(std::string const &run,
                                                std::string const &theta) {
  std::vector<std::string> cells = {run, theta, "", "", "", "", "", "", ""};
  return validateRow(cells).validElseNone();
}
} // namespace CustomInterfaces
} // namespace MantidQt
