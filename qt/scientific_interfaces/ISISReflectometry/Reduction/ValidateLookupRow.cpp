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

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

template <typename T> class AppendErrorIfNotType : public boost::static_visitor<boost::optional<T>> {
public:
  AppendErrorIfNotType(std::vector<int> &invalidParams, int baseColumn)
      : m_invalidParams(invalidParams), m_baseColumn(baseColumn) {}

  boost::optional<T> operator()(T const &result) const { return result; }

  boost::optional<T> operator()(int errorColumn) const {
    m_invalidParams.emplace_back(m_baseColumn + errorColumn);
    return boost::none;
  }

  boost::optional<T> operator()(const std::vector<int> &errorColumns) const {
    std::transform(errorColumns.cbegin(), errorColumns.cend(), std::back_inserter(m_invalidParams),
                   [this](int column) -> int { return m_baseColumn + column; });
    return boost::none;
  }

private:
  std::vector<int> &m_invalidParams;
  int m_baseColumn;
};

using CellText = LookupRow::ValueArray;

boost::optional<boost::optional<double>> LookupRowValidator::parseThetaOrWhitespace(CellText const &cellText) {
  if (isEntirelyWhitespace(cellText[0])) {
    return boost::optional<double>();
  } else {
    auto theta = ::MantidQt::CustomInterfaces::ISISReflectometry::parseTheta(cellText[0]);
    if (theta.is_initialized()) {
      return theta;
    }
  }
  m_invalidColumns.emplace_back(0);
  return boost::none;
}

boost::optional<TransmissionRunPair> LookupRowValidator::parseTransmissionRuns(CellText const &cellText) {
  auto transmissionRunsOrError =
      ::MantidQt::CustomInterfaces::ISISReflectometry::parseTransmissionRuns(cellText[1], cellText[2]);
  return boost::apply_visitor(AppendErrorIfNotType<TransmissionRunPair>(m_invalidColumns, 1), transmissionRunsOrError);
}

boost::optional<boost::optional<std::string>>
LookupRowValidator::parseTransmissionProcessingInstructions(CellText const &cellText) {
  auto optionalInstructionsOrNoneIfError =
      ::MantidQt::CustomInterfaces::ISISReflectometry::parseProcessingInstructions(cellText[3]);
  if (!optionalInstructionsOrNoneIfError.is_initialized())
    m_invalidColumns.emplace_back(3);
  return optionalInstructionsOrNoneIfError;
}

boost::optional<RangeInQ> LookupRowValidator::parseQRange(CellText const &cellText) {
  auto qRangeOrError =
      ::MantidQt::CustomInterfaces::ISISReflectometry::parseQRange(cellText[4], cellText[5], cellText[6]);
  return boost::apply_visitor(AppendErrorIfNotType<RangeInQ>(m_invalidColumns, 4), qRangeOrError);
}

boost::optional<boost::optional<double>> LookupRowValidator::parseScaleFactor(CellText const &cellText) {
  auto optionalScaleFactorOrNoneIfError =
      ::MantidQt::CustomInterfaces::ISISReflectometry::parseScaleFactor(cellText[7]);
  if (!optionalScaleFactorOrNoneIfError.is_initialized())
    m_invalidColumns.emplace_back(7);
  return optionalScaleFactorOrNoneIfError;
}

boost::optional<boost::optional<std::string>>
LookupRowValidator::parseProcessingInstructions(CellText const &cellText) {
  auto optionalInstructionsOrNoneIfError =
      ::MantidQt::CustomInterfaces::ISISReflectometry::parseProcessingInstructions(cellText[8]);
  if (!optionalInstructionsOrNoneIfError.is_initialized())
    m_invalidColumns.emplace_back(8);
  return optionalInstructionsOrNoneIfError;
}

boost::optional<boost::optional<std::string>>
LookupRowValidator::parseBackgroundProcessingInstructions(CellText const &cellText) {
  auto optionalInstructionsOrNoneIfError =
      ::MantidQt::CustomInterfaces::ISISReflectometry::parseProcessingInstructions(cellText[9]);
  if (!optionalInstructionsOrNoneIfError.is_initialized())
    m_invalidColumns.emplace_back(9);
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

  if (maybeDefaults.is_initialized())
    return ValidationResult<LookupRow, std::vector<int>>(maybeDefaults.get());
  else
    return ValidationResult<LookupRow, std::vector<int>>(m_invalidColumns);
}

ValidationResult<LookupRow, std::vector<int>> validateLookupRow(CellText const &cells) {
  auto validate = LookupRowValidator();
  return validate(cells);
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
