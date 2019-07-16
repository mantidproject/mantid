// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_PERTHETADEFAULTSTABLEVALIDATOR_H
#define MANTID_ISISREFLECTOMETRY_PERTHETADEFAULTSTABLEVALIDATOR_H
#include "Common/ValidationResult.h"
#include "PerThetaDefaultsTableValidationError.h"
#include "Reduction/PerThetaDefaults.h"
#include <array>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL PerThetaDefaultsTableValidator {
public:
  using ContentType = std::vector<PerThetaDefaults::ValueArray>;
  using ResultType = ValidationResult<std::vector<PerThetaDefaults>,
                                      PerThetaDefaultsTableValidationError>;

  ResultType operator()(ContentType const &perThetaDefaultsContent,
                        double thetaTolerance) const;

  ValidationResult<boost::blank, ThetaValuesValidationError>
  validateThetaValues(std::vector<PerThetaDefaults> perThetaDefaults,
                      double tolerance) const;

  void validateAllPerThetaDefaultRows(
      ContentType const &perThetaDefaultsContent,
      std::vector<PerThetaDefaults> &perThetaDefaults,
      std::vector<InvalidDefaultsError> &validationErrors) const;

  int countWildcards(
      std::vector<PerThetaDefaults> const &perThetaDefaults) const;

  void sortInPlaceWildcardsFirstThenByTheta(
      std::vector<PerThetaDefaults> &perThetaDefaults) const;

  bool hasUniqueThetas(std::vector<PerThetaDefaults> perThetaDefaults,
                       int wildcardCount, double tolerance) const;
  void appendThetaErrorForAllRows(
      std::vector<InvalidDefaultsError> &validationErrors,
      std::size_t rowCount) const;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_PERTHETADEFAULTSTABLEVALIDATOR_H
