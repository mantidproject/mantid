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
#include <optional>
namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class LookupRowValidator

    The LookupRowValidator does the work to validate whether entries in
    the lookup table on the ExperimentSettings tab are valid.
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL LookupRowValidator {
public:
  ValidationResult<LookupRow, std::vector<int>> operator()(LookupRow::ValueArray const &cellText);

private:
  std::optional<std::optional<double>> parseThetaOrWhitespace(LookupRow::ValueArray const &cellText);
  std::optional<TransmissionRunPair> parseTransmissionRuns(LookupRow::ValueArray const &cellText);
  std::optional<std::optional<std::string>>
  parseTransmissionProcessingInstructions(LookupRow::ValueArray const &cellText);
  std::optional<RangeInQ> parseQRange(LookupRow::ValueArray const &cellText);
  std::optional<std::optional<double>> parseScaleFactor(LookupRow::ValueArray const &cellText);
  std::optional<std::map<std::string, std::string>> parseOptions(LookupRow::ValueArray const &cellText);
  std::optional<std::optional<std::string>> parseProcessingInstructions(LookupRow::ValueArray const &cellText);
  std::optional<std::optional<std::string>>
  parseBackgroundProcessingInstructions(LookupRow::ValueArray const &cellText);

  std::vector<int> m_invalidColumns;
};

ValidationResult<LookupRow, std::vector<int>> validateLookupRow(LookupRow::ValueArray const &cellText);

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
