// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "LookupTableValidator.h"
#include "../../Reduction/ValidateLookupRow.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {
constexpr double EPSILON = std::numeric_limits<double>::epsilon();

bool equalWithinTolerance(double val1, double val2, double tolerance) {
  bool result = std::abs(val1 - val2) <= (tolerance + EPSILON);
  return result;
}
} // namespace

namespace MantidQt::CustomInterfaces::ISISReflectometry {

auto LookupTableValidator::operator()(ContentType const &lookupTableContent, double thetaTolerance) const
    -> ResultType {

  auto lookupTable = LookupTableRows();
  auto validationErrors = std::vector<InvalidLookupRowCells>();
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

ValidationResult<boost::blank, LookupCriteriaError>
LookupTableValidator::validateThetaValues(LookupTableRows lookupTable, double tolerance) const {
  using Result = ValidationResult<boost::blank, LookupCriteriaError>;
  auto ok = Result(boost::blank());
  if (!lookupTable.empty()) {
    auto const wildcardCount = countWildcards(lookupTable);
    if (wildcardCount <= 1) {
      if (hasUniqueSearchCriteria(std::move(lookupTable), wildcardCount, tolerance))
        return ok;
      else
        return Result(LookupCriteriaError::NonUniqueSearchCriteria);
    } else {
      return Result(LookupCriteriaError::MultipleWildcards);
    }
  } else {
    return ok;
  }
}

void LookupTableValidator::validateAllLookupRows(ContentType const &lookupTableContent, LookupTableRows &lookupTable,
                                                 std::vector<InvalidLookupRowCells> &validationErrors) const {
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

bool LookupTableValidator::hasUniqueSearchCriteria(LookupTableRows lookupTable, int wildcardCount,
                                                   double tolerance) const {
  if (lookupTable.size() < 2)
    return true;

  sortInPlaceByThetaThenTitleMatcher(lookupTable);

  auto lookupRowsMatch = [tolerance](LookupRow const &lhs, LookupRow const &rhs) -> bool {
    if (lhs.titleMatcher() != rhs.titleMatcher()) {
      return false;
    }
    return equalWithinTolerance(*lhs.thetaOrWildcard(), *rhs.thetaOrWildcard(), tolerance);
  };

  bool foundDuplicate = false;
  for (auto iter = lookupTable.cbegin() + wildcardCount + 1; !foundDuplicate && iter != lookupTable.cend(); ++iter) {
    foundDuplicate = lookupRowsMatch(*iter, *prev(iter));
  }

  return !foundDuplicate;
}

int LookupTableValidator::countWildcards(LookupTableRows const &lookupTable) const {
  return static_cast<int>(std::count_if(lookupTable.cbegin(), lookupTable.cend(),
                                        [](LookupRow const &lookupRow) -> bool { return lookupRow.isWildcard(); }));
}

void LookupTableValidator::sortInPlaceByThetaThenTitleMatcher(LookupTableRows &lookupTable) const {
  lookupTable.erase(
      std::remove_if(lookupTable.begin(), lookupTable.end(), [](auto const &row) { return row.isWildcard(); }),
      lookupTable.end());

  auto lookupRowLessThan = [](LookupRow const &lhs, LookupRow const &rhs) -> bool {
    // This method should never be called with wildcard rows
    assert(lhs.thetaOrWildcard().is_initialized());
    assert(rhs.thetaOrWildcard().is_initialized());
    if (*lhs.thetaOrWildcard() != *rhs.thetaOrWildcard()) {
      return *lhs.thetaOrWildcard() < *rhs.thetaOrWildcard();
    }
    auto const lhsTitle = lhs.titleMatcher().get_value_or(boost::regex());
    auto const rhsTitle = rhs.titleMatcher().get_value_or(boost::regex());
    return lhsTitle < rhsTitle;
  };

  std::sort(lookupTable.begin(), lookupTable.end(), lookupRowLessThan);
}

void LookupTableValidator::appendThetaErrorForAllRows(std::vector<InvalidLookupRowCells> &validationErrors,
                                                      std::size_t rowCount) const {
  for (auto row = 0u; row < rowCount; ++row)
    validationErrors.emplace_back(row, std::unordered_set<int>({LookupRow::Column::THETA}));
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
