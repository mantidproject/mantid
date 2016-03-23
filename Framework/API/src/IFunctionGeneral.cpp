#include "MantidAPI/IFunctionGeneral.h"

namespace Mantid {
namespace API {

Kernel::Logger IFunctionGeneral::g_log("IFunctionGeneral");

void IFunctionGeneral::function(const FunctionDomain &domain,
                                FunctionValues &values) const {
  try {
    auto &generalDomain = dynamic_cast<const FunctionDomainGeneral &>(domain);
    functionGeneral(generalDomain, values);
  } catch (std::bad_cast) {
    throw std::invalid_argument(
        "Provided domain is not of type FunctionDomainGeneral.");
  }
}

void IFunctionGeneral::functionDeriv(const FunctionDomain &domain,
                                     Jacobian &jacobian) {
  calNumericalDeriv(domain, jacobian);
}

} // namespace API
} // namespace Mantid
