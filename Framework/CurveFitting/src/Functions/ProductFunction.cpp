// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/ProductFunction.h"
#include "MantidAPI/FunctionFactory.h"

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;

DECLARE_FUNCTION(ProductFunction)

/// override IFunction base class methods
std::string ProductFunction::name() const { return "ProductFunction"; }

/** Function you want to fit to.
 *  @param domain :: The buffer for writing the calculated values. Must be big enough to accept dataSize() values
 *  @param values :: The function values to evaluate for the function domain
 */
void ProductFunction::function(const API::FunctionDomain &domain, API::FunctionValues &values) const {
  API::FunctionValues tmp(domain);
  values.setCalculated(1.0);
  for (size_t iFun = 0; iFun < nFunctions(); ++iFun) {
    domain.reset();
    getFunction(iFun)->function(domain, tmp);
    values *= tmp;
  }
}

/**
 * Calculate the derivatives numerically.
 * @param domain :: Function domein.
 * @param jacobian :: Jacobian - stores the calculated derivatives
 */
void ProductFunction::functionDeriv(const API::FunctionDomain &domain, API::Jacobian &jacobian) {
  calNumericalDeriv(domain, jacobian);
}

/// overwrite IFunction base class method, which declares function parameters
void ProductFunction::init() {};

} // namespace Mantid::CurveFitting::Functions
