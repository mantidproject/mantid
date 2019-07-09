// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_CUSTOMINTERFACES_PERTHETADEFAUTSVALIDATOR_H_
#define MANTID_CUSTOMINTERFACES_PERTHETADEFAUTSVALIDATOR_H_
#include "Common/DllConfig.h"
#include "Common/ValidationResult.h"
#include "ParseReflectometryStrings.h"
#include "PerThetaDefaults.h"
#include "TransmissionRunPair.h"
#include <array>
#include <boost/optional.hpp>
namespace MantidQt {
namespace CustomInterfaces {

/** @class PerThetaDefaultsValidator

    The PerThetaDefaultsValidator does the work to validate whether entries in
    the per-theta defaults table on the ExperimentSettings tab are valid.
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL PerThetaDefaultsValidator {
public:
  ValidationResult<PerThetaDefaults, std::vector<int>>
  operator()(PerThetaDefaults::ValueArray const &cellText);

private:
  boost::optional<boost::optional<double>>
  parseThetaOrWhitespace(PerThetaDefaults::ValueArray const &cellText);
  boost::optional<TransmissionRunPair>
  parseTransmissionRuns(PerThetaDefaults::ValueArray const &cellText);
  boost::optional<boost::optional<std::string>>
  parseTransmissionProcessingInstructions(
      PerThetaDefaults::ValueArray const &cellText);
  boost::optional<RangeInQ>
  parseQRange(PerThetaDefaults::ValueArray const &cellText);
  boost::optional<boost::optional<double>>
  parseScaleFactor(PerThetaDefaults::ValueArray const &cellText);
  boost::optional<std::map<std::string, std::string>>
  parseOptions(PerThetaDefaults::ValueArray const &cellText);
  boost::optional<boost::optional<std::string>>
  parseProcessingInstructions(PerThetaDefaults::ValueArray const &cellText);

  std::vector<int> m_invalidColumns;
};

ValidationResult<PerThetaDefaults, std::vector<int>>
validatePerThetaDefaults(PerThetaDefaults::ValueArray const &cellText);

} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_PERTHETADEFAUTSVALIDATOR_H_
