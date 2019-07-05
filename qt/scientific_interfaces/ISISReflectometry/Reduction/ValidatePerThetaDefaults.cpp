// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ValidatePerThetaDefaults.h"
#include "AllInitialized.h"
#include "Common/Parse.h"
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

using CellText = PerThetaDefaults::ValueArray;

boost::optional<boost::optional<double>>
PerThetaDefaultsValidator::parseThetaOrWhitespace(CellText const &cellText) {
  if (isEntirelyWhitespace(cellText[0])) {
    return boost::optional<double>();
  } else {
    auto theta = ::MantidQt::CustomInterfaces::parseTheta(cellText[0]);
    if (theta.is_initialized()) {
      return theta;
    }
  }
  m_invalidColumns.emplace_back(0);
  return boost::none;
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

boost::optional<boost::optional<std::string>>
PerThetaDefaultsValidator::parseTransmissionProcessingInstructions(
    CellText const &cellText) {
  auto optionalInstructionsOrNoneIfError =
      ::MantidQt::CustomInterfaces::parseProcessingInstructions(cellText[3]);
  if (!optionalInstructionsOrNoneIfError.is_initialized())
    m_invalidColumns.emplace_back(3);
  return optionalInstructionsOrNoneIfError;
}

boost::optional<RangeInQ>
PerThetaDefaultsValidator::parseQRange(CellText const &cellText) {
  auto qRangeOrError = ::MantidQt::CustomInterfaces::parseQRange(
      cellText[4], cellText[5], cellText[6]);
  return boost::apply_visitor(
      AppendErrorIfNotType<RangeInQ>(m_invalidColumns, 4), qRangeOrError);
}

boost::optional<boost::optional<double>>
PerThetaDefaultsValidator::parseScaleFactor(CellText const &cellText) {
  auto optionalScaleFactorOrNoneIfError =
      ::MantidQt::CustomInterfaces::parseScaleFactor(cellText[7]);
  if (!optionalScaleFactorOrNoneIfError.is_initialized())
    m_invalidColumns.emplace_back(7);
  return optionalScaleFactorOrNoneIfError;
}

boost::optional<boost::optional<std::string>>
PerThetaDefaultsValidator::parseProcessingInstructions(
    CellText const &cellText) {
  auto optionalInstructionsOrNoneIfError =
      ::MantidQt::CustomInterfaces::parseProcessingInstructions(cellText[8]);
  if (!optionalInstructionsOrNoneIfError.is_initialized())
    m_invalidColumns.emplace_back(8);
  return optionalInstructionsOrNoneIfError;
}

ValidationResult<PerThetaDefaults, std::vector<int>> PerThetaDefaultsValidator::
operator()(CellText const &cellText) {
  auto maybeTheta = parseThetaOrWhitespace(cellText);
  auto maybeTransmissionRuns = parseTransmissionRuns(cellText);
  auto maybeTransmissionProcessingInstructions =
      parseTransmissionProcessingInstructions(cellText);
  auto maybeQRange = parseQRange(cellText);
  auto maybeScaleFactor = parseScaleFactor(cellText);
  auto maybeProcessingInstructions = parseProcessingInstructions(cellText);
  auto maybeDefaults = makeIfAllInitialized<PerThetaDefaults>(
      maybeTheta, maybeTransmissionRuns,
      maybeTransmissionProcessingInstructions, maybeQRange, maybeScaleFactor,
      maybeProcessingInstructions);

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
} // namespace CustomInterfaces
} // namespace MantidQt
