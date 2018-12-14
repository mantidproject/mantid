// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "PerThetaDefaultsTableValidator.h"
#include "../../Reduction/ValidatePerThetaDefaults.h"

#include <cmath>

namespace MantidQt {
namespace CustomInterfaces {

auto PerThetaDefaultsTableValidator::
operator()(ContentType const &perThetaDefaultsContent,
           double thetaTolerance) const -> ResultType {

  auto defaults = std::vector<PerThetaDefaults>();
  auto validationErrors = std::vector<InvalidDefaultsError>();
  validateAllPerThetaDefaultRows(perThetaDefaultsContent, defaults,
                                 validationErrors);
  auto thetaValidationResult = validateThetaValues(defaults, thetaTolerance);
  if (thetaValidationResult.isValid()) {
    if (validationErrors.empty())
      return ResultType(std::move(defaults));
    else
      return ResultType(PerThetaDefaultsTableValidationError(
          std::move(validationErrors), boost::none));
  } else {
    appendThetaErrorForAllRows(validationErrors,
                               perThetaDefaultsContent.size());
    return ResultType(PerThetaDefaultsTableValidationError(
        std::move(validationErrors), thetaValidationResult.assertError()));
  }
}

ValidationResult<boost::blank, ThetaValuesValidationError>
PerThetaDefaultsTableValidator::validateThetaValues(
    std::vector<PerThetaDefaults> perThetaDefaults, double tolerance) const {
  using Result = ValidationResult<boost::blank, ThetaValuesValidationError>;
  auto ok = Result(boost::blank());
  if (!perThetaDefaults.empty()) {
    auto const wildcardCount = countWildcards(perThetaDefaults);
    if (wildcardCount <= 1) {
      if (hasUniqueThetas(std::move(perThetaDefaults), wildcardCount,
                          tolerance))
        return ok;
      else
        return Result(ThetaValuesValidationError::NonUniqueTheta);
    } else {
      return Result(ThetaValuesValidationError::MultipleWildcards);
    }
  } else {
    return ok;
  }
}

void PerThetaDefaultsTableValidator::validateAllPerThetaDefaultRows(
    ContentType const &perThetaDefaultsContent,
    std::vector<PerThetaDefaults> &perThetaDefaults,
    std::vector<InvalidDefaultsError> &validationErrors) const {
  auto row = 0;
  for (auto const &rowTemplateContent : perThetaDefaultsContent) {
    auto rowValidationResult = validatePerThetaDefaults(rowTemplateContent);
    if (rowValidationResult.isValid())
      perThetaDefaults.emplace_back(rowValidationResult.assertValid());
    else
      validationErrors.emplace_back(row, rowValidationResult.assertError());
    row++;
  }
}

bool PerThetaDefaultsTableValidator::hasUniqueThetas(
    std::vector<PerThetaDefaults> perThetaDefaults, int wildcardCount,
    double tolerance) const {
  if (perThetaDefaults.size() < 2)
    return true;

  sortInPlaceWildcardsFirstThenByTheta(perThetaDefaults);
  auto thetasWithinTolerance =
      [tolerance](PerThetaDefaults const &lhs,
                  PerThetaDefaults const &rhs) -> bool {
    double const difference =
        lhs.thetaOrWildcard().get() - rhs.thetaOrWildcard().get();
    return std::abs(difference) < tolerance;
  };

  bool foundDuplicate = false;
  for (auto iter = perThetaDefaults.cbegin() + wildcardCount + 1;
       !foundDuplicate && iter != perThetaDefaults.cend(); ++iter) {
    foundDuplicate = thetasWithinTolerance(*iter, *prev(iter));
  }

  return !foundDuplicate;
}

int PerThetaDefaultsTableValidator::countWildcards(
    std::vector<PerThetaDefaults> const &perThetaDefaults) const {
  return static_cast<int>(
      std::count_if(perThetaDefaults.cbegin(), perThetaDefaults.cend(),
                    [](PerThetaDefaults const &defaults) -> bool {
                      return defaults.isWildcard();
                    }));
}

void PerThetaDefaultsTableValidator::sortInPlaceWildcardsFirstThenByTheta(
    std::vector<PerThetaDefaults> &perThetaDefaults) const {
  auto thetaLessThan = [](PerThetaDefaults const &lhs,
                          PerThetaDefaults const &rhs) -> bool {
    if (lhs.isWildcard())
      return true;
    else if (rhs.isWildcard())
      return false;
    else
      return lhs.thetaOrWildcard().get() < rhs.thetaOrWildcard().get();
  };
  std::sort(perThetaDefaults.begin(), perThetaDefaults.end(), thetaLessThan);
}

void PerThetaDefaultsTableValidator::appendThetaErrorForAllRows(
    std::vector<InvalidDefaultsError> &validationErrors,
    std::size_t rowCount) const {
  for (auto row = 0u; row < rowCount; ++row)
    validationErrors.emplace_back(row, std::vector<int>({0}));
}
} // namespace CustomInterfaces
} // namespace MantidQt
