#include "MantidAPI/IFunctionGeneral.h"

namespace Mantid {
namespace API {

Kernel::Logger IFunctionGeneral::g_log("IFunctionGeneral");

void IFunctionGeneral::function(const FunctionDomain &domain,
                                FunctionValues &values) const {
  auto actualValuesSize = values.size();
  auto requiredValuesSize = getValuesSize(domain);
  if (actualValuesSize > 0 && requiredValuesSize > 0 &&
      actualValuesSize != requiredValuesSize) {
    throw std::runtime_error(
        "IFunctionGeneral: values object doesn't match domain.");
  }
  try {
    auto &generalDomain = dynamic_cast<const FunctionDomainGeneral &>(domain);
    functionGeneral(generalDomain, values);
  } catch (const std::bad_cast &) {
    throw std::invalid_argument(
        "Provided domain is not of type FunctionDomainGeneral.");
  }
}

void IFunctionGeneral::functionDeriv(const FunctionDomain &domain,
                                     Jacobian &jacobian) {
  calNumericalDeriv(domain, jacobian);
}

size_t IFunctionGeneral::getValuesSize(const FunctionDomain &domain) const {
  if (domain.size() == 0 || getNumberDomainColumns() == 0) {
    return getDefaultDomainSize() * getNumberValuesPerArgument();
  }
  return domain.size() * getNumberValuesPerArgument();
}

size_t IFunctionGeneral::getDefaultDomainSize() const { return 0; }

} // namespace API
} // namespace Mantid
