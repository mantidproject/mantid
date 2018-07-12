#ifndef MANTID_ISISREFLECTOMETRY_PERTHETADEFAULTSTABLEVALIDATOR_H
#define MANTID_ISISREFLECTOMETRY_PERTHETADEFAULTSTABLEVALIDATOR_H
#include "PerThetaDefaultsTableValidationError.h"
#include "../../Reduction/PerThetaDefaults.h"
#include "../../ValidationResult.h"

namespace MantidQt {
namespace CustomInterfaces {

class PerThetaDefaultsTableValidator {
public:
  using ResultType = ValidationResult<std::vector<PerThetaDefaults>,
                                      PerThetaDefaultsTableValidationError>;

  ResultType operator()(
      std::vector<std::array<std::string, 8>> const &perThetaDefaultsContent,
      double thetaTolerance) const;

  ValidationResult<boost::blank, ThetaValuesValidationError>
  validateThetaValues(std::vector<PerThetaDefaults> perThetaDefaults,
                      double tolerance) const;

  void validateAllPerThetaDefaultRows(
      std::vector<std::array<std::string, 8>> const &perThetaDefaultsContent,
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
}
}
#endif // MANTID_ISISREFLECTOMETRY_PERTHETADEFAULTSTABLEVALIDATOR_H
