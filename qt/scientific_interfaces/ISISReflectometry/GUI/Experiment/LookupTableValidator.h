// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/ValidationResult.h"
#include "LookupTableValidationError.h"
#include "Reduction/LookupRow.h"
#include <array>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL LookupTableValidator {
public:
  using ContentType = std::vector<LookupRow::ValueArray>;
  using LookupTableRows = std::vector<LookupRow>;
  using ResultType = ValidationResult<LookupTableRows, LookupTableValidationError>;

  ResultType operator()(ContentType const &lookupTableContent, double thetaTolerance) const;

  ValidationResult<boost::blank, ThetaValuesValidationError> validateThetaValues(LookupTableRows lookupTable,
                                                                                 double tolerance) const;

  void validateAllLookupRows(ContentType const &lookupTableContent, LookupTableRows &lookupTable,
                             std::vector<InvalidDefaultsError> &validationErrors) const;

  int countWildcards(LookupTableRows const &lookupTable) const;

  void sortInPlaceWildcardsFirstThenByTheta(LookupTableRows &lookupTable) const;

  bool hasUniqueThetas(LookupTableRows lookupTable, int wildcardCount, double tolerance) const;
  void appendThetaErrorForAllRows(std::vector<InvalidDefaultsError> &validationErrors, std::size_t rowCount) const;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
