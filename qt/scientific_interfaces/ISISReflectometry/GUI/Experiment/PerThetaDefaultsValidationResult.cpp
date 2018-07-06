#include "PerThetaDefaultsValidationResult.h"
namespace MantidQt {
namespace CustomInterfaces {
PerThetaDefaultsValidationResult::PerThetaDefaultsValidationResult(
    std::vector<PerThetaDefaults> defaults,
    std::vector<InvalidDefaultsError> validationErrors, bool hasUniqueThetas)
    : m_defaults(std::move(defaults)),
      m_validationErrors(std::move(validationErrors)),
      m_hasUniqueThetas(hasUniqueThetas) {}

bool PerThetaDefaultsValidationResult::isValid() const {
  return hasUniqueThetas() && m_validationErrors.empty();
}

bool PerThetaDefaultsValidationResult::hasUniqueThetas() const {
  return m_hasUniqueThetas;
}

std::vector<PerThetaDefaults> const &
PerThetaDefaultsValidationResult::defaults() const {
  return m_defaults;
}

std::vector<InvalidDefaultsError> const &
PerThetaDefaultsValidationResult::errors() const {
  return m_validationErrors;
}

}
}
