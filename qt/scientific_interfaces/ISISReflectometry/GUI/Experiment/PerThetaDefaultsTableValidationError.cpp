#include "PerThetaDefaultsTableValidationError.h"
#include "ThetaValuesValidationError.h"
namespace MantidQt {
namespace CustomInterfaces {

PerThetaDefaultsTableValidationError::PerThetaDefaultsTableValidationError(
    std::vector<InvalidDefaultsError> validationErrors, boost::optional<ThetaValuesValidationError> fullTableError)
    : m_validationErrors(std::move(validationErrors)),
      m_fullTableError(fullTableError) {}

bool PerThetaDefaultsTableValidationError::hasUniqueThetas() const {
  return !m_fullTableError.is_initialized();
}

std::vector<InvalidDefaultsError> const &
PerThetaDefaultsTableValidationError::errors() const {
  return m_validationErrors;
}
}
}
