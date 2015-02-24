#include "MantidCurveFitting/PeakParameterFunction.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/FunctionFactory.h"

namespace Mantid {
namespace CurveFitting {

using namespace API;

DECLARE_FUNCTION(PeakParameterFunction)

/**
 * @brief Calculates centre, height, fwhm and intensity of the wrapped function.
 *
 * This function expects a domain with size 4, because IPeakFunction has 4
 * special parameters. These parameters are stored as the output values in the
 * order centre, height, fwhm, intensity.
 *
 * The xValues are ignored, it does not matter what the domain contains.
 *
 * @param out :: Values of IPeakFunction's special parameters.
 * @param xValues :: Domain, ignored.
 * @param nData :: Domain size, must be 4.
 */
void PeakParameterFunction::function1D(double *out, const double *xValues,
                                       const size_t nData) const {
  UNUSED_ARG(xValues);
  if (nData != 4) {
    throw std::invalid_argument("Can only work with domain of size 4.");
  }

  boost::shared_ptr<IPeakFunction> peakFunction =
      boost::dynamic_pointer_cast<IPeakFunction>(getDecoratedFunction());

  if (!peakFunction) {
    throw std::invalid_argument(
        "Decorated function needs to be a valid IPeakFunction.");
  }

  out[0] = peakFunction->centre();
  out[1] = peakFunction->height();
  out[2] = peakFunction->fwhm();
  out[3] = peakFunction->intensity();
}

/// Uses numerical derivatives to calculate Jacobian of the function.
void PeakParameterFunction::functionDeriv(const FunctionDomain &domain,
                                          Jacobian &jacobian) {
  calNumericalDeriv(domain, jacobian);
}

} // namespace CurveFitting
} // namespace Mantid
