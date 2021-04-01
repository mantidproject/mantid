// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PerThetaDefaultsTableValidationError.h"

#include <utility>

#include "ThetaValuesValidationError.h"
namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

PerThetaDefaultsTableValidationError::PerThetaDefaultsTableValidationError(
    // cppcheck-suppress passedByValue
    std::vector<InvalidDefaultsError> validationErrors, boost::optional<ThetaValuesValidationError> fullTableError)
    : m_validationErrors(std::move(validationErrors)), m_fullTableError(std::move(fullTableError)) {}

std::vector<InvalidDefaultsError> const &PerThetaDefaultsTableValidationError::errors() const {
  return m_validationErrors;
}

boost::optional<ThetaValuesValidationError> PerThetaDefaultsTableValidationError::fullTableError() const {
  return m_fullTableError;
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
