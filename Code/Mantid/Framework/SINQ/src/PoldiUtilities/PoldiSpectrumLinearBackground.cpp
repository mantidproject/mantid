#include "MantidSINQ/PoldiUtilities/PoldiSpectrumLinearBackground.h"
#include "MantidAPI/FunctionFactory.h"

namespace Mantid {
namespace Poldi {

using namespace API;

DECLARE_FUNCTION(PoldiSpectrumLinearBackground)

/// Default constructor
PoldiSpectrumLinearBackground::PoldiSpectrumLinearBackground()
    : ParamFunction(), IFunction1DSpectrum() {}

/// Calculates the function values as f(x) = A1 * wi
void PoldiSpectrumLinearBackground::function1DSpectrum(
    const FunctionDomain1DSpectrum &domain, FunctionValues &values) const {
  double wsIndexDouble = static_cast<double>(domain.getWorkspaceIndex());

  values.setCalculated(wsIndexDouble * getParameter(0));
}

/// Sets the Jacobian, which is wi at any point.
void PoldiSpectrumLinearBackground::functionDeriv1DSpectrum(
    const FunctionDomain1DSpectrum &domain, Jacobian &jacobian) {
  double wsIndexDouble = static_cast<double>(domain.getWorkspaceIndex());

  for (size_t i = 0; i < domain.size(); ++i) {
    jacobian.set(i, 0, wsIndexDouble);
  }
}

/// Declares the function's single parameter A1.
void PoldiSpectrumLinearBackground::init() { declareParameter("A1"); }

} // namespace Poldi
} // namespace Mantid
