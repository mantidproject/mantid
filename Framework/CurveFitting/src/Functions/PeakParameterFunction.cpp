// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Functions/PeakParameterFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;

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

  if (!m_peakFunction) {
    throw std::runtime_error("IPeakFunction has not been set.");
  }

  out[0] = m_peakFunction->centre();
  out[1] = m_peakFunction->height();
  out[2] = m_peakFunction->fwhm();
  out[3] = m_peakFunction->intensity();
}

/// Uses numerical derivatives to calculate Jacobian of the function.
void PeakParameterFunction::functionDeriv(const FunctionDomain &domain,
                                          Jacobian &jacobian) {
  calNumericalDeriv(domain, jacobian);
}

/// Make sure the decorated function is IPeakFunction and store it.
void PeakParameterFunction::beforeDecoratedFunctionSet(
    const IFunction_sptr &fn) {
  boost::shared_ptr<IPeakFunction> peakFunction =
      boost::dynamic_pointer_cast<IPeakFunction>(fn);

  if (!peakFunction) {
    throw std::invalid_argument(
        "Decorated function needs to be an IPeakFunction.");
  }

  m_peakFunction = peakFunction;
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
