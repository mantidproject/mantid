// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "Common/ValidationResult.h"
#include "ParseReflectometryStrings.h"
#include "Reduction/ReductionJobs.h"
#include "Row.h"
#include "TransmissionRunPair.h"
#include <optional>
namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class RowValidator

    The RowValidator does the work to check whether cells in a row on the runs
    table are valid
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL RowValidator {
public:
  ValidationResult<Row, std::vector<int>> operator()(std::vector<std::string> const &cellText);

private:
  std::optional<std::vector<std::string>> parseRunNumbers(std::vector<std::string> const &cellText);
  std::optional<double> parseTheta(std::vector<std::string> const &cellText);
  std::optional<TransmissionRunPair> parseTransmissionRuns(std::vector<std::string> const &cellText);
  std::optional<RangeInQ> parseQRange(std::vector<std::string> const &cellText);
  std::optional<double> parseScaleFactor(std::vector<std::string> const &cellText);
  std::optional<ReductionOptionsMap> parseOptions(std::vector<std::string> const &cellText);

  std::vector<int> m_invalidColumns;
};

using RowValidationResult = ValidationResult<Row, std::vector<int>>;

RowValidationResult validateRow(std::vector<std::string> const &cellText);

std::optional<Row> validateRowFromRunAndTheta(std::string const &run, std::string const &theta);
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
