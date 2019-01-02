// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "PerThetaDefaultsTableValidationError.h"
#include "ThetaValuesValidationError.h"
namespace MantidQt {
namespace CustomInterfaces {

PerThetaDefaultsTableValidationError::PerThetaDefaultsTableValidationError(
    // cppcheck-suppress passedByValue
    std::vector<InvalidDefaultsError> validationErrors,
    boost::optional<ThetaValuesValidationError> fullTableError)
    : m_validationErrors(std::move(validationErrors)),
      m_fullTableError(fullTableError) {}

std::vector<InvalidDefaultsError> const &
PerThetaDefaultsTableValidationError::errors() const {
  return m_validationErrors;
}

boost::optional<ThetaValuesValidationError>
PerThetaDefaultsTableValidationError::fullTableError() const {
  return m_fullTableError;
}
} // namespace CustomInterfaces
} // namespace MantidQt
