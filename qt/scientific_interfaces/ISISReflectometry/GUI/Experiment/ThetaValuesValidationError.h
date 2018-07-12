#ifndef MANTID_ISISREFLECTOMETRY_THETAVALUESVALIDATIONERROR_H
#define MANTID_ISISREFLECTOMETRY_THETAVALUESVALIDATIONERROR_H
namespace MantidQt {
namespace CustomInterfaces {

enum class ThetaValuesValidationError {
  None,
  MultipleWildcards,
  NonUniqueTheta
};

}
}
#endif // MANTID_ISISREFLECTOMETRY_THETAVALUESVALIDATIONERROR_H
