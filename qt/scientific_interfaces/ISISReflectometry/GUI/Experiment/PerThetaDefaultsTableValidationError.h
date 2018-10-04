#ifndef MANTID_ISISREFLECTOMETRY_PERTHETADEFAULTSTABLEVALIDATIONERROR_H
#define MANTID_ISISREFLECTOMETRY_PERTHETADEFAULTSTABLEVALIDATIONERROR_H
#include "../../Reduction/PerThetaDefaults.h"
#include "InvalidDefaultsError.h"
#include "ThetaValuesValidationError.h"
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL PerThetaDefaultsTableValidationError {
public:
  PerThetaDefaultsTableValidationError(
      std::vector<InvalidDefaultsError> validationErrors,
      boost::optional<ThetaValuesValidationError> fullTableError);

  std::vector<InvalidDefaultsError> const &errors() const;
  boost::optional<ThetaValuesValidationError> fullTableError() const;

private:
  std::vector<InvalidDefaultsError> m_validationErrors;
  boost::optional<ThetaValuesValidationError> m_fullTableError;
};

} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_PERTHETADEFAULTSTABLEVALIDATIONERROR_H
