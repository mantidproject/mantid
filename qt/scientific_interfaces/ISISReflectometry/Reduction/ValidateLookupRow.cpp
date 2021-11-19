// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ValidateLookupRow.h"
#include "AllInitialized.h"
#include "Common/Parse.h"
#include <boost/algorithm/string/trim.hpp>
#include <boost/tokenizer.hpp>
#include <boost/variant.hpp>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

namespace MantidQt::CustomInterfaces::ISISReflectometry {

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

using CellText = LookupRow::ValueArray;

std::optional<std::optional<double>> LookupRowValidator::parseThetaOrWhitespace(CellText const &cellText) {
  if (isEntirelyWhitespace(cellText[LookupRow::Column::THETA])) {
    return std::optional<double>();
  } else {
    auto theta = ISISReflectometry::parseTheta(cellText[LookupRow::Column::THETA]);
    if (theta.has_value()) {
      return theta;
    }
  }
  m_invalidColumns.emplace_back(LookupRow::Column::THETA);
  return std::nullopt;
}

std::optional<TransmissionRunPair> LookupRowValidator::parseTransmissionRuns(CellText const &cellText) {
  auto transmissionRunsOrError = ISISReflectometry::parseTransmissionRuns(cellText[LookupRow::Column::FIRST_TRANS],
                                                                          cellText[LookupRow::Column::SECOND_TRANS]);
  return boost::apply_visitor(
      AppendErrorIfNotType<TransmissionRunPair>(m_invalidColumns, LookupRow::Column::FIRST_TRANS),
      transmissionRunsOrError);
}

std::optional<std::optional<std::string>>
LookupRowValidator::parseTransmissionProcessingInstructions(CellText const &cellText) {
  auto optionalInstructionsOrNoneIfError =
      ISISReflectometry::parseProcessingInstructions(cellText[LookupRow::Column::TRANS_SPECTRA]);
  if (!optionalInstructionsOrNoneIfError.has_value())
    m_invalidColumns.emplace_back(LookupRow::Column::TRANS_SPECTRA);
  return optionalInstructionsOrNoneIfError;
}

std::optional<RangeInQ> LookupRowValidator::parseQRange(CellText const &cellText) {
  auto qRangeOrError = ISISReflectometry::parseQRange(
      cellText[LookupRow::Column::QMIN], cellText[LookupRow::Column::QMAX], cellText[LookupRow::Column::QSTEP]);
  return boost::apply_visitor(AppendErrorIfNotType<RangeInQ>(m_invalidColumns, LookupRow::Column::QMIN), qRangeOrError);
}

std::optional<std::optional<double>> LookupRowValidator::parseScaleFactor(CellText const &cellText) {
  auto optionalScaleFactorOrNoneIfError = ISISReflectometry::parseScaleFactor(cellText[LookupRow::Column::SCALE]);
  if (!optionalScaleFactorOrNoneIfError.has_value())
    m_invalidColumns.emplace_back(LookupRow::Column::SCALE);
  return optionalScaleFactorOrNoneIfError;
}

std::optional<std::optional<std::string>>
LookupRowValidator::parseProcessingInstructions(CellText const &cellText) {
  auto optionalInstructionsOrNoneIfError =
      ISISReflectometry::parseProcessingInstructions(cellText[LookupRow::Column::RUN_SPECTRA]);
  if (!optionalInstructionsOrNoneIfError.has_value())
    m_invalidColumns.emplace_back(LookupRow::Column::RUN_SPECTRA);
  return optionalInstructionsOrNoneIfError;
}

std::optional<std::optional<std::string>>
LookupRowValidator::parseBackgroundProcessingInstructions(CellText const &cellText) {
  auto optionalInstructionsOrNoneIfError =
      ISISReflectometry::parseProcessingInstructions(cellText[LookupRow::Column::BACKGROUND_SPECTRA]);
  if (!optionalInstructionsOrNoneIfError.has_value())
    m_invalidColumns.emplace_back(LookupRow::Column::BACKGROUND_SPECTRA);
  return optionalInstructionsOrNoneIfError;
}

ValidationResult<LookupRow, std::vector<int>> LookupRowValidator::operator()(CellText const &cellText) {
  auto maybeTheta = parseThetaOrWhitespace(cellText);
  auto maybeTransmissionRuns = parseTransmissionRuns(cellText);
  auto maybeTransmissionProcessingInstructions = parseTransmissionProcessingInstructions(cellText);
  auto maybeQRange = parseQRange(cellText);
  auto maybeScaleFactor = parseScaleFactor(cellText);
  auto maybeProcessingInstructions = parseProcessingInstructions(cellText);
  auto maybeBackgroundProcessingInstructions = parseBackgroundProcessingInstructions(cellText);
  auto maybeDefaults = makeIfAllInitialized<LookupRow>(
      maybeTheta, maybeTransmissionRuns, maybeTransmissionProcessingInstructions, maybeQRange, maybeScaleFactor,
      maybeProcessingInstructions, maybeBackgroundProcessingInstructions);

  if (maybeDefaults.has_value())
    return ValidationResult<LookupRow, std::vector<int>>(maybeDefaults.value());
  else
    return ValidationResult<LookupRow, std::vector<int>>(m_invalidColumns);
}

ValidationResult<LookupRow, std::vector<int>> validateLookupRow(CellText const &cells) {
  auto validate = LookupRowValidator();
  return validate(cells);
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
