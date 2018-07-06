#ifndef MANTID_ISISREFLECTOMETRY_PERTHETADEFAULTSVALIDATIONRESULT_H
#define MANTID_ISISREFLECTOMETRY_PERTHETADEFAULTSVALIDATIONRESULT_H
#include <vector>
#include "InvalidDefaultsError.h"
#include "../../Reduction/PerThetaDefaults.h"

namespace MantidQt {
namespace CustomInterfaces {

class PerThetaDefaultsValidationResult {
public:
  PerThetaDefaultsValidationResult(
      std::vector<PerThetaDefaults> defaults,
      std::vector<InvalidDefaultsError> validationErrors, bool hasUniqueThetas);

  bool hasUniqueThetas() const;
  bool isValid() const;
  std::vector<PerThetaDefaults> const &defaults() const;
  std::vector<InvalidDefaultsError> const &errors() const;

  std::vector<PerThetaDefaults> m_defaults;
  std::vector<InvalidDefaultsError> m_validationErrors;
  bool m_hasUniqueThetas;
};
}
}
#endif // MANTID_ISISREFLECTOMETRY_PERTHETADEFAULTSVALIDATIONRESULT_H
