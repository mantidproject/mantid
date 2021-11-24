// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ValidateRow.h"
#include "AllInitialized.h"
#include "Common/Parse.h"
#include <boost/algorithm/string/trim.hpp>
#include <boost/tokenizer.hpp>
#include <boost/variant.hpp>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

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

template <typename T> class AppendErrorIfNotType : public boost::static_visitor<std::optional<T>> {
public:
  AppendErrorIfNotType(std::vector<int> &invalidParams, int baseColumn)
      : m_invalidParams(invalidParams), m_baseColumn(baseColumn) {}

  std::optional<T> operator()(T const &result) const { return result; }

  std::optional<T> operator()(int errorColumn) const {
    m_invalidParams.emplace_back(m_baseColumn + errorColumn);
    return std::nullopt;
  }

  std::optional<T> operator()(const std::vector<int> &errorColumns) const {
    std::transform(errorColumns.cbegin(), errorColumns.cend(), std::back_inserter(m_invalidParams),
                   [this](int column) -> int { return m_baseColumn + column; });
    return std::nullopt;
  }

private:
  std::vector<int> &m_invalidParams;
  int m_baseColumn;
};

std::optional<std::vector<std::string>> RowValidator::parseRunNumbers(std::vector<std::string> const &cellText) {
  auto runNumbers = ::MantidQt::CustomInterfaces::ISISReflectometry::parseRunNumbers(cellText[RUNS_COLUMN]);
  if (!runNumbers.has_value())
    m_invalidColumns.emplace_back(RUNS_COLUMN);
  return runNumbers;
}

std::optional<double> RowValidator::parseTheta(std::vector<std::string> const &cellText) {
  auto theta = ::MantidQt::CustomInterfaces::ISISReflectometry::parseTheta(cellText[THETA_COLUMN]);
  if (!theta.has_value())
    m_invalidColumns.emplace_back(THETA_COLUMN);
  return theta;
}

std::optional<TransmissionRunPair> RowValidator::parseTransmissionRuns(std::vector<std::string> const &cellText) {
  auto transmissionRunsOrError = ::MantidQt::CustomInterfaces::ISISReflectometry::parseTransmissionRuns(
      cellText[FIRST_TRANS_COLUMN], cellText[SECOND_TRANS_COLUMN]);
  return boost::apply_visitor(AppendErrorIfNotType<TransmissionRunPair>(m_invalidColumns, FIRST_TRANS_COLUMN),
                              transmissionRunsOrError);
}

std::optional<RangeInQ> RowValidator::parseQRange(std::vector<std::string> const &cellText) {
  auto qRangeOrError = ::MantidQt::CustomInterfaces::ISISReflectometry::parseQRange(
      cellText[QMIN_COLUMN], cellText[QMAX_COLUMN], cellText[QSTEP_COLUMN]);
  return boost::apply_visitor(AppendErrorIfNotType<RangeInQ>(m_invalidColumns, QMIN_COLUMN), qRangeOrError);
}

std::optional<std::optional<double>> RowValidator::parseScaleFactor(std::vector<std::string> const &cellText) {
  auto optionalScaleFactorOrNoneIfError =
      ::MantidQt::CustomInterfaces::ISISReflectometry::parseScaleFactor(cellText[SCALE_COLUMN]);
  if (!optionalScaleFactorOrNoneIfError.has_value())
    m_invalidColumns.emplace_back(SCALE_COLUMN);

  return optionalScaleFactorOrNoneIfError;
}

std::optional<std::map<std::string, std::string>> RowValidator::parseOptions(std::vector<std::string> const &cellText) {
  auto options = ::MantidQt::CustomInterfaces::ISISReflectometry::parseOptions(cellText[OPTIONS_COLUMN]);
  if (!options.has_value())
    m_invalidColumns.emplace_back(OPTIONS_COLUMN);
  return options;
}

ValidationResult<Row, std::vector<int>> RowValidator::operator()(std::vector<std::string> const &cellText) {
  auto maybeRunNumbers = parseRunNumbers(cellText);
  auto maybeTheta = parseTheta(cellText);
  auto maybeTransmissionRuns = parseTransmissionRuns(cellText);
  auto maybeQRange = parseQRange(cellText);
  auto maybeScaleFactor = parseScaleFactor(cellText);
  auto maybeOptions = parseOptions(cellText);

  if (allInitialized(maybeRunNumbers, maybeTransmissionRuns)) {
    auto wsNames = workspaceNames(maybeRunNumbers.value(), maybeTransmissionRuns.value());
    auto maybeRow =
        makeIfAllInitialized<Row>(maybeRunNumbers, maybeTheta, maybeTransmissionRuns, maybeQRange, maybeScaleFactor,
                                  maybeOptions, std::optional<ReductionWorkspaces>(wsNames));
    if (maybeRow.has_value())
      return RowValidationResult(maybeRow.value());
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

std::optional<Row> validateRowFromRunAndTheta(std::string const &run, std::string const &theta) {
  std::vector<std::string> cells = {run, theta, "", "", "", "", "", "", ""};
  return validateRow(cells).validElseNone();
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
