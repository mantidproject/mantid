// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_CUSTOMINTERFACES_PERTHETADEFAUTSVALIDATOR_H_
#define MANTID_CUSTOMINTERFACES_PERTHETADEFAUTSVALIDATOR_H_
#include "../ValidationResult.h"
#include "DllConfig.h"
#include "ParseReflectometryStrings.h"
#include "PerThetaDefaults.h"
#include "TransmissionRunPair.h"
#include "ValidationResult.h"
#include <boost/optional.hpp>
namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL PerThetaDefaultsValidator {
public:
  static auto constexpr INPUT_FIELD_COUNT = 8;

  ValidationResult<PerThetaDefaults, std::vector<int>>
  operator()(std::array<std::string, INPUT_FIELD_COUNT> const &cellText);

private:
  boost::optional<boost::optional<double>> parseThetaOrWhitespace(
      std::array<std::string, INPUT_FIELD_COUNT> const &cellText);
  boost::optional<TransmissionRunPair> parseTransmissionRuns(
      std::array<std::string, INPUT_FIELD_COUNT> const &cellText);
  boost::optional<RangeInQ>
  parseQRange(std::array<std::string, INPUT_FIELD_COUNT> const &cellText);
  boost::optional<boost::optional<double>>
  parseScaleFactor(std::array<std::string, INPUT_FIELD_COUNT> const &cellText);
  boost::optional<std::map<std::string, std::string>>
  parseOptions(std::array<std::string, INPUT_FIELD_COUNT> const &cellText);
  boost::optional<boost::optional<std::string>> parseProcessingInstructions(
      std::array<std::string, INPUT_FIELD_COUNT> const &cellText);

  std::vector<int> m_invalidColumns;
};

ValidationResult<PerThetaDefaults, std::vector<int>> validatePerThetaDefaults(
    std::array<std::string, PerThetaDefaultsValidator::INPUT_FIELD_COUNT> const
        &cellText);

} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_PERTHETADEFAUTSVALIDATOR_H_
