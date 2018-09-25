#include "ValidateRow.h"
#include "AllInitialized.h"
#include "Reduction/WorkspaceNamesFactory.h"
#include "../Parse.h"
#include <boost/algorithm/string/trim.hpp>
#include <boost/tokenizer.hpp>
#include <boost/variant.hpp>

namespace MantidQt {
namespace CustomInterfaces {

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
  auto runNumbers = ::MantidQt::CustomInterfaces::parseRunNumbers(cellText[0]);
  if (!runNumbers.is_initialized())
    m_invalidColumns.emplace_back(0);
  return runNumbers;
}

boost::optional<double>
RowValidator::parseTheta(std::vector<std::string> const &cellText) {
  auto theta = ::MantidQt::CustomInterfaces::parseTheta(cellText[1]);
  if (!theta.is_initialized())
    m_invalidColumns.emplace_back(1);
  return theta;
}

boost::optional<TransmissionRunPair>
RowValidator::parseTransmissionRuns(std::vector<std::string> const &cellText) {
  auto transmissionRunsOrError =
      ::MantidQt::CustomInterfaces::parseTransmissionRuns(cellText[2],
                                                          cellText[3]);
  return boost::apply_visitor(
      AppendErrorIfNotType<TransmissionRunPair>(m_invalidColumns, 2),
      transmissionRunsOrError);
}

boost::optional<boost::optional<RangeInQ>>
RowValidator::parseQRange(std::vector<std::string> const &cellText) {
  auto qRangeOrError = ::MantidQt::CustomInterfaces::parseQRange(
      cellText[4], cellText[5], cellText[6]);
  return boost::apply_visitor(
      AppendErrorIfNotType<boost::optional<RangeInQ>>(m_invalidColumns, 4),
      qRangeOrError);
}

boost::optional<boost::optional<double>>
RowValidator::parseScaleFactor(std::vector<std::string> const &cellText) {
  auto optionalScaleFactorOrNoneIfError =
      ::MantidQt::CustomInterfaces::parseScaleFactor(cellText[7]);
  if (!optionalScaleFactorOrNoneIfError.is_initialized())
    m_invalidColumns.emplace_back(7);
  return optionalScaleFactorOrNoneIfError;
}

boost::optional<std::map<std::string, std::string>>
RowValidator::parseOptions(std::vector<std::string> const &cellText) {
  auto options = ::MantidQt::CustomInterfaces::parseOptions(cellText[8]);
  if (!options.is_initialized())
    m_invalidColumns.emplace_back(8);
  return options;
}

template <typename WorkspaceNamesFactory>
ValidationResult<Row, std::vector<int>> RowValidator::
operator()(std::vector<std::string> const &cellText,
           WorkspaceNamesFactory const &workspaceNames) {
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
      return RowValidationResult(maybeRow.get());
    else
      return RowValidationResult(m_invalidColumns);
  } else {
    return RowValidationResult(m_invalidColumns);
  }
}

RowValidationResult
validateRow(Jobs const &, WorkspaceNamesFactory const &workspaceNamesFactory,
            std::vector<std::string> const &cells) {
  auto validate = RowValidator();
  RowValidationResult result = validate(
      cells, [&workspaceNamesFactory](
                 std::vector<std::string> const &runNumbers,
                 std::pair<std::string, std::string> const &transmissionRuns)
                 -> boost::optional<ReductionWorkspaces> {
                   return workspaceNamesFactory.makeNames(runNumbers,
                                                          transmissionRuns);
                 });
  return result;
}

boost::optional<Row>
validateRowFromRunAndTheta(Jobs const &jobs,
                           WorkspaceNamesFactory const &workspaceNamesFactory,
                           std::string const &run, std::string const &theta) {
  std::vector<std::string> cells = {run, theta, "", "", "", "", "", "", ""};
  return validateRow(jobs, workspaceNamesFactory, cells).validElseNone();
}
} // namespace CustomInterfaces
} // namespace MantidQt
