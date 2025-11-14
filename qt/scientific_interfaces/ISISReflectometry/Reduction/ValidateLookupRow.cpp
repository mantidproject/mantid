// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ValidateLookupRow.h"
#include "Common/Parse.h"
#include <boost/variant.hpp>
#include <unordered_set>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

namespace MantidQt::CustomInterfaces::ISISReflectometry {

template <typename T> class InsertErrorIfNotType : public boost::static_visitor<std::optional<T>> {
public:
  InsertErrorIfNotType(std::unordered_set<int> &invalidParams, int baseColumn)
      : m_invalidParams(invalidParams), m_baseColumn(baseColumn) {}

  std::optional<T> operator()(T const &result) const { return result; }

  std::optional<T> operator()(int errorColumn) const {
    m_invalidParams.insert(m_baseColumn + errorColumn);
    return std::nullopt;
  }

  std::optional<T> operator()(const std::vector<int> &errorColumns) const {
    std::transform(errorColumns.cbegin(), errorColumns.cend(), std::inserter(m_invalidParams, m_invalidParams.end()),
                   [this](int column) -> int { return m_baseColumn + column; });
    return std::nullopt;
  }

private:
  std::unordered_set<int> &m_invalidParams;
  int m_baseColumn;
};

using CellText = LookupRow::ValueArray;

std::optional<double> LookupRowValidator::parseThetaOrWhitespace(CellText const &cellText) {
  if (isEntirelyWhitespace(cellText[LookupRow::Column::THETA])) {
    return std::optional<double>();
  }
  auto theta = parseTheta(cellText[LookupRow::Column::THETA]);
  if (theta.has_value()) {
    return theta;
  }
  m_invalidColumns.insert(LookupRow::Column::THETA);
  return std::nullopt;
}

std::optional<boost::regex> LookupRowValidator::parseTitleMatcherOrWhitespace(CellText const &cellText) {
  auto const &text = cellText[LookupRow::Column::TITLE];
  if (isEntirelyWhitespace(text)) {
    // Mark validator as passed, but the enclosed value empty
    return std::optional<boost::regex>(std::nullopt);
  }
  // This check relies on us checking for whitespace chars before calling parseTitleMatcher
  if (auto result = parseTitleMatcher(text)) {
    return result;
  }
  m_invalidColumns.insert(LookupRow::Column::TITLE);
  return std::nullopt;
}

std::optional<TransmissionRunPair> LookupRowValidator::parseTransmissionRuns(CellText const &cellText) {
  auto transmissionRunsOrError = ISISReflectometry::parseTransmissionRuns(cellText[LookupRow::Column::FIRST_TRANS],
                                                                          cellText[LookupRow::Column::SECOND_TRANS]);
  return boost::apply_visitor(
      InsertErrorIfNotType<TransmissionRunPair>(m_invalidColumns, LookupRow::Column::FIRST_TRANS),
      transmissionRunsOrError);
}

std::optional<std::string> LookupRowValidator::parseTransmissionProcessingInstructions(CellText const &cellText) {
  auto [optionalInstructions, isValid] =
      ISISReflectometry::parseProcessingInstructions(cellText[LookupRow::Column::TRANS_SPECTRA]);
  if (!isValid) {
    m_invalidColumns.insert(LookupRow::Column::TRANS_SPECTRA);
  }
  return optionalInstructions;
}

std::optional<RangeInQ> LookupRowValidator::parseQRange(CellText const &cellText) {
  auto qRangeOrError = ISISReflectometry::parseQRange(
      cellText[LookupRow::Column::QMIN], cellText[LookupRow::Column::QMAX], cellText[LookupRow::Column::QSTEP]);
  return boost::apply_visitor(InsertErrorIfNotType<RangeInQ>(m_invalidColumns, LookupRow::Column::QMIN), qRangeOrError);
}

std::optional<double> LookupRowValidator::parseScaleFactor(CellText const &cellText) {
  auto [optionalScaleFactor, isValid] = ISISReflectometry::parseScaleFactor(cellText[LookupRow::Column::SCALE]);
  if (!isValid) {
    m_invalidColumns.insert(LookupRow::Column::SCALE);
  }
  return optionalScaleFactor;
}

std::optional<std::string> LookupRowValidator::parseProcessingInstructions(CellText const &cellText) {
  auto [optionalInstructions, isValid] =
      ISISReflectometry::parseProcessingInstructions(cellText[LookupRow::Column::RUN_SPECTRA]);
  if (!isValid) {
    m_invalidColumns.insert(LookupRow::Column::RUN_SPECTRA);
  }
  return optionalInstructions;
}

std::optional<std::string> LookupRowValidator::parseBackgroundProcessingInstructions(CellText const &cellText) {
  auto [optionalInstructions, isValid] =
      ISISReflectometry::parseProcessingInstructions(cellText[LookupRow::Column::BACKGROUND_SPECTRA]);
  if (!isValid) {
    m_invalidColumns.insert(LookupRow::Column::BACKGROUND_SPECTRA);
  }
  return optionalInstructions;
}

std::optional<std::string> LookupRowValidator::parseROIDetectorIDs(CellText const &cellText) {
  auto [optionalROIDetectors, isValid] =
      ISISReflectometry::parseProcessingInstructions(cellText[LookupRow::Column::ROI_DETECTOR_IDS]);
  if (!isValid) {
    m_invalidColumns.insert(LookupRow::Column::ROI_DETECTOR_IDS);
  }
  return optionalROIDetectors;
}

void LookupRowValidator::validateThetaAndRegex() {
  // Check we have a theta value, when we have a titleMatcher
  if (m_titleMatcherOrInvalid.has_value() && !m_thetaOrInvalid.has_value()) {
    m_invalidColumns.insert(LookupRow::Column::THETA);
    m_invalidColumns.insert(LookupRow::Column::TITLE);
    m_thetaOrInvalid = std::nullopt;
    m_titleMatcherOrInvalid = std::nullopt;
  }
}

ValidationResult<LookupRow, std::unordered_set<int>> LookupRowValidator::operator()(CellText const &cellText) {
  m_thetaOrInvalid = parseThetaOrWhitespace(cellText);
  m_titleMatcherOrInvalid = parseTitleMatcherOrWhitespace(cellText);
  validateThetaAndRegex();

  auto optionalTransmissionRuns = parseTransmissionRuns(cellText);
  auto optionalTransmissionProcessingInstructions = parseTransmissionProcessingInstructions(cellText);
  auto optionalQRange = parseQRange(cellText);
  auto optionalScaleFactor = parseScaleFactor(cellText);
  auto optionalProcessingInstructions = parseProcessingInstructions(cellText);
  auto optionalBackgroundProcessingInstructions = parseBackgroundProcessingInstructions(cellText);
  auto optionalROIDetectorIDs = parseROIDetectorIDs(cellText);
  if (!m_invalidColumns.empty()) {
    return ValidationResult<LookupRow, std::unordered_set<int>>(m_invalidColumns);
  }

  // if we reach this point, no column is invalid, but there may be empty columns.
  // We just need to pass on the values (Even though they are nullopt) of each column
  auto lookupRow = LookupRow(m_thetaOrInvalid, m_titleMatcherOrInvalid, optionalTransmissionRuns.value(),
                             std::move(optionalTransmissionProcessingInstructions), optionalQRange.value(),
                             optionalScaleFactor, std::move(optionalProcessingInstructions),
                             std::move(optionalBackgroundProcessingInstructions), std::move(optionalROIDetectorIDs));

  return ValidationResult<LookupRow, std::unordered_set<int>>(std::move(lookupRow));
}

ValidationResult<LookupRow, std::unordered_set<int>> validateLookupRow(CellText const &cells) {
  auto validate = LookupRowValidator();
  return validate(cells);
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
