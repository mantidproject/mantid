#ifndef MANTID_ISISREFLECTOMETRY_PERTHETADEFAULTSTABLEVALIDATIONERROR_H
#define MANTID_ISISREFLECTOMETRY_PERTHETADEFAULTSTABLEVALIDATIONERROR_H
#include <vector>
#include "InvalidDefaultsError.h"
#include "ThetaValuesValidationError.h"
#include "../../Reduction/PerThetaDefaults.h"

namespace MantidQt {
namespace CustomInterfaces {

class PerThetaDefaultsTableValidationError {
public:
  PerThetaDefaultsTableValidationError(
      std::vector<InvalidDefaultsError> validationErrors, boost::optional<ThetaValuesValidationError> fullTableError);

  bool hasUniqueThetas() const;
  std::vector<InvalidDefaultsError> const &errors() const;

private:
  std::vector<InvalidDefaultsError> m_validationErrors;
  boost::optional<ThetaValuesValidationError> m_fullTableError;
};

}
}
#endif // MANTID_ISISREFLECTOMETRY_PERTHETADEFAULTSTABLEVALIDATIONERROR_H
