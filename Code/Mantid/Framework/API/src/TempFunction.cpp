//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Exception.h"
#include "MantidAPI/TempFunction.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ParameterTie.h"

#include <sstream>
#include <iostream>

namespace Mantid {
namespace API {

TempFunction::TempFunction(IFunctionMW *function)
    : IFunction(), m_function(function) {}

void TempFunction::function(FunctionDomain &domain) const {
  FunctionDomain1D *domain1D = dynamic_cast<FunctionDomain1D *>(&domain);
  if (!domain1D) {
    throw std::invalid_argument("Function defined only on 1D domain");
  }
  m_function->functionMW(&domain1D->m_calculated[0], &domain1D->m_X[0],
                         domain1D->size());
}

void TempFunction::functionDeriv(FunctionDomain &domain, Jacobian &jacobian) {
  FunctionDomain1D *domain1D = dynamic_cast<FunctionDomain1D *>(&domain);
  if (!domain1D) {
    throw std::invalid_argument("Function defined only on 1D domain");
  }
  m_function->functionDerivMW(&jacobian, &domain1D->m_X[0], domain1D->size());
}

/*--- FunctionDomain1D ---*/

/**
 * Create a domain from interval bounds and number of divisions.
 * @param start :: Starting point
 * @param end :: End point
 * @param n :: Number of points
 */
FunctionDomain1D::FunctionDomain1D(double start, double end, size_t n)
    : FunctionDomain(n) {
  // n > 0, FunctionDomain(n) must throw if it isn't
  double dx = (end - start) / (static_cast<double>(n) - 1.0);
  m_X.resize(n);
  double x = start;
  for (size_t i = 0; i < n; ++i, x += dx) {
    m_X[i] = x;
  }
}

/**
 * Create a domain form a vector.
 * @param xvalues :: Points
 */
FunctionDomain1D::FunctionDomain1D(const std::vector<double> &xvalues)
    : FunctionDomain(xvalues.size()) {
  m_X.assign(xvalues.begin(), xvalues.end());
}

} // namespace API
} // namespace Mantid
