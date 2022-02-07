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
  using ResultType = ValidationResult<LookupTable, LookupTableValidationError>;

  ResultType operator()(ContentType const &lookupTableContent, double thetaTolerance) const;

  ValidationResult<boost::blank, ThetaValuesValidationError> validateThetaValues(LookupTable lookupTable,
                                                                                 double tolerance) const;

  void validateAllLookupRows(ContentType const &lookupTableContent, LookupTable &lookupTable,
                             std::vector<InvalidDefaultsError> &validationErrors) const;

  int countWildcards(LookupTable const &lookupTable) const;

  void sortInPlaceWildcardsFirstThenByTheta(LookupTable &lookupTable) const;

  bool hasUniqueThetas(LookupTable lookupTable, int wildcardCount, double tolerance) const;
  void appendThetaErrorForAllRows(std::vector<InvalidDefaultsError> &validationErrors, std::size_t rowCount) const;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
