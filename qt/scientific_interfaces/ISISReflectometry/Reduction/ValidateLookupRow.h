// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "Common/ValidationResult.h"
#include "LookupRow.h"
#include "ParseReflectometryStrings.h"
#include "TransmissionRunPair.h"
#include <array>
#include <boost/optional.hpp>
#include <unordered_set>
namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class LookupRowValidator

    The LookupRowValidator does the work to validate whether entries in
    the lookup table on the ExperimentSettings tab are valid.
 */

template <typename T> using ValidatorT = boost::optional<T>;

class MANTIDQT_ISISREFLECTOMETRY_DLL LookupRowValidator {
public:
  ValidationResult<LookupRow, std::unordered_set<int>> operator()(LookupRow::ValueArray const &cellText);

private:
  ValidatorT<boost::optional<double>> parseThetaOrWhitespace(LookupRow::ValueArray const &cellText);
  ValidatorT<std::optional<boost::regex>> parseTitleMatcherOrWhitespace(LookupRow::ValueArray const &cellText);
  ValidatorT<TransmissionRunPair> parseTransmissionRuns(LookupRow::ValueArray const &cellText);
  ValidatorT<boost::optional<std::string>>
  parseTransmissionProcessingInstructions(LookupRow::ValueArray const &cellText);
  ValidatorT<RangeInQ> parseQRange(LookupRow::ValueArray const &cellText);
  ValidatorT<boost::optional<double>> parseScaleFactor(LookupRow::ValueArray const &cellText);
  ValidatorT<std::map<std::string, std::string>> parseOptions(LookupRow::ValueArray const &cellText);
  ValidatorT<boost::optional<std::string>> parseProcessingInstructions(LookupRow::ValueArray const &cellText);
  ValidatorT<boost::optional<std::string>> parseBackgroundProcessingInstructions(LookupRow::ValueArray const &cellText);
  ValidatorT<boost::optional<std::string>> parseROIDetectorIDs(LookupRow::ValueArray const &cellText);

  void validateThetaAndRegex();

  std::unordered_set<int> m_invalidColumns;
  ValidatorT<boost::optional<double>> m_thetaOrInvalid;
  ValidatorT<std::optional<boost::regex>> m_titleMatcherOrInvalid;
};

ValidationResult<LookupRow, std::unordered_set<int>> validateLookupRow(LookupRow::ValueArray const &cellText);

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
