// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/IFunction1DSpectrum.h"

namespace Mantid {
namespace API {

Kernel::Logger IFunction1DSpectrum::g_log("IFunction1DSpectrum");

void IFunction1DSpectrum::function(const FunctionDomain &domain,
                                   FunctionValues &values) const {
  try {
    const auto &spectrumDomain =
        dynamic_cast<const FunctionDomain1DSpectrum &>(domain);
    function1DSpectrum(spectrumDomain, values);
  } catch (const std::bad_cast &) {
    throw std::invalid_argument(
        "Provided domain is not of type FunctionDomain1DSpectrum.");
  }
}

void IFunction1DSpectrum::functionDeriv(const FunctionDomain &domain,
                                        Jacobian &jacobian) {
  try {
    const auto &spectrumDomain =
        dynamic_cast<const FunctionDomain1DSpectrum &>(domain);
    functionDeriv1DSpectrum(spectrumDomain, jacobian);
  } catch (const std::bad_cast &) {
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
