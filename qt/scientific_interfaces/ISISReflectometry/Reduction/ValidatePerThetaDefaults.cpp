#include "ValidatePerThetaDefaults.h"
#include "AllInitialized.h"
#include "Reduction/WorkspaceNamesFactory.h"
#include "../Parse.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"
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

using CellText =
    std::array<std::string, PerThetaDefaultsValidator::INPUT_FIELD_COUNT>;

boost::optional<boost::optional<double>>
PerThetaDefaultsValidator::parseThetaOrWhitespace(CellText const &cellText) {
  auto theta = ::MantidQt::CustomInterfaces::parseTheta(cellText[0]);
  if (theta.is_initialized()) {
    return theta;
  } else if (isEntirelyWhitespace(cellText[0])) {
    return boost::optional<double>();
  } else {
    m_invalidColumns.emplace_back(0);
    return boost::none;
  }
}

boost::optional<TransmissionRunPair>
PerThetaDefaultsValidator::parseTransmissionRuns(CellText const &cellText) {
  auto transmissionRunsOrError =
      ::MantidQt::CustomInterfaces::parseTransmissionRuns(cellText[1],
                                                          cellText[2]);
  return boost::apply_visitor(
      AppendErrorIfNotType<TransmissionRunPair>(m_invalidColumns, 1),
      transmissionRunsOrError);
}

boost::optional<boost::optional<RangeInQ>>
PerThetaDefaultsValidator::parseQRange(CellText const &cellText) {
  auto qRangeOrError = ::MantidQt::CustomInterfaces::parseQRange(
      cellText[3], cellText[4], cellText[5]);
  return boost::apply_visitor(
      AppendErrorIfNotType<boost::optional<RangeInQ>>(m_invalidColumns, 3),
      qRangeOrError);
}

boost::optional<boost::optional<double>>
PerThetaDefaultsValidator::parseScaleFactor(CellText const &cellText) {
  auto optionalScaleFactorOrNoneIfError =
      ::MantidQt::CustomInterfaces::parseScaleFactor(cellText[6]);
  if (!optionalScaleFactorOrNoneIfError.is_initialized())
    m_invalidColumns.emplace_back(6);
  return optionalScaleFactorOrNoneIfError;
}

boost::optional<std::map<std::string, std::string>>
PerThetaDefaultsValidator::parseOptions(CellText const &cellText) {
  auto options = ::MantidQt::CustomInterfaces::parseOptions(cellText[7]);
  if (!options.is_initialized())
    m_invalidColumns.emplace_back(7);
  return options;
}

// cppcheck-suppress syntaxError
ValidationResult<PerThetaDefaults, std::vector<int>> PerThetaDefaultsValidator::
operator()(CellText const &cellText) {
  auto maybeTheta = parseThetaOrWhitespace(cellText);
  auto maybeTransmissionRuns = parseTransmissionRuns(cellText);
  auto maybeQRange = parseQRange(cellText);
  auto maybeScaleFactor = parseScaleFactor(cellText);
  auto maybeOptions = parseOptions(cellText);
  auto maybeDefaults = makeIfAllInitialized<PerThetaDefaults>(
      maybeTheta, maybeTransmissionRuns, maybeQRange, maybeScaleFactor,
      maybeOptions);

  if (maybeDefaults.is_initialized())
    return ValidationResult<PerThetaDefaults, std::vector<int>>(
        maybeDefaults.get());
  else
    return ValidationResult<PerThetaDefaults, std::vector<int>>(
        m_invalidColumns);
}

ValidationResult<PerThetaDefaults, std::vector<int>>
validatePerThetaDefaults(CellText const &cells) {
  auto validate = PerThetaDefaultsValidator();
  return validate(cells);
}
}
}
