#include "MantidAPI/IFunction1DSpectrum.h"

namespace Mantid {
namespace API {

Kernel::Logger IFunction1DSpectrum::g_log("IFunction1DSpectrum");

void IFunction1DSpectrum::function(const FunctionDomain &domain,
                                   FunctionValues &values) const {
  try {
    const FunctionDomain1DSpectrum &spectrumDomain =
        dynamic_cast<const FunctionDomain1DSpectrum &>(domain);
    function1DSpectrum(spectrumDomain, values);
  } catch (std::bad_cast) {
    throw std::invalid_argument(
        "Provided domain is not of type FunctionDomain1DSpectrum.");
  }
}

void IFunction1DSpectrum::functionDeriv(const FunctionDomain &domain,
                                        Jacobian &jacobian) {
  try {
    const FunctionDomain1DSpectrum &spectrumDomain =
        dynamic_cast<const FunctionDomain1DSpectrum &>(domain);
    functionDeriv1DSpectrum(spectrumDomain, jacobian);
  } catch (std::bad_cast) {
    throw std::invalid_argument(
        "Provided domain is not of type FunctionDomain1DSpectrum.");
  }
}

void IFunction1DSpectrum::functionDeriv1DSpectrum(
    const FunctionDomain1DSpectrum &domain, Jacobian &jacobian) {
  calNumericalDeriv(domain, jacobian);
}

} // namespace API
} // namespace Mantid
