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
#include <unordered_set>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

namespace MantidQt::CustomInterfaces::ISISReflectometry {

template <typename T> class InsertErrorIfNotType : public boost::static_visitor<ValidatorT<T>> {
public:
  InsertErrorIfNotType(std::unordered_set<int> &invalidParams, int baseColumn)
      : m_invalidParams(invalidParams), m_baseColumn(baseColumn) {}

  ValidatorT<T> operator()(T const &result) const { return result; }

  ValidatorT<T> operator()(int errorColumn) const {
    m_invalidParams.insert(m_baseColumn + errorColumn);
    return boost::none;
  }

  ValidatorT<T> operator()(const std::vector<int> &errorColumns) const {
    std::transform(errorColumns.cbegin(), errorColumns.cend(), std::inserter(m_invalidParams, m_invalidParams.end()),
                   [this](int column) -> int { return m_baseColumn + column; });
    return boost::none;
  }

private:
  std::unordered_set<int> &m_invalidParams;
  int m_baseColumn;
};

using CellText = LookupRow::ValueArray;

ValidatorT<boost::optional<double>> LookupRowValidator::parseThetaOrWhitespace(CellText const &cellText) {
  if (isEntirelyWhitespace(cellText[LookupRow::Column::THETA])) {
    return boost::optional<double>();
  } else {
    auto theta = ISISReflectometry::parseTheta(cellText[LookupRow::Column::THETA]);
    if (theta.is_initialized()) {
      return theta;
    }
  }
  m_invalidColumns.insert(LookupRow::Column::THETA);
  return boost::none;
}

ValidatorT<std::optional<boost::regex>> LookupRowValidator::parseTitleMatcherOrWhitespace(CellText const &cellText) {
  auto const &text = cellText[LookupRow::Column::TITLE];
  if (isEntirelyWhitespace(text)) {
    // Mark validator as passed, but the enclosed value empty
    return boost::make_optional<std::optional<boost::regex>>(std::nullopt);
  }

  // This check relies on us checking for whitespace chars before calling parseTitleMatcher
  if (auto result = parseTitleMatcher(text)) {
    return result;
  } else {
    m_invalidColumns.insert(LookupRow::Column::TITLE);
    return boost::none;
  }
}

ValidatorT<TransmissionRunPair> LookupRowValidator::parseTransmissionRuns(CellText const &cellText) {
  auto transmissionRunsOrError = ISISReflectometry::parseTransmissionRuns(cellText[LookupRow::Column::FIRST_TRANS],
                                                                          cellText[LookupRow::Column::SECOND_TRANS]);
  return boost::apply_visitor(
      InsertErrorIfNotType<TransmissionRunPair>(m_invalidColumns, LookupRow::Column::FIRST_TRANS),
      transmissionRunsOrError);
}

ValidatorT<boost::optional<std::string>>
LookupRowValidator::parseTransmissionProcessingInstructions(CellText const &cellText) {
  auto optionalInstructionsOrNoneIfError =
      ISISReflectometry::parseProcessingInstructions(cellText[LookupRow::Column::TRANS_SPECTRA]);
  if (!optionalInstructionsOrNoneIfError.is_initialized())
    m_invalidColumns.insert(LookupRow::Column::TRANS_SPECTRA);
  return optionalInstructionsOrNoneIfError;
}

ValidatorT<RangeInQ> LookupRowValidator::parseQRange(CellText const &cellText) {
  auto qRangeOrError = ISISReflectometry::parseQRange(
      cellText[LookupRow::Column::QMIN], cellText[LookupRow::Column::QMAX], cellText[LookupRow::Column::QSTEP]);
  return boost::apply_visitor(InsertErrorIfNotType<RangeInQ>(m_invalidColumns, LookupRow::Column::QMIN), qRangeOrError);
}

ValidatorT<boost::optional<double>> LookupRowValidator::parseScaleFactor(CellText const &cellText) {
  auto optionalScaleFactorOrNoneIfError = ISISReflectometry::parseScaleFactor(cellText[LookupRow::Column::SCALE]);
  if (!optionalScaleFactorOrNoneIfError.is_initialized())
    m_invalidColumns.insert(LookupRow::Column::SCALE);
  return optionalScaleFactorOrNoneIfError;
}

ValidatorT<boost::optional<std::string>> LookupRowValidator::parseProcessingInstructions(CellText const &cellText) {
  auto optionalInstructionsOrNoneIfError =
      ISISReflectometry::parseProcessingInstructions(cellText[LookupRow::Column::RUN_SPECTRA]);
  if (!optionalInstructionsOrNoneIfError.is_initialized())
    m_invalidColumns.insert(LookupRow::Column::RUN_SPECTRA);
  return optionalInstructionsOrNoneIfError;
}

ValidatorT<boost::optional<std::string>>
LookupRowValidator::parseBackgroundProcessingInstructions(CellText const &cellText) {
  auto optionalInstructionsOrNoneIfError =
      ISISReflectometry::parseProcessingInstructions(cellText[LookupRow::Column::BACKGROUND_SPECTRA]);
  if (!optionalInstructionsOrNoneIfError.is_initialized())
    m_invalidColumns.insert(LookupRow::Column::BACKGROUND_SPECTRA);
  return optionalInstructionsOrNoneIfError;
}

ValidatorT<boost::optional<std::string>> LookupRowValidator::parseROIDetectorIDs(CellText const &cellText) {
  auto optionalROIDetectorsOrNoneIfError =
      ISISReflectometry::parseProcessingInstructions(cellText[LookupRow::Column::ROI_DETECTOR_IDS]);
  if (!optionalROIDetectorsOrNoneIfError.is_initialized())
    m_invalidColumns.insert(LookupRow::Column::ROI_DETECTOR_IDS);
  return optionalROIDetectorsOrNoneIfError;
}

void LookupRowValidator::validateThetaAndRegex() {
  // If either value didn't even parse, there's nothing further to check, so return
  if (!m_thetaOrInvalid.is_initialized() || !m_titleMatcherOrInvalid.is_initialized())
    return;

  // Check we have a theta value, when we have a titleMatcher
  if (m_titleMatcherOrInvalid.get().has_value() && !m_thetaOrInvalid.get().is_initialized()) {
    m_invalidColumns.insert(LookupRow::Column::THETA);
    m_invalidColumns.insert(LookupRow::Column::TITLE);
    m_thetaOrInvalid = boost::none;
    m_titleMatcherOrInvalid = boost::none;
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

  auto maybeDefaults = makeIfAllInitialized<LookupRow>(m_thetaOrInvalid, m_titleMatcherOrInvalid, maybeTransmissionRuns,
                                                       maybeTransmissionProcessingInstructions, maybeQRange,
                                                       maybeScaleFactor, maybeProcessingInstructions,
                                                       maybeBackgroundProcessingInstructions, maybeROIDetectorIDs);

  if (maybeDefaults.is_initialized())
    return ValidationResult<LookupRow, std::unordered_set<int>>(maybeDefaults.get());
  else
    return ValidationResult<LookupRow, std::unordered_set<int>>(m_invalidColumns);
}

ValidationResult<LookupRow, std::unordered_set<int>> validateLookupRow(CellText const &cells) {
  auto validate = LookupRowValidator();
  return validate(cells);
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
