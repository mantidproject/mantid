// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ValidateLookupRow.h"
#include "AllInitialized.h"
#include "Common/Parse.h"
#include <boost/variant.hpp>
#include <unordered_set>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

namespace MantidQt::CustomInterfaces::ISISReflectometry {

template <typename T> class InsertErrorIfNotType : public boost::static_visitor<ValidatorT<T>> {
public:
  InsertErrorIfNotType(std::unordered_set<int> &invalidParams, int baseColumn)
      : m_invalidParams(invalidParams), m_baseColumn(baseColumn) {}

  ValidatorT<T> operator()(T const &result) const { return std::make_pair(result, true); }

  ValidatorT<T> operator()(int errorColumn) const {
    m_invalidParams.insert(m_baseColumn + errorColumn);
    return std::make_pair(std::nullopt, false);
  }

  ValidatorT<T> operator()(const std::vector<int> &errorColumns) const {
    std::transform(errorColumns.cbegin(), errorColumns.cend(), std::inserter(m_invalidParams, m_invalidParams.end()),
                   [this](int column) -> int { return m_baseColumn + column; });
    return std::make_pair(std::nullopt, false);
  }

private:
  std::unordered_set<int> &m_invalidParams;
  int m_baseColumn;
};

using CellText = LookupRow::ValueArray;

ValidatorT<std::optional<double>> LookupRowValidator::parseThetaOrWhitespace(CellText const &cellText) {
  if (isEntirelyWhitespace(cellText[LookupRow::Column::THETA])) {
    return std::make_pair(std::optional<double>(), true);
  }
  auto theta = parseTheta(cellText[LookupRow::Column::THETA]);
  if (theta.has_value()) {
    return std::make_pair(theta, true);
  }
  m_invalidColumns.insert(LookupRow::Column::THETA);
  return std::make_pair(std::nullopt, false);
}

ValidatorT<std::optional<boost::regex>> LookupRowValidator::parseTitleMatcherOrWhitespace(CellText const &cellText) {
  auto const &text = cellText[LookupRow::Column::TITLE];
  if (isEntirelyWhitespace(text)) {
    // Mark validator as passed, but the enclosed value empty
    return std::make_pair(std::make_optional<std::optional<boost::regex>>(std::nullopt), true);
  }

  // This check relies on us checking for whitespace chars before calling parseTitleMatcher
  if (auto result = parseTitleMatcher(text)) {
    return std::make_pair(result, true);
    ;
  } else {
    m_invalidColumns.insert(LookupRow::Column::TITLE);
    return std::make_pair(std::nullopt, false);
  }
}

ValidatorT<TransmissionRunPair> LookupRowValidator::parseTransmissionRuns(CellText const &cellText) {
  auto transmissionRunsOrError = ISISReflectometry::parseTransmissionRuns(cellText[LookupRow::Column::FIRST_TRANS],
                                                                          cellText[LookupRow::Column::SECOND_TRANS]);
  return boost::apply_visitor(
      InsertErrorIfNotType<TransmissionRunPair>(m_invalidColumns, LookupRow::Column::FIRST_TRANS),
      transmissionRunsOrError);
}

ValidatorT<std::optional<std::string>>
LookupRowValidator::parseTransmissionProcessingInstructions(CellText const &cellText) {
  auto optionalInstructionsOrNoneIfError =
      ISISReflectometry::parseProcessingInstructions(cellText[LookupRow::Column::TRANS_SPECTRA]);
  if (!optionalInstructionsOrNoneIfError.has_value())
    m_invalidColumns.insert(LookupRow::Column::TRANS_SPECTRA);
  return std::make_pair(optionalInstructionsOrNoneIfError, optionalInstructionsOrNoneIfError.has_value());
}

ValidatorT<RangeInQ> LookupRowValidator::parseQRange(CellText const &cellText) {
  auto qRangeOrError = ISISReflectometry::parseQRange(
      cellText[LookupRow::Column::QMIN], cellText[LookupRow::Column::QMAX], cellText[LookupRow::Column::QSTEP]);
  return boost::apply_visitor(InsertErrorIfNotType<RangeInQ>(m_invalidColumns, LookupRow::Column::QMIN), qRangeOrError);
}

ValidatorT<std::optional<double>> LookupRowValidator::parseScaleFactor(CellText const &cellText) {
  auto optionalScaleFactorOrNoneIfError = ISISReflectometry::parseScaleFactor(cellText[LookupRow::Column::SCALE]);
  if (!optionalScaleFactorOrNoneIfError.has_value())
    m_invalidColumns.insert(LookupRow::Column::SCALE);
  return std::make_pair(optionalScaleFactorOrNoneIfError, optionalScaleFactorOrNoneIfError.has_value());
  ;
}

ValidatorT<std::optional<std::string>> LookupRowValidator::parseProcessingInstructions(CellText const &cellText) {
  auto optionalInstructionsOrNoneIfError =
      ISISReflectometry::parseProcessingInstructions(cellText[LookupRow::Column::RUN_SPECTRA]);
  if (!optionalInstructionsOrNoneIfError.has_value())
    m_invalidColumns.insert(LookupRow::Column::RUN_SPECTRA);
  return std::make_pair(optionalInstructionsOrNoneIfError, optionalInstructionsOrNoneIfError.has_value());
}

ValidatorT<std::optional<std::string>>
LookupRowValidator::parseBackgroundProcessingInstructions(CellText const &cellText) {
  auto optionalInstructionsOrNoneIfError =
      ISISReflectometry::parseProcessingInstructions(cellText[LookupRow::Column::BACKGROUND_SPECTRA]);
  if (!optionalInstructionsOrNoneIfError.has_value())
    m_invalidColumns.insert(LookupRow::Column::BACKGROUND_SPECTRA);
  return std::make_pair(optionalInstructionsOrNoneIfError, optionalInstructionsOrNoneIfError.has_value());
}

ValidatorT<std::optional<std::string>> LookupRowValidator::parseROIDetectorIDs(CellText const &cellText) {
  auto optionalROIDetectorsOrNoneIfError =
      ISISReflectometry::parseProcessingInstructions(cellText[LookupRow::Column::ROI_DETECTOR_IDS]);
  if (!optionalROIDetectorsOrNoneIfError.has_value())
    m_invalidColumns.insert(LookupRow::Column::ROI_DETECTOR_IDS);
  return std::make_pair(optionalROIDetectorsOrNoneIfError, optionalROIDetectorsOrNoneIfError.has_value());
}

void LookupRowValidator::validateThetaAndRegex() {
  // If either value didn't even parse, there's nothing further to check, so return
  if (!m_thetaOrInvalid.second || !m_titleMatcherOrInvalid.second)
    return;

  // Check we have a theta value, when we have a titleMatcher
  if (m_titleMatcherOrInvalid.first.has_value() && !m_thetaOrInvalid.first.has_value()) {
    m_invalidColumns.insert(LookupRow::Column::THETA);
    m_invalidColumns.insert(LookupRow::Column::TITLE);
    m_thetaOrInvalid = std::make_pair(std::nullopt, false);
    m_titleMatcherOrInvalid = std::make_pair(std::nullopt, false);
  }
}

ValidationResult<LookupRow, std::unordered_set<int>> LookupRowValidator::operator()(CellText const &cellText) {
  m_thetaOrInvalid = parseThetaOrWhitespace(cellText);
  m_titleMatcherOrInvalid = parseTitleMatcherOrWhitespace(cellText);
  validateThetaAndRegex();

  auto maybeTransmissionRuns = parseTransmissionRuns(cellText);
  auto maybeTransmissionProcessingInstructions = parseTransmissionProcessingInstructions(cellText);
  auto maybeQRange = parseQRange(cellText);
  auto maybeScaleFactor = parseScaleFactor(cellText);
  auto maybeProcessingInstructions = parseProcessingInstructions(cellText);
  auto maybeBackgroundProcessingInstructions = parseBackgroundProcessingInstructions(cellText);
  auto maybeROIDetectorIDs = parseROIDetectorIDs(cellText);

  auto maybeDefaults = makeIfAllInitializedPairs<LookupRow>(
      m_thetaOrInvalid, m_titleMatcherOrInvalid, maybeTransmissionRuns, maybeTransmissionProcessingInstructions,
      maybeQRange, maybeScaleFactor, maybeProcessingInstructions, maybeBackgroundProcessingInstructions,
      maybeROIDetectorIDs);

  if (maybeDefaults.has_value()) {
    return ValidationResult<LookupRow, std::unordered_set<int>>(maybeDefaults.value());
  }
  return ValidationResult<LookupRow, std::unordered_set<int>>(m_invalidColumns);
}

ValidationResult<LookupRow, std::unordered_set<int>> validateLookupRow(CellText const &cells) {
  auto validate = LookupRowValidator();
  return validate(cells);
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
