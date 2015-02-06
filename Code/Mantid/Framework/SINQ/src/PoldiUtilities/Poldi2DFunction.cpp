#include "MantidSINQ/PoldiUtilities/Poldi2DFunction.h"
#include <math.h>

namespace Mantid {
namespace Poldi {
using namespace API;

Poldi2DFunction::Poldi2DFunction()
    : IFunction1DSpectrum(), CompositeFunction(), m_iteration(0) {}

/**
 * Calculates function values for domain. In contrast to CompositeFunction, the
 *summation
 * of values is performed in a different way: The values are set to zero once at
 *the beginning,
 * then it is expected of the member functions to use
 *FunctionValues::addToCalculated or add
 * their values otherwise, without erasing the values.
 *
 * @param domain :: Function domain which is passed on to the member functions.
 * @param values :: Function values.
 */
void Poldi2DFunction::function(const FunctionDomain &domain,
                               FunctionValues &values) const {
  CompositeFunction::function(domain, values);

  if (m_iteration > 0) {
    for (size_t i = 0; i < values.size(); ++i) {
      values.setFitWeight(i, 1.0 / sqrt(values.getCalculated(i) + 0.1));
    }
  }
}

/**
 * Calculates function derivatives. Simply calls
 *CompositeFunction::functionDeriv
 *
 * @param domain :: Function domain which is passed on to the member functions.
 * @param jacobian :: Jacobian.
 */
void Poldi2DFunction::functionDeriv(const FunctionDomain &domain,
                                    Jacobian &jacobian) {
  CompositeFunction::functionDeriv(domain, jacobian);
}

/**
 * Empty implementation of IFunction1DSpectrum::function1DSpectrum which must be
 *present to
 * completely implement the interface.
 *
 * @param domain :: Unused domain.
 * @param values :: Unused function values.
 */
void Poldi2DFunction::function1DSpectrum(const FunctionDomain1DSpectrum &domain,
                                         FunctionValues &values) const {
  UNUSED_ARG(domain);
  UNUSED_ARG(values);
}

/**
 * Calculates the 1D spectrum for this function
 *
 * This method checks all member functions whether they implement the
 * IPoldiFunction1D interface. Those who do are used for calculating
 * the function on the 1D d-domain.
 *
 * @param indices :: Allowed workspace indices.
 * @param domain :: d-based 1D-domain.
 * @param values :: Function values.
 */
void Poldi2DFunction::poldiFunction1D(const std::vector<int> &indices,
                                      const FunctionDomain1D &domain,
                                      FunctionValues &values) const {
  for (size_t i = 0; i < nFunctions(); ++i) {
    IFunction_sptr currentFunction = getFunction(i);
    boost::shared_ptr<IPoldiFunction1D> spectrumFunction =
        boost::dynamic_pointer_cast<IPoldiFunction1D>(currentFunction);

    if (spectrumFunction) {
      spectrumFunction->poldiFunction1D(indices, domain, values);
    }
  }
}

void Poldi2DFunction::iterationFinished() { ++m_iteration; }

} // namespace Poldi
} // namespace Mantid
