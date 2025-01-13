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
  // First check the individual rows for errors
  auto validationErrors = std::vector<InvalidLookupRowCells>();
  validateAllLookupRows(lookupTableContent, lookupTable, validationErrors);
  // Now cross-check search criteria across all rows against each other (in case of duplicates etc.)
  auto searchCriteriaValidationResult = validateSearchCriteria(lookupTable, thetaTolerance);

  if (searchCriteriaValidationResult.isValid()) {
    if (validationErrors.empty()) {
      // No errors - return the valid table
      return ResultType(std::move(lookupTable));
    } else {
      // Return the row errors (but no table errors)
      return ResultType(LookupTableValidationError(std::move(validationErrors), boost::none));
    }
  } else {
    // Mark all rows with the search criteria errors, then return both row and table errors
    appendSearchCriteriaErrorForAllRows(validationErrors, lookupTableContent.size());
    return ResultType(
        LookupTableValidationError(std::move(validationErrors), searchCriteriaValidationResult.assertError()));
  }
}

ValidationResult<boost::blank, LookupCriteriaError>
LookupTableValidator::validateSearchCriteria(LookupTableRows lookupTable, double tolerance) const {
  using Result = ValidationResult<boost::blank, LookupCriteriaError>;
  auto ok = Result(boost::blank());
  // If the table is empty there's nothing to check
  if (lookupTable.empty()) {
    return ok;
  }
  // Ensure there is at most one wildcard
  auto const wildcardCount = countWildcards(lookupTable);
  if (wildcardCount > 1) {
    return Result(LookupCriteriaError::MultipleWildcards);
  }
  // Ensure search criteria are unique
  if (!hasUniqueSearchCriteria(std::move(lookupTable), tolerance)) {
    return Result(LookupCriteriaError::NonUniqueSearchCriteria);
  }
  return ok;
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

bool LookupTableValidator::hasUniqueSearchCriteria(LookupTableRows lookupTable, double tolerance) const {
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
  for (auto iter = lookupTable.cbegin() + 1; !foundDuplicate && iter != lookupTable.cend(); ++iter) {
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
    auto const lhsTitle = lhs.titleMatcher().value_or(boost::regex());
    auto const rhsTitle = rhs.titleMatcher().value_or(boost::regex());
    return lhsTitle < rhsTitle;
  };

  std::sort(lookupTable.begin(), lookupTable.end(), lookupRowLessThan);
}

void LookupTableValidator::appendSearchCriteriaErrorForAllRows(std::vector<InvalidLookupRowCells> &validationErrors,
                                                               std::size_t rowCount) const {
  for (auto row = 0u; row < rowCount; ++row)
    validationErrors.emplace_back(row, std::unordered_set<int>({LookupRow::Column::THETA, LookupRow::Column::TITLE}));
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
