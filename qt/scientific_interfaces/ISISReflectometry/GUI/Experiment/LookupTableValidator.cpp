// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "LookupTableValidator.h"
#include "../../Reduction/ValidateLookupRow.h"

#include <cmath>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

auto LookupTableValidator::operator()(ContentType const &lookupTableContent, double thetaTolerance) const
    -> ResultType {

  auto lookupTable = LookupTable();
  auto validationErrors = std::vector<InvalidDefaultsError>();
  validateAllLookupRows(lookupTableContent, lookupTable, validationErrors);
  auto thetaValidationResult = validateThetaValues(lookupTable, thetaTolerance);
  if (thetaValidationResult.isValid()) {
    if (validationErrors.empty())
      return ResultType(std::move(lookupTable));
    else
      return ResultType(LookupTableValidationError(std::move(validationErrors), boost::none));
  } else {
    appendThetaErrorForAllRows(validationErrors, lookupTableContent.size());
    return ResultType(LookupTableValidationError(std::move(validationErrors), thetaValidationResult.assertError()));
  }
}

ValidationResult<boost::blank, ThetaValuesValidationError>
LookupTableValidator::validateThetaValues(LookupTable lookupTable, double tolerance) const {
  using Result = ValidationResult<boost::blank, ThetaValuesValidationError>;
  auto ok = Result(boost::blank());
  if (!lookupTable.empty()) {
    auto const wildcardCount = countWildcards(lookupTable);
    if (wildcardCount <= 1) {
      if (hasUniqueThetas(std::move(lookupTable), wildcardCount, tolerance))
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

void LookupTableValidator::validateAllLookupRows(ContentType const &lookupTableContent, LookupTable &lookupTable,
                                                 std::vector<InvalidDefaultsError> &validationErrors) const {
  auto row = 0;
  for (auto const &lookupRowContent : lookupTableContent) {
    auto rowValidationResult = validateLookupRow(lookupRowContent);
    if (rowValidationResult.isValid())
      lookupTable.emplace_back(rowValidationResult.assertValid());
    else
      validationErrors.emplace_back(row, rowValidationResult.assertError());
    row++;
  }
}

bool LookupTableValidator::hasUniqueThetas(LookupTable lookupTable, int wildcardCount, double tolerance) const {
  if (lookupTable.size() < 2)
    return true;

  sortInPlaceWildcardsFirstThenByTheta(lookupTable);
  auto thetasWithinTolerance = [tolerance](LookupRow const &lhs, LookupRow const &rhs) -> bool {
    double const difference = lhs.thetaOrWildcard().get() - rhs.thetaOrWildcard().get();
    return std::abs(difference) < tolerance;
  };

  bool foundDuplicate = false;
  for (auto iter = lookupTable.cbegin() + wildcardCount + 1; !foundDuplicate && iter != lookupTable.cend(); ++iter) {
    foundDuplicate = thetasWithinTolerance(*iter, *prev(iter));
  }

  return !foundDuplicate;
}

int LookupTableValidator::countWildcards(LookupTable const &lookupTable) const {
  return static_cast<int>(std::count_if(lookupTable.cbegin(), lookupTable.cend(),
                                        [](LookupRow const &lookupRow) -> bool { return lookupRow.isWildcard(); }));
}

void LookupTableValidator::sortInPlaceWildcardsFirstThenByTheta(LookupTable &lookupTable) const {
  auto thetaLessThan = [](LookupRow const &lhs, LookupRow const &rhs) -> bool {
    if (lhs.isWildcard())
      return true;
    else if (rhs.isWildcard())
      return false;
    else
      return lhs.thetaOrWildcard().get() < rhs.thetaOrWildcard().get();
  };
  std::sort(lookupTable.begin(), lookupTable.end(), thetaLessThan);
}

void LookupTableValidator::appendThetaErrorForAllRows(std::vector<InvalidDefaultsError> &validationErrors,
                                                      std::size_t rowCount) const {
  for (auto row = 0u; row < rowCount; ++row)
    validationErrors.emplace_back(row, std::vector<int>({0}));
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
